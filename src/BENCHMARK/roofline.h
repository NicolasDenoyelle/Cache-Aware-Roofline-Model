#ifndef BENCHMARK_H
#define BENCHMARK_H
#include <stdio.h>
#include <stdlib.h>
#include "topology.h"
#include "types.h"

/***********************************  LIB GLOBALS ****************************************/

extern float            cpu_freq;   /* In Hz */
extern unsigned         n_threads;  /* Set number of threads */
extern off_t            L1_size;    /* size of L1_cache */

int roofline_lib_init(hwloc_topology_t topo, const char * threads_location, int with_hyperthreading, LARM_policy policy);
void roofline_lib_finalize(void);

void roofline_flops    (FILE * output, const int op_type);
void roofline_bandwidth(FILE * output, const hwloc_obj_t memory, const int op_type);
void roofline_oi       (FILE * output, const hwloc_obj_t memory, const int op_type, const unsigned flops, const unsigned bytes);


/****************************************    Statistics    ****************************************/


/**
 * Compute a logarithmic array of sizes
 * @arg start: The first element of the array. 
 * @arg end: The last element of the array. end
 * @arg n(in/out): the number of element in array.
 * @return A logarithmic array of sizes starting with start. Sizes are truncated to the closest integer value.
 **/
size_t * roofline_linear_array(const size_t start, const size_t end, const size_t base, int * n);

#endif /* BENCHMARK_H */
