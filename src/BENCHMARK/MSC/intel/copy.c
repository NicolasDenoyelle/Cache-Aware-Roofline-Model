#include "intel.h"
#include "../../list.h"

#define copy_write roofline_storent_ins
#define copy_read  roofline_load_ins

#define asm_copy(src, dst)			\
  copy_read(STRIDE_0,src,REG_0)			\
  copy_write(STRIDE_0,dst,REG_0)		\
  copy_read(STRIDE_1,src,REG_1)			\
  copy_write(STRIDE_1,dst,REG_1)		\
  copy_read(STRIDE_2,src,REG_2)			\
  copy_write(STRIDE_2,dst,REG_2)		\
  copy_read(STRIDE_3,src,REG_3)			\
  copy_write(STRIDE_3,dst,REG_3)		\
  copy_read(STRIDE_4,src,REG_4)			\
  copy_write(STRIDE_4,dst,REG_4)		\
  copy_read(STRIDE_5,src,REG_5)			\
  copy_write(STRIDE_5,dst,REG_5)		\
  copy_read(STRIDE_6,src,REG_6)			\
  copy_write(STRIDE_6,dst,REG_6)		\
  copy_read(STRIDE_7,src,REG_7)			\
  copy_write(STRIDE_7,dst,REG_7)		\
  copy_read(STRIDE_8,src,REG_8)			\
  copy_write(STRIDE_8,dst,REG_8)		\
  copy_read(STRIDE_9,src,REG_9)			\
  copy_write(STRIDE_9,dst,REG_9)		\
  copy_read(STRIDE_10,src,REG_10)		\
  copy_write(STRIDE_10,dst,REG_10)		\
  copy_read(STRIDE_11,src,REG_11)		\
  copy_write(STRIDE_11,dst,REG_11)		\
  copy_read(STRIDE_12,src,REG_12)		\
  copy_write(STRIDE_12,dst,REG_12)		\
  copy_read(STRIDE_13,src,REG_13)		\
  copy_write(STRIDE_13,dst,REG_13)		\
  copy_read(STRIDE_14,src,REG_14)		\
  copy_write(STRIDE_14,dst,REG_14)		\
  copy_read(STRIDE_15,src,REG_15)		\
  copy_write(STRIDE_15,dst,REG_15)		\
  copy_read(STRIDE_16,src,REG_16)		\
  copy_write(STRIDE_16,dst,REG_16)		\
  copy_read(STRIDE_17,src,REG_17)		\
  copy_write(STRIDE_17,dst,REG_17)		\
  copy_read(STRIDE_18,src,REG_18)		\
  copy_write(STRIDE_18,dst,REG_18)		\
  copy_read(STRIDE_19,src,REG_19)		\
  copy_write(STRIDE_19,dst,REG_19)		\
  copy_read(STRIDE_20,src,REG_20)		\
  copy_write(STRIDE_20,dst,REG_20)		\
  copy_read(STRIDE_21,src,REG_21)		\
  copy_write(STRIDE_21,dst,REG_21)		\
  copy_read(STRIDE_22,src,REG_22)		\
  copy_write(STRIDE_22,dst,REG_22)		\
  copy_read(STRIDE_23,src,REG_23)		\
  copy_write(STRIDE_23,dst,REG_23)		\
  copy_read(STRIDE_24,src,REG_24)		\
  copy_write(STRIDE_24,dst,REG_24)		\
  copy_read(STRIDE_25,src,REG_25)		\
  copy_write(STRIDE_25,dst,REG_25)		\
  copy_read(STRIDE_26,src,REG_26)		\
  copy_write(STRIDE_26,dst,REG_26)		\
  copy_read(STRIDE_27,src,REG_27)		\
  copy_write(STRIDE_27,dst,REG_27)		\
  copy_read(STRIDE_28,src,REG_28)		\
  copy_write(STRIDE_28,dst,REG_28)		\
  copy_read(STRIDE_29,src,REG_29)		\
  copy_write(STRIDE_29,dst,REG_29)

void benchmark_copy(roofline_stream dst, roofline_stream src, roofline_output out, long repeat){
  zero_simd();
  
  roofline_output_begin_measure(out);
  __asm__ __volatile__ (						\
    "loop_copy_overhead_repeat:\n\t"					\
    "mov %1, %%r11\n\t"							\
    "mov %3, %%r12\n\t"							\
    "mov %2, %%r13\n\t"							\
    "buffer_copy_overhead_increment:\n\t"				\
    "add $"STRINGIFY(CHUNK_SIZE)", %%r11\n\t"				\
    "add $"STRINGIFY(CHUNK_SIZE)", %%r12\n\t"				\
    "sub $"STRINGIFY(CHUNK_SIZE)", %%r13\n\t"				\
    "jnz buffer_copy_overhead_increment\n\t"				\
    "sub $1, %0\n\t"							\
    "jnz loop_copy_overhead_repeat\n\t"					\
    :: "r" (1), "r" (src->stream), "r" (src->size), "r" (dst->stream)	\
    : "%r11", "%r12", "%r13", SIMD_CLOBBERED_REGS, "memory");
  roofline_output_end_measure(out, 0, 0, 0);
    
  roofline_output_begin_measure(out);
  __asm__ __volatile__ (						\
    "loop_copy_repeat:\n\t"						\
    "mov %1, %%r11\n\t"							\
    "mov %3, %%r12\n\t"							\
    "mov %2, %%r13\n\t"							\
    "buffer_copy_increment:\n\t"					\
    asm_copy("%%r11", "%%r12")						\
    "add $"STRINGIFY(CHUNK_SIZE)", %%r11\n\t"				\
    "add $"STRINGIFY(CHUNK_SIZE)", %%r12\n\t"				\
    "sub $"STRINGIFY(CHUNK_SIZE)", %%r13\n\t"				\
    "jnz buffer_copy_increment\n\t"					\
    "sub $1, %0\n\t"							\
    "jnz loop_copy_repeat\n\t"						\
    :: "r" (repeat), "r" (src->stream), "r" (src->size), "r" (dst->stream) \
    : "%r11", "%r12", "%r13", SIMD_CLOBBERED_REGS, "memory");
  roofline_output_end_measure(out, (src->size+dst->size)*repeat, 0, (src->size+dst->size)*repeat/SIMD_BYTES);
}


