#include <math.h>
#include "roofline.h"
#include "topology.h"
#include "utils.h"
#include "output.h"
#include "stream.h"
#include "types.h"

/* options */
static char *        output = NULL;      /* Where to print output */
static char *        mem_str = NULL;     /* Do we restrict memory to some memories */
static hwloc_obj_t * mem = NULL;         /* Do we restrict memory to some memories */
static unsigned      n_mem = 0;          /* The number of memories to benchmark */
static int           hyperthreading = 0; /* If compiled with openmp do we use hyperthreading */
static double        oi = 0;             /* -1 => perform validation benchmarks, >0 => perform validation benchmarks on value */
static unsigned int  roofline_types = 0; /* What rooflines do we want in byte array */
static char *        thread_location = "Node:0"; /* Threads location */
static int           matrix = 0;         /* Do we benchmark matrix ? */
static LARM_policy   policy = LARM_FIRSTTOUCH;

extern unsigned      NUMA_domain_depth; //defined in roofline.c, gives the depth of HWLOC_OBJ_NUMANODE

static void usage(char * argv0){
  printf("%s <options...>\n\n", argv0);
  printf("OPTIONS:\n\t");
  printf("-h, --help: print this help message\n\t");
  printf("-v, --validate: perform roofline validation checking. This will run benchmark for several operational intensity trying to hit the roofs.\n\t");
  printf("-oi, --operational-intensity: perform roofline validation checking on set operational intensity.\n\t");
  printf("-t, --type  <\"LOAD|LOAD_NT|STORE|STORE_NT|2LD1ST|MUL|ADD|MAD\">: choose the roofline types among load, load_nt, store, store_nt, or 2loads/1store for memory, add, mul, and fma for fpeak.\n\t");
  printf("-m, --memory <hwloc_obj:id|hwloc_obj:id|...>: benchmark a single or several memory level(s).\n\t");
  printf("--CARM: Build the Cache Aware Roofline Model.\n\t");
  printf("-s, --src <hwloc_ibj:idx>: Build the model with threads at leaves of src obj. Default is Node:0.\n\t");
  printf("-mat, --matrix: Benchmark bandwidth matrix at --src level.\n\t");
  printf("-o, --output <output>: Set output file to write results.\n\t");
  printf("-p, --policy <policy>: Set the allocation policy when target memory scope more than one node:.\n\t\t");
  printf("firsttouch: bind data near threads.\n\t\t");
  printf("interleave: bind data round robin on nodes.\n\t\t");
  printf("firsttouch_HBM: firsttouch on HBM nodes.\n\t\t");
  printf("interleave_DDR: interleave on DDR nodes.\n\t\t");
  printf("interleave_HBM: interleave on HBM nodes.\n\t\t");  
#if defined(_OPENMP)
  printf("\t-ht, --with-hyperthreading: use hyperthreading for benchmarks\n");
#endif    
  exit(EXIT_SUCCESS);
}

static void parse_type(char * type){
  char * t = strtok(type,"|");
  while(t!=NULL){
    roofline_types = (roofline_types | roofline_type_from_str(t));
    t = strtok(NULL,"|");
  }
}

static void parse_args(int argc, char ** argv){
  int i;

  i= 0;
  while(++i < argc){
    if(!strcmp(argv[i],"--help") || !strcmp(argv[i],"-h"))
      usage(argv[0]);
    else if(!strcmp(argv[i],"--validate") || !strcmp(argv[i],"-v"))
      oi = -1;
    else if(!strcmp(argv[i],"--operational-intensity") || !strcmp(argv[i],"-oi"))
      oi = atof(argv[++i]);
    else if(!strcmp(argv[i],"--type") || !strcmp(argv[i],"-t")){
      parse_type(argv[++i]);
    }
#if defined(_OPENMP)	
    else if(!strcmp(argv[i],"--with-hyperthreading") || !strcmp(argv[i],"-ht"))
      hyperthreading = 1;
#endif
    else if(!strcmp(argv[i],"--memory") || !strcmp(argv[i],"-m")){
      mem_str = argv[++i];
    }
    else if(!strcmp(argv[i],"--src") || !strcmp(argv[i],"-s")){
      thread_location = argv[++i];
    }
    else if(!strcmp(argv[i],"--CARM")){
      hyperthreading = 0;
      roofline_types = ROOFLINE_MAD|ROOFLINE_2LD1ST;
      thread_location = "Machine:0";
    }
    else if(!strcmp(argv[i],"--matrix") || !strcmp(argv[i],"-mat")){
      matrix = 1;
    }
    else if(!strcmp(argv[i],"--output") || !strcmp(argv[i],"-o")){
      output = argv[++i];
    }
    else if(!strcmp(argv[i],"--policy") || !strcmp(argv[i],"-p")){
      if(!strcmp(argv[i+1], "firsttouch")){ policy = LARM_FIRSTTOUCH; }
      else if(!strcmp(argv[i+1], "interleave")){ policy = LARM_INTERLEAVE; }
      else if(!strcmp(argv[i+1], "firsttouch_HBM")){ policy = LARM_FIRSTTOUCH_HBM; }
      else if(!strcmp(argv[i+1], "interleave_DDR")){ policy = LARM_INTERLEAVE_DDR; }
      else if(!strcmp(argv[i+1], "interleave_HBM")){ policy = LARM_INTERLEAVE_HBM; }      
      else{ fprintf(stderr, "Unrecognized policy.\n"); }
      ++i;
    }
    else ERR_EXIT("Unrecognized option: %s\n", argv[i]);
  }
}

