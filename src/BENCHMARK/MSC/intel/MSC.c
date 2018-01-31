#include "intel.h"

void
benchmark_fpeak(int op_type, roofline_output out, long repeat)
{
  benchmark_flops(repeat, out, op_type);
}

void
benchmark_stream(roofline_stream data, roofline_output out, int op_type, long repeat)
{
  benchmark_mov(data, out, repeat, op_type);
}

#if defined (__AVX512__)
#elif defined (__AVX__)
#elif (__SSE__) || defined (__SSE2__) || defined (__SSE4_1__)
#endif

size_t oi_chunk_size = 64;

size_t get_chunk_size(int op_type){
  if(op_type & ~(ROOFLINE_LOAD|ROOFLINE_LOAD_NT|ROOFLINE_STORE|ROOFLINE_STORE_NT|ROOFLINE_2LD1ST))
    return oi_chunk_size;
  else if(op_type & (ROOFLINE_LOAD|ROOFLINE_LOAD_NT|ROOFLINE_STORE|ROOFLINE_STORE_NT|ROOFLINE_2LD1ST))
    return CHUNK_SIZE;
  else return 64;
}

int benchmark_types_supported(){
  int supported = ROOFLINE_LOAD|ROOFLINE_STORE|ROOFLINE_2LD1ST|ROOFLINE_MUL|ROOFLINE_ADD|ROOFLINE_MAD;
#ifdef __FMA__
  supported = supported | ROOFLINE_FMA;
#endif
#if defined (__AVX512__ ) || defined (__AVX2__)
  supported = supported | ROOFLINE_LOAD_NT;
#endif
#if defined (__AVX512__ ) || defined (__AVX2__)  || defined (__AVX2__) || defined (_AVX_) || defined (_SSE_4_1_) || defined (_SSE2_)
  supported = supported | ROOFLINE_STORE_NT;
#endif
  return supported;
}

void validation_stream(roofline_stream data,
		       roofline_output out,
		       int op_type,
		       long repeat,
		       unsigned flops,
		       unsigned bytes);

