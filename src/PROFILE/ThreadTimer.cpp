extern "C" {
#include <string.h>
};
#include <string>
#include "ThreadTimer.hpp"

using namespace std;
using namespace CARM;

void ThreadTimer::init_unw_addr_space(){
  _unw_local_addr_space =  unw_create_addr_space(&_UPT_accessors, 0);
  _UPT_info = _UPT_create(_tid);
}

ThreadTimer::ThreadTimer(): _tid(-1), _tot_samples(0){
  CPU_ZERO(&_cpuset);
  init_unw_addr_space();
}

ThreadTimer::ThreadTimer(const pid_t& tid, const unsigned& id): _tid(tid), _id(id), _tot_samples(0){
  CPU_ZERO(&_cpuset);
  if(sched_getaffinity(_tid, sizeof(_cpuset), &_cpuset) == -1){ perror("sched_getaffinity"); }
  init_unw_addr_space();
}

ThreadTimer::ThreadTimer(const ThreadTimer& copy): Timer(copy),
						   _cpuset(copy._cpuset),
						   _tid(copy._tid),
						   _id(copy._id),
						   _tot_samples(copy._tot_samples){
  init_unw_addr_space();
}

ThreadTimer::~ThreadTimer(){
  unw_destroy_addr_space(_unw_local_addr_space);
  _UPT_destroy(_UPT_info);
}

cpu_set_t ThreadTimer::getcpuset() const {return _cpuset;}
pid_t     ThreadTimer::gettid() const {return _tid;}
unsigned  ThreadTimer::getid() const {return _id;}
  
void ThreadTimer::sample_backtrace(){  
  char sym[256]; unw_word_t offset;
  unw_init_remote(&_unw_cursor, _unw_local_addr_space, _UPT_info);
  while (unw_step(&_unw_cursor) > 0) {
    memset(sym, 0, sizeof(sym));
    if (unw_get_proc_name(&_unw_cursor, sym, sizeof(sym), &offset) == 0) { _fsamples[string(sym)] ++; }
    _tot_samples++;    
  }
}

map<string, unsigned long>& ThreadTimer::timings(){
  unsigned long nano = nanoseconds();
  for(auto f = _fsamples.begin(); f != _fsamples.end(); f++){
    _ftimings[f->first] = f->second * nano / _tot_samples;
  }    
  return _ftimings;
}
  
void ThreadTimer::print(FILE * output){
  unsigned long micro = microseconds();
  fprintf(output, "thread:%d (%lu us)\n", _id, micro);

  for(auto f = _fsamples.begin(); f!=_fsamples.end(); f++){
    fprintf(output, "%6lu samples %3lu%% %10lu(us) %s\n",
	    f->second,
	    100 * f->second /_tot_samples,
	    micro*f->second /_tot_samples,
      	    f->first.c_str());
  }
  fprintf(output, "\n");
}

