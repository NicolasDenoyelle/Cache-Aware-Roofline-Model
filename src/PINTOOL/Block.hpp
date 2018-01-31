#ifndef CARM_BLOCK_HPP
#define CARM_BLOCK_HPP

#include <utility>

namespace CARM{
  typedef std::pair<unsigned long, unsigned long> block_id;
  
  class Block{
  protected:
    block_id       _id;     //Begin and end adresses
    unsigned long  _flops;  //amout of flops in block
    unsigned long  _read;   //amount of reads
    unsigned long  _write;  //amount of writes
    unsigned long  _readB;  //amount of bytes read
    unsigned long  _writeB; //amount of bytes written
    unsigned long  _usec;   //time spent in block
  
  public:
    Block();      
    Block(const block_id &id);
    Block(const unsigned long &flops,
	  const unsigned long &reads,
	  const unsigned long &writes,
	  const unsigned long &read_bytes,
	  const unsigned long &write_bytes,	     
	  const unsigned long &usec);
    Block(const Block &copy);  

    static bool comp_usec(const Block& lhs, const Block& rhs);    
    
    void          reset();
    void          countFlops(const unsigned long &amount);
    void          countRead(const unsigned long &amount, const unsigned long &size);
    void          countWrite(const unsigned long &amount, const unsigned long &size);        
    void          countUsec(const unsigned long &);
    Block&        operator+=(const Block&);
  
    block_id      id()                               const;
    unsigned long flops()                            const;
    unsigned long bytes()                            const;
    unsigned long write()                            const;
    unsigned long read()                             const;
    unsigned long writeBytes()                       const;
    unsigned long readBytes()                        const;            
    unsigned long usec()                             const;    
    float         AI()                               const;
    float         GFlops()                           const;
  };
}

#endif
