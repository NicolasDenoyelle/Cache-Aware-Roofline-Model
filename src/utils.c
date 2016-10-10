#if defined(_OPENMP)
#include <omp.h>
#endif
#include "roofline.h"
#include "MSC/MSC.h"
#include <math.h>

size_t * roofline_log_array(size_t start, size_t end, int * n){
  size_t * sizes, size;
  double multiplier, val;
  int i;

  if(end<start)
    return NULL;

  if(*n <= 0)
    *n = ROOFLINE_N_SAMPLES;

  roofline_alloc(sizes,sizeof(*sizes)* (*n));
  size = start;    
  multiplier = 1;
  val = (double)start;

  if(start > 0){
    multiplier = pow((double)end/(double)start,1.0/(double)(*n));
    val = ((double)start*multiplier);
  }
  else{
    val = multiplier = pow((double)end,1.0/(double)(*n));
  }

  i=0;
  while(size<=end && i<*n){
    sizes[i] = size;
    val*=multiplier;
    size = (size_t)val;
    i++;
  }

  if(i==0){
    *n=0; free(sizes); return NULL;
  }
  *n=i;
  return sizes;
}

int roofline_type_from_str(const char * type){
  if(!strcmp(type, "LOAD") || !strcmp(type, "load")){return ROOFLINE_LOAD;}
  if(!strcmp(type, "LOAD_NT") || !strcmp(type, "load_nt")){return ROOFLINE_LOAD_NT;}
  if(!strcmp(type, "STORE") || !strcmp(type, "store")){return ROOFLINE_STORE;}
  if(!strcmp(type, "STORE_NT") || !strcmp(type, "store_nt")){return ROOFLINE_STORE_NT;}
  if(!strcmp(type, "2LD1ST") || !strcmp(type, "2ld1st")){return ROOFLINE_2LD1ST;}
  if(!strcmp(type, "COPY") || !strcmp(type, "copy")){return ROOFLINE_COPY;}
  if(!strcmp(type, "ADD") || !strcmp(type, "add")){return ROOFLINE_ADD;}
  if(!strcmp(type, "MUL") || !strcmp(type, "mul")){return ROOFLINE_MUL;}
  if(!strcmp(type, "MAD") || !strcmp(type, "mad")){return ROOFLINE_MAD;}
  if(!strcmp(type, "FMA") || !strcmp(type, "fma")){return ROOFLINE_FMA;}
  return -1;
}

const char * roofline_type_str(int type){
  if(type & ROOFLINE_LOAD) return "load";
  if(type & ROOFLINE_LOAD_NT) return "load_nt";
  if(type & ROOFLINE_STORE) return "store";
  if(type & ROOFLINE_STORE_NT) return "store_nt";
  if(type & ROOFLINE_2LD1ST) return "2LD1ST";
  if(type & ROOFLINE_COPY) return "copy";
  if(type & ROOFLINE_ADD) return "ADD";
  if(type & ROOFLINE_MUL) return "MUL";
  if(type & ROOFLINE_MAD) return "MAD";
  if(type & ROOFLINE_FMA) return "FMA";
  return "";
}

inline void roofline_print_header(FILE * output){
  fprintf(output, "%12s %10s %10s %10s %10s %10s %10s\n",
	  "Obj", "Throughput", "GByte/s", "GFlop/s", "Flops/Byte", "n_threads", "type");
}

void roofline_print_sample(FILE * output, hwloc_obj_t obj, struct roofline_sample_out * sample_out, const int type)
{
  long cyc = sample_out->ts_end - sample_out->ts_start;
  char obj_str[12];
  memset(obj_str,0,sizeof(obj_str));
  hwloc_obj_type_snprintf(obj_str, 10, obj, 0);
  snprintf(obj_str+strlen(obj_str),5,":%d",obj->logical_index);
  fprintf(output, "%12s %10.2f %10.2f %10.2f %10.6f %10u %10s\n",
	  obj_str, 
	  (float)sample_out->instructions / (float) cyc,
	  (float)(sample_out->bytes * cpu_freq) / (float)(cyc*1e9), 
	  (float)(sample_out->flops * cpu_freq) / (float)(1e9*cyc),
	  (float)(sample_out->flops) / (float)(sample_out->bytes),
	  n_threads,
	  roofline_type_str(type));
  fflush(output);
}

int roofline_hwloc_obj_snprintf(hwloc_obj_t obj, char * info_in, size_t n){
  int nc;
    
  memset(info_in,0,n);
  nc = hwloc_obj_type_snprintf(info_in, n, obj, 0);
  nc += snprintf(info_in+nc,n-nc,":%d ",obj->logical_index);
  return nc;
}

