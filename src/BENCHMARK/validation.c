#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include "intel.h"
#include "../../stats.h"
#include "../../roofline.h"

extern size_t oi_chunk_size;

/***************************************** OI BENCHMARKS GENERATION ******************************************/
#if defined (__AVX__)  || defined (__AVX2__)  ||defined (__AVX512__)
static void dprint_FUOP_by_ins(int fd, const char * op, unsigned * regnum){
  dprintf(fd, "\"%s %%%%%s%d, %%%%%s%d, %%%%%s%d\\n\\t\"\\\n", op, SIMD_REG, *regnum, SIMD_REG, *regnum, SIMD_REG, *regnum);
}
#else 
static void dprint_FUOP_by_ins(int fd, const char * op, unsigned * regnum){
  dprintf(fd, "\"%s %%%%%s%d, %%%%%s%d\\n\\t\"\\\n", op, SIMD_REG, *regnum, SIMD_REG, *regnum);
}
#endif

static void dprint_FUOP(int fd, int type, unsigned * i, unsigned * regnum){
  switch(type){
  case ROOFLINE_ADD:
    dprint_FUOP_by_ins(fd, SIMD_ADD, regnum);
    break;
  case ROOFLINE_MUL:
    dprint_FUOP_by_ins(fd, SIMD_MUL, regnum);
    break;
  case ROOFLINE_MAD:
    if((*i)%2) dprint_FUOP_by_ins(fd, SIMD_MUL, regnum);
    else dprint_FUOP_by_ins(fd, SIMD_ADD, regnum);
    break;
#if defined (__FMA__)
  case ROOFLINE_FMA:
    dprint_FUOP_by_ins(fd, SIMD_FMA, regnum);
    break;
#endif
  default:
    break;
  }
  *regnum = (*regnum+1)%SIMD_N_REGS;
  *i += 1;
}


static void dprint_MUOP(int fd, int type, unsigned *i, off_t * offset, unsigned * regnum){
  switch(type){
  case ROOFLINE_LOAD:
    dprintf(fd, "\"%s %lu(%%%%r11), %%%%%s%d\\n\\t\"\\\n", SIMD_LOAD, *offset, SIMD_REG, *regnum);
    break;
  case ROOFLINE_LOAD_NT:
    dprintf(fd, "\"%s %lu(%%%%r11), %%%%%s%d\\n\\t\"\\\n", SIMD_LOAD_NT, *offset, SIMD_REG, *regnum);
    break;
  case ROOFLINE_STORE:
    dprintf(fd, "\"%s %%%%%s%d, %lu(%%%%r11)\\n\\t\"\\\n", SIMD_STORE, SIMD_REG, *regnum, *offset);
    break;
  case ROOFLINE_STORE_NT:
    dprintf(fd, "\"%s %%%%%s%d, %lu(%%%%r11)\\n\\t\"\\\n", SIMD_STORE_NT, SIMD_REG, *regnum, *offset);
    break;
  case ROOFLINE_2LD1ST:
    if((*i)%3) return dprint_MUOP(fd, ROOFLINE_LOAD, i, offset, regnum);
    else return dprint_MUOP(fd, ROOFLINE_STORE, i, offset, regnum);
    break;
  case ROOFLINE_COPY:
    if((*i)%2) return dprint_MUOP(fd, ROOFLINE_LOAD, i, offset, regnum);
    else return dprint_MUOP(fd, ROOFLINE_STORE, i, offset, regnum);
    break;
  default:
    break;
  }
  
  *offset+=SIMD_BYTES;
  *regnum = (*regnum+1)%(SIMD_N_REGS);
  *i += 1;
}

static void dprint_zero_simd(int fd){
  dprintf(fd, "static void zero_simd(){\n");
  dprintf(fd, "__asm__ __volatile__ (\\\n\t");
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_0));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_1));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_2));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_3));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_4));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_5));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_6));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_7));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_8));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_9));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_10));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_11));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_12));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_13));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_14));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_15));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_16));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_17));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_18));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_19));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_20));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_21));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_22));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_23));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_24));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_25));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_26));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_27));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_28));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_29));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_30));
  dprintf(fd, "\"%s\\n\\t\"\\\n\t", zero_reg(REG_31));
  dprintf(fd,"::: \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"memory\" );\n}\n\n", SIMD_CLOBBERED_REGS);
}

