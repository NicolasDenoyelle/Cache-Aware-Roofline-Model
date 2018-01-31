#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <hwloc.h>

/* Allocation policies */
typedef enum{LARM_FIRSTTOUCH, LARM_INTERLEAVE, LARM_FIRSTTOUCH_HBM, LARM_INTERLEAVE_DDR, LARM_INTERLEAVE_HBM} LARM_policy;

extern hwloc_topology_t topology;   /* Current machine topology */
extern hwloc_obj_t      root;       /* The subroot of topology to select the amount of threads */
extern size_t           max_size;   /* The 32 * last level cache size */

void        roofline_hwloc_check_version();
int         roofline_hwloc_objtype_is_cache(const hwloc_obj_type_t type);
int         roofline_hwloc_obj_snprintf(const hwloc_obj_t obj, char * info_in, const size_t n);
hwloc_obj_t roofline_hwloc_parse_obj(const char*);

/* CPU binding */
hwloc_obj_t roofline_hwloc_cpubind(const hwloc_obj_type_t);
hwloc_obj_t roofline_hwloc_get_cpubind();
int         roofline_hwloc_check_cpubind(hwloc_cpuset_t cpuset);

/* Memory binding */
hwloc_obj_t roofline_hwloc_set_area_membind(const hwloc_obj_t, void *, const size_t, LARM_policy);
hwloc_obj_t roofline_hwloc_local_domain();
int         roofline_hwloc_obj_is_memory(const hwloc_obj_t);
size_t      roofline_hwloc_get_memory_size(const hwloc_obj_t);
hwloc_obj_t roofline_hwloc_get_next_memory(const hwloc_obj_t);
hwloc_obj_t roofline_hwloc_get_under_memory(const hwloc_obj_t);
int         roofline_hwloc_get_memory_bounds(const hwloc_obj_t memory, size_t * lower, size_t * upper, const int op_type);

/* special indexing */
hwloc_obj_t roofline_hwloc_NUMA_domain(int logical_index);
int         roofline_hwloc_nb_parent_objs_by_depth(unsigned depth);
hwloc_obj_t roofline_hwloc_next_parent_obj(hwloc_obj_t obj);

/* Merging cpusets */
void roofline_hwloc_accumulate(hwloc_obj_t * dst, hwloc_obj_t * src);

#endif /* TOPOLOGY_H */

