#include "ProfileInfoLoader.h"

#include "llvm/Support/CommandLine.h"

#include "BBTraceStream.h"
#include "ProfileCommon.h"
#include "ProfileInfoTypes.h"

#include <vector>

using namespace llvm;

static cl::opt<std::string>
BBTraceFilename("bbtrace-file", cl::init("llvmprof.out"),
                cl::value_desc("filename"),
                cl::desc("Profile file loaded by -ondemand-bbtrace"));

namespace {
	//This class allows us to load basic block traces through a named pipe
	//Drastically reducing the disc footprint.
	//It will also work from a normal file.
	struct OnDemandBBTrace : public ModulePass, public BBTraceStream {
		std::string Filename;
		bool started=false;
		bool BBTraceFinished=false;

		std::vector<BasicBlock*> BBMap;

		FILE* F;
		std::vector<uint64_t> buffer;
		std::vector<uint64_t>::iterator it;

		~OnDemandBBTrace() {
			fclose(F);
		}
		void loadNextBBTracePacket() {
			buffer.clear();

			// Keep reading packets until we run out of them.
			uint64_t PacketType;
			while (fread(&PacketType, sizeof(uint64_t), 1, F) == 1) {
				// If the low eight bits of the packet are zero, we must be dealing with an
				// endianness mismatch.  Byteswap all words read from the profiling
				// information.
				bool ShouldByteSwap = (char)PacketType == 0;
				PacketType = ByteSwap(PacketType, ShouldByteSwap);

				switch (PacketType) {
					case ArgumentInfo: {
						uint64_t ArgLength;
						if (fread(&ArgLength, sizeof(uint64_t), 1, F) != 1) {
							errs() << getPassName() << ": arguments packet truncated!\n";
							perror(0);
							exit(1);
						}
						ArgLength = ByteSwap(ArgLength, ShouldByteSwap);

						// Read in the arguments...
						std::vector<char> Chars(ArgLength+4);

						if (ArgLength) {
							if (fread(&Chars[0], (ArgLength+7) & ~7, 1, F) != 1) {
								errs() << getPassName() << ": arguments packet truncated!\n";
								perror(0);
								exit(1);
							}
						}
						break;
					}

					case FunctionInfo:
					case BlockInfo:
					case EdgeInfo:
					case OptEdgeInfo:
						SkipProfilingBlock (getPassName(), F, ShouldByteSwap);
						break;

					case BBTraceInfo:
						if(BBTraceFinished) {
							errs() << getPassName() << ": Warning, tools can only handle one basic block trace per llvmprof.out file.  All subsequent traces are being ignored\n";
							SkipProfilingBlock (getPassName(), F, ShouldByteSwap);
						} else {
							BBTraceFinished=ReadBBTraceProfilingBlock(getPassName(), F, ShouldByteSwap, buffer);
							it=buffer.begin();	
						}
						return;
						break;

					default:
						errs() << getPassName()<< ": Unknown packet type #" << PacketType << "!\n";
						exit(1);
				}
			}
		}
		uint64_t getNextUint() {
			uint64_t result=*it++;
			if(it==buffer.end()) {
				loadNextBBTracePacket();
			}
			return result;
		}
		BasicBlock* uintToBB(uint64_t id) {
			if(id>=BBMap.size())  {
				errs() <<"ERROR: Unrecognized bb in function call\n";
				exit(1);
			}
			return BBMap[id];
		}
		public:
			static char ID; // Class identification, replacement for typeinfo
			OnDemandBBTrace(const std::string& filename="") : ModulePass(ID),Filename(filename) {
				if (filename.empty()) Filename = BBTraceFilename;
				F=NULL;
			}

			virtual void getAnalysisUsage(AnalysisUsage &AU) const {
				AU.setPreservesAll();
			}

			/// getAdjustedAnalysisPointer - This method is used when a pass implements
				/// an analysis interface through multiple inheritance.  If needed, it
				/// should override this to adjust the this pointer as needed for the
				/// specified pass info.
				virtual void *getAdjustedAnalysisPointer(AnalysisID PI) {
					if (PI == &BBTraceStream::ID)
						return (BBTraceStream*)this;
					return this;
				}



			virtual bool runOnModule(Module &M) {
				F = fopen(Filename.c_str(), "rb");
				if (F == 0) {
					errs() <<  getPassName() <<": Error opening '" << Filename << "': ";
					perror(0);
					exit(1);
				}
				std::unordered_map<BasicBlock*, int> reverse_map;
				label_basic_blocks(M,BBMap, reverse_map); 

			}
			virtual const char *getPassName() const {
				return "OnDemandBBTrace";
			}


			virtual bool startBBTraceStream() {
				if(started) {
					dbgs()<<"Failed start\n";
					return false;
				} else {
					started=true;
					loadNextBBTracePacket();
					if(buffer.empty()) {
						errs()  << "ERROR: No basic block trace found in profiling file "<<Filename<<"\n";
						exit(1);
					}
					return true;
				}
			}

			virtual BBTraceStream::Packet BBTraceStreamNext() {

				BBTraceStream::Packet packet;
				if(buffer.empty())  {
					packet.ptype=BBTraceStream::PacketType::BBEOF;
				} else {
					uint64_t bbid=getNextUint ();

					if(bbid==BBTraceStream::MemOpID) {
						if(buffer.empty()) {
							errs()<<"Error, truncated MemOp packet in Basic Block trace stream\n";
							exit(1);
						}
						packet.ptype=BBTraceStream::PacketType::MemOp;
						packet.MemAddr=getNextUint();
					}else if(bbid==BBTraceStream::FunCallID) {
						if(buffer.empty()) {
							errs()<<"Error, truncated FunCall packet in Basic Block trace stream\n";
							exit(1);
						}
						packet.ptype=BBTraceStream::FunCall;
						packet.BB=uintToBB (getNextUint());
					} else if(bbid==BBTraceStream::FunRetID) {
						packet.ptype=BBTraceStream::FunRet;
					} else {
						packet.ptype=BBTraceStream::BB;
						packet.BB=uintToBB(bbid);
					}
				}
				return packet;
			}
	};
}
char OnDemandBBTrace::ID = 0;
static llvm::RegisterPass<OnDemandBBTrace> X("ondemand-bbtrace", "Load Basic Block trace on demand", true, true);

static llvm::RegisterAnalysisGroup<BBTraceStream> Y(X);