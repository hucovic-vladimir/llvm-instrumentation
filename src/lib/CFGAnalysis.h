#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include <map>

using namespace llvm;
using namespace std;

class CFGAnalysis {
	public: 
		static bool isAcyclic(Function &F, FunctionAnalysisManager &FAM);
		static BasicBlock* getSingleExit(Function &F);
		static bool isTerminatorBlock(BasicBlock &BB);
		static map<BasicBlock*, BasicBlock*> getBackEdges(Function &F, FunctionAnalysisManager &FAM); 
	private:
};
