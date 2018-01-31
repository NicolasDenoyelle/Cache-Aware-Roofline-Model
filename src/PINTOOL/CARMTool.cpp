#include "pin.H"
#include <map>
#include "Block.cpp"
#include "Function.cpp"
#include "Thread.cpp"
#include "Results.cpp"

using namespace std;
using namespace CARM;

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "output", "stdout", "Output to specified file");
KNOB<string> KnobInputFile(KNOB_MODE_WRITEONCE, "pintool", "input", "", "Update trace from a specified file");

Results records; //Maintain a backtrace of functions and count associate CARM metrics thread by thread
bool    update;  //True if input has been set

/*************************************** Routine (RTN) instrumentation ***************************************/
map<ADDRINT, CARM::Function>funInfos;        //The list of tracked routines

CARM::Function RTNInfo(RTN rtn){
  ADDRINT addr = RTN_Address(rtn);
  string  name = RTN_Name(rtn);
  int column, line;
  string source;
  
  PIN_GetSourceLocation(addr, &column, &line, &source);  
  string::size_type pos = source.rfind("/", source.length());
  if(pos != string::npos && pos < source.length()){
    source = source.substr(pos+1, source.length());
  }

  return CARM::Function(name, source, line, 0, 0);
}

VOID BeginRTN(ADDRINT addr, THREADID tid){
  Function& f = funInfos[addr];
  records.startFunction(tid, f);
}

VOID EndRTN(ADDRINT addr, THREADID tid){
  records.stopFunction(tid);
}

VOID InstrumentRTN(RTN rtn){
  ADDRINT addr = RTN_Address(rtn);
  CARM::Function fun = RTNInfo(rtn);
  bool instrument = !update || records.hasFunction(fun.name());
    
  if(fun.name()[0] != '.' && fun.source().length()>0 && instrument)
  {
    //printf("Look if we instrument Routine %s\n", fun.name().c_str());
    funInfos.insert(pair<ADDRINT, CARM::Function>(addr, fun));
    LOG("Instrument Routine: " + RTN_Name(rtn) + "\n");
    RTN_Open(rtn);
    RTN_InsertCall(rtn, IPOINT_BEFORE, (AFUNPTR)BeginRTN, IARG_ADDRINT, addr,  IARG_THREAD_ID, IARG_END);
    RTN_InsertCall(rtn, IPOINT_AFTER,  (AFUNPTR)EndRTN, IARG_ADDRINT, addr, IARG_THREAD_ID, IARG_END);  
    RTN_Close(rtn);
  }
}

void InstrumentIMG(IMG img, __attribute__ ((unused)) void* v){
  if(!IMG_Valid(img)){ LOG("Invalid Image provided \n"); return;}
  LOG("Instrument Image: " + IMG_Name(img) + "\n");
  /* Iterate over elf sections to find routines */  
  for( SEC sec= IMG_SecHead(img); SEC_Valid(sec); sec = SEC_Next(sec) ){
    /* Iterate over all routines */   
    for(RTN rtn= SEC_RtnHead(sec); RTN_Valid(rtn); rtn = RTN_Next(rtn)){
	InstrumentRTN(rtn);
    }
  }
}

void InstrumentAPP(__attribute__ ((unused)) VOID* v){ InstrumentIMG(APP_ImgHead(), NULL); }
/***********************************************************************************************************/


/********************************** Thread registeration with creation context *****************************/
PIN_RWMUTEX                 lock;            //Lock for safe multithread access.
map<OS_THREAD_ID, THREADID> osToPinId;       

VOID ThreadStart(THREADID tid,
		  __attribute__ ((unused)) CONTEXT *ctxt,
		  __attribute__ ((unused)) INT32 flags,
		  __attribute__ ((unused)) VOID *v)
{
  PIN_RWMutexWriteLock(&lock);
  OS_THREAD_ID id        = PIN_GetTid();
  OS_THREAD_ID parent_id = PIN_GetParentTid();
  osToPinId[id] = tid;
  records.registerThread(tid, records.getContext(osToPinId[parent_id]));
  PIN_RWMutexUnlock(&lock);
}
/***********************************************************************************************************/

