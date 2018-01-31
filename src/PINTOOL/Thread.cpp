#include "Thread.hpp"
#include <cstring>
#include <algorithm>

using namespace std;
using namespace CARM;

Thread::Thread():_tid(0), _accumulator(){}

Thread::Thread(const int &tid, Function* context):_tid(tid), _accumulator(){ _backtrace.push_back(context); }

Thread::Thread(FILE * f):_accumulator(){
  //set default fields
  _tid = -1;
  
  //scan until tid changes or EOF
  int n = 0;
  char fline[512]; memset(fline, 0, sizeof(fline));
  char name[512];
  char source[512];
  int line;
  int tid;
  unsigned node, cpu;
  unsigned long flops, read, write, readB, writeB;
  unsigned long usec;

  while(fgets(fline, sizeof(fline), f) != NULL){
    //skip header    
    if(fline[0] == '#'){continue;}
    //set default values
    memset(name, 0, sizeof(name));
    memset(source, 0, sizeof(name));
    line = 0;
    tid = _tid;
    read = write = readB = writeB = flops = usec = 0;
    node=0;
    cpu=0;

    n = sscanf(fline, "%d %u %u %lu %lu %lu %lu %lu %lu %s %d %s\n",
	       &tid, &cpu, &node, &flops, &read, &write, &readB, &writeB, &usec, source, &line, name);
    
    if(n != 12){ break; }
    //Roll back to the beginning of the line of a new different thread.
    if(_tid>=0 && tid != _tid){ size_t len = strlen(fline); while(len--){ungetc(fline[len], f);} break; }
    _tid = tid;
    Function fun(name, source, line, cpu, node);    
    fun += Block(flops,read, write, readB, writeB, usec);
    _records.insert(make_pair(string(name), fun));
    memset(fline, 0, sizeof(fline));
  }
}

void Thread::setContext(Function* context){
  _backtrace.clear();
  _backtrace.push_back(context);
}


int Thread::tid() const{ return _tid; }

Function* Thread::current(){
  if(_backtrace.empty()){return NULL;}
  else{return _backtrace.back();}
}

void Thread::insertFunction(const Function& fun){
  map<string, Function>::iterator found = _records.find(fun.name());
  if(found != _records.end()){
    found->second += fun;
  }
  else{
    _records.insert(pair<string, Function>(fun.name(), fun));
  }
};


Function* Thread::getFunction(const string& name){
  map<string, Function>::iterator found = _records.find(name);
  if(found == _records.end()){ return NULL; }
  return &(found->second);
}

void Thread::beginFunction(const Function& fun){
  //forward updates in upper function
  for(auto it=_backtrace.begin(); it!=_backtrace.end(); it++){ if(*it != NULL) **it += _accumulator; }
  //reset accumulator for current function
  _accumulator.reset();
  //update backtrace with new function
  map<string, Function>::iterator found = _records.find(fun.name());
  if(found != _records.end()){
    found->second.set(fun);
//    found->second.print();
    found->second.update_cpu_infos();
    _backtrace.push_back(&found->second);
  }
  else{
    pair<map<string, Function>::iterator, bool> inserted = _records.insert(pair<string, Function>(fun.name(), fun));
    inserted.first->second.update_cpu_infos();
    _backtrace.push_back(&inserted.first->second);
  }
}

void Thread::countBlock(const Block& b){ _accumulator += b; }

void Thread::endFunction(){
  if(!_backtrace.empty() && _backtrace.back() != NULL){
    for(auto it=_backtrace.begin(); it!=_backtrace.end(); it++){ if(*it!=NULL) **it += _accumulator; }
    _accumulator.reset();
    _backtrace.pop_back();
  }
}

void Thread::header(FILE * output) const{
  fprintf(output, "#%s %s %s %s %s %s %s %s %s %s %s %s\n",
	  "tid",
	  "cpu",
	  "node",
	  "flops",
 	  "read",
	  "write",
	  "read_bytes",
	  "write_bytes",
	  "usec",
	  "source",
	  "line",
	  "function");
}

void Thread::print(FILE * output) const{
  list<string> fnames = sort_by_time();
  while(!fnames.empty()){
    map<string, Function>::const_iterator found = _records.find(fnames.front());
    const Function& fun = found->second;

    //Skip functions with no block information.
    if(fun.flops() == 0      &&
       fun.read() == 0       &&
       fun.write() == 0      &&
       fun.readBytes() == 0  &&
       fun.writeBytes() == 0 &&
       fun.usec() == 0){ fnames.pop_front(); continue; }

    fprintf(output, "%u %u %u %lu %lu %lu %lu %lu %lu %s %d %s\n",
	    _tid,
	    fun.cpu(),
	    fun.node(),
	    fun.flops(),
	    fun.read(),
	    fun.write(),
	    fun.readBytes(),
	    fun.writeBytes(),	      
	    fun.usec(),
      	    fun.source().c_str(),
	    fun.line(),
	    fun.name().c_str());
    fnames.pop_front();
  }
}

Block Thread::accumulate() const{
  Block b;
  for(auto rec = _records.begin(); rec != _records.end(); rec++){ b += rec->second; }
  return b;
}

list<string> Thread::functions() const{
  list<string> l;
  for(auto it = _records.begin(); it != _records.end(); it++){ l.push_back(it->second.name()); }
  return l;
}

list<string> Thread::sort_by_time() const{
  vector<Function> lf;
  for(auto it = _records.begin(); it != _records.end(); it++){ lf.push_back(it->second); }
  std::sort(lf.begin(), lf.end(), Block::comp_usec);
  list<string> ret;
  for(auto it = lf.rbegin(); it != lf.rend(); it++){ ret.push_back(it->name()); }
  return ret;
}

