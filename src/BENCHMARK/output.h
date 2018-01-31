#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdio.h>
#include <inttypes.h>
#include <hwloc.h>

typedef struct roofline_output_s{
  /* All sample type specific data */
  uint64_t ts_high_start, ts_low_start; /* Timestamp in cycles where the roofline started */
  uint64_t ts_high_end, ts_low_end;     /* Timestamp in cycles where the roofline ended */
  uint64_t cycles;                      /* Cycles elapsed: ts_end - ts_start  */    
  uint64_t overhead;                    /* Cycles of spent in overhead */  
  uint64_t instructions;                /* The number of instructions */
  uint64_t bytes;                       /* The amount of bytes transfered */
  uint64_t flops;                       /* The amount of flops computed */
  unsigned n;                           /* The number of accumulated samples */
  hwloc_obj_t mem_location;             /* Location where data is allocated */
  hwloc_obj_t thr_location;             /* Location where thread collecting data is pinned */  
} * roofline_output;

roofline_output new_roofline_output(hwloc_obj_t thr_location, hwloc_obj_t mem_location);
void            delete_roofline_output(roofline_output);
void            roofline_output_clear(roofline_output);
void            roofline_output_accumulate(roofline_output dst, const roofline_output src);
float           roofline_output_throughput(const roofline_output);
int             roofline_compare_throughput(const void * x, const void * y);
int             roofline_compare_cycles(const void * x, const void * y);
void            roofline_output_begin_measure(roofline_output);
void            roofline_output_end_measure(roofline_output o, const uint64_t bytes, const uint64_t flops, const uint64_t ins);
void            roofline_output_print_header(FILE * output);
void            roofline_output_print(FILE * output, const roofline_output sample_out, const int type);

#endif /* OUTPUT_H */

