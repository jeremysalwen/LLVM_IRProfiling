#include "BBTraceStream.h"
#include "llvm/Pass.h"

using namespace llvm;


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
		virtual BBTraceStream::Packet BBTraceStreamNext() {
		 BBTraceStream::Packet packet;
			packet.ptype=BBTraceStream::PacketType::BBEOF;
			return packet;
		}
	};
}
static llvm::RegisterPass<NoBBTrace> X("no-bbtrace", "No Basic Block Trace", true, true);
char NoBBTrace::ID = 0;
static llvm::RegisterAnalysisGroup<BBTraceStream,true> Y(X);