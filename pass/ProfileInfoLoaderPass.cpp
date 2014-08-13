//===- ProfileInfoProfileInfoLoaderPass.cpp - LLVM Pass to load profile info ---------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a concrete implementation of profiling information that
// loads the information from a profile dump file.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "profile-loader"

#include "ProfileCommon.h"
#include "ProfileInfoLoaderPass.h"
#include "ProfileInfoLoader.h"
#include "Passes.h"

#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"
#include "llvm/Support/raw_ostream.h"
#include <set>

using namespace llvm;

STATISTIC(NumEdgesRead, "The # of edges read.");

static cl::opt<std::string>
ProfileInfoFilename("profile-info-file", cl::init("llvmprof.out"),
                    cl::value_desc("filename"),
                    cl::desc("Profile file loaded by -profile-loader"));

char ProfileInfoLoaderPass::ID = 0;
static RegisterPass<ProfileInfoLoaderPass> X("profile-loader",
                                             "Load profile information from llvmprof.out", false, true);
static RegisterAnalysisGroup<ProfileInfo> R(X);
static RegisterAnalysisGroup<BBTraceStream> S(X);

char &llvm::ProfileInfoLoaderPassID = ProfileInfoLoaderPass::ID;

ProfileInfoLoaderPass::ProfileInfoLoaderPass(const std::string &filename)
: ModulePass(ID), Filename(filename) {
	if (filename.empty()) Filename = ProfileInfoFilename;
}

void ProfileInfoLoaderPass::readEdgeOrRemember(Edge edge, Edge &tocalc, 
                                               unsigned &uncalc, double &count) {
	double w;
	if ((w = getEdgeWeight(edge)) == MissingValue) {
		tocalc = edge;
		uncalc++;
	} else {
		count+=w;
	}
}

// recurseBasicBlock - Visits all neighbours of a block and then tries to
// calculate the missing edge values.
void ProfileInfoLoaderPass::recurseBasicBlock(const BasicBlock *BB) {

	// break recursion if already visited
	if (BBisUnvisited.find(BB) == BBisUnvisited.end()) return;
	BBisUnvisited.erase(BB);
	if (!BB) return;

	for (succ_const_iterator bbi = succ_begin(BB), bbe = succ_end(BB);
	     bbi != bbe; ++bbi) {
		recurseBasicBlock(*bbi);
	}
	for (const_pred_iterator bbi = pred_begin(BB), bbe = pred_end(BB);
	     bbi != bbe; ++bbi) {
		recurseBasicBlock(*bbi);
	}

	Edge tocalc;
	if (CalculateMissingEdge(BB, tocalc)) {
		SpanningTree.erase(tocalc);
	}
}

void ProfileInfoLoaderPass::readEdge(ProfileInfo::Edge e,
                                     std::vector<uint64_t> &ECs) {
	if (ReadCount < ECs.size()) {
		double weight = ECs[ReadCount++];
		if (weight != ProfileInfoLoader::Uncounted) {
			// Here the data realm changes from the uint64_t of the file to the
			// double of the ProfileInfo. This conversion would only be safe for
			// values up to 2^52
			EdgeInformation[getFunction(e)][e] += (double)weight;

			DEBUG(dbgs() << "--Read Edge Counter for " << e
			      << " (# "<< (ReadCount-1) << "): "
			      << (uint64_t)getEdgeWeight(e) << "\n");
		} else {
			// This happens only if reading optimal profiling information, not when
			// reading regular profiling information.
			SpanningTree.insert(e);
		}
	}
}

