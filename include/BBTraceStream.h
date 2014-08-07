
#ifndef BB_TRACE_STREAM_H
#define BB_TRACE_STREAM_H

#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

  class BasicBlock;
  class Function;

  class BBTraceStream {

  public:
    static char ID; // Class identification, replacement for typeinfo
   BBTraceStream() {};
    ~BBTraceStream() {};  // We want to be subclassed

	virtual bool startBBTraceStream()=0;
	virtual BasicBlock* BBTraceStreamNext()=0;
  };

} // End llvm namespace

#endif
