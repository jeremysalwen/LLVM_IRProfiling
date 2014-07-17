#ifndef LLVM_ANALYSIS_PROFILEINFOLOADERPASS_H
#define LLVM_ANALYSIS_PROFILEINFOLOADERPASS_H

#include "llvm/Pass.h"

#include "ProfileInfo.h"

namespace llvm {
  class LoaderPass : public ModulePass, public ProfileInfo {
    std::string Filename;
    std::set<Edge> SpanningTree;
    std::set<const BasicBlock*> BBisUnvisited;
    unsigned ReadCount;
  public:
    static char ID; // Class identification, replacement for typeinfo
    explicit LoaderPass(const std::string &filename = "");

    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
    }

    virtual const char *getPassName() const {
      return "Profiling information loader";
    }

    // recurseBasicBlock() - Calculates the edge weights for as much basic
    // blocks as possbile.
    virtual void recurseBasicBlock(const BasicBlock *BB);
    virtual void readEdgeOrRemember(Edge, Edge&, unsigned &, double &);
    virtual void readEdge(ProfileInfo::Edge, std::vector<uint64_t>&);

    /// getAdjustedAnalysisPointer - This method is used when a pass implements
    /// an analysis interface through multiple inheritance.  If needed, it
    /// should override this to adjust the this pointer as needed for the
    /// specified pass info.
    virtual void *getAdjustedAnalysisPointer(AnalysisID PI) {
      if (PI == &ProfileInfo::ID)
        return (ProfileInfo*)this;
      return this;
    }
    
    /// run - Load the profile information from the specified file.
    virtual bool runOnModule(Module &M);
  };
}  // End of llvm namespace

#endif