#ifndef BB_TRACE_STREAM_H
#define BB_TRACE_STREAM_H

#include "llvm/IR/BasicBlock.h"

#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

#include <type_traits>

namespace llvm {

  class Function;

  class BBTraceStream {
	static const std::aligned_storage<1, alignof(BasicBlock)> FunCallTagBB;
	static const std::aligned_storage<1, alignof(BasicBlock)> FunRetTagBB;
  public:

	//We choose these numbers so the first bit says whether it includes a Basic Block pointer
	//And the second bit says whether it includes a memory address
	//I'm not sure if this is necessary
	enum PacketType {
		                      //ID   Valid          Valid
		                      //      MemAddr   BB
		BBMask=1,    //000 0               1
		MemMask=2, //000 1               0
		
		BB=5,            //001 0               1
		FunCall=9,     //010 0               1
		FunRet=12,    //011 0               0
		MemOp=18,  //100 1               0
		BBEOF=20,        //101 0               0
	};
		  
	struct Packet {
		PacketType ptype;
		union {
			BasicBlock* BB;
			uint64_t MemAddr;
		};
	};
		  
	//The special trace labels for function calls and returns.
	static const uint64_t BBEOFID=-1;
	static const uint64_t FunCallID=-2;
	static const uint64_t FunRetID =-3;

	//The special trace labels for memory tracing.   Followed by the memory address read or written.
	static const uint64_t MemOpID=-4;
			  
    static char ID; // Class identification, replacement for typeinfo
   BBTraceStream() {};
    ~BBTraceStream() {};  // We want to be subclassed

	virtual bool startBBTraceStream()=0;
	virtual Packet BBTraceStreamNext()=0;
	
  };

} // End llvm namespace

#endif
