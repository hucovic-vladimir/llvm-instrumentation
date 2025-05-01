#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"

using namespace llvm;

class CFGAnalysis {
	public: 
		static bool isAcyclic(Function &F, FunctionAnalysisManager &FAM);
		static BasicBlock* getSingleExit(Function &F);
		static bool isTerminatorBlock(BasicBlock &BB);
	private:
};
