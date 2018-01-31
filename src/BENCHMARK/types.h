#ifndef TYPES_H
#define TYPES_H

#include <hwloc.h>

#define ROOFLINE_LOAD         1
#define ROOFLINE_LOAD_NT      (1<<1)
#define ROOFLINE_STORE        (1<<2)
#define ROOFLINE_STORE_NT     (1<<3)
#define ROOFLINE_2LD1ST       (1<<4)
#define ROOFLINE_MUL          (1<<5)
#define ROOFLINE_ADD          (1<<6)
#define ROOFLINE_MAD          (1<<7)
#define ROOFLINE_FMA          (1<<8)
#define ROOFLINE_LATENCY_LOAD (1<<9)

int          roofline_type_from_str(const char * type);
const char * roofline_type_str(const int type);
int          roofline_types_snprintf(const int types, char * str, const size_t len);
unsigned     roofline_default_types(hwloc_obj_t obj);
unsigned     roofline_filter_types(hwloc_obj_t obj, int type);

#endif /* TYPES_H */

