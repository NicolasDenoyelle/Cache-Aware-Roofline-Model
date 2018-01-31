extern "C"{
#include <stdio.h>
#include <sys/ptrace.h>  
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <libunwind.h>
#include <libunwind-ptrace.h>
#include <signal.h>
#include <string.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>  
};

#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "Timer.hpp"
#include "ThreadTimer.hpp"
#include "../PINTOOL/Results.hpp"

using namespace std;
using namespace CARM;

#define LOG(...) fprintf(stderr, __VA_ARGS__);

class Profile{
private:
  static pid_t                   _pid;
  static map<pid_t, ThreadTimer> _timings;
  static int                     _thread_count;
  static struct itimerval        _tval; // Interval timer
  
  static void setupHandler(){
    static struct sigaction act;
    act.sa_handler = (__sighandler_t)(&Profile::interrupt);
    act.sa_flags = 0;
    sigfillset(&(act.sa_mask));
    sigdelset(&(act.sa_mask), SIGALRM);
    sigaction(SIGALRM, &act, NULL);
  }
  
  static void schedule_interrupt(){
    long base = 1000; //1ms
    long rnd  = rand()%base; //[1,2]ms
    
    _tval.it_interval = {0,base+rnd}; /* Interval for periodic timer every 1000 us */
    _tval.it_value = {0,base+rnd};    /* Time until next expiration */
    setitimer(ITIMER_REAL, &_tval, NULL);
  }
  
  static void stop_interrupt(){
    _tval.it_interval = {0,0};
    _tval.it_value = {0,0};
    setitimer(ITIMER_REAL, &_tval, NULL);
  }
  
  static void interrupt(__attribute__ ((unused)) int signum){
    //Mark pause
    for(auto t = _timings.begin(); t != _timings.end(); t++){t->second.pause();}

    //Interrupt and record sample
    for(auto t = _timings.begin(); t != _timings.end(); t++){
      if(ptrace(PTRACE_INTERRUPT, t->first, NULL, NULL) == -1){perror("PTRACE_INTERRUPT"); continue;}      
      t->second.sample_backtrace();
    }

    //Resume signal
    for(auto t = _timings.begin(); t != _timings.end(); t++){
      if(waitpid(t->first, NULL, __WALL) != t->first){
	perror("waitpid");
      } else{
	ptrace(PTRACE_CONT, t->first, NULL, NULL); //If it fails, it is retried later in constructor on waitpid.
      }
    }
  
    //Resume timers
    for(auto t = _timings.begin(); t != _timings.end(); t++){
      t->second.resume();	
    }

    //Schedule next interrupt
    schedule_interrupt();    
  }

  static void registerChild(const pid_t& tid){
    pair<map<pid_t, ThreadTimer>::iterator,bool> inserted = _timings.insert(pair<pid_t,ThreadTimer>(tid, ThreadTimer(tid, _thread_count++)));
    if(inserted.second){
      inserted.first->second.start();
    }
  }
  
  static void terminate(const pid_t& tid, __attribute__ ((unused)) const int& status){
    ThreadTimer& t = _timings[tid];
    t.stop();
  }
  
