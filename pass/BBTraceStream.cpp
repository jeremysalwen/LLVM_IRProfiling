#include "BBTraceStream.h"
#include "llvm/Pass.h"

using namespace llvm;

const std::aligned_storage<1, alignof(BasicBlock)> BBTraceStream::FunCallTagBB;
const std::aligned_storage<1, alignof(BasicBlock)> BBTraceStream::FunRetTagBB;
constexpr BasicBlock* BBTraceStream::FunCallTag;
constexpr BasicBlock* BBTraceStream::FunRetTag;

char BBTraceStream::ID = 0;
static RegisterAnalysisGroup<BBTraceStream> P("Basic Block Trace");

namespace {
	struct NoBBTrace : public ImmutablePass, public BBTraceStream {
		static char ID; // Class identification, replacement for typeinfo
		NoBBTrace() : ImmutablePass(ID) {
		}

		/// getAdjustedAnalysisPointer - This method is used when a pass implements
			/// an analysis interface through multiple inheritance.  If needed, it
			/// should override this to adjust the this pointer as needed for the
			/// specified pass info.
			virtual void *getAdjustedAnalysisPointer(AnalysisID PI) {
				if (PI == &BBTraceStream
				    ::ID)
					return (BBTraceStream*)this;
				return this;
			}

		virtual const char *getPassName() const {
			return "NoBBTrace";
		}
		virtual bool startBBTraceStream() {
			return false;
		}
		virtual BasicBlock* BBTraceStreamNext() {
			return NULL;
		}
	};
}
static llvm::RegisterPass<NoBBTrace> X("no-bbtrace", "No Basic Block Trace", true, true);
char NoBBTrace::ID = 0;
static llvm::RegisterAnalysisGroup<BBTraceStream,true> Y(X);