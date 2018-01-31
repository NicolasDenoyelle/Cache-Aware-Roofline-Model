#include <math.h>
#include "roofline.h"
#include "topology.h"
#include "stream.h"
#include "output.h"
#include "utils.h"
#include "stats.h"
#include "MSC/MSC.h"

hwloc_topology_t topology = NULL;            /* Current machine topology  */
size_t           alignement = 0;             /* Level 1 cache line size */
float            cpu_freq = 0;               /* In Hz */
unsigned         n_threads = 1;              /* The number of threads for benchmark */
unsigned int     roofline_types;             /* What rooflines do we want in byte array */
hwloc_obj_t      root;                       /* The root of topology to select the amount of threads */
unsigned         NUMA_domain_depth;                 /* Depth of first HWLOC_OBJ_NUMANODE */

static LARM_policy policy;                   /* The data allocation policy when target memory is larger than one node */ 
static hwloc_obj_type_t leaf_type;
static unsigned  n_parent_nodes;

#if defined(_OPENMP)
  int roofline_lib_init(hwloc_topology_t topo,
			const char * threads_location,
			int with_hyperthreading,
			LARM_policy p)
#else
  int roofline_lib_init(hwloc_topology_t topo,
			const char * threads_location,
			__attribute__ ((unused)) int with_hyperthreading,
			LARM_policy p)
#endif
{
  /* Check hwloc version */
  roofline_hwloc_check_version();

  /* Initialize topology */
  if(topo != NULL) hwloc_topology_dup(&topology, topo);
  else{
    if(hwloc_topology_init(&topology) ==-1){
      fprintf(stderr, "hwloc_topology_init failed.\n");
      return -1;
    }
    hwloc_topology_set_icache_types_filter(topology, HWLOC_TYPE_FILTER_KEEP_ALL);
    if(hwloc_topology_load(topology) ==-1){
      fprintf(stderr, "hwloc_topology_load failed.\n");
      return -1;
    }
  }
    
  /* Get first node and number of threads */
  if(threads_location == NULL){
    root = hwloc_get_obj_by_depth(topology, HWLOC_TYPE_DEPTH_NUMANODE, 0);
  }
  else{
    root = roofline_hwloc_parse_obj(threads_location);
    if(root == NULL){goto lib_err_with_topology;}
    if(root->arity == 0){
      roofline_mkstr(root_str, 32);
      roofline_hwloc_obj_snprintf(root, root_str, sizeof(root_str));
      fprintf(stderr, "hwloc obj %s has no children\n", root_str);
      goto lib_err_with_topology;
    }
  }

  /* bind future threads to root */
  leaf_type = with_hyperthreading ? HWLOC_OBJ_PU:HWLOC_OBJ_CORE;
  
#if defined(_OPENMP)
  if(with_hyperthreading)
    n_threads = hwloc_get_nbobjs_inside_cpuset_by_type(topology, root->cpuset, HWLOC_OBJ_PU);
  else
    n_threads = hwloc_get_nbobjs_inside_cpuset_by_type(topology, root->cpuset, HWLOC_OBJ_CORE);	
  omp_set_num_threads(n_threads);
#endif
  
  /* get first cache linesize */
  hwloc_obj_t L1 = roofline_hwloc_get_next_memory(NULL);
  if(L1==NULL) ERR_EXIT("No cache found.\n");
  if(!roofline_hwloc_objtype_is_cache(L1->type) || 
     (L1->attr->cache.type != HWLOC_OBJ_CACHE_UNIFIED && 
      L1->attr->cache.type != HWLOC_OBJ_CACHE_DATA)){
    ERR_EXIT("First memory obj is not a data cache.\n");
  }
  alignement = L1->attr->cache.linesize;

  /* Save some other topological variables for later use */  
  NUMA_domain_depth = roofline_hwloc_NUMA_domain(0)->depth;
  n_parent_nodes    = roofline_hwloc_nb_parent_objs_by_depth(NUMA_domain_depth);

  /* Set allocation policy */
  policy = p;
  
  /* Check if cpu frequency has been defined */
#ifndef CPU_FREQ
  cpu_freq_str = getenv("CPU_FREQ");
  if(cpu_freq_str == NULL)
    ERR_EXIT("Please define the machine cpu frequency (in Hertz) with build option CPU_FREQ or the environement variable of the same name");
  cpu_freq = atof(cpu_freq_str);
#else 
  cpu_freq = CPU_FREQ;
#endif 

  return 0;

lib_err_with_topology:
  hwloc_topology_destroy(topology);
  return -1;
}

