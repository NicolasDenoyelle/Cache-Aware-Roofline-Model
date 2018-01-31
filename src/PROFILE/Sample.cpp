extern "C" {
#ifndef _GNU_SOURCE  
#define _GNU_SOURCE
#endif
#include <unistd.h>    
#include <hwloc.h>
#ifdef _PAPI_
#include <papi.h>
#endif  
}
#include "Sample.hpp"

#include <cstring>

using namespace std;
using namespace CARM;

std::vector<Sample*>  Sample::_samples(0);

#ifdef _PAPI_
void Sample::PAPI_handle_error(const int& err)
{
  if(err!=0)
    fprintf(stderr,"PAPI error %d: ",err);
  switch(err){
  case PAPI_EINVAL:
    fprintf(stderr,"Invalid argument.\n");
    break;
  case PAPI_ENOINIT:
    fprintf(stderr,"PAPI library is not initialized.\n");
    break;
  case PAPI_ENOMEM:
    fprintf(stderr,"Insufficient memory.\n");
    break;
  case PAPI_EISRUN:
    fprintf(stderr,"Eventset is already_counting events.\n");
    break;
  case PAPI_ECNFLCT:
    fprintf(stderr,"This event cannot be counted simultaneously with another event in the monitor eventset.\n");
    break;
  case PAPI_ENOEVNT:
    fprintf(stderr,"This event is not available on the underlying hardware.\n");
    break;
  case PAPI_ESYS:
    fprintf(stderr, "A system or C library call failed inside PAPI, errno:%s\n",strerror(errno)); 
    break;
  case PAPI_ENOEVST:
    fprintf(stderr,"The EventSet specified does not exist.\n");
    break;
  case PAPI_ECMP:
    fprintf(stderr,"This component does not support the underlying hardware.\n");
    break;
  case PAPI_ENOCMP:
    fprintf(stderr,"Argument is not a valid component. PAPI_ENOCMP\n");
    break;
  case PAPI_EBUG:
    fprintf(stderr,"Internal error, please send mail to the developers.\n");
    break;
  default:
    perror("");
    break;
  }
}

#define PAPI_call_check(call,check_against, ...) do{	\
    int err = call;					\
    if(err!=check_against){				\
      fprintf(stderr, __VA_ARGS__);			\
      PAPI_handle_error(err);				\
      exit(EXIT_FAILURE);				\
    }							\
  } while(0)

int Sample::PAPI_count_event(int& eventset, const char * eventname){
  const unsigned len = strlen(eventname);
  char * name = (char*)malloc(len); memset(name, 0, len); snprintf(name, len, "%s", eventname);
  int err = PAPI_add_named_event(eventset, name);
  if(err!=PAPI_OK){ fprintf(stderr, "Failed to add %s event\n", name);
    PAPI_handle_error(err); exit(EXIT_FAILURE);
  }
  free(name);
}

void Sample::PAPI_eventset_init(const int& cpu, int * eventset){
#ifdef _OPENMP
#pragma omp critical
    {
#endif /* _OPENMP */
      
      *eventset = PAPI_NULL;      
      PAPI_call_check(PAPI_create_eventset(eventset), PAPI_OK, "PAPI eventset initialization failed\n");
      /* assign eventset to a component */
      PAPI_call_check(PAPI_assign_eventset_component(*eventset, 0), PAPI_OK, "Failed to assign eventset to commponent: ");
      /* bind eventset to cpu */
      PAPI_option_t cpu_option;
      cpu_option.cpu.eventset=*eventset;
      cpu_option.cpu.cpu_num = cpu;
      PAPI_call_check(PAPI_set_opt(PAPI_CPU_ATTACH,&cpu_option),
		      PAPI_OK,
		      "Failed to bind eventset to cpu: ");

      PAPI_count_event(*eventset, "FP_ARITH:SCALAR_DOUBLE");      
      if(_BYTES == 16){
	PAPI_count_event(*eventset, "FP_ARITH:128B_PACKED_DOUBLE");
      } else if(_BYTES == 32){
        PAPI_count_event(*eventset, "FP_ARITH:256B_PACKED_DOUBLE");
      }
      PAPI_count_event(*eventset, "MEM_UOPS_RETIRED:ALL_STORES");
      PAPI_count_event(*eventset, "MEM_UOPS_RETIRED:ALL_LOADS");
#ifdef _OPENMP
    }
#endif /* _OPENMP */
}
#endif /* _PAPI_ */

