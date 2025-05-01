#include "CFGAnalysis.h"
#include "llvm/Analysis/CycleAnalysis.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

bool CFGAnalysis::isAcyclic(Function &F, FunctionAnalysisManager &FAM) {
	const CycleInfo &CI = FAM.getResult<CycleAnalysis>(F);
	return CI.toplevel_begin() == CI.toplevel_end();
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
