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
	static constexpr BasicBlock* FunCallTag=  static_cast<BasicBlock*>(const_cast<void*>(static_cast<const void*>(&FunCallTagBB)));
	static constexpr BasicBlock* FunRetTag=   static_cast<BasicBlock*>(const_cast<void*>(static_cast<const void*>(&FunRetTagBB))); 
		  
	//The special trace labels for function calls and returns.
	static const uint64_t BBEOF=-1;
	static const uint64_t FunCallID=-2;
	static const uint64_t FunRetID =-3;
			  
    static char ID; // Class identification, replacement for typeinfo
   BBTraceStream() {};
    ~BBTraceStream() {};  // We want to be subclassed

	virtual bool startBBTraceStream()=0;
	virtual BasicBlock* BBTraceStreamNext()=0;
  };

} // End llvm namespace

#endif
