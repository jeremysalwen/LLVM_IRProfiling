#include "ProfileCommon.h"

using namespace llvm;

void label_basic_blocks(Module& M,std::vector<BasicBlock*>& map, std::unordered_map<BasicBlock*, int>& reverse_map) {
	int i=0;
	for(typename Module::iterator fi=M.begin(),fe=M.end(); fi!=fe; ++fi) {
		Function& F=*fi;
		for (typename Function::iterator bbi=F.begin(), bbe = F.end();  bbi != bbe; ++bbi) {
			BasicBlock& b=*bbi;
			map.push_back(&b);
			reverse_map[&b]=i;
			i++;
		}
	}
}