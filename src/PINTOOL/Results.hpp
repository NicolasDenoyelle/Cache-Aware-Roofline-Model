#ifndef RESULTS_HPP
#define RESULTS_HPP

#include <map>
#include <string>
#include <cstdio>
#include "Thread.hpp"
#include "Function.hpp"

namespace CARM{
  class Results{
  private:
    std::map<unsigned, Thread> _threads;
    std::list<std::string>     _functions;
    bool                       _insert_new_functions; //If instanciated with already existing data only update those.
    
  public:
    Results();
    Results(FILE * input);
    void read(FILE * input);
    void print(FILE* output) const;    
    void registerThread(const unsigned& tid, Function* context);  //Register a new thread to count functions metrics.
    Function* getContext(const unsigned& tid);                    //Function currently executed by thread tid.
    void startFunction(const unsigned& tid, const Function& fun); //Start function in tid backtrace.
    void countBlock(const unsigned& tid, const Block& block);     //Count flops and bytes in current function of thread tid.  
    void stopFunction(const unsigned& tid);                       //Stop current function in tid backtrace.
    bool hasFunction(const std::string& name);                    //Return if a function has already been entered by any thread.
    void      setFunction(const unsigned& tid, const Function& function);
    Function* getFunction(const unsigned& tid, const std::string& name);    
  };
};

#endif

