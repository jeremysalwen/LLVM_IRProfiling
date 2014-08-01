//===- ProfileInfoLoad.cpp - Load profile information from disk -----------===//
//
//                      The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The ProfileInfoLoader class is used to load and represent profiling
// information read in from the dump file.
//
//===----------------------------------------------------------------------===//

#include "ProfileInfoLoader.h"
#include "ProfileInfoTypes.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdio>
#include <cstdlib>
using namespace llvm;

// ByteSwap - Byteswap 'Var' if 'Really' is true.
//
static inline uint64_t ByteSwap(uint64_t Var, bool Really) {
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

static uint64_t AddCounts(uint64_t A, uint64_t B) {
  // If either value is undefined, use the other.
  if (A == ProfileInfoLoader::Uncounted) return B;
  if (B == ProfileInfoLoader::Uncounted) return A;
  return A + B;
}

static void ReadProfilingBlock(const char *ToolName, FILE *F,
                               bool ShouldByteSwap,
                               std::vector<uint64_t> &Data) {
  // Read the number of entries...
  uint64_t NumEntries;
  if (fread(&NumEntries, sizeof(uint64_t), 1, F) != 1) {
    errs() << ToolName << ": data packet truncated at num entries!\n";
    perror(0);
    exit(1);
  }
  NumEntries = ByteSwap(NumEntries, ShouldByteSwap);

  // Read the counts...
  std::vector<uint64_t> TempSpace(NumEntries);

  // Read in the block of data...
  if (fread(&TempSpace[0], sizeof(uint64_t)*NumEntries ,1, F) != 1) {
    errs() << ToolName << ": data packet truncated at profiling block!\n";
    perror(0);
    exit(1);
  }

  // Make sure we have enough space... The space is initialised to -1 to
  // facitiltate the loading of missing values for OptimalEdgeProfiling.
  if (Data.size() < NumEntries)
    Data.resize(NumEntries, ProfileInfoLoader::Uncounted);

  // Accumulate the data we just read into the data.
  if (!ShouldByteSwap) {
    for (uint64_t i = 0; i != NumEntries; ++i) {
      Data[i] = AddCounts(TempSpace[i], Data[i]);
    }
  } else {
    for (uint64_t i = 0; i != NumEntries; ++i) {
      Data[i] = AddCounts(ByteSwap(TempSpace[i], true), Data[i]);
    }
  }
}
//When we do basic block tracing, the composition operator is concatenation, not summing
//this function reads the profiling block and /appends/ it to the array instead of adding it like in path/edge profiling
//returns true if this is the last block
static bool ReadBBTraceProfilingBlock(const char *ToolName, FILE *F,
                               bool ShouldByteSwap,
                               std::vector<uint64_t> &Data) {
	 // Read the number of entries...
  uint64_t NumEntries;
  if (fread(&NumEntries, sizeof(uint64_t), 1, F) != 1) {
    errs() << ToolName << ": data packet truncated at num entries!\n";
    perror(0);
    exit(1);
  }
  NumEntries = ByteSwap(NumEntries, ShouldByteSwap);

  uint64_t NewEntryStart=Data.size();
  Data.resize(Data.size()+NumEntries);
	
  // Read in the block of data...
  if (fread(&Data[NewEntryStart], sizeof(uint64_t)*NumEntries ,1, F) != 1) {
    errs() << ToolName << ": data packet truncated at profiling block!\n";
    perror(0);
    exit(1);
  }
	
  // Byte swap if necessary
  if (ShouldByteSwap) {
    for (uint64_t i = NewEntryStart; i != Data.size(); ++i) {
      Data[i] = ByteSwap(Data[i], true);
    }
  }
  if(Data.back()==-1) {
	  Data.pop_back();
	  return true;
  }
	return false;
}

static void SkipProfilingBlock(const char *ToolName, FILE *F,
                               bool ShouldByteSwap) {
  // Read the number of entries...
  uint64_t NumEntries;
  if (fread(&NumEntries, sizeof(uint64_t), 1, F) != 1) {
    errs() << ToolName << ": data packet truncated at num entries!\n";
    perror(0);
    exit(1);
  }
  NumEntries = ByteSwap(NumEntries, ShouldByteSwap);
  fseek(F,NumEntries*sizeof(uint64_t),SEEK_CUR);
}

const uint64_t ProfileInfoLoader::Uncounted = ~0U;

// ProfileInfoLoader ctor - Read the specified profiling data file, exiting the
// program if the file is invalid or broken.
//
ProfileInfoLoader::ProfileInfoLoader(const char *ToolName,
                                     const std::string &Filename)
  : Filename(Filename) {
  FILE *F = fopen(Filename.c_str(), "rb");
  if (F == 0) {
    errs() << ToolName << ": Error opening '" << Filename << "': ";
    perror(0);
    exit(1);
  }

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
        errs() << ToolName << ": arguments packet truncated!\n";
        perror(0);
        exit(1);
      }
      ArgLength = ByteSwap(ArgLength, ShouldByteSwap);

      // Read in the arguments...
      std::vector<char> Chars(ArgLength+4);

      if (ArgLength)
        if (fread(&Chars[0], (ArgLength+7) & ~7, 1, F) != 1) {
          errs() << ToolName << ": arguments packet truncated!\n";
          perror(0);
          exit(1);
        }
      CommandLines.push_back(std::string(&Chars[0], &Chars[ArgLength]));
      break;
    }

    case FunctionInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, FunctionCounts);
      break;

    case BlockInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, BlockCounts);
      break;

    case EdgeInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, EdgeCounts);
      break;

    case OptEdgeInfo:
      ReadProfilingBlock(ToolName, F, ShouldByteSwap, OptimalEdgeCounts);
      break;

    case BBTraceInfo:
	  if(BBTraceFinished) {
		  errs() << ToolName << ": Warning, tools can only handle one basic block trace per llvmprof.out file.  All subsequent traces are being ignored\n";
		  SkipProfilingBlock (ToolName, F, ShouldByteSwap);
	  } else {
 	     BBTraceFinished=ReadBBTraceProfilingBlock(ToolName, F, ShouldByteSwap, BBTrace);
	  }
      break;

    default:
      errs() << ToolName << ": Unknown packet type #" << PacketType << "!\n";
      exit(1);
    }
  }

  fclose(F);
}