static void  dprint_header(int fd) {
  dprintf(fd, "#include <stdlib.h>\n");
  dprintf(fd, "#include \"stream.h\"\n");
  dprintf(fd, "#include \"output.h\"\n");
  dprintf(fd, "#include \"utils.h\"\n");
  dprint_zero_simd(fd);
}
 
static void dprint_oi_bench_begin(int fd, const char * id, const char * name){
  dprintf(fd, "void %s(roofline_stream data, roofline_output * out, __attribute__ ((unused)) int op_type, long repeat){\n", name);
  dprintf(fd, "volatile uint64_t c_low=0, c_low1=0, c_high=0, c_high1=0;\n");
  dprintf(fd, "zero_simd();\n");
  dprintf(fd, "roofline_rdtsc(c_high,c_low);\n");
  dprintf(fd, "__asm__ __volatile__ (\\\n");
  dprintf(fd, "\"loop_%s_repeat:\\n\\t\"\\\n", id);
  dprintf(fd, "\"mov %%1, %%%%r11\\n\\t\"\\\n");
  dprintf(fd, "\"mov %%2, %%%%r12\\n\\t\"\\\n");
  dprintf(fd, "\"buffer_%s_increment:\\n\\t\"\\\n", id);
}

static void dprint_oi_bench_end(int fd, const char * id, off_t offset){
  dprintf(fd,"\"add $%lu, %%%%r11\\n\\t\"\\\n", offset);
  dprintf(fd,"\"sub $%lu, %%%%r12\\n\\t\"\\\n", offset);
  dprintf(fd,"\"jnz buffer_%s_increment\\n\\t\"\\\n", id);
  dprintf(fd,"\"sub $1, %%0\\n\\t\"\\\n");
  dprintf(fd,"\"jnz loop_%s_repeat\\n\\t\"\\\n", id);
  dprintf(fd,":: \"r\" (repeat), \"r\" (data->stream), \"r\" (data->size)\\\n"); 
  dprintf(fd,": \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"memory\" );\n", SIMD_CLOBBERED_REGS);
  dprintf(fd,"roofline_rdtsc(c_high1,c_low1);\n");
  dprintf(fd, "out->ts_end = roofline_rdtsc_diff(c_high1, c_low1);\n");
  dprintf(fd, "out->ts_start = roofline_rdtsc_diff(c_high, c_low);\n");
}

static int roofline_compile_lib(char * c_path, char* so_path){
  char cmd[2048];
#ifdef DEBUG1
  memset(cmd,0,sizeof(cmd));
  snprintf(cmd, sizeof(cmd), "cat %s", c_path);
  system(cmd);
#endif
  memset(cmd,0,sizeof(cmd));
  sprintf(cmd, "%s -g -fPIC -shared %s -rdynamic -o %s %s", STRINGIFY(CC), c_path, so_path, STRINGIFY(OMP_FLAG));
  return system(cmd);
}

