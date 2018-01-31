#include "../MSC.h"

#if defined (__AVX512__)
#define SIMD_REG           "zmm"       /* Name of the vector(simd) register (asm) */
#define SIMD_N_REGS        32          /* Number of vector(simd) registers */
#define SIMD_BYTES         64          /* Number Bytes in the vector register */
#define SIMD_FLOPS         8           /* Number of double in the vector register */
#define SIMD_STORE_NT      "vmovntpd"  /* Instruction for non temporal store */
#define SIMD_LOAD_NT       "vmovapd" /* Instruction for non temporal load */
#define SIMD_STORE         "vmovapd"   /* Instruction for store */
#define SIMD_LOAD          "vmovapd"   /* Instruction for load */
#define SIMD_MUL           "vmulpd"    /* Instruction for multiplication */
#define SIMD_ADD           "vaddpd"    /* Instruction for addition */
#elif defined (__AVX2__)
#define SIMD_REG           "ymm"
#define SIMD_N_REGS        16
#define SIMD_BYTES         32
#define SIMD_FLOPS         4
#define SIMD_STORE_NT      "vmovntpd"
#define SIMD_LOAD_NT       "vmovapd"
#define SIMD_STORE         "vmovapd"
#define SIMD_LOAD          "vmovapd"
#define SIMD_MUL           "vmulpd"
#define SIMD_ADD           "vaddpd"
#elif defined (__AVX__)
#define SIMD_REG           "ymm"
#define SIMD_N_REGS        16
#define SIMD_BYTES         32
#define SIMD_FLOPS         4
#define SIMD_STORE_NT      "vmovntpd"
#define SIMD_LOAD_NT       "vmovapd"
#define SIMD_STORE         "vmovapd"
#define SIMD_LOAD          "vmovapd"
#define SIMD_MUL           "vmulpd"
#define SIMD_ADD           "vaddpd"
#elif defined (__SSE4_1__)
#define SIMD_REG           "xmm"
#define SIMD_N_REGS        16
#define SIMD_BYTES         16
#define SIMD_FLOPS         2
#define SIMD_STORE_NT      "movntpd"
#define SIMD_LOAD_NT       "movapd"
#define SIMD_STORE         "movapd"
#define SIMD_LOAD          "movapd"
#define SIMD_MUL           "mulpd"
#define SIMD_ADD           "addpd"
#elif defined (__SSE2__)
#define SIMD_REG           "xmm"
#define SIMD_N_REGS        16
#define SIMD_BYTES         16
#define SIMD_FLOPS         2
#define SIMD_STORE_NT      "movntpd"
#define SIMD_LOAD_NT       "movapd"
#define SIMD_STORE         "movapd"
#define SIMD_LOAD          "movapd"
#define SIMD_MUL           "mulpd"
#define SIMD_ADD           "addpd"
#elif defined (__SSE__)
#define SIMD_REG           "xmm"
#define SIMD_N_REGS        16
#define SIMD_BYTES         16
#define SIMD_FLOPS         4
#define SIMD_STORE_NT      "movntps"
#define SIMD_LOAD_NT       "movaps"
#define SIMD_STORE         "movaps"
#define SIMD_LOAD          "movaps"
#define SIMD_MUL           "mulps"
#define SIMD_ADD           "addps"
#endif

#define SIMD_FMA "vfmadd132pd" /* Fuse multiply add instruction */

#define SIMD_CLOBBERED_REG "xmm" /* Name of the clobbered registers (asm) */
#define SIMD_CLOBBERED_REGS "%xmm0", "%xmm1", "%xmm2", "%xmm3", "%xmm4", "%xmm5", "%xmm6", "%xmm7", "%xmm8", "%xmm9", "%xmm10", "%xmm11", "%xmm12", "%xmm13", "%xmm14", "%xmm15" /* Comma separated list of clobbered registers (asm) */

#define STRIDE_0 "0"
#define REG_0  "0"
#define REG_1  "1"
#define REG_2  "2"
#define REG_3  "3"
#define REG_4  "4"
#define REG_5  "5"
#define REG_6  "6"
#define REG_7  "7"
#define REG_8  "8"
#define REG_9  "9"
#define REG_10 "10"
#define REG_11 "11"
#define REG_12 "12"
#define REG_13 "13"
#define REG_14 "14"
#define REG_15 "15"

