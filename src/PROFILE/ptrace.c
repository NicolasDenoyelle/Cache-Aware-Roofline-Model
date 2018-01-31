#include <stdio.h>
#include <errno.h>
#include <sys/ptrace.h>  
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

#include <time.h>
#include <sys/time.h>
#include <signal.h>
#include <libunwind.h>
#include <libunwind-ptrace.h>

struct thread_s{
  int              pid;
  unw_addr_space_t unw_local_addr_space;
  void *           UPT_info;
  unw_cursor_t     unw_cursor;
};

void init_thread(struct thread_s *ti, int pid){
  ti->pid = pid;
  ti->unw_local_addr_space =  unw_create_addr_space(&_UPT_accessors, 0);
  ti->UPT_info = _UPT_create(pid);
}

void fini_thread(struct thread_s *ti){
  unw_destroy_addr_space(ti->unw_local_addr_space);
  _UPT_destroy(ti->UPT_info);
}

void thread_backtrace(struct thread_s *ti){
  char sym[256]; unw_word_t offset;
  unw_init_remote(&ti->unw_cursor, ti->unw_local_addr_space, ti->UPT_info);
  while (unw_step(&ti->unw_cursor) > 0) {
    memset(sym, 0, sizeof(sym));
    if (unw_get_proc_name(&ti->unw_cursor, sym, sizeof(sym), &offset) == 0) {
      printf("%d: %s\n", ti->pid, sym);
    }
  }
}

struct thread_s threads[512];
int num_threads;

void handler(__attribute__ ((unused)) int signum){
  int i;
  for(i = 0; i < num_threads; i++){
    struct thread_s t = threads[i];
    
    /* interrupt thread */
    if(ptrace(PTRACE_INTERRUPT, t.pid, NULL, NULL) == -1){
      perror("PTRACE_INTERRUPT");
      continue;
    }

    /* print back trace */
    thread_backtrace(&t);
    
    /* Resume */
    if(waitpid(t.pid, NULL, __WALL) != t.pid){
      perror("waitpid");
    } else{
      ptrace(PTRACE_CONT, t.pid, NULL, NULL);
    }
  }
}

#define INTERVAL 100000 /* 100ms */

void schedule_interrupt(){
  struct itimerval tval = {{0,INTERVAL},{0,INTERVAL}};
  setitimer(ITIMER_REAL, &tval, NULL);
}

void stop_interrupt(){
  struct itimerval tval = {{0,0},{0,0}};
  setitimer(ITIMER_REAL, &tval, NULL);
}

void set_interrupt_handler(){
  static struct sigaction act;
  act.sa_handler = (__sighandler_t)(&handler);
  act.sa_flags = 0;
  sigfillset(&(act.sa_mask));
  sigdelset(&(act.sa_mask), SIGALRM);
  sigaction(SIGALRM, &act, NULL);
}

int main(int argc, char * argv[]){
  if(argc <= 1 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")){
    printf("%s prog <args...>\n", *argv); return 0;
  }
  
  pid_t pid = fork();

  if(pid == 0) {
    /* start program to trace */
    if(execvp(argv[1], argv+1) == -1){ perror("execvp");  return -1; }
    return 0;
  }
    
  /* Tracer code */
  else if(pid > 0){
    /* attach and set options to trace threads creation and process exit */
    if(ptrace(PTRACE_SEIZE, pid, NULL, (void*)(PTRACE_O_TRACECLONE|PTRACE_O_TRACEEXIT)) == -1){
      perror("PTRACE_SEIZE");
      return 1;
    }
    init_thread(&threads[num_threads], pid);

    set_interrupt_handler();
    schedule_interrupt();
    
    /* wait childrens until process exits */
    do{
      int status;
      pid_t child = waitpid(-1, &status, __WALL);

      /* Interrupted by alarm signal */ 
      if(child == -1 && EINTR == errno){
	continue;
      }
      /* Error on waitpid */
      else if (child == -1){
	perror("waitpid"); break;
      }
      /* should never happen since we don't use WNOHANG */
      else if(child == 0){
	continue;
      }
      if(WIFSIGNALED(status)){
	printf("%d: exit with signal %d: %s\n", child, WTERMSIG(status), strsignal(WTERMSIG(status)));
	break;
      }      
      if(WIFEXITED(status)){
	printf("%d: exit from waitpid\n", child);
	break;
      }
      /* Child Stopped */
      if(WIFSTOPPED(status)){
	int sig = WSTOPSIG(status);
	if(sig == SIGTRAP){
	  /* Get ptrace event that triggered the stop */
	  int event = status >> 16;
	  unsigned long eventmsg;
	  if(ptrace(PTRACE_GETEVENTMSG, child, NULL, (void*)(&eventmsg)) == -1){
	    perror("PTRACE_GETEVENTMSG");
	    continue;
	  }
	  
	  if(event == PTRACE_EVENT_EXIT){
	    printf("%d: exit from ptrace\n", child);	    
	  }

	  if(event == PTRACE_EVENT_FORK       ||
	     event == PTRACE_EVENT_VFORK      ||
	     event == PTRACE_EVENT_VFORK_DONE ||
	     event == PTRACE_EVENT_CLONE){
	    init_thread(&threads[num_threads++], eventmsg);
	    printf("%d: spawns child %d\n", child, eventmsg);
	  }
	  /* Resume stopped child */
	  if(ptrace(PTRACE_CONT, child, NULL, NULL) == -1) { perror("PTRACE_CONT(interrupt)"); }	  
	}
      }
    } while(1);

    /* Cleanup */
    stop_interrupt();
    int i;
    for(i=0; i<num_threads; i++){
      fini_thread(&threads[i]);	    
    }
  }
  
  /* fork error */
  else {
    perror("fork");
  }
  
  return 0;
}