off_t roofline_benchmark_write_oi_bench(int fd, const char * name, int mem_type, int flop_type, unsigned mem_ins, unsigned flop_ins){
  
  off_t offset = 0; size_t len;
  unsigned i, max = 40;
  unsigned muops = 0, fuops = 0, regnum = 0;
  off_t maxsize = 4096;
  char * idx;

  len = 2+strlen(roofline_type_str(mem_type))+strlen(roofline_type_str(flop_type));
  idx = malloc(len); memset(idx,0,len);
  snprintf(idx,len,"%s_%s", roofline_type_str(mem_type), roofline_type_str(flop_type));

  dprint_oi_bench_begin(fd, idx, name);
  do{
    for(i=0; i<mem_ins; i++) dprint_MUOP(fd, mem_type, &muops, &offset, &regnum);
    for(i=0; i<flop_ins; i++) dprint_FUOP(fd, flop_type, &fuops, &regnum);
  } while(regnum != SIMD_N_REGS-1 && offset < maxsize && max--);
  dprint_oi_bench_end(fd, idx, offset);
  
  dprintf(fd, "out->instructions = repeat * data->size * %u / %u;\n", fuops+muops, muops*SIMD_BYTES);
  dprintf(fd, "out->flops = repeat * data->size * %u * %u / %u;\n",
	  flop_ins*SIMD_FLOPS,
	  flop_type == ROOFLINE_FMA ? 2 : 1,
	  mem_ins*SIMD_BYTES);
  dprintf(fd, "out->bytes = repeat * data->size;\n");
  dprintf(fd, "}\n\n");
  free(idx);
  return offset;
}

static void * roofline_load_lib(const char * path, const char * funname){
  void * benchmark, * handle;
    
  dlerror();
  handle = dlopen(path, RTLD_NOW|RTLD_DEEPBIND);
  if(handle == NULL){
    ERR("Failed to load dynamic library %s: %s\n", path, dlerror());
    return NULL;
  }

  dlerror();
  benchmark = dlsym(handle, funname);
  if(benchmark == NULL){
    ERR("Failed to load function %s from %s: %s\n", funname, path, dlerror());
  }
  return benchmark;
}

void * benchmark_validation(int op_type, unsigned flops, unsigned bytes){
  const char * func_name = "roofline_oi_benchmark";
  char tempname[1024], * c_path, * so_path;
  size_t len;
  int fd;
  void * benchmark = NULL;
  
  int mem_type = op_type & (ROOFLINE_LOAD|ROOFLINE_LOAD_NT|ROOFLINE_STORE|ROOFLINE_STORE_NT|ROOFLINE_2LD1ST|ROOFLINE_COPY);
  int flop_type = op_type & (ROOFLINE_MUL|ROOFLINE_ADD|ROOFLINE_MAD|ROOFLINE_FMA);  
  /* Parameters checking */
  if(!mem_type  || ! flop_type){
    ERR("Both memory and compute type must be included in type\n");
    return NULL;
  }
  if(bytes == 0 || flops == 0){
    ERR("bytes and flops cant be 0.\n");
    return NULL;
  }

  /* Convert flops and bytes into instruction number */
  unsigned mem_ins=bytes, flop_ins = flops*SIMD_BYTES/SIMD_FLOPS;
  if(flop_type == ROOFLINE_FMA){mem_ins*=2;}
  unsigned pgcd = roofline_PGCD(flop_ins, mem_ins);
  mem_ins /= pgcd; flop_ins/=pgcd;
  
  roofline_debug2("benchmark validation for %u bytes and %u flops will interleave %u memory instructions with %u flop instructions\n", bytes, flops, mem_ins, flop_ins);
  
  /* Create the filenames */
  snprintf(tempname, sizeof(tempname), "roofline_benchmark_%s_%uB_%s_%uFlops",
	   roofline_type_str(mem_type), bytes,
	   roofline_type_str(flop_type), flops);
  len = strlen(tempname)+3; c_path =  malloc(len); memset(c_path,  0, len); snprintf(c_path, len, "%s.c", tempname);
  len = strlen(tempname)+4; so_path = malloc(len); memset(so_path, 0, len); snprintf(so_path, len, "%s.so", tempname);

  /* Generate benchmark */
  fd = open(c_path, O_WRONLY|O_CREAT, 0644);
  /* Write the appropriate roofline to the file */
  dprint_header(fd);
  oi_chunk_size = roofline_benchmark_write_oi_bench(fd, func_name, mem_type, flop_type, mem_ins, flop_ins);
  /* Compile the roofline function */
  close(fd);
  roofline_compile_lib(c_path, so_path);
  /* Load the roofline function */
  benchmark = roofline_load_lib(so_path, func_name);
    
  unlink(c_path);
  unlink(so_path);
  free(c_path);
  free(so_path);
  return benchmark;
}