int roofline_hwloc_check_cpubind(hwloc_cpuset_t cpuset, int print){
  int ret;
  hwloc_bitmap_t checkset = hwloc_bitmap_alloc();
  if(hwloc_get_cpubind(topology, checkset, HWLOC_CPUBIND_THREAD) == -1){
    perror("get_cpubind");
    hwloc_bitmap_free(checkset);  
    return 0; 
  }
  if(print){
    hwloc_obj_t bound = hwloc_get_first_largest_obj_inside_cpuset(topology, checkset);
    hwloc_obj_t bind = hwloc_get_first_largest_obj_inside_cpuset(topology, cpuset);
    printf("bind=%s:%d, bound %s:%d\n",
	   hwloc_type_name(bind->type),bind->logical_index,
	   hwloc_type_name(bound->type),bound->logical_index);
  }
  ret = hwloc_bitmap_isequal(cpuset,checkset);
  hwloc_bitmap_free(checkset);  
  return ret;
}

int roofline_hwloc_check_membind(hwloc_cpuset_t nodeset, int print){
  hwloc_membind_policy_t policy;
  hwloc_bitmap_t checkset;
  int ret; 
  hwloc_obj_t mem_obj;

  if(nodeset == NULL)
    return 0;
  
  checkset = hwloc_bitmap_alloc();
  
  if(hwloc_get_membind(topology, checkset, &policy, 0) == -1){
    perror("get_membind");
    hwloc_bitmap_free(checkset);  
    return 0; 
  }

  if(print){
    char * policy_name;
    switch(policy){
    case HWLOC_MEMBIND_DEFAULT:
      policy_name = "DEFAULT";
      break;
    case HWLOC_MEMBIND_FIRSTTOUCH:
      policy_name = "FIRSTTOUCH";
      break;
    case HWLOC_MEMBIND_BIND:
      policy_name = "BIND";
      break;
    case HWLOC_MEMBIND_INTERLEAVE:
      policy_name = "INTERLEAVE";
      break;
    case HWLOC_MEMBIND_NEXTTOUCH:
      policy_name = "NEXTTOUCH";
      break;
    case HWLOC_MEMBIND_MIXED:
      policy_name = "MIXED";
      break;
    default:
      policy_name=NULL;
      break;
    }
    mem_obj = hwloc_get_first_largest_obj_inside_cpuset(topology, checkset);
    printf("membind(%s)=%s:%d\n",policy_name,hwloc_type_name(mem_obj->type),mem_obj->logical_index);
  }

  ret = hwloc_bitmap_isequal(nodeset,checkset);
  hwloc_bitmap_free(checkset);  
  return ret;
} 

inline int roofline_hwloc_objtype_is_cache(hwloc_obj_type_t type){
  return type==HWLOC_OBJ_L1CACHE || type==HWLOC_OBJ_L2CACHE || type==HWLOC_OBJ_L3CACHE || type==HWLOC_OBJ_L4CACHE || type==HWLOC_OBJ_L5CACHE;
}

hwloc_obj_t roofline_hwloc_parse_obj(char* arg){
  hwloc_obj_type_t type; 
  char * name;
  int err, depth; 
  char * idx;
  int logical_index;

  name = strtok(arg,":");

  if(name==NULL)
    return NULL;
    
  err = hwloc_type_sscanf_as_depth(name, &type, topology, &depth);
  if(err == HWLOC_TYPE_DEPTH_UNKNOWN){
    fprintf(stderr,"type %s cannot be found, level=%d\n",name,depth);
    return NULL;
  }
  if(depth == HWLOC_TYPE_DEPTH_MULTIPLE){
    fprintf(stderr,"type %s multiple caches match for\n",name);
    return NULL;
  }

  idx = strtok(NULL,":");
  logical_index = 0;
  if(idx!=NULL)
    logical_index = atoi(idx);
  return hwloc_get_obj_by_depth(topology,depth,logical_index);
}

extern hwloc_obj_t first_node;          /* The first node where to bind threads */
int roofline_hwloc_cpubind(){
  hwloc_cpuset_t cpuset;
#if defined(_OPENMP)
  hwloc_obj_t PU, core;
  unsigned n_core;
  unsigned tid;
  tid = omp_get_thread_num();
  n_core = hwloc_get_nbobjs_inside_cpuset_by_type(topology, first_node->cpuset, HWLOC_OBJ_CORE);
  core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, tid%n_core);
  PU = hwloc_get_obj_inside_cpuset_by_type(topology, core->cpuset, HWLOC_OBJ_PU, tid/n_core);
#pragma omp critical
  cpuset = PU->cpuset;
#else
  cpuset = hwloc_get_obj_by_type(topology,HWLOC_OBJ_PU, 0)->cpuset;
#endif
  if(hwloc_set_cpubind(topology,cpuset, HWLOC_CPUBIND_THREAD|HWLOC_CPUBIND_STRICT|HWLOC_CPUBIND_NOMEMBIND) == -1){
    perror("cpubind");
    return 0;
  }
  if(!roofline_hwloc_check_cpubind(cpuset,0)){
    fprintf(stderr, "Binding error: ");
    roofline_hwloc_check_cpubind(cpuset,1);
    exit(EXIT_FAILURE);
  }
  return 1;
}

