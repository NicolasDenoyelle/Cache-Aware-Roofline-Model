#ifndef TIMER_HPP
#define TIMER_HPP

#include <time.h>

namespace CARM{
  class Timer{
  public:
    typedef enum status_e {STOPPED, STARTED, PAUSED} status_t;
  private:
    static const clockid_t _clock = CLOCK_MONOTONIC; //The type of clock used
    timespec        _ts; //time at start
    timespec        _te; //time at stop
    timespec        _ps; //time at pause
    timespec        _pe; //time at resume
    unsigned long   _paused; //Time paused
    status_t        _status;
    timespec result() const;
    unsigned long pause_ns() const;
  
  public:
    static clockid_t getclock();
    
    Timer();
    Timer(const Timer& copy);
    ~Timer();
    void start();
    void pause();
    void pause(const timespec& ps);    
    void resume();
    void resume(const timespec& pe);    
    void stop();
    status_t status() const;
    unsigned long nanoseconds() const;
    unsigned long microseconds() const;
    unsigned long milliseconds() const;
  };
};
#endif