bool ProfileInfoLoaderPass::runOnModule(Module &M) {
	ProfileInfoLoader PIL("profile-loader", Filename);

	EdgeInformation.clear();
	std::vector<uint64_t> Counters = PIL.getRawEdgeCounts();
	if (Counters.size() > 0) {
		ReadCount = 0;
		for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
			if (F->isDeclaration()) continue;
			DEBUG(dbgs() << "Working on " << F->getName() << "\n");
			readEdge(getEdge(0,&F->getEntryBlock()), Counters);
			for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
				TerminatorInst *TI = BB->getTerminator();
				for (unsigned s = 0, e = TI->getNumSuccessors(); s != e; ++s) {
					readEdge(getEdge(BB,TI->getSuccessor(s)), Counters);
				}
			}
		}
		if (ReadCount != Counters.size()) {
			errs() << "WARNING: profile information is inconsistent with "
				<< "the current program!\n";
		}
		NumEdgesRead = ReadCount;
	}

	Counters = PIL.getRawOptimalEdgeCounts();
	if (Counters.size() > 0) {
		ReadCount = 0;
		for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
			if (F->isDeclaration()) continue;
			DEBUG(dbgs() << "Working on " << F->getName() << "\n");
			readEdge(getEdge(0,&F->getEntryBlock()), Counters);
			for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB) {
				TerminatorInst *TI = BB->getTerminator();
				if (TI->getNumSuccessors() == 0) {
					readEdge(getEdge(BB,0), Counters);
				}
				for (unsigned s = 0, e = TI->getNumSuccessors(); s != e; ++s) {
					readEdge(getEdge(BB,TI->getSuccessor(s)), Counters);
				}
			}
			while (SpanningTree.size() > 0) {

				unsigned size = SpanningTree.size();

				BBisUnvisited.clear();
				for (std::set<Edge>::iterator ei = SpanningTree.begin(),
				     ee = SpanningTree.end(); ei != ee; ++ei) {
					BBisUnvisited.insert(ei->first);
					BBisUnvisited.insert(ei->second);
				}
				while (BBisUnvisited.size() > 0) {
					recurseBasicBlock(*BBisUnvisited.begin());
				}

				if (SpanningTree.size() == size) {
					DEBUG(dbgs()<<"{");
					for (std::set<Edge>::iterator ei = SpanningTree.begin(),
					     ee = SpanningTree.end(); ei != ee; ++ei) {
						DEBUG(dbgs()<< *ei <<",");
					}
					assert(0 && "No edge calculated!");
				}

			}
		}
		if (ReadCount != Counters.size()) {
			errs() << "WARNING: profile information is inconsistent with "
				<< "the current program!\n";
		}
		NumEdgesRead = ReadCount;
	}

	BlockInformation.clear();
	Counters = PIL.getRawBlockCounts();
	if (Counters.size() > 0) {
		ReadCount = 0;
		for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
			if (F->isDeclaration()) continue;
			for (Function::iterator BB = F->begin(), E = F->end(); BB != E; ++BB)
				if (ReadCount < Counters.size())
				// Here the data realm changes from the uint64_t of the file to the
				// double of the ProfileInfo. This conversion would only be safe for
				// values up to 2^52
				BlockInformation[F][BB] = (double)Counters[ReadCount++];
		}
		if (ReadCount != Counters.size()) {
			errs() << "WARNING: profile information is inconsistent with "
				<< "the current program!\n";
		}
	}

	FunctionInformation.clear();
	Counters = PIL.getRawFunctionCounts();
	if (Counters.size() > 0) {
		ReadCount = 0;
		for (Module::iterator F = M.begin(), E = M.end(); F != E; ++F) {
			if (F->isDeclaration()) continue;
			if (ReadCount < Counters.size())
				// Here the data realm changes from the uint64_t of the file to the
				// double of the ProfileInfo. This conversion would only be safe for
				// values up to 2^52
				FunctionInformation[F] = (double)Counters[ReadCount++];
		}
		if (ReadCount != Counters.size()) {
			errs() << "WARNING: profile information is inconsistent with "
				<< "the current program!\n";
		}
	}
	BBTrace.clear();
	const std::vector<uint64_t>& trace=PIL.getRawBBTrace();
	if(trace.size()>0) {
		std::vector<BasicBlock*> BBMap;
		std::unordered_map<BasicBlock*, int> reverse_BBmap;
		label_basic_blocks(M,BBMap,reverse_BBmap);
		for(size_t i=0; i<trace.size(); i++) {
			BBTraceStream::Packet packet;
			
			uint64_t bbid=trace[i];
			if(bbid==BBTraceStream::MemOpID) {
				i++;
				if(i>=trace.size()) {
					errs()<<"Error, truncated MemOp packet in Basic Block trace stream\n";
					exit(1);
				}
				packet.ptype=BBTraceStream::PacketType::MemOp;
				packet.MemAddr=trace[i];
			}else if(bbid==BBTraceStream::FunCallID) {
				i++;
				if(i>=trace.size()) {
					errs()<<"Error, truncated FunCall packet in Basic Block trace stream\n";
					exit(1);
				}
				packet.ptype=BBTraceStream::FunCall;
				if(trace[i]>BBMap.size()) {
					errs() <<"Error, bad block ID in FunCall packet in Basic Block Trace\n";
					exit(1);
				}
				packet.BB=BBMap[trace[i]];
			} else if(bbid==BBTraceStream::FunRetID) {
				packet.ptype=BBTraceStream::FunRet;
			} else {
				packet.ptype=BBTraceStream::BB;
					if(bbid>BBMap.size()) {
					errs() <<"Error, bad block ID in Basic Block Trace\n";
					exit(1);
				}
				packet.BB=BBMap[bbid];
			}
			BBTrace.push_back(packet);
		}
	}
	return false;
}


bool ProfileInfoLoaderPass::startBBTraceStream() {
	BBTraceIndex=0;
	return true;
}

BBTraceStream::Packet ProfileInfoLoaderPass::BBTraceStreamNext() {
	if(BBTraceIndex==BBTrace.size()){
		BBTraceStream::Packet EOFPacket;
		EOFPacket.ptype=BBTraceStream::BBEOF;
		return EOFPacket;
	}
	return BBTrace[BBTraceIndex++];
}