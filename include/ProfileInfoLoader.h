//===- ProfileInfoLoader.h - Load & convert profile information -*- C++ -*-===//
//
//                      The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The ProfileInfoLoader class is used to load and represent profiling
// information read in from the dump file.  If conversions between formats are
// needed, it can also do this.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_PROFILEINFOLOADER_H
#define LLVM_ANALYSIS_PROFILEINFOLOADER_H

#include <string>
#include <utility>
#include <vector>

namespace llvm {

	class Module;
	class Function;
	class BasicBlock;

	class ProfileInfoLoader {
		const std::string &Filename;
		std::vector<std::string> CommandLines;
		std::vector<uint64_t>    FunctionCounts;
		std::vector<uint64_t>    BlockCounts;
		std::vector<uint64_t>    EdgeCounts;
		std::vector<uint64_t>    OptimalEdgeCounts;
		std::vector<uint64_t>    BBTrace;
		private:
			//This variable makes sure we don't append basic block traces to each other
			bool BBTraceFinished=false;
		public:
			// ProfileInfoLoader ctor - Read the specified profiling data file, exiting
			// the program if the file is invalid or broken.
			ProfileInfoLoader(const char *ToolName, const std::string &Filename);

			static const uint64_t Uncounted;

			unsigned getNumExecutions() const { return CommandLines.size(); }
			const std::string &getExecution(unsigned i) const { return CommandLines[i]; }

			const std::string &getFileName() const { return Filename; }

			// getRawFunctionCounts - This method is used by consumers of function
			// counting information.
			//
			const std::vector<uint64_t> &getRawFunctionCounts() const {
				return FunctionCounts;
			}

			// getRawBlockCounts - This method is used by consumers of block counting
			// information.
			//
			const std::vector<uint64_t> &getRawBlockCounts() const {
				return BlockCounts;
			}

			// getEdgeCounts - This method is used by consumers of edge counting
			// information.
			//
			const std::vector<uint64_t> &getRawEdgeCounts() const {
				return EdgeCounts;
			}

			// getEdgeOptimalCounts - This method is used by consumers of optimal edge 
			// counting information.
			//
			const std::vector<uint64_t> &getRawOptimalEdgeCounts() const {
				return OptimalEdgeCounts;
			}

			const std::vector<uint64_t> &getRawBBTrace() const {
				return BBTrace;
			}
	};
	// ByteSwap - Byteswap 'Var' if 'Really' is true.
	//
	inline uint64_t ByteSwap(uint64_t Var, bool Really) {
		if (!Really) return Var;
		return ((Var & (255UL<< 0U)) << 56U) |
			((Var & (255UL<< 8U)) <<  40U) |
			((Var & (255UL<<16U)) <<  24U) |
			((Var & (255UL<<24U)) << 8U) |
			((Var & (255UL<< 32U)) >> 8U) |
			((Var & (255UL<< 40U)) >>  24U) |
			((Var & (255UL<<48U)) >> 40U) |
			((Var & (255UL<<56U)) >> 56U);
	}

	void ReadProfilingBlock(const char *ToolName, FILE *F,bool ShouldByteSwap, std::vector<uint64_t> &Data);
	bool ReadBBTraceProfilingBlock(const char *ToolName, FILE *F, bool ShouldByteSwap,  std::vector<uint64_t> &Data);
	void SkipProfilingBlock(const char *ToolName, FILE *F,  bool ShouldByteSwap);
} // End llvm namespace

#endif