#ifdef _PAPI
Sample::Sample(const int& cpu): _timestamp({0,0}), _flops(0), _bytes(0){  
  PAPI_eventset_init(cpu, &(_eventset));
  _values[0] = _values[1] = _values[2] = _values[3] = 0;
  PAPI_start(_eventset);
#else
Sample::Sample(__attribute__ ((unused)) const int& cpu): _timestamp({0,0}), _flops(0), _bytes(0){  
#endif /* _PAPI_ */
}

Sample::~Sample(){
#ifdef _PAPI_
  PAPI_stop(_eventset, _values);  
  PAPI_destroy_eventset(&(_eventset));
#endif
}

#ifdef _PAPI_
unsigned Sample::_BYTES = 8;
unsigned Sample::_FLOPS = 1;
#endif

void Sample::init(){
  /* Initialize topology */
  hwloc_topology_t topology;
  if(hwloc_topology_init(&topology) ==-1){
    fprintf(stderr, "hwloc_topology_init failed.\n");
    exit(EXIT_FAILURE);
  }
  hwloc_topology_set_all_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_NONE);
  hwloc_topology_set_type_filter(topology, HWLOC_OBJ_PU, HWLOC_TYPE_FILTER_KEEP_ALL);
  if(hwloc_topology_load(topology) ==-1){
    fprintf(stderr, "hwloc_topology_load failed.\n");
    exit(EXIT_FAILURE);
  }

#ifdef _PAPI_
  /* Check whether flops counted will be AVX or SSE */
  int eax = 0, ebx = 0, ecx = 0, edx = 0;
  /* Check SSE */
  eax = 1;
  __asm__ __volatile__ ("CPUID\n\t": "=c" (ecx), "=d" (edx): "a" (eax));
  if ((edx & 1 << 25) || (edx & 1 << 26)) {_BYTES = 16; _FLOPS=2;}
  /* Check AVX */
  if ((ecx & 1 << 28) || (edx & 1 << 26)) {_BYTES = 32; _FLOPS=4;}
  eax = 7; ecx = 0;
  __asm__ __volatile__ ("CPUID\n\t": "=b" (ebx), "=c" (ecx): "a" (eax), "c" (ecx));
  if ((ebx & 1 << 5)) {_BYTES = 32; _FLOPS=4;}
  /* AVX512. Not checked */
  if ((ebx & 1 << 16)) {_BYTES = 64; _FLOPS=8;}  
  		    
  /* Initialize PAPI library */
  PAPI_call_check(PAPI_library_init(PAPI_VER_CURRENT), PAPI_VER_CURRENT, "PAPI version mismatch\n");
  PAPI_call_check(PAPI_is_initialized(), PAPI_LOW_LEVEL_INITED, "PAPI initialization failed\n");
#endif

  /* Create on sample per PU */
  for(int i=0; i<hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU); i++){
    hwloc_obj_t PU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
    Sample * sample = new Sample(PU->os_index);
    _samples.insert(_samples.begin()+PU->os_index, sample);
  }

  hwloc_topology_destroy(topology);
}

void Sample::fini(){
  for(auto sample = _samples.begin(); sample != _samples.end(); sample++){ delete *sample; }
}


const Sample* Sample::read(const int& cpu){
  Sample * s = _samples[cpu];
  if(s != NULL){
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &(s->_timestamp));
#ifdef _PAPI_
    PAPI_read(s->_eventset, s->_values);
#endif
  }
  return s;
}

uint64_t Sample::flops() const{
#ifdef _PAPI_
  return _values[0] + _FLOPS*_values[1];
#else
  return 0;
#endif
}

uint64_t Sample::bytes() const{
#ifdef _PAPI_
  uint64_t muops = _values[3] + _values[2];
  uint64_t fuops = _values[0] + _values[1];
  if(fuops > 0){
      return (8*muops/fuops)*(_FLOPS*_values[1] + _values[0]);
  } else {
      return 8*muops;
  }
#else
  return 0;
#endif  
}

uint64_t Sample::timestamp() const{
  return _timestamp.tv_nsec + 1000000000*_timestamp.tv_sec;  
}

