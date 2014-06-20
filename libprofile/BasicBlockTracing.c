/*===-- BasicBlockTracing.c - Support library for basic block tracing -----===*\
|*
|*                     The LLVM Compiler Infrastructure
|*
|* This file is distributed under the University of Illinois Open Source
|* License. See LICENSE.TXT for details.
|* 
|*===----------------------------------------------------------------------===*|
|* 
|* This file implements the call back routines for the basic block tracing
|* instrumentation pass.  This should be used with the -trace-basic-blocks
|* LLVM pass.
|*
\*===----------------------------------------------------------------------===*/

#include "Profiling.h"
#include <stdlib.h>
#include <stdio.h>

static uint64_t *ArrayStart, *ArrayEnd, *ArrayCursor;

/* WriteAndFlushBBTraceData - write out the currently accumulated trace data
 * and reset the cursor to point to the beginning of the buffer.
 */
static void WriteAndFlushBBTraceData () {
  write_profiling_data(BBTraceInfo, ArrayStart, (ArrayCursor - ArrayStart));
  ArrayCursor = ArrayStart;
}

/* BBTraceAtExitHandler - When the program exits, just write out any remaining 
 * data and free the trace buffer.
 */
static void BBTraceAtExitHandler(void) {
  WriteAndFlushBBTraceData ();
  free (ArrayStart);
}

/* llvm_trace_basic_block - called upon hitting a new basic block. */
void llvm_trace_basic_block (uint64_t BBNum) {
  *ArrayCursor++ = BBNum;
  if (ArrayCursor == ArrayEnd)
    WriteAndFlushBBTraceData ();
}

/* llvm_start_basic_block_tracing - This is the main entry point of the basic
 * block tracing library.  It is responsible for setting up the atexit
 * handler and allocating the trace buffer.
 */
int llvm_start_basic_block_tracing(int argc, const char **argv,
                              uint64_t *arrayStart,uint64_t numElements) {
  int Ret;
  const unsigned BufferSize = 128 * 1024;
  uint64_t ArraySize;

  Ret = save_arguments(argc, argv);

  /* Allocate a buffer to contain BB tracing data */
  ArraySize = BufferSize / sizeof (uint64_t);
  ArrayStart = malloc (ArraySize * sizeof (uint64_t));
  ArrayEnd = ArrayStart + ArraySize;
  ArrayCursor = ArrayStart;

  /* Set up the atexit handler. */
  atexit (BBTraceAtExitHandler);

  return Ret;
}
