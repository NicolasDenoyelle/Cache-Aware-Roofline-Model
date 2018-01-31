#include <hwloc.h>
#include "types.h"
#include "topology.h"
#include "MSC/MSC.h"

int roofline_type_from_str(const char * type){
  if(!strcmp(type, "LOAD") || !strcmp(type, "load")){return ROOFLINE_LOAD;}
  if(!strcmp(type, "LOAD_NT") || !strcmp(type, "load_nt")){return ROOFLINE_LOAD_NT;}
  if(!strcmp(type, "STORE") || !strcmp(type, "store")){return ROOFLINE_STORE;}
  if(!strcmp(type, "STORE_NT") || !strcmp(type, "store_nt")){return ROOFLINE_STORE_NT;}
  if(!strcmp(type, "2LD1ST") || !strcmp(type, "2ld1st")){return ROOFLINE_2LD1ST;}
  if(!strcmp(type, "ADD") || !strcmp(type, "add")){return ROOFLINE_ADD;}
  if(!strcmp(type, "MUL") || !strcmp(type, "mul")){return ROOFLINE_MUL;}
  if(!strcmp(type, "MAD") || !strcmp(type, "mad")){return ROOFLINE_MAD;}
  if(!strcmp(type, "FMA") || !strcmp(type, "fma")){return ROOFLINE_FMA;}
  if(!strcmp(type, "LATENCY_LOAD") || !strcmp(type, "latency_load")){return ROOFLINE_LATENCY_LOAD;}
  return -1;
}

const char * roofline_type_str(const int type){
  if(type & ROOFLINE_LOAD) return "LOAD";
  if(type & ROOFLINE_LOAD_NT) return "LOAD_NT";
  if(type & ROOFLINE_STORE) return "STORE";
  if(type & ROOFLINE_STORE_NT) return "STORE_NT";
  if(type & ROOFLINE_2LD1ST) return "2LD1ST";
  if(type & ROOFLINE_ADD) return "ADD";
  if(type & ROOFLINE_MUL) return "MUL";
  if(type & ROOFLINE_MAD) return "MAD";
  if(type & ROOFLINE_FMA) return "FMA";
  if(type & ROOFLINE_LATENCY_LOAD) return "LATENCY_LOAD";  
  return "";
}

int roofline_types_snprintf(const int types, char * str, const size_t len){
  int n = 0;
  if(types & ROOFLINE_LOAD)
    n+=snprintf(str+n,len-n,"%s, ", roofline_type_str(ROOFLINE_LOAD));
  if(types & ROOFLINE_LOAD_NT)
    n+=snprintf(str+n,len-n,"%s, ", roofline_type_str(ROOFLINE_LOAD_NT));
  if(types & ROOFLINE_STORE)
    n+=snprintf(str+n,len-n,"%s, ", roofline_type_str(ROOFLINE_STORE));
  if(types & ROOFLINE_STORE_NT)
    n+=snprintf(str+n,len-n,"%s, ", roofline_type_str(ROOFLINE_STORE_NT));
  if(types & ROOFLINE_2LD1ST)
    n+=snprintf(str+n,len-n,"%s, ", roofline_type_str(ROOFLINE_2LD1ST));
  if(types & ROOFLINE_ADD)
    n+=snprintf(str+n,len-n,"%s, ", roofline_type_str(ROOFLINE_ADD));
  if(types & ROOFLINE_MUL)
    n+=snprintf(str+n,len-n,"%s, ", roofline_type_str(ROOFLINE_MUL));
  if(types & ROOFLINE_MAD)
    n+=snprintf(str+n,len-n,"%s, ", roofline_type_str(ROOFLINE_MAD));
  if(types & ROOFLINE_FMA)
    n+=snprintf(str+n,len-n,"%s, ", roofline_type_str(ROOFLINE_FMA));
  if(types & ROOFLINE_LATENCY_LOAD)
    n+=snprintf(str+n,len-n,"%s, ", roofline_type_str(ROOFLINE_LATENCY_LOAD));
  if(str[n-2] == ','){ str[n-2] = '\0'; str[n-1] = '\0'; }
  return n;
}

unsigned roofline_default_types(hwloc_obj_t obj){
  int supported = benchmark_types_supported() | ROOFLINE_LATENCY_LOAD; 
  if(obj->type == HWLOC_OBJ_L1CACHE){
    return (ROOFLINE_LOAD|ROOFLINE_STORE|ROOFLINE_2LD1ST) & supported;
  } 
  else if(obj->type == HWLOC_OBJ_L2CACHE ||
	  obj->type == HWLOC_OBJ_L3CACHE ||
	  obj->type == HWLOC_OBJ_L4CACHE ||
	  obj->type == HWLOC_OBJ_L5CACHE){
    return (ROOFLINE_STORE|ROOFLINE_LOAD) & supported;
  }
  else if(obj->type == HWLOC_OBJ_NUMANODE || obj->type == HWLOC_OBJ_MACHINE){    
    return (ROOFLINE_STORE_NT|ROOFLINE_STORE|ROOFLINE_LOAD) & supported;
  }
  else if (obj->type == HWLOC_OBJ_PU || obj->type == HWLOC_OBJ_CORE)
    return (ROOFLINE_ADD|ROOFLINE_MAD|ROOFLINE_MUL|ROOFLINE_FMA) & supported;
  else return 0;
}

unsigned roofline_filter_types(hwloc_obj_t obj, int type){
  int supported = benchmark_types_supported() | ROOFLINE_LATENCY_LOAD;
  int FP_possible = (ROOFLINE_ADD|ROOFLINE_MAD|ROOFLINE_MUL|ROOFLINE_FMA) & supported;
  int CACHE_possible = (ROOFLINE_LOAD|ROOFLINE_STORE|ROOFLINE_2LD1ST|ROOFLINE_LATENCY_LOAD) & supported;
  int NUMA_possible = (CACHE_possible|ROOFLINE_LOAD_NT|ROOFLINE_STORE_NT) & supported;
  int obj_type = 0;
  
  if(obj->type == HWLOC_OBJ_L1CACHE){
    if(type & ROOFLINE_LOAD_NT) fprintf(stderr, "skip load_nt type not meaningful for %s\n", hwloc_obj_type_string(obj->type));
    if(type & ROOFLINE_STORE_NT) fprintf(stderr, "skip store_nt type not meaningful for %s\n", hwloc_obj_type_string(obj->type));
    obj_type = type & CACHE_possible;
  }    
  else if(obj->type == HWLOC_OBJ_L2CACHE ||
	  obj->type == HWLOC_OBJ_L3CACHE ||
	  obj->type == HWLOC_OBJ_L4CACHE ||
	  obj->type == HWLOC_OBJ_L5CACHE){
    if(type & ROOFLINE_LOAD_NT) fprintf(stderr, "skip load_nt type not meaningful for %s\n", hwloc_obj_type_string(obj->type));
    if(type & ROOFLINE_STORE_NT) fprintf(stderr, "skip store_nt type not meaningful for %s\n", hwloc_obj_type_string(obj->type));
    obj_type = type & CACHE_possible;
  }
  else if((int)obj->depth <= hwloc_get_type_depth(topology, HWLOC_OBJ_NUMANODE)){
    obj_type = type & NUMA_possible;
  }
  else if (obj->type == HWLOC_OBJ_PU || obj->type == HWLOC_OBJ_CORE){
    obj_type = type & FP_possible;
  }
  return obj_type;
}

