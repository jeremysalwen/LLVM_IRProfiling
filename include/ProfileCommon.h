#ifndef PROFILE_COMMON_H
#define PROFILE_COMMON_H

#include "vector"
#include "unordered_map"

#include "llvm/IR/Module.h"
#include "llvm/IR/BasicBlock.h"

void label_basic_blocks(llvm::Module& M,std::vector<llvm::BasicBlock*>& map, std::unordered_map<llvm::BasicBlock*, int>& reverse_map);

#endif