#ifndef SAMPLE_HPP
#define SAMPLE_HPP

#include <stdint.h>
#include <sys/time.h>
#include <vector>

namespace CARM{
  class Sample{
  private:
#ifdef _PAPI_
    static unsigned              _BYTES;
    static unsigned              _FLOPS;
#endif
    static std::vector<Sample*>  _samples;  //indexed by cpu os index

#ifdef _PAPI_
    int                          _eventset; //Eventset of this sample used to read events
    long long                    _values[4];//Values to be store on counter read    
#endif    
    timespec                     _timestamp;//Timestamp
    uint64_t                     _flops;    //The amount of flops computed
    uint64_t                     _bytes;    //The amount of bytes of type transfered

    Sample();
    Sample(const int& cpu);
    ~Sample();
    
#ifdef _PAPI_
    static void PAPI_handle_error(const int& err);
    void PAPI_eventset_init(const int& cpu, int * eventset);
    void PAPI_count_event(int& eventset, const char * eventname);
#endif    
    
  public:
    static void          init();
    static void          fini();
    static const Sample* read(const int &cpu);

    uint64_t flops() const;
    uint64_t bytes() const;
    uint64_t timestamp() const; //In nanoseconds
  };
};

#endif