inline void roofline_lib_finalize(void)
{
  hwloc_topology_destroy(topology);
}


static unsigned roofline_get_nb_outputs(){
  if(root->depth>=NUMA_domain_depth){ return 1; }
  return roofline_hwloc_nb_parent_objs_by_depth(NUMA_domain_depth);
}

static roofline_output roofline_get_local_output(roofline_output outputs){
  roofline_output ret = outputs;
#pragma omp critical
  {
    if(root->depth<NUMA_domain_depth){
      hwloc_obj_t local_domain = roofline_hwloc_local_domain();  
      ret = outputs+local_domain->logical_index;
    } else { goto return_self; }
  return_self:;
  }
  return ret;
}

static roofline_output roofline_setup_output(){
  unsigned i, n = roofline_get_nb_outputs(); //number of numanodes in root
  roofline_output out;
  roofline_alloc(out, sizeof(struct roofline_output_s)*n);
  for(i=0;i<n; i++){
    roofline_output_clear(out+i);
    out[i].mem_location = NULL;
    out[i].thr_location = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NUMANODE, i);      
  }
  return out;
}

static void roofline_print_outputs(FILE * output, const roofline_output outputs, const int op_type){
  unsigned j;
  for(j=0; j<roofline_get_nb_outputs(); j++){
    outputs[j].n--;
    roofline_output_print(output, outputs+j, op_type);
  }
}

static void roofline_delete_outputs(const roofline_output outputs){ free(outputs); }

void roofline_fpeak(FILE * output, int op_type)
{
  roofline_output out = roofline_setup_output();
  long repeat = roofline_autoset_repeat(NULL, op_type, NULL);

#ifdef _OPENMP
#pragma omp parallel
  {
#endif  
    int i;
    hwloc_obj_t binding = roofline_hwloc_cpubind(leaf_type);
    roofline_output local_out  = new_roofline_output(binding, NULL);
    roofline_output global_out = roofline_get_local_output(out);
    
    for(i=0; i<ROOFLINE_N_SAMPLES; i++){
      roofline_output_clear(global_out);
      roofline_output_clear(local_out);      
      benchmark_fpeak(op_type, local_out, repeat);
      
#ifdef _OPENMP
#pragma omp barrier
#pragma omp critical
#endif
      roofline_output_accumulate(global_out, local_out);
#ifdef _OPENMP
#pragma omp barrier
#pragma omp single	
#endif
      roofline_print_outputs(output, out, op_type);
    }
    roofline_delete_outputs(local_out);
#ifdef _OPENMP    
  }
#endif
  roofline_delete_outputs(out);
}

static void roofline_memory(FILE * output, const hwloc_obj_t memory, const int op_type, void * benchmark)
{
  size_t * sizes, low_size, up_size;
  roofline_output out = roofline_setup_output();
  long repeat = 100;
  void (*  benchmark_function)(roofline_stream, roofline_output, int, long) = benchmark;
  static int stop = 0;
  unsigned n_leaves = hwloc_get_nbobjs_inside_cpuset_by_type(topology, memory->cpuset, leaf_type);
  n_leaves = roofline_MIN(n_leaves, n_threads);
  if(memory->depth<=NUMA_domain_depth){ n_leaves = roofline_MAX(n_leaves, n_threads); }
    
  /* Generate samples size */
  int n_sizes = ROOFLINE_N_SAMPLES;
  if(roofline_hwloc_get_memory_bounds(memory, &low_size, &up_size, op_type) == -1){ return; }
  up_size = up_size/n_leaves;
  low_size = low_size/n_leaves;  
  sizes = roofline_linear_sizes(op_type, low_size, up_size, &n_sizes);
  if(sizes==NULL){roofline_debug2("Computing array of input sizes from %lu to %lu failed\n", low_size, up_size); return;}
#ifdef DEBUG2
  roofline_debug2("linear array of sizes from %lu to %lu of %d sizes\n", low_size, up_size, n_sizes);
  do{int i;
    for(i=0;i<n_sizes;i++){roofline_debug2("%lu ", sizes[i]);}
    roofline_debug2("\n");
  } while(0);
#endif
  
#ifdef _OPENMP
#pragma omp parallel
  {
#endif

    /* Bind the threads */
    hwloc_obj_t thr_loc = roofline_hwloc_cpubind(leaf_type);
    if(thr_loc == NULL){ ERR("Thread binding failed"); stop = 1; }
    
#ifdef _OPENMP
#pragma omp barrier
#endif
    if(stop) goto end_parallel_mem_bench;

    /*Initialize IO */    
    roofline_stream src        = new_roofline_stream(up_size, op_type);
    hwloc_obj_t mem_loc        = roofline_hwloc_set_area_membind(memory, src->stream, src->alloc_size, policy);
    roofline_output local_out  = new_roofline_output(thr_loc, memory->depth<NUMA_domain_depth?mem_loc:memory);
    roofline_output global_out = roofline_get_local_output(out);
    
    /* measure for several sizes inside bounds */
    int i; for(i=0;i<n_sizes;i++){      
      /* Set buffer size */
      if(sizes[i] < low_size || (i>0 && sizes[i] == sizes[i-1])){ goto skip_size; }    
      src->size = sizes[i];
      if(op_type == ROOFLINE_LATENCY_LOAD){roofline_set_latency_stream(src, src->size);}

#ifdef _OPENMP
#pragma omp single
      {
#endif
#ifdef DEBUG2
	
	if(sizes[i]>GB)
	  roofline_debug2("size = %luGB per thread, %luGB total per %s of size %luGB\n",
			  sizes[i]/GB,
			  sizes[i]*n_leaves/GB,
			  hwloc_obj_type_string(memory->type),
			  roofline_hwloc_get_memory_size(memory)/GB);
	else if(sizes[i]>MB)
	  roofline_debug2("size = %luMB per thread, %luMB total per %s of size %luMB\n",
			  sizes[i]/MB,
			  sizes[i]*n_leaves/MB,
			  hwloc_obj_type_string(memory->type),
			  roofline_hwloc_get_memory_size(memory)/MB);	
	else if(sizes[i]>KB)
	  roofline_debug2("size = %luKB per thread, %luKB total per %s of size %luKB\n",
			  sizes[i]/KB,
			  sizes[i]*n_leaves/KB,
			  hwloc_obj_type_string(memory->type),
			  roofline_hwloc_get_memory_size(memory)/KB);
	else
	  roofline_debug2("size = %luB per thread, %luB total per %s of size %luB\n",
			  sizes[i],
			  sizes[i]*n_leaves,
			  hwloc_obj_type_string(memory->type),
			  roofline_hwloc_get_memory_size(memory));
#endif
#ifdef _OPENMP
	
      }
#endif
	
      /* Set the length of the benchmark to have a small variance */
      if(repeat>1){repeat = roofline_autoset_repeat(src, op_type, benchmark);}

      /* Clear output */
      roofline_output_clear(local_out);    
      roofline_output_clear(global_out);

      /* Benchmark */
      if(op_type == ROOFLINE_LOAD    || op_type == ROOFLINE_LOAD_NT ||
	 op_type == ROOFLINE_STORE   || op_type == ROOFLINE_STORE_NT||
	 op_type == ROOFLINE_2LD1ST)
      {
	benchmark_stream(src, local_out, op_type, repeat);
      } else if(op_type == ROOFLINE_LATENCY_LOAD){
	roofline_latency_stream_load(src, local_out, 0, repeat);
      } else if(benchmark != NULL){
	benchmark_function(src, local_out, op_type, repeat);
      }
     
#ifdef _OPENMP
#ifdef DEBUG2
      roofline_output_print(output, local_out, op_type);      
#endif      
#pragma omp critical
#endif
      roofline_output_accumulate(global_out, local_out);
#ifdef _OPENMP      
#pragma omp barrier
#pragma omp single
#endif
      roofline_print_outputs(output, out, op_type);
      
    skip_size:;
    }

  end_parallel_mem_bench:
    delete_roofline_stream(src);
    delete_roofline_output(local_out);    
#ifdef _OPENMP      
  }
#endif
  /* Cleanup */
  roofline_delete_outputs(out);
}