  static void terminate(){
    for(auto timing = _timings.begin(); timing != _timings.end(); timing++){
      if(timing->second.status() != Timer::STOPPED){
	timing->second.stop();
      }
    }
  }
    
public:
  static void profile(const pid_t& pid){
    pid_t child; int status;
    _thread_count = 0;
    _pid = pid;
    
    //Ptrace set options. Use seize in order to use ptrace interrupt
    if(ptrace(PTRACE_SEIZE, pid, NULL, (void*)(PTRACE_O_TRACECLONE|PTRACE_O_TRACEEXIT)) == -1){
      perror("PTRACE_SETOPTIONS");
      return;
    }

    //register pid in timings
    registerChild(pid);
    //set interrupt handler for the sampling
    setupHandler();
    //schedule interrupt for backtrace sampling
    schedule_interrupt();
    
    do{
      child = waitpid(-1, &status, __WALL);
      
      if(child == -1){
	if(EINTR == errno){ continue; } //Ignore if stopped by alarm interruption.
	else{ perror("waitpid"); return; }
      } else if(child == 0){ continue; }
      if(WIFEXITED(status)){
	//Stop not caught thread exits
	terminate();
	break;
      }
      else if(WIFSIGNALED(status)){/* kill(child, WTERMSIG(status)); break; */}
      else if(WIFSTOPPED(status)){
	int sig = WSTOPSIG(status);
	if(sig == SIGTRAP){
	  int event = status >> 16; unsigned long eventmsg;
	  if(ptrace(PTRACE_GETEVENTMSG, child, NULL, (void*)(&eventmsg)) == -1){perror("PTRACE_GETEVENTMSG"); goto resume_tracee;}
	  if(event == PTRACE_EVENT_EXIT){ terminate(child, (int)eventmsg); }

	  if(event == PTRACE_EVENT_FORK       ||
	     event == PTRACE_EVENT_VFORK      ||
	     event == PTRACE_EVENT_VFORK_DONE ||
	     event == PTRACE_EVENT_CLONE)
	  { registerChild(eventmsg); }

	  /* Now done in interrupt handler */
	  /* Profiler signal when call PTRACE_INTERRUPT in linux/ptrace.h but not in sys/ptrace.h */
	  //if(event>>8 == PTRACE_EVENT_STOP) 
	  //_timings[child].sample_backtrace();	
	}
	else { LOG("Thread %d stopped with signal %d: %s\n", child, WSTOPSIG(status), strsignal(WSTOPSIG(status))); }
      resume_tracee:;
	if(ptrace(PTRACE_CONT, child, NULL, NULL) == -1) { perror("PTRACE_CONT(interrupt)"); }
      } else {break;}
    } while(1);
    stop_interrupt();
  }

  static void print(FILE * input, FILE* output){
    Results results;
    if(input != NULL){results.read(input);}
    for(auto pid = _timings.begin(); pid != _timings.end(); pid ++){
      ThreadTimer&                ttimer  = pid->second;
      pid_t                       id      = ttimer.getid();
      map<string, unsigned long>& timings = pid->second.timings();
      for(auto timing = timings.begin(); timing != timings.end(); timing++){
	//printf("%3d %16lu %s\n", id, timing->second, timing->first.c_str());	
	Function f(timing->first);
	Block b(0,0,0,0,0, timing->second/1000);
	f.set(b);
	results.setFunction(id,f);
      }
    }
    results.print(output);
  }
};

pid_t                   Profile::_pid = 0;
int                     Profile::_thread_count = 0;
map<pid_t, ThreadTimer> Profile::_timings;
struct itimerval        Profile::_tval;

void usage(const char * bin_name){
  fprintf(stderr, "%s (options) -- prog <prog_args> ...\n\n", bin_name);
  fprintf(stderr, "Options:\n\t");
  fprintf(stderr, "-i, --input <file>: input a file containing functions timing, flops and bytes, in order to update the timings\n\t");
  fprintf(stderr, "-o, --output <file>: Output to a file\n");
}

FILE *in = NULL, *out=NULL;

int parse(int argc, char * argv[]){
  int i;
  if(argc < 2){ usage(argv[0]); exit(1); }
  for(i = 1; i<argc; i++){
    if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "--input")){
      in = fopen(argv[i+1], "r");
      if(in == NULL){ perror("fopen"); exit(1); }
      i++;
    }
    else if(!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")){
      out = fopen(argv[i+1], "w");
      if(out == NULL){ perror("fopen"); exit(1); }
      i++;
    }
    else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")){
      usage(argv[0]);
      exit(0);
    }
    else if(!strcmp(argv[i], "--")){
      i++;
      break;
    }
    else {
      break;
    }
  }
  if(out == NULL){ out = stdout; }
  return i;
}

int main(int argc, char * argv[]){
  pid_t _pid;
  int shift = parse(argc, argv);
  _pid = fork();

  if(_pid == 0) { //Tracee start program
    //if(ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) { perror("PTRACE_TRACEME"); return -1; } //Monitor app
    if(execvp(argv[shift], argv+shift) == -1){ perror("execvp");  return -1; } //Start App
    return 0;
  }   
  else if(_pid > 0){ //Tracer handle events
    Profile::profile(_pid);
    Profile::print(in, out);    
  } else {
    perror("fork");
  }
  
  return 0;
}