/***************************** Identify and chracterize CARM blocks in TRACES ******************************/
map<block_id, Block>        blocks;          //map Block metrics for each block of the code.

unsigned long INS_FlopCount(INS ins){
  OPCODE op = INS_Opcode(ins);
  if(op == XED_ICLASS_ADDSD  ||
     op == XED_ICLASS_ADDSS  ||
     op == XED_ICLASS_DIVSD  ||
     op == XED_ICLASS_DIVSS  ||
     op == XED_ICLASS_FDIV   ||
     op == XED_ICLASS_FDIVP  ||
     op == XED_ICLASS_FDIVR  ||
     op == XED_ICLASS_FDIVRP ||
     op == XED_ICLASS_FIADD  ||
     op == XED_ICLASS_FIDIV  ||
     op == XED_ICLASS_FIDIVR ||
     op == XED_ICLASS_FIMUL  ||
     op == XED_ICLASS_FISUB  ||
     op == XED_ICLASS_FISUBR ||
     op == XED_ICLASS_FMUL   ||
     op == XED_ICLASS_FMULP  ||
     op == XED_ICLASS_FSUB   ||
     op == XED_ICLASS_FSUBR  ||
     op == XED_ICLASS_FSUBRP ||
     op == XED_ICLASS_MULSD  ||
     op == XED_ICLASS_MULSS  ||
     op == XED_ICLASS_SUBSD  ||
     op == XED_ICLASS_SUBSS)
  { return 1; }
  else if(op == XED_ICLASS_ADDPD     ||
	  op == XED_ICLASS_ADDPS     ||
	  op == XED_ICLASS_ADDSUBPD  ||
	  op == XED_ICLASS_ADDSUBPS  ||
	  op == XED_ICLASS_DIVPD     ||
	  op == XED_ICLASS_DIVPS     ||
	  op == XED_ICLASS_HADDPD    ||
	  op == XED_ICLASS_HADDPS    ||
	  op == XED_ICLASS_HSUBPD    ||
	  op == XED_ICLASS_HSUBPS    ||
	  op == XED_ICLASS_MULPD     ||
	  op == XED_ICLASS_MULPS     ||
	  op == XED_ICLASS_SUBPD     ||
	  op == XED_ICLASS_SUBPS)
  { return 2; }
  else if(op == XED_ICLASS_VADDPD      ||
	  op == XED_ICLASS_VADDPS      ||
	  op == XED_ICLASS_VADDSD      ||
	  op == XED_ICLASS_VADDSS      ||
	  op == XED_ICLASS_VDIVPD      ||
	  op == XED_ICLASS_VDIVPS      ||
	  op == XED_ICLASS_VDIVSD      ||
	  op == XED_ICLASS_VDIVSS      ||
	  op == XED_ICLASS_VMULPD      ||
	  op == XED_ICLASS_VMULPS      ||
	  op == XED_ICLASS_VMULSD      ||
	  op == XED_ICLASS_VMULSS      ||
	  op == XED_ICLASS_VSUBPD      ||
	  op == XED_ICLASS_VSUBPS      ||
	  op == XED_ICLASS_VSUBSD      ||
	  op == XED_ICLASS_VSUBSS)
  { return 4; }
  else if(op == XED_ICLASS_VFMADD132PD    ||
	  op == XED_ICLASS_VFMADD132PS    ||
	  op == XED_ICLASS_VFMADD132SD    ||
	  op == XED_ICLASS_VFMADD132SS    ||
	  op == XED_ICLASS_VFMADD213PD    ||
	  op == XED_ICLASS_VFMADD213PS    ||
	  op == XED_ICLASS_VFMADD213SD    ||
	  op == XED_ICLASS_VFMADD213SS    ||
	  op == XED_ICLASS_VFMADD231PD    ||
	  op == XED_ICLASS_VFMADD231PS    ||
	  op == XED_ICLASS_VFMADD231SD    ||
	  op == XED_ICLASS_VFMADD231SS    ||
	  op == XED_ICLASS_VFMADDPD       ||
	  op == XED_ICLASS_VFMADDPS       ||
	  op == XED_ICLASS_VFMADDSD       ||
	  op == XED_ICLASS_VFMADDSS       ||
	  op == XED_ICLASS_VFMADDSUB132PD ||
	  op == XED_ICLASS_VFMADDSUB132PS ||
	  op == XED_ICLASS_VFMADDSUB213PD ||
	  op == XED_ICLASS_VFMADDSUB213PS ||
	  op == XED_ICLASS_VFMADDSUB231PD ||
	  op == XED_ICLASS_VFMADDSUB231PS ||
	  op == XED_ICLASS_VFMADDSUBPD    ||
	  op == XED_ICLASS_VFMADDSUBPS    ||
	  op == XED_ICLASS_VFMSUB132PD    ||
	  op == XED_ICLASS_VFMSUB132PS    ||
	  op == XED_ICLASS_VFMSUB132SD    ||
	  op == XED_ICLASS_VFMSUB132SS    ||
	  op == XED_ICLASS_VFMSUB213PD    ||
	  op == XED_ICLASS_VFMSUB213PS    ||
	  op == XED_ICLASS_VFMSUB213SD    ||
	  op == XED_ICLASS_VFMSUB213SS    ||
	  op == XED_ICLASS_VFMSUB231PD    ||
	  op == XED_ICLASS_VFMSUB231PS    ||
	  op == XED_ICLASS_VFMSUB231SD    ||
	  op == XED_ICLASS_VFMSUB231SS    ||
	  op == XED_ICLASS_VFMSUBADD132PD ||
	  op == XED_ICLASS_VFMSUBADD132PS ||
	  op == XED_ICLASS_VFMSUBADD213PD ||
	  op == XED_ICLASS_VFMSUBADD213PS ||
	  op == XED_ICLASS_VFMSUBADD231PD ||
	  op == XED_ICLASS_VFMSUBADD231PS ||
	  op == XED_ICLASS_VFMSUBADDPD    ||
	  op == XED_ICLASS_VFMSUBADDPS    ||
	  op == XED_ICLASS_VFMSUBPD       ||
	  op == XED_ICLASS_VFMSUBPS       ||
	  op == XED_ICLASS_VFMSUBSD       ||
	  op == XED_ICLASS_VFMSUBSS)
  {
    unsigned long flops = 4;
    REG reg = INS_XedExactMapToPinReg(0);
    if(REG_is_ymm(reg)){ flops *= 2; }
    if(REG_is_zmm(reg)){ flops *= 4; }    
    return flops;
  }
  else if(op == XED_ICLASS_VFNMADD132PD ||
	  op == XED_ICLASS_VFNMADD132PS ||
	  op == XED_ICLASS_VFNMADD132SD ||
	  op == XED_ICLASS_VFNMADD132SS ||
	  op == XED_ICLASS_VFNMADD213PD ||
	  op == XED_ICLASS_VFNMADD213PS ||
	  op == XED_ICLASS_VFNMADD213SD ||
	  op == XED_ICLASS_VFNMADD213SS ||
	  op == XED_ICLASS_VFNMADD231PD ||
	  op == XED_ICLASS_VFNMADD231PS ||
	  op == XED_ICLASS_VFNMADD231SD ||
	  op == XED_ICLASS_VFNMADD231SS ||
	  op == XED_ICLASS_VFNMADDPD    ||
	  op == XED_ICLASS_VFNMADDPS    ||
	  op == XED_ICLASS_VFNMADDSD    ||
	  op == XED_ICLASS_VFNMADDSS    ||
	  op == XED_ICLASS_VFNMSUB132PD ||
	  op == XED_ICLASS_VFNMSUB132PS ||
	  op == XED_ICLASS_VFNMSUB132SD ||
	  op == XED_ICLASS_VFNMSUB132SS ||
	  op == XED_ICLASS_VFNMSUB213PD ||
	  op == XED_ICLASS_VFNMSUB213PS ||
	  op == XED_ICLASS_VFNMSUB213SD ||
	  op == XED_ICLASS_VFNMSUB213SS ||
	  op == XED_ICLASS_VFNMSUB231PD ||
	  op == XED_ICLASS_VFNMSUB231PS ||
	  op == XED_ICLASS_VFNMSUB231SD ||
	  op == XED_ICLASS_VFNMSUB231SS ||
	  op == XED_ICLASS_VFNMSUBPD    ||
	  op == XED_ICLASS_VFNMSUBPS    ||
	  op == XED_ICLASS_VFNMSUBSD    ||
	  op == XED_ICLASS_VFNMSUBSS)
  {
    unsigned long flops = 3;
    REG reg = INS_XedExactMapToPinReg(0);
    if(REG_is_ymm(reg)){ flops *= 2; }
    if(REG_is_zmm(reg)){ flops *= 4; }    
    return flops;
  }
  //TODO: handle dppd instruction.
  return 0;
}

