#ifndef CARM_FUNCTION_HPP
#define CARM_FUNCTION_HPP

#include "Block.hpp"
#include <string>
#include <sys/time.h>

namespace CARM{
  class Function: public Block{
  private:
    std::string    _name;      //Function name
    std::string    _source;    //Source file
    int            _line;      //Source line
    int            _cpu;       //The cpu where function is starting
    int            _node;      //The NUMA node where function is starting    
    
  public:
    Function();

    Function(const std::string& name);
    
    Function(const std::string& name,
	     const std::string& source,
	     const int& line);
    
    Function(const std::string& name,
	     const std::string& source,
	     const int& line,
	     const int& cpu,
	     const int& node);
    
    Function(const Function& copy);
    
    void          update_cpu_infos();

    bool operator==(const Function& rhs) const; //Comparison on name

    void set(const Block& b);    
    void set(const Function& f);
    std::string   name()    const;
    std::string   source()  const;
    int           line()    const;
    unsigned      cpu()     const;
    unsigned      node()    const;
    void          print()   const;
  };
}

#endif
