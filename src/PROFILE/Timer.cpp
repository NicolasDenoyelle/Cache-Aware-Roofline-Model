#include "Timer.hpp"
#include <cstdio>

using namespace CARM;

Timer::Timer():_ts({0,0}), _te({0,0}), _ps({0,0}), _pe({0,0}), _paused(0){_status=STOPPED;}

Timer::Timer(const Timer& copy):_ts(copy._ts), _te(copy._te), _ps(copy._ps), _pe(copy._pe), _paused(copy._paused){_status=STOPPED;}

clockid_t Timer::getclock(){return _clock;}

Timer::~Timer(){}  

void Timer::start(){_status=STARTED; clock_gettime(_clock, &_ts);}

void Timer::pause(){if(_status != PAUSED) {clock_gettime(_clock, &_ps); _status=PAUSED;}}

void Timer::pause(const timespec& ps){_ps = ps;}

void Timer::resume(){_status=STARTED; clock_gettime(_clock, &_pe); _paused += pause_ns();}  

void Timer::resume(const timespec& pe){_status=STARTED; _pe = pe; _paused += pause_ns();}  

void Timer::stop(){clock_gettime(_clock, &_te); _status=STOPPED;}

Timer::status_t Timer::status() const {return _status;}

unsigned long Timer::nanoseconds() const{
  timespec r = result();
  return 1000000000*r.tv_sec + r.tv_nsec;
}

unsigned long Timer::microseconds() const{
  timespec r = result();
  return 1000000*r.tv_sec + r.tv_nsec/1000;
}

unsigned long Timer::milliseconds() const{
  timespec r = result();
  return 1000*r.tv_sec + r.tv_nsec/1000000;
}

timespec Timer::result() const {
    timespec r;
    r.tv_sec = _te.tv_sec-_ts.tv_sec+_ps.tv_sec-_pe.tv_sec;
    r.tv_nsec = _te.tv_nsec-_ts.tv_nsec+_ps.tv_nsec-_pe.tv_nsec;
    return r;
}

unsigned long Timer::pause_ns() const { return 1000000000*(_pe.tv_sec-_ps.tv_sec) + (_pe.tv_nsec-_ps.tv_nsec); }