int roofline_hwloc_membind(hwloc_obj_t obj){
  hwloc_obj_t parent_node;

  /* bind cpuset local memory if there are multiple memories */
  if(hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) == 1)
    return 0;

  parent_node = obj;
  if(parent_node->type!=HWLOC_OBJ_NODE)
    parent_node = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, parent_node);

  if(parent_node == NULL){
    fprintf(stderr, "This cpuset has no ancestor Node\n");
    return 0;
  }
  if(hwloc_set_membind(topology,parent_node->nodeset,HWLOC_MEMBIND_BIND,HWLOC_MEMBIND_BYNODESET) == -1){
    perror("membind");
    return 0;
  }
  return roofline_hwloc_check_membind(parent_node->cpuset,0);
}

int roofline_hwloc_obj_is_memory(hwloc_obj_t obj){
  hwloc_obj_cache_type_t type;

  if (obj->type == HWLOC_OBJ_NODE)
    return 1;
  if(roofline_hwloc_objtype_is_cache(obj->type)){
    type = obj->attr->cache.type;
    if(type == HWLOC_OBJ_CACHE_UNIFIED || type == HWLOC_OBJ_CACHE_DATA){
      return 1;
    }
  }
  return 0;
}

size_t roofline_hwloc_get_memory_size(hwloc_obj_t obj){
  hwloc_obj_cache_type_t type;
  if(obj==NULL)
    return 0;
  if (obj->type == HWLOC_OBJ_NODE)
    return obj->memory.local_memory;
  if(roofline_hwloc_objtype_is_cache(obj->type)){
    type = obj->attr->cache.type;
    if(type == HWLOC_OBJ_CACHE_UNIFIED || type == HWLOC_OBJ_CACHE_DATA){
      return ((struct hwloc_cache_attr_s *)obj->attr)->size;
    }
  }
  return 0;
}

hwloc_obj_t roofline_hwloc_get_instruction_cache(void){
  hwloc_obj_t obj;

  obj = hwloc_get_obj_by_depth(topology, hwloc_topology_get_depth(topology)-1,0);
  if(obj == NULL){errEXIT("No obj at topology leaves\n");}
  while(obj != NULL && !roofline_hwloc_objtype_is_cache(obj->type) && 
	obj->attr->cache.type != HWLOC_OBJ_CACHE_INSTRUCTION){
    obj = obj->parent;
  }
  if(obj==NULL){fprintf(stderr, "No instruction cache\n");}
  return obj;
}

inline size_t roofline_hwloc_get_instruction_cache_size(void){
  return ((struct hwloc_cache_attr_s *)roofline_hwloc_get_instruction_cache()->attr)->size;
}


hwloc_obj_t roofline_hwloc_get_under_memory(hwloc_obj_t obj){
  hwloc_obj_t child;
  if(obj==NULL)
    return NULL;
  child = obj->first_child;
  /* Handle memory without child case */
  if(child == NULL && obj->type == HWLOC_OBJ_NODE){
    while(child == NULL && (obj = obj->prev_sibling) != NULL)
      child = obj->first_child;
  }
  while(child != NULL && !roofline_hwloc_obj_is_memory(child)){
    child = hwloc_get_obj_inside_cpuset_by_depth(topology,child->cpuset, child->depth+1,0);
  };
  return child;
}


hwloc_obj_t roofline_hwloc_get_next_memory(hwloc_obj_t obj){
  hwloc_obj_t tmp,root, node0, ancestor;
  root = hwloc_get_root_obj(topology);
    
  /* If current_obj is not set, start from the bottom of the topology to return the first memory */
  if(obj == NULL){
    obj = hwloc_get_obj_by_depth(topology,hwloc_topology_get_depth(topology)-1,0);
  }
  /* If current_mem_obj is a node, then next memory is a node at same depth */
  if(obj->type==HWLOC_OBJ_NODE){
    node0 = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, 0);
    ancestor = node0->parent;
    /* find node outside of common ancestor to avoid repeating symetries */
    while(!hwloc_bitmap_isincluded(obj->cpuset, ancestor->cpuset)) ancestor = ancestor->parent;
    if(ancestor == root) return NULL;
    tmp = obj;
    while(tmp && hwloc_bitmap_isincluded(tmp->cpuset, ancestor->cpuset)) tmp = tmp->next_cousin;
    return tmp;
  }
  
  /* obj is not a node or there is no remaining node, then climb the topology on left side */
  while(obj != root){
    obj = hwloc_get_obj_by_depth(topology,obj->depth-1,0);
    if(roofline_hwloc_obj_is_memory(obj)){
      return obj;
    }
  }
  /* No memory left in topology */
  return NULL;
}

