#ifndef BCPROF_ANALYSIS_PASSES_H
#define BCPROF_ANALYSIS_PASSES_H
namespace llvm {
	class PassRegistry;
	class ImmutablePass;
	
extern char &ProfileEstimatorPassID;

void initializeNoProfileInfoPass(llvm::PassRegistry&);

void initializeNoPathProfileInfoPass(llvm::PassRegistry&);
void initializeProfileInfoAnalysisGroup(llvm::PassRegistry&);
void initializeProfileEstimatorPassPass(PassRegistry&);
void initializeOptimalEdgeProfilerPass(llvm::PassRegistry&);
	
ImmutablePass *createNoProfileInfoPass();

}
#endif //BCPROF_ANALYSIS_PASSES_H