static void bench_memory(FILE * out, hwloc_obj_t mem){
  int mem_type = 0, flop_type = 0;
  hwloc_obj_t core = hwloc_get_obj_by_depth(topology, hwloc_topology_get_depth(topology)-1, 0);
  
  if(roofline_types == 0){
    mem_type = roofline_default_types(mem);
    flop_type = roofline_default_types(core);
  }
  else{
    mem_type = roofline_filter_types(mem, roofline_types);
    flop_type = roofline_filter_types(core, roofline_types);
  }
  
  unsigned flops = 1, bytes = 8*sizeof(roofline_stream_t);
  
  if(mem_type != 0){
    if(oi<=0){ roofline_bandwidth(out, mem, mem_type); }
    if(oi<0){
      for(bytes = 32; bytes > 1; bytes -= 2){roofline_oi(out, mem, mem_type|flop_type, 1, bytes);}
      for(flops=1; flops <= 13; flops++){roofline_oi(out, mem, mem_type|flop_type, flops, 1);}
    } else if(oi > 0){
      if(oi < 1){bytes = 1/oi;}
      flops = round(bytes*oi);
      roofline_oi(out, mem, mem_type|flop_type, flops, bytes);
    }
  }
}

static FILE * open_output(char * out){
  FILE * fout;

  if(out == NULL)
    return NULL;
  fout = fopen(out,"w");
  if(fout == NULL) PERR_EXIT("fopen");
  return fout;
}

int main(int argc, char * argv[]){  
  FILE * out;
  parse_args(argc,argv);

  if(roofline_lib_init(NULL, thread_location, hyperthreading, policy)==-1)
    ERR_EXIT("roofline library init failure\n");

  if(mem_str != NULL){
    char * save_ptr;
    char * item = strtok_r(mem_str, "|", &save_ptr);    
    mem = malloc(sizeof(hwloc_obj_t)*hwloc_get_type_depth(topology, HWLOC_OBJ_PU));
    while(item != NULL){
      mem[n_mem] = roofline_hwloc_parse_obj(item);
      if(mem[n_mem] != NULL && roofline_hwloc_obj_is_memory(mem[n_mem])){n_mem++;}
      item = strtok_r(NULL, "|", &save_ptr);
    }
  }

  out = open_output(output);
  if(out==NULL) out = stdout;

  /* print header */
  roofline_output_print_header(out);

  /* roofline for flops */
  hwloc_obj_t core = hwloc_get_obj_by_depth(topology, hwloc_topology_get_depth(topology)-1, 0);
  unsigned flop_deftype = roofline_default_types(core);
  if(oi <= 0){ roofline_flops(out, roofline_types == 0 ? flop_deftype : roofline_types); }

  /* Benchmark matrix */
  if(matrix){
    hwloc_obj_t dst = NULL;
    hwloc_obj_t src = roofline_hwloc_parse_obj(thread_location);
    if(src == NULL) {
      fprintf(stderr,"Provided source location %s does not match any object in topology\n", thread_location);
      goto exit;
    }
    if(!roofline_hwloc_obj_is_memory(src)){
      src = roofline_hwloc_get_next_memory(src);
    }
    if((int)src->depth > hwloc_get_type_depth(topology, HWLOC_OBJ_NUMANODE)){
      fprintf(stderr,"Cannot build matrix of memory below NUMANode\n");
      goto exit;
    }
    for(src = hwloc_get_obj_by_depth(topology,src->depth,0); src != NULL; src = src->next_cousin){
      root = src;
      if(root->arity == 0 && root->type == HWLOC_OBJ_NUMANODE) continue;
      for(dst = hwloc_get_obj_by_depth(topology,src->depth,0); dst != NULL; dst = dst->next_cousin){
	bench_memory(out, dst);
      }
    }
    goto exit;
  }
        
  /* roofline every memory obj */
  if(mem == NULL){
    hwloc_obj_t memory = NULL;
    while((memory = roofline_hwloc_get_next_memory(memory)) != NULL && memory->depth >= NUMA_domain_depth){
      bench_memory(out, memory);
    }
  }
  else{
    unsigned i;
    for(i=0; i<n_mem; i++){
      bench_memory(out, mem[i]);
    }
    free(mem);
  }

exit:
  fclose(out);
  roofline_lib_finalize();
  return EXIT_SUCCESS;
}


