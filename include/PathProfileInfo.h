//===- PathProfileInfo.h --------------------------------------*- C++ -*---===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file outlines the interface used by optimizers to load path profiles.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ANALYSIS_PATHPROFILEINFO_H
#define LLVM_ANALYSIS_PATHPROFILEINFO_H

#include "PathNumbering.h"
#include "llvm/IR/BasicBlock.h"

namespace llvm {

class ProfilePath;
class ProfilePathEdge;
class PathProfileInfo;

typedef std::vector<ProfilePathEdge> ProfilePathEdgeVector;
typedef std::vector<ProfilePathEdge>::iterator ProfilePathEdgeIterator;

typedef std::vector<BasicBlock*> ProfilePathBlockVector;
typedef std::vector<BasicBlock*>::iterator ProfilePathBlockIterator;

typedef std::map<uint64_t,ProfilePath*> ProfilePathMap;
typedef std::map<uint64_t,ProfilePath*>::iterator ProfilePathIterator;

typedef std::map<Function*,unsigned int> FunctionPathCountMap;
typedef std::map<Function*,ProfilePathMap> FunctionPathMap;
typedef std::map<Function*,ProfilePathMap>::iterator FunctionPathIterator;

class ProfilePathEdge {
public:
  ProfilePathEdge(BasicBlock* source, BasicBlock* target,
                  unsigned duplicateNumber);

  inline unsigned getDuplicateNumber() { return _duplicateNumber; }
  inline BasicBlock* getSource() { return _source; }
  inline BasicBlock* getTarget() { return _target; }

protected:
  BasicBlock* _source;
  BasicBlock* _target;
  unsigned _duplicateNumber;
};

class ProfilePath {
public:
  ProfilePath(uint64_t, uint64_t count,
              double countStdDev, PathProfileInfo* ppi);

  double getFrequency() const;

  inline uint64_t getNumber() const { return _number; }
  inline uint64_t getCount() const { return _count; }
  inline double getCountStdDev() const { return _countStdDev; }

  ProfilePathEdgeVector* getPathEdges() const;
  ProfilePathBlockVector* getPathBlocks() const;

  BasicBlock* getFirstBlockInPath() const;

private:
  uint64_t _number;
  uint64_t _count;
  double _countStdDev;

  // double pointer back to the profiling info
  PathProfileInfo* _ppi;
};

// TODO: overload [] operator for getting path
// Add: getFunctionCallCount()
class PathProfileInfo {
  public:
  PathProfileInfo();
  ~PathProfileInfo();

  void setCurrentFunction(Function* F);
  Function* getCurrentFunction() const;
  BasicBlock* getCurrentFunctionEntry();

  ProfilePath* getPath(uint64_t number);
  uint64_t getPotentialPathCount();

  ProfilePathIterator pathBegin();
  ProfilePathIterator pathEnd();
  uint64_t pathsRun();

  static char ID; // Pass identification
  std::string argList;

protected:
  FunctionPathMap _functionPaths;
  FunctionPathCountMap _functionPathCounts;

private:
  BallLarusDag* _currentDag;
  Function* _currentFunction;

  friend class ProfilePath;
};
} // end namespace llvm

#endif
