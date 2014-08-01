//===- TraceBasicBlocks.cpp - Insert basic-block trace instrumentation ----===//
//
//                      The LLVM Compiler Infrastructure
//
// This file was developed by the LLVM research group and is distributed under
// the University of Illinois Open Source License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass instruments the specified program with calls into a runtime
// library that cause it to output a trace of basic blocks as a side effect
// of normal execution.
//
//===----------------------------------------------------------------------===//

#include "ProfilingUtils.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Constants.h"
#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include <set>
using namespace llvm;

namespace {
	class TraceBasicBlocks : public ModulePass {
		LLVMContext* Context;
		void InsertInstrumentationCall(BasicBlock* BB, Constant* InstrFn, unsigned BBNumber);	  
		bool runOnModule(Module &M);
		public:
			static char ID; // Pass identification, replacement for typeid
			TraceBasicBlocks() : ModulePass(ID) {}
	};
	
// Register the path profiler as a pass
char TraceBasicBlocks::ID = 0;
RegisterPass<TraceBasicBlocks> X("trace-basic-blocks",
                              "Insert instrumentation for basic block tracing");
}

 
  void TraceBasicBlocks::InsertInstrumentationCall (BasicBlock *BB,
                                       Constant* InstrFn,
                                       unsigned BBNumber) {

  // Insert the call after any alloca or PHI instructions.
  BasicBlock::iterator InsertPos = BB->getFirstInsertionPt();
  while (isa<AllocaInst>(InsertPos))  ++InsertPos;
  CallInst::Create(InstrFn, ConstantInt::get (Type::getInt64Ty(*Context), BBNumber),
               "", InsertPos);
}

bool TraceBasicBlocks::runOnModule(Module &M)  {
  Context =&M.getContext();
  Function *Main = M.getFunction("main");
  if (Main == 0) {
   errs() << "WARNING: cannot insert basic-block trace instrumentation"
         << " into a module with no main function!\n";
    return false;  // No main, no instrumentation!
  }
  const char* FnName="llvm_trace_basic_block";
  Constant *InstrFn = M.getOrInsertFunction (FnName, Type::getVoidTy(*Context),
                                             Type::getInt64Ty(*Context), NULL);
	
  unsigned BBNumber = 0;
  for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F)
    for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
		dbgs() << "InsertInstrumentationCall (\"" << BB->getName ()
       << "\", \"" << F->getName() << "\", " << BBNumber << ")\n";

      InsertInstrumentationCall (BB, InstrFn, BBNumber);
      ++BBNumber;
    }

  // Add the initialization call to main.
	
  InsertProfilingInitCall(Main, "llvm_start_basic_block_tracing");
  return true;
}

