#ifndef STREAM_H
#define STREAM_H

#include <stdlib.h>
#include "output.h"

typedef double * roofline_stream_t;
typedef struct roofline_stream_s{
  roofline_stream_t stream;     /* The buffer for stream */
  size_t            alloc_size; /* Size of stream */
  size_t            size;       /* Actually usable part of the stream */
} * roofline_stream;

roofline_stream new_roofline_stream(const size_t size, const int op_type);
void            delete_roofline_stream(roofline_stream);
void            roofline_stream_set_size(roofline_stream in, const size_t size, const int op_type);
size_t          roofline_stream_base_size(const unsigned n_split, const int op_type);
size_t *        roofline_linear_sizes(const int type, const size_t start, const size_t end, int * n_elem);
void            roofline_set_latency_stream(roofline_stream data, const size_t size);
void            roofline_latency_stream_load(roofline_stream data, roofline_output out, int type, const long repeat);

#endif /* STREAM_H */