#if defined (__AVX512__)

#define REG_16 "16"
#define REG_17 "17"
#define REG_18 "18"
#define REG_19 "19"
#define REG_20 "20"
#define REG_21 "21"
#define REG_22 "22"
#define REG_23 "23"
#define REG_24 "24"
#define REG_25 "25"
#define REG_26 "26"
#define REG_27 "27"
#define REG_28 "28"
#define REG_29 "29"
#define REG_30 "30"
#define REG_31 "31"

#else

#define REG_16 "0"
#define REG_17 "1"
#define REG_18 "2"
#define REG_19 "3"
#define REG_20 "4"
#define REG_21 "5"
#define REG_22 "6"
#define REG_23 "7"
#define REG_24 "8"
#define REG_25 "9"
#define REG_26 "10"
#define REG_27 "11"
#define REG_28 "12"
#define REG_29 "13"
#define REG_30 "14"
#define REG_31 "15"

#endif /* __AVX512__ */

#if defined (__AVX512__)
#define CHUNK_SIZE 1536

#define STRIDE_1  "64"
#define STRIDE_2  "128"
#define STRIDE_3  "192"
#define STRIDE_4  "256"
#define STRIDE_5  "320"
#define STRIDE_6  "384"
#define STRIDE_7  "448"
#define STRIDE_8  "512"
#define STRIDE_9  "576"
#define STRIDE_10 "640"
#define STRIDE_11 "704"
#define STRIDE_12 "768"
#define STRIDE_13 "832"
#define STRIDE_14 "896"
#define STRIDE_15 "960"
#define STRIDE_16 "1024"
#define STRIDE_17 "1088"
#define STRIDE_18 "1152"
#define STRIDE_19 "1216"
#define STRIDE_20 "1280"
#define STRIDE_21 "1344"
#define STRIDE_22 "1408"
#define STRIDE_23 "1472"
#define STRIDE_24 "1536"
#define STRIDE_25 "1600"
#define STRIDE_26 "1664"
#define STRIDE_27 "1728"
#define STRIDE_28 "1792"
#define STRIDE_29 "1856"
#define STRIDE_30 "1920"
#define STRIDE_31 "1984"

#elif defined (__AVX__) || defined (__AVX2__)
#define CHUNK_SIZE 768

#define STRIDE_1  "32"
#define STRIDE_2  "64"
#define STRIDE_3  "96"
#define STRIDE_4  "128"
#define STRIDE_5  "160"
#define STRIDE_6  "192"
#define STRIDE_7  "224"
#define STRIDE_8  "256"
#define STRIDE_9  "288"
#define STRIDE_10 "320"
#define STRIDE_11 "352"
#define STRIDE_12 "384"
#define STRIDE_13 "416"
#define STRIDE_14 "448"
#define STRIDE_15 "480"
#define STRIDE_16 "512"
#define STRIDE_17 "544"
#define STRIDE_18 "576"
#define STRIDE_19 "608"
#define STRIDE_20 "640"
#define STRIDE_21 "672"
#define STRIDE_22 "704"
#define STRIDE_23 "736"
#define STRIDE_24 "768"
#define STRIDE_25 "800"
#define STRIDE_26 "832"
#define STRIDE_27 "864"
#define STRIDE_28 "896"
#define STRIDE_29 "928"
#define STRIDE_30 "960"
#define STRIDE_31 "992"

#else
#define CHUNK_SIZE 384

#define STRIDE_1  "16"
#define STRIDE_2  "32"
#define STRIDE_3  "48"
#define STRIDE_4  "64"
#define STRIDE_5  "80"
#define STRIDE_6  "96"
#define STRIDE_7  "112"
#define STRIDE_8  "128"
#define STRIDE_9  "144"
#define STRIDE_10 "160"
#define STRIDE_11 "176"
#define STRIDE_12 "192"
#define STRIDE_13 "208"
#define STRIDE_14 "224"
#define STRIDE_15 "240"
#define STRIDE_16 "256"
#define STRIDE_17 "272"
#define STRIDE_18 "288"
#define STRIDE_19 "304"
#define STRIDE_20 "320"
#define STRIDE_21 "336"
#define STRIDE_22 "352"
#define STRIDE_23 "368"
#define STRIDE_24 "384"
#define STRIDE_25 "400"
#define STRIDE_26 "416"
#define STRIDE_27 "432"
#define STRIDE_28 "448"
#define STRIDE_29 "464"
#define STRIDE_30 "480"
#define STRIDE_31 "496"