void roofline_bandwidth(FILE * output, const hwloc_obj_t mem, const int type){
  char mem_str[128]; memset(mem_str, 0, sizeof(mem_str));
  roofline_hwloc_obj_snprintf(mem,mem_str,sizeof(mem_str));
  
  int restrict_type = roofline_filter_types(mem, type);
  if(restrict_type & ROOFLINE_LOAD){
    printf("benchmark %s memory level for %-4s operation\n", mem_str, roofline_type_str(ROOFLINE_LOAD));
    roofline_memory(output,mem,ROOFLINE_LOAD, NULL);
  }
  if(restrict_type & ROOFLINE_LOAD_NT){
    printf("benchmark %s memory level for %-4s operation\n", mem_str, roofline_type_str(ROOFLINE_LOAD_NT));
    roofline_memory(output,mem,ROOFLINE_LOAD_NT, NULL);
  }
  if(restrict_type & ROOFLINE_STORE){
    printf("benchmark %s memory level for %-4s operation\n", mem_str, roofline_type_str(ROOFLINE_STORE));
    roofline_memory(output,mem,ROOFLINE_STORE, NULL);
  }
  if(restrict_type & ROOFLINE_STORE_NT){
    printf("benchmark %s memory level for %-4s operation\n", mem_str, roofline_type_str(ROOFLINE_STORE_NT));
    roofline_memory(output,mem,ROOFLINE_STORE_NT, NULL);
  }
  if(restrict_type & ROOFLINE_2LD1ST){
    printf("benchmark %s memory level for %-4s operation\n", mem_str, roofline_type_str(ROOFLINE_2LD1ST));
    roofline_memory(output,mem,ROOFLINE_2LD1ST, NULL);
  }
  if(restrict_type & ROOFLINE_LATENCY_LOAD){
    printf("benchmark %s memory level for %-4s operation\n", mem_str, roofline_type_str(ROOFLINE_LATENCY_LOAD));
    roofline_memory(output,mem,ROOFLINE_LATENCY_LOAD, NULL);
  }  
}

void roofline_flops(FILE * output, const int type){
  hwloc_obj_t core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, 0);
  int restrict_type = roofline_filter_types(core, type);   
  if(restrict_type & ROOFLINE_ADD){
    printf("benchmark compute unit for %-8s operation\n", roofline_type_str(ROOFLINE_ADD));
    roofline_fpeak(output,ROOFLINE_ADD);
  }
  if(restrict_type & ROOFLINE_MUL){
    printf("benchmark compute unit for %-8s operation\n", roofline_type_str(ROOFLINE_MUL));
    roofline_fpeak(output,ROOFLINE_MUL);
  }
  if(restrict_type & ROOFLINE_MAD){
    printf("benchmark compute unit for %-8s operation\n", roofline_type_str(ROOFLINE_MAD));
    roofline_fpeak(output,ROOFLINE_MAD);
  }
  if(restrict_type & ROOFLINE_FMA){
    printf("benchmark compute unit for %-8s operation\n", roofline_type_str(ROOFLINE_FMA));
    roofline_fpeak(output,ROOFLINE_FMA);
  }
}

void roofline_oi(FILE * output, const hwloc_obj_t mem, const int type, const unsigned flops, const unsigned bytes){
  void * bench;
  int i, j, mem_type, flop_type, t;
  hwloc_obj_t core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, 0);
  const int mem_types[6] = {ROOFLINE_LOAD, ROOFLINE_LOAD_NT, ROOFLINE_STORE, ROOFLINE_STORE_NT, ROOFLINE_2LD1ST};
  const int flop_types[4] = {ROOFLINE_ADD, ROOFLINE_MUL, ROOFLINE_MAD, ROOFLINE_FMA};
  char mem_str[128]; memset(mem_str, 0, sizeof(mem_str));
  roofline_hwloc_obj_snprintf(mem,mem_str,sizeof(mem_str));

  flop_type = roofline_filter_types(core, type);
  mem_type = roofline_filter_types(mem, type);
     
  for(i=0;i<6;i++){
    if(mem_types[i] & mem_type){
      for(j=3;j>=0;j--){
	if(flop_types[j] & flop_type){
	  printf("validation of %-4s(%uFlops) and %-8s(%uBytes) operations on %s memory level\n",
		 roofline_type_str(flop_types[j]), flops, roofline_type_str(mem_types[i]), bytes, mem_str);
	  t = flop_types[j]|mem_types[i];
	  bench = benchmark_validation(t, flops, bytes);
	  if(bench == NULL){continue;}
	  roofline_memory(output, mem, t, bench);
	  break;
	}
      }
    }
  }
}

