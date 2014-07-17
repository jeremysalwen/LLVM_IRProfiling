#ifndef BCPROF_ANALYSIS_PASSES_H
#define BCPROF_ANALYSIS_PASSES_H
namespace llvm {
	class PassRegistry;
	class ImmutablePass;
	
extern char &ProfileEstimatorPassID;
extern char &PathProfileLoaderPassID;
extern char &ProfileMetadataLoaderPassID;
extern char &ProfileInfoLoaderPassID;
}
#endif //BCPROF_ANALYSIS_PASSES_H
