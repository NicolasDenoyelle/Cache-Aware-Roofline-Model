#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "utils.h"
#include "stream.h"
#include "stats.h"
#include "topology.h"

extern size_t alignement; /* Level 1 cache line size */

static size_t roofline_memalign(double ** data, size_t size){
    int err;
    if(data != NULL){
	err = posix_memalign((void**)data, alignement, size);
	switch(err){
	case 0:
	    break;
	case EINVAL:
	    fprintf(stderr,"The alignment argument was not a power of two, or was not a multiple of sizeof(void *).\n");
	    break;
	case ENOMEM:
	    fprintf(stderr,"There was insufficient memory to fulfill the allocation request.\n");
	}
	if(*data == NULL)
	    fprintf(stderr,"Chunk is NULL\n");
	if(err || *data == NULL)
	  ERR_EXIT("Roofline memalign unhandled error\n");
    }
    return size;
}

size_t get_chunk_size(int type);/* Minimum chunk size in MSC.h */

static size_t resize_splitable_chunk(const size_t size, const int type){
  size_t ret, chunk_size = get_chunk_size(type);
  if(chunk_size%alignement != 0) chunk_size = roofline_PPCM(chunk_size, alignement);
  if(size%chunk_size == 0) return size;
  else{ ret = chunk_size*(size/chunk_size); return ret; }
}

size_t * roofline_linear_sizes(const int type, const size_t start, const size_t end, int * n_elem){
  size_t * sizes;
  int i, n;
  size_t size;
  
  if(end<start) return NULL;
  n = *n_elem;
  if(n <= 0) n = ROOFLINE_N_SAMPLES;  
  roofline_alloc(sizes,sizeof(*sizes)*n);

  for(i=0; i<n; i++){    
    size = start + i*(end-start)/n;
    sizes[i] = resize_splitable_chunk(size, type);
  }
  return sizes;
}


roofline_stream new_roofline_stream(const size_t size, const int op_type){
  roofline_stream ret;  
  roofline_alloc(ret, sizeof(*ret));  
  roofline_memalign(&(ret->stream), size);
  ret->alloc_size = size;
  ret->size = resize_splitable_chunk(size, op_type);
  return ret;
}

size_t roofline_stream_base_size(const unsigned n_split, const int op_type){
    return n_split * get_chunk_size(op_type);
}

void roofline_stream_set_size(roofline_stream in, const size_t size, const int op_type){
  if(size>in->alloc_size){
    free(in->stream);
    roofline_memalign(&(in->stream), size);
    in->alloc_size = size;
  }
  in->size = resize_splitable_chunk(size, op_type);
}

void delete_roofline_stream(roofline_stream in){
  free(in->stream);
  free(in);
}

void latency_stream_init(roofline_stream data)
{
  uint64_t *links = (uint64_t *)(data->stream), *rands, val;
  unsigned i, seed = 44567754, n = data->size/sizeof(*links);  
  roofline_alloc(rands, n*sizeof(*links));
  for(i=0;i<n;i++){ rands[i] = i; }
  for(i=n;i>0;i--){
    val = rand_r(&seed)%i;
    links[i-1] = rands[val];
    rands[val] = rands[i-1]; 
  }
  free(rands);
}

void roofline_set_latency_stream(roofline_stream data, const size_t size)
{
  while(data->alloc_size<size){ data->alloc_size*=2;}
  if((data->stream = realloc(data->stream, data->alloc_size)) == NULL){
    perror("realloc"); exit(EXIT_FAILURE);
  }
  latency_stream_init(data);
}

void roofline_latency_stream_load(roofline_stream data, roofline_output out, __attribute__ ((unused)) int type, const long repeat)
{
  unsigned long n = repeat*data->size/sizeof(uint32_t);
  roofline_output_begin_measure(out);
  __asm__ __volatile__ (						\
    "xor %%rax, %%rax\n\t"						\
    "loop_latency_benchmark:\n\t"					\
    "movq (%[s],%%rax,8), %%rax\n\t"					\
    "sub $1, %[n]\n\t"							\
    "jnz loop_latency_benchmark\n\t"					\
    :: [s] "r" (data->stream), [n] "r" (n): "%rax");
  /* for(i=0; i<n; i++){ index = indexes[index]; } */
  roofline_output_end_measure(out, repeat*data->size, 0, n);
}