#endif

#if defined (__AVX512__)
#define zero_reg(regnum) "vpxorq %%" SIMD_REG regnum ", %%" SIMD_REG regnum ",%%" SIMD_REG regnum
#elif defined(__AVX2__)
#define zero_reg(regnum) "vpxor %%ymm" regnum ", %%ymm" regnum ",%%ymm" regnum
#elif defined (__AVX__) || defined (__SSE__) || defined (__SSE4_1__) || defined (__SSE2__)
#define zero_reg(regnum) "pxor %%xmm" regnum ", %%xmm" regnum
#endif

#define zero_simd()				\
  __asm__ __volatile__ (			\
    zero_reg(REG_0) "\n\t"			\
    zero_reg(REG_1) "\n\t"			\
    zero_reg(REG_2) "\n\t"			\
    zero_reg(REG_3) "\n\t"			\
    zero_reg(REG_4) "\n\t"			\
    zero_reg(REG_5) "\n\t"			\
    zero_reg(REG_6) "\n\t"			\
    zero_reg(REG_7) "\n\t"			\
    zero_reg(REG_8) "\n\t"			\
    zero_reg(REG_9) "\n\t"			\
    zero_reg(REG_10) "\n\t"			\
    zero_reg(REG_11) "\n\t"			\
    zero_reg(REG_12) "\n\t"			\
    zero_reg(REG_13) "\n\t"			\
    zero_reg(REG_14) "\n\t"			\
    zero_reg(REG_15) "\n\t"			\
    zero_reg(REG_16) "\n\t"			\
    zero_reg(REG_17) "\n\t"			\
    zero_reg(REG_18) "\n\t"			\
    zero_reg(REG_19) "\n\t"			\
    zero_reg(REG_20) "\n\t"			\
    zero_reg(REG_21) "\n\t"			\
    zero_reg(REG_22) "\n\t"			\
    zero_reg(REG_23) "\n\t"			\
    zero_reg(REG_24) "\n\t"			\
    zero_reg(REG_25) "\n\t"			\
    zero_reg(REG_26) "\n\t"			\
    zero_reg(REG_27) "\n\t"			\
    zero_reg(REG_28) "\n\t"			\
    zero_reg(REG_29) "\n\t"			\
    zero_reg(REG_30) "\n\t"			\
    zero_reg(REG_31) "\n\t"			\
    :::SIMD_CLOBBERED_REGS)

#if defined (__AVX__) || defined (__AVX2__) || defined (__AVX512__)
#define roofline_flop_ins(op, regnum)   op " %%" SIMD_REG regnum", %%" SIMD_REG regnum", %%" SIMD_REG regnum"\n\t"
#else
#define roofline_flop_ins(op, regnum)   op " %%" SIMD_REG regnum", %%" SIMD_REG regnum"\n\t"
#endif

#define roofline_load_ins(stride,srcreg, regnum) SIMD_LOAD " "stride"("srcreg"),%%" SIMD_REG regnum"\n\t"
#define roofline_loadnt_ins(stride,srcreg, regnum) SIMD_LOAD_NT " "stride"("srcreg"),%%" SIMD_REG regnum"\n\t"
#define roofline_store_ins(stride,dstreg, regnum) SIMD_STORE " %%" SIMD_REG  regnum ", " stride "("dstreg")\n\t"
#define roofline_storent_ins(stride,dstreg, regnum) SIMD_STORE_NT " %%" SIMD_REG  regnum ", " stride "("dstreg")\n\t"

void benchmark_flops(long repeat, roofline_output out, int op_type);
void benchmark_mov(roofline_stream data, roofline_output out, long repeat, int op_type);
void benchmark_2ld1st(roofline_stream dst, roofline_stream src, roofline_output out, long repeat);
void benchmark_copy(roofline_stream dst, roofline_stream src, roofline_output out, long repeat);
