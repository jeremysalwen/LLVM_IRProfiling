//===- ProfileDataLoader.cpp - Load profile information from disk ---------===//
//
//                      The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// The ProfileDataLoader class is used to load raw profiling data from the dump
// file.
//
//===----------------------------------------------------------------------===//

#include "ProfileDataLoader.h"
#include "llvm/ADT/ArrayRef.h"
#include "ProfileDataTypes.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include <cstdio>
#include <cstdlib>
using namespace llvm;



/// AddCounts - Add 'A' and 'B', accounting for the fact that the value of one
/// (or both) may not be defined.
static uint64_t AddCounts(uint64_t A, uint64_t B) {
  // If either value is undefined, use the other.
  // Undefined + undefined = undefined.
  if (A == ProfileDataLoader::Uncounted) return B;
  if (B == ProfileDataLoader::Uncounted) return A;

  return A + B;
}

/// ReadProfilingData - Load 'NumEntries' items of type 'T' from file 'F'
template <typename T>
static void ReadProfilingData(const char *ToolName, FILE *F,
                              T *Data, size_t NumEntries) {
  // Read in the block of data...
  if (fread(Data, sizeof(T), NumEntries, F) != NumEntries)
    report_fatal_error(Twine(ToolName) + ": Profiling data truncated");
}

/// ReadProfilingNumEntries - Read how many entries are in this profiling data
/// packet.
static uint64_t ReadProfilingNumEntries(const char *ToolName, FILE *F,
                                        bool ShouldByteSwap) {
  uint64_t Entry;
  ReadProfilingData<uint64_t>(ToolName, F, &Entry, 1);
  return ShouldByteSwap ? ByteSwap_64(Entry) : Entry;
}

/// ReadProfilingBlock - Read the number of entries in the next profiling data
/// packet and then accumulate the entries into 'Data'.
static void ReadProfilingBlock(const char *ToolName, FILE *F,
                               bool ShouldByteSwap,
                               SmallVectorImpl<uint64_t> &Data) {
  // Read the number of entries...
  uint64_t NumEntries = ReadProfilingNumEntries(ToolName, F, ShouldByteSwap);

  // Read in the data.
  SmallVector<uint64_t, 8> TempSpace(NumEntries);
  ReadProfilingData<uint64_t>(ToolName, F, TempSpace.data(), NumEntries);

  // Make sure we have enough space ...
  if (Data.size() < NumEntries)
    Data.resize(NumEntries, ProfileDataLoader::Uncounted);

  // Accumulate the data we just read into the existing data.
  for (uint64_t i = 0; i < NumEntries; ++i) {
    uint64_t Entry = ShouldByteSwap ? ByteSwap_64(TempSpace[i]) : TempSpace[i];
    Data[i] = AddCounts(Entry, Data[i]);
  }
}

/// ReadProfilingArgBlock - Read the command line arguments that the progam was
/// run with when the current profiling data packet(s) were generated.
static void ReadProfilingArgBlock(const char *ToolName, FILE *F,
                                  bool ShouldByteSwap,
                                  SmallVectorImpl<std::string> &CommandLines) {
  // Read the number of bytes ...
  uint64_t ArgLength = ReadProfilingNumEntries(ToolName, F, ShouldByteSwap);

  // Read in the arguments (if there are any to read).  Round up the length to
  // the nearest 4-byte multiple.
  SmallVector<char, 8> Args(ArgLength+4);
  if (ArgLength)
    ReadProfilingData<char>(ToolName, F, Args.data(), (ArgLength+7) & ~7);

  // Store the arguments.
  CommandLines.push_back(std::string(&Args[0], &Args[ArgLength]));
}

const uint64_t ProfileDataLoader::Uncounted = ~0U;

/// ProfileDataLoader ctor - Read the specified profiling data file, reporting
/// a fatal error if the file is invalid or broken.
ProfileDataLoader::ProfileDataLoader(const char *ToolName,
                                     const std::string &Filename)
  : Filename(Filename) {
  FILE *F = fopen(Filename.c_str(), "rb");
  if (F == 0)
    report_fatal_error(Twine(ToolName) + ": Error opening '" +
                       Filename + "': ");

  // Keep reading packets until we run out of them.
  uint64_t PacketType;
  while (fread(&PacketType, sizeof(uint64_t), 1, F) == 1) {
    // If the low eight bits of the packet are zero, we must be dealing with an
    // endianness mismatch.  Byteswap all words read from the profiling
    // information.  This can happen when the compiler host and target have
    // different endianness.
    bool ShouldByteSwap = (char)PacketType == 0;
    PacketType = ShouldByteSwap ? ByteSwap_64(PacketType) : PacketType;

    switch (PacketType) {
      case ArgumentInfo:
        ReadProfilingArgBlock(ToolName, F, ShouldByteSwap, CommandLines);
        break;

      case EdgeInfo:
        ReadProfilingBlock(ToolName, F, ShouldByteSwap, EdgeCounts);
        break;

      default:
        report_fatal_error(std::string(ToolName)
                           + ": Unknown profiling packet type");
        break;
    }
  }

  fclose(F);
}
