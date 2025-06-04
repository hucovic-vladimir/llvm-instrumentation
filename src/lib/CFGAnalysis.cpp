#include "CFGAnalysis.h"
#include "llvm/Analysis/CycleAnalysis.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Dominators.h"

using namespace llvm;

bool CFGAnalysis::isAcyclic(Function &F, FunctionAnalysisManager &FAM) {
	const CycleInfo &CI = FAM.getResult<CycleAnalysis>(F);
	return CI.toplevel_begin() == CI.toplevel_end();
}

map<BasicBlock*, BasicBlock*> CFGAnalysis::getBackEdges(Function &F, FunctionAnalysisManager &FAM) {
   const DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
   std::map<BasicBlock*, BasicBlock*> backEdges;
   
   for (BasicBlock &BB : F) {
   	for (BasicBlock *Succ : successors(&BB)) {
   		// If successor dominates current block, it's a back-edge
   		if (DT.dominates(Succ, &BB)) {
   			backEdges[&BB] = Succ;
   		}
   	}
   }
   
   return backEdges;
}

BasicBlock* CFGAnalysis::getSingleExit(Function &F) {
	for(BasicBlock& BB : F) {
		if(dyn_cast<ReturnInst>(BB.getTerminator()))
			return &BB;
	}
	return nullptr;
}

bool CFGAnalysis::isTerminatorBlock(BasicBlock &BB) {
	if(BB.empty() || !BB.getTerminator())
		return true;
	return succ_begin(&BB) == succ_end(&BB);
}
