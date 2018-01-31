#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include "../roofline.h"
#include "MSC.h"

size_t chunk_size = 2;
ROOFLINE_STREAM_TYPE res = 0.34;
#define LENGTH 128
ROOFLINE_STREAM_TYPE flops[LENGTH];
#define SIZE_T sizeof(ROOFLINE_STREAM_TYPE)

void mul(unsigned long reps){
    unsigned long i, j;
 for(i = 0; i<reps;i++){
#ifdef _OPENMP
#pragma omp parallel for
#endif
     for(j=0;j<LENGTH;j++) flops[j] *= flops[j];
 }
}

void add(unsigned long reps){
    unsigned long i,j;
    for(i = 0; i<reps;i++){
#ifdef _OPENMP
#pragma omp parallel for
#endif
	for(j=0;j<LENGTH;j++) flops[j] += flops[j];
    }
}

void mad(unsigned long reps){
    unsigned long i, j;
   for(i = 0; i<reps;i++){
#ifdef _OPENMP
#pragma omp parallel for
#endif
	for(j=0;j<LENGTH;j++) flops[j] += flops[j]*flops[j];
    }
}

void fpeak_benchmark(const struct roofline_sample_in * in, struct roofline_sample_out * out, int type){
    void (* bench)(unsigned long) = NULL;
    switch(type){
    case ROOFLINE_MUL:
	bench = mul;
	out->flops = 8*in->loop_repeat;
	break;
    case ROOFLINE_MAD:
	bench = mad;
	out->flops = 8*in->loop_repeat*2;
	break;
    case ROOFLINE_ADD:
	bench = mul;
	out->flops = 16*in->loop_repeat;
	break;
    default:
	return;
	break;
    }
    
    uint64_t c_low0=0, c_low1=0, c_high0=0, c_high1=0;
    roofline_rdtsc(c_high0, c_low0);
    bench(in->loop_repeat);
    roofline_rdtsc(c_high1, c_low1);
    out->ts_start = roofline_rdtsc_diff(c_high0, c_low0);
    out->ts_end = roofline_rdtsc_diff(c_high1, c_low1);
    out->instructions = out->flops;
}

void load(ROOFLINE_STREAM_TYPE * src, size_t size){
    unsigned i;
    ROOFLINE_STREAM_TYPE res = 0.34;
#pragma vector temporal(src)
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for(i=0;i<size;i++){res+=src[i];}
}

void load_nt(ROOFLINE_STREAM_TYPE * src, size_t size){
    unsigned i;
    ROOFLINE_STREAM_TYPE res = 0.34;
#ifdef __INTEL_COMPILER
#pragma vector nontemporal(src)
#endif
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for(i=0;i<size;i++){res+=src[i];}
}

void store(ROOFLINE_STREAM_TYPE * src, size_t size){
    unsigned i;
    ROOFLINE_STREAM_TYPE res = 0.34;
#ifdef __INTEL_COMPILER
#pragma vector temporal(src)
#endif
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for(i=0;i<size;i++){src[i] = res;}
}

void store_nt(ROOFLINE_STREAM_TYPE * src, size_t size){
    unsigned i;
    ROOFLINE_STREAM_TYPE res  = 0.34;
#ifdef __INTEL_COMPILER
#pragma vector nontemporal(src)
#endif
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for(i=0;i<size;i++){src[i] = res;}
}

void copy(ROOFLINE_STREAM_TYPE * src, size_t size){
    unsigned i;
    size_t half_size = size/2;
    ROOFLINE_STREAM_TYPE * to = &(src[half_size]);
#ifdef __INTEL_COMPILER
#pragma vector nontemporal(src, to)
#endif
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for(i=0;i<half_size;i++){to[i] = src[i];}
}

void bandwidth_benchmark(const struct roofline_sample_in * in, struct roofline_sample_out * out, int type){
    unsigned i;
    uint64_t c_low0=0, c_low1=0, c_high0=0, c_high1=0;

    void (*bench)(ROOFLINE_STREAM_TYPE*, size_t) = NULL;
    switch(type){
    case ROOFLINE_LOAD:
	bench = load;
	break;
    case ROOFLINE_LOAD_NT:
	bench = load_nt;
	break;
    case ROOFLINE_STORE:
	bench = store;
	break;
    case ROOFLINE_STORE_NT:
	bench = store_nt;
	break;
    case ROOFLINE_COPY:
	bench=copy;
	break;
    default:
	return;
	break;
    }

    roofline_rdtsc(c_high0, c_low0);
    for(i = 0; i<in->loop_repeat; i++)
	bench(in->stream, in->stream_size/SIZE_T);
    roofline_rdtsc(c_high1, c_low1);
    out->ts_start = roofline_rdtsc_diff(c_high0, c_low0);
    out->ts_end = roofline_rdtsc_diff(c_high1, c_low1);    
    out->bytes = in->stream_size*in->loop_repeat;
    out->instructions = in->loop_repeat*in->stream_size/SIZE_T;
 }

void (*roofline_oi_bench(__attribute__ ((unused)) const double oi, __attribute__ ((unused)) const int type)) (const struct roofline_sample_in * in, struct roofline_sample_out * out, int type){
    return NULL;
}

 int benchmark_types_supported(){
    return ROOFLINE_LOAD|ROOFLINE_LOAD_NT|ROOFLINE_STORE|ROOFLINE_STORE_NT|ROOFLINE_COPY|ROOFLINE_MUL|ROOFLINE_ADD|ROOFLINE_MAD;
}