void CARM_ExtractINS(INS ins, CARM::Block& metrics){
  if(INS_IsMemoryRead(ins)){
    metrics.countRead(1, INS_MemoryReadSize(ins));
  } else if(INS_IsMemoryWrite(ins)){
    metrics.countWrite(1, INS_MemoryWriteSize(ins));
  } else {
    metrics.countFlops(INS_FlopCount(ins));
  }
}


Block& getBlock(const block_id &key){
  PIN_RWMutexReadLock(&lock);
  std::map<block_id, Block>::iterator found = blocks.find(key);
  PIN_RWMutexUnlock(&lock);
  
  if(found != blocks.end()){
    return found->second;
  } else {
    PIN_RWMutexWriteLock(&lock);
    std::pair<std::map<block_id,Block>::iterator,bool> inserted = blocks.insert(std::pair<block_id,Block>(key,Block(key)));
    PIN_RWMutexUnlock(&lock);
    return inserted.first->second;	
  }
}

void CountBlock(THREADID tid, ADDRINT begin, ADDRINT end){
  PIN_RWMutexReadLock(&lock);
  records.countBlock(tid, blocks[CARM::block_id(begin, end)]);
  PIN_RWMutexUnlock(&lock);
}

void InstrumentTRACE(TRACE trace, __attribute__ ((unused)) VOID* v){
  if(! TRACE_Original(trace)){return;} //Avoid to instrument twice
  
  //Build the key and block
  ADDRINT head = INS_Address(BBL_InsHead(TRACE_BblHead(trace)));
  ADDRINT tail = INS_Address(BBL_InsTail(TRACE_BblTail(trace)));
  CARM::block_id key(head,tail);
  CARM::Block& block = getBlock(key);
  
  //Update block with CARM metrics in the trace
  for(BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl)){
    //Count flops and bytes of this block  
    UINT32 numIns = BBL_NumIns(bbl);
    INS ins = BBL_InsHead(bbl);
    do{
      CARM_ExtractINS(ins, block);
      ins = INS_Next(ins);
    } while(--numIns);  
  }

  //Instrument Trace to count update
  if(block.bytes() > 0 || block.flops() > 0){
    TRACE_InsertCall(trace, IPOINT_BEFORE, (AFUNPTR)CountBlock, IARG_THREAD_ID, IARG_ADDRINT, head, IARG_ADDRINT, tail, IARG_END);
  }
}

/***********************************************************************************************************/

void Cleanup(__attribute__ ((unused)) INT32 Code, __attribute__ ((unused)) VOID* arg){
  PIN_RWMutexFini(&lock);
  FILE * output = stdout;
  if(KnobOutputFile.Value() != string("stdout")){ output = fopen(KnobOutputFile.Value().c_str(), "w"); }
  records.print(output);
  fclose(output);
}

int main(int argc, char *argv[])
{
  if (PIN_Init(argc,argv)) { fprintf(stderr,"Error detected while parsing command line\n");}
  if(KnobInputFile.Value().length() == 0){ update = false; } else {
    update = true;
    FILE* input = fopen(KnobInputFile.Value().c_str(), "r");
    if(input == NULL){
      perror("fopen");
    } else {
      records.read(input);
      fclose(input);
    }
  }
  PIN_InitSymbols();
  PIN_RWMutexInit(&lock);
  IMG_AddInstrumentFunction(InstrumentIMG,NULL);
  TRACE_AddInstrumentFunction(InstrumentTRACE, NULL);    
  PIN_AddThreadStartFunction((THREAD_START_CALLBACK)ThreadStart, NULL);  
  PIN_AddFiniFunction(Cleanup, NULL);
  PIN_StartProgram();
}

