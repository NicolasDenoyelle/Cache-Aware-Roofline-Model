#ifndef THREADTIMER_HPP
#define THREADTIMER_HPP

#include <string>
#include <map>
extern "C"{
#include <stdio.h>
#include <libunwind.h>
#include <libunwind-ptrace.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>  
};

#include "Timer.hpp"

namespace CARM{
  class ThreadTimer: public Timer{
  private:
    cpu_set_t                              _cpuset;
    pid_t                                  _tid;
    unsigned                               _id;
    std::map<std::string, unsigned long>   _fsamples;
    std::map<std::string, unsigned long>   _ftimings;  
    unsigned long                          _tot_samples;
    unw_addr_space_t                       _unw_local_addr_space;
    void *                                 _UPT_info;
    unw_cursor_t                           _unw_cursor;
  
    void init_unw_addr_space();
  public:
    ThreadTimer();
    ThreadTimer(const pid_t& tid, const unsigned& id);
    ThreadTimer(const ThreadTimer& copy);
    ~ThreadTimer();
    cpu_set_t getcpuset() const;
    pid_t gettid() const;
    unsigned getid() const;
    void sample_backtrace();
    std::map<std::string, unsigned long>& timings();
    void print(FILE * output);
  };
};

#endif
