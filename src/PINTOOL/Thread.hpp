#ifndef CARM_THREAD_HPP
#define CARM_THREAD_HPP

#include "Function.hpp"
#include <map>
#include <vector>
#include <string>
#include <list>
#include <cstdio>

namespace CARM{ 
  class Thread{
  private:
    int                                _tid;
    std::map<std::string, Function>    _records;         //Blocks of functions (identified by name) entered by this thread.
    std::list<Function*>               _backtrace;       //Function backtrace of current thread.
    Block                              _accumulator;

    std::list<std::string> sort_by_time() const;

  public:
    Thread();
    Thread(FILE * f);
    Thread(const int &tid, Function* context);
    
    int tid() const;
    Function* current();
    std::list<std::string> functions() const;
    void setContext(Function* context);
    void insertFunction(const Function&);
    Function* getFunction(const std::string& name);    
    
    /* Block modifiers */
    //Begin a function and record Block metrics.
    void  beginFunction(const Function& fun);
    //Count the metrics of an identified block. If no function has been entered, no update occures.
    void  countBlock(const Block&);
    //Stop couting block as part of this function.  
    void  endFunction();

    void  header(FILE * output) const;
    void  print(FILE * output) const;
    Block accumulate() const;    
  };
}
#endif

