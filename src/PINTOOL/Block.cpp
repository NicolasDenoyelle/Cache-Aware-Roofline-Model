#include "Block.hpp"

using namespace std;
using namespace CARM;

Block::Block(): _id({0,0}),
		_flops(0),
		_read(0),
		_write(0),
		_readB(0),
		_writeB(0),
		_usec(0){}

Block::Block(const block_id &id): _id(id),
				  _flops(0),
				  _read(0),
				  _write(0),
				  _readB(0),
				  _writeB(0),
				  _usec(0){}

Block::Block(const Block& copy): _id(copy._id),
				 _flops(copy._flops),
				 _read(copy._read),
				 _write(copy._write),
				 _readB(copy._readB),
				 _writeB(copy._writeB),
				 _usec(copy._usec){}

Block::Block(const unsigned long &flops,
	     const unsigned long &reads,
	     const unsigned long &writes,
	     const unsigned long &read_bytes,
	     const unsigned long &write_bytes,	     
	     const unsigned long &usec): _id({0,0}),
					 _flops(flops),
					 _read(reads),
					 _write(writes),
					 _readB(read_bytes),
					 _writeB(write_bytes),					 
					 _usec(usec){}

void Block::reset(){
  _flops=0;
  _read=0;
  _write=0;
  _readB=0;
  _writeB=0;  
  _usec=0;
}

void          Block::countFlops(const unsigned long &numflops){ _flops+=numflops; }
void          Block::countRead(const unsigned long &amount, const unsigned long &size){_write+=amount; _writeB+=size;}
void          Block::countWrite(const unsigned long &amount, const unsigned long &size){_read+=amount; _readB+=size;}        
void          Block::countUsec(const unsigned long &usec){_usec+=usec;}
Block&        Block::operator+=(const Block &rhs){
  _flops+=rhs._flops;
  _read+=rhs._read;
  _write+=rhs._write;
  _readB+=rhs._readB;
  _writeB+=rhs._writeB;
  _usec+=rhs._usec;  
  return *this;
}

block_id      Block::id()          const {return _id;}
unsigned long Block::flops()       const {return _flops;}
unsigned long Block::bytes()       const {return _readB+_writeB;}
unsigned long Block::readBytes()   const {return _readB;}
unsigned long Block::writeBytes()  const {return _writeB;}
unsigned long Block::read()        const {return _read;}
unsigned long Block::write()       const {return _write;}
unsigned long Block::usec()        const {return _usec;}
float         Block::AI()          const {return (float)_flops/(float)(_readB+_writeB);}
float         Block::GFlops()      const {return (float)_flops/((float)_usec*1e3);}

bool Block::comp_usec(const Block& lhs, const Block& rhs){
  return lhs._usec < rhs._usec;
}

