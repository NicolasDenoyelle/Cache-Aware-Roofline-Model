#include "Function.hpp"
#ifndef _GNU_SOURCE  
#define _GNU_SOURCE
#endif
#include <unistd.h>
#include <sys/syscall.h>

using namespace std;
using namespace CARM;

Function::Function(): Block(), _name(""), _source("???"), _line(-1), _cpu(-1), _node(-1){}

Function::Function(const string& name):Block(),
				       _name(name),
				       _source("???"),
				       _line(-1),
				       _cpu(-1),
				       _node(-1){ update_cpu_infos(); }


Function::Function(const string& name,
		   const string& source,
		   const int& line):Block(),
				    _name(name),
				    _source(source),
				    _line(line),
				    _cpu(-1),
				    _node(-1){ update_cpu_infos(); }

Function::Function(const string& name,
		   const string& source,
		   const int& line,
		   const int& cpu,
		   const int& node):Block(),
					 _name(name),
					 _source(source),
					 _line(line),
					 _cpu(cpu),
					 _node(node){}

Function::Function(const Function& copy): Block(copy),
					  _name(copy._name),
					  _source(copy._source),					  
					  _line(copy._line),
					  _cpu(copy._cpu),
					  _node(copy._node){}

void Function::set(const Block& b){
  if(_flops == 0){ _flops = b.flops(); }
  if(_read == 0){ _read = b.read(); }  
  if(_write == 0){ _write = b.write(); }
  if(_readB == 0){ _readB = b.readBytes(); }  
  if(_writeB == 0){ _writeB = b.writeBytes(); }
  if(_usec == 0){ _usec = b.usec(); }    
}

void Function::set(const Function& f){
  set( (Block)f );
  if(_name.length() == 0){ _name = f._name; }
  if(_source == string("???")){ _source = f._source; }
  if(_line == -1){ _line = f._line; }
  if(_cpu == -1){ _cpu = f._cpu; }
  if(_node == -1){ _node = f._node; }
}

string        Function::name()    const{ return _name;}
string        Function::source()  const{ return _source;}
int           Function::line()    const{ return _line;}
unsigned      Function::cpu()     const{ return _cpu;}
unsigned      Function::node()    const{ return _node;}    

void Function::update_cpu_infos(){
#ifdef SYS_getcpu
  if(syscall(SYS_getcpu, &_cpu, &_node, NULL) == -1){ perror("syscall getcpu"); }
#endif
}

bool Function::operator==(const Function& rhs) const{
  return rhs._name == _name;
}

void Function::print() const{
  printf("%s",  _name.c_str());
  if(_line >= 0){ printf(" in %s:%d\n", _source.c_str(), _line); }
  printf("\n");
  printf("%-10s:%lu\n", "flops", flops());
  printf("%-10s:%lu\n", "bytes", bytes());
  printf("%-10s:%lu\n", "usec", usec());    
}

