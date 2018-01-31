#include "Results.hpp"
#include <algorithm>

using namespace std;
using namespace CARM;

Results::Results(){ _insert_new_functions = true; }

Results::Results(FILE * input){
  read(input);
}

void Results::read(FILE * input){
  _insert_new_functions = false;  
  do{
    Thread t(input);
    if(t.tid() < 0){ break; }
    _threads.insert(make_pair(t.tid(), t));
    list<string> lf = t.functions();
    _functions.insert(_functions.end(), lf.begin(), lf.end());
  } while(1);
  _functions.sort();
  _functions.unique();
}

void Results::registerThread(const unsigned& tid, Function* context){
  if(context == NULL) { return; }
  map<unsigned, Thread>::iterator thread = _threads.find(tid);
  if(thread != _threads.end()){ thread->second.setContext(context); } else { _threads[tid] = CARM::Thread(tid, context); }  
}

Function* Results::getContext(const unsigned& tid){
  return _threads[tid].current();
}

void Results::startFunction(const unsigned& tid, const Function& fun){  
  if(!binary_search(_functions.begin(), _functions.end(), fun.name())){
    list<string>::iterator pos = upper_bound(_functions.begin(), _functions.end(), fun.name());
    _functions.insert(pos, fun.name());
  }
  map<unsigned, Thread>::iterator thread = _threads.find(tid);
  if(thread != _threads.end()){
    thread->second.beginFunction(fun);
  }
  else if(_insert_new_functions){
    _threads[tid].beginFunction(fun);
  }
}

void Results::countBlock(const unsigned& tid, const Block& block){
  _threads[tid].countBlock(block);
}

void Results::stopFunction(const unsigned& tid){  
  _threads[tid].endFunction();  
}

void Results::print(FILE * output) const{
  {CARM::Thread head; head.header(output);}
  for(map<unsigned, Thread>::const_iterator thread = _threads.begin(); thread!= _threads.end(); thread++){
    thread->second.print(output);
  }
}

bool Results::hasFunction(const string& name){
  return binary_search(_functions.begin(), _functions.end(), name);
}

void Results::setFunction(const unsigned& tid, const Function& function){
  Function* f = getFunction(tid, function.name());
  if(f!=NULL){ f->set(function); }
  else{
    map<unsigned, Thread>::iterator t = _threads.find(tid);
    if(t == _threads.end()){ t = _threads.insert(make_pair(tid, Thread(tid, NULL))).first; }
    t->second.insertFunction(function);
  }
}

Function* Results::getFunction(const unsigned& tid, const string& name){
  map<unsigned, Thread>::iterator t = _threads.find(tid);
  if(t == _threads.end()) { return NULL; }
  else return t->second.getFunction(name);
}
