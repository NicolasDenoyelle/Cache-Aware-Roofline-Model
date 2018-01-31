#ifndef MSC_H
#define MSC_H

#include "../output.h"
#include "../stream.h"
#include "../types.h"
#include "../utils.h"


size_t get_chunk_size(int op_type);

int benchmark_types_supported();

void benchmark_fpeak(int op_type,
		     roofline_output out,
		     long repeat);

void benchmark_stream(roofline_stream data,
			     roofline_output out,
			     int op_type,
			     long repeat);

void * benchmark_validation(int op_type, unsigned flops, unsigned bytes);
#endif /* MSC_H */
