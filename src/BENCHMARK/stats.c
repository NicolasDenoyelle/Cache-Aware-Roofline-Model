#include <math.h>
#include "MSC/MSC.h"
#include "list.h"
#include "types.h"
#include "utils.h"
#include "output.h"

static unsigned n_samples = 5;
extern float    cpu_freq;   /* In Hz */

unsigned roofline_PGCD(unsigned a, unsigned b){
  unsigned max,min, r;
  max = roofline_MAX(a,b);
  min = roofline_MIN(a,b);
  while((r=max%min)!=0){max = min; min = r;}
  return min;
}

unsigned roofline_PPCM(unsigned a, unsigned b){
  return a*b/roofline_PGCD(a,b);
}

static float roofline_throughput_var(list samples){
  unsigned i, n = list_length(samples);
  list_sort(samples, roofline_compare_throughput);
  
  float median = roofline_output_throughput(list_get(samples, n/2));
  float ssqerr=0, err;
  for(i=0; i<n; i++){
    err = roofline_output_throughput(list_get(samples, i))-median;
    ssqerr += err*err;
  }
  return sqrt(ssqerr/n);
}

long roofline_autoset_repeat(roofline_stream src, const int op_type, const void * benchmark)
{
  unsigned i;
  list samples;
  float var = 0, median=0;
  roofline_output median_output, sample;
  uint64_t cycles;
  static int test_stop = 0;
  long repeat=1;
  void (*  benchmark_function)(roofline_stream,
			       roofline_output,
			       int,
			       long) = (void (*)(roofline_stream,
						 roofline_output,
						 int,
						 long)) benchmark;
  samples = new_list(sizeof(roofline_output), n_samples, (void (*)(void*))delete_roofline_output);
  for(i=0; i<n_samples; i++){ list_push(samples, new_roofline_output(NULL,NULL)); }
  while(1){
    cycles = 0;        
    for(i=0; i<n_samples; i++){
      sample = list_get(samples, i);
      roofline_output_clear(sample);
      
      if(op_type == ROOFLINE_ADD ||
	 op_type == ROOFLINE_MUL ||
	 op_type == ROOFLINE_MAD ||
	 op_type == ROOFLINE_FMA){
	benchmark_fpeak(op_type, sample, repeat);
      }
      else if(op_type == ROOFLINE_LOAD    ||
	      op_type == ROOFLINE_LOAD_NT ||
	      op_type == ROOFLINE_STORE   ||
	      op_type == ROOFLINE_STORE_NT||
	      op_type == ROOFLINE_2LD1ST){
	benchmark_stream(src, sample, op_type, repeat);
      }
      else if(op_type == ROOFLINE_LATENCY_LOAD){
	roofline_latency_stream_load(src, sample, 0, repeat);
      }
      else if(benchmark != NULL){
	benchmark_function(src, sample, op_type, repeat);
      }
      else{
	ERR("Not suitable op_type %s\n", roofline_type_str(op_type));
	goto roofline_repeat_error;
      }
      cycles += sample->overhead+sample->cycles;
    }
    var = roofline_throughput_var(samples);
    median_output = list_get(samples, n_samples/2);
    median = roofline_output_throughput(median_output);
    /* bound sample execution between 10 ms and 1s */

#ifdef _OPENMP
#pragma omp single
    {
#endif
      test_stop = 0;
      if(cycles*(1e3/cpu_freq)>1000) {test_stop = 1;}
      if((median_output->cycles)*(1e3/cpu_freq)>=10 || var*100 < median){ test_stop = 1; }
      if(cycles*(1e3/cpu_freq)<10) {test_stop = 0;}      
#ifdef _OPENMP      
    }
#pragma omp barrier
#endif
    if(test_stop){break;}
    repeat *= 2;
  }

#ifdef DEBUG1
#ifdef _OPENMP      
#pragma omp single
#endif  
  roofline_debug1("variance = %f, throughput = %f, time=%luus, repeat=%ld\n",
		  var,
		  median,
		  (unsigned long)(median_output->cycles*(1e3/cpu_freq)),
		  repeat);
#endif
  
  delete_list(samples);  
  return repeat;

roofline_repeat_error:
  delete_list(samples);  
  return 1;
}


