#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"

using namespace llvm;

class CFGTransformer {
	public:
		static void transformToSingleExit(Function &F);
	private:
		static void getReturnInstructions(Function &F, std::vector<ReturnInst*> &returnInstructions);
		static void redirectReturns(std::vector<ReturnInst*> &returnInstructions, PHINode* phiInstruction, bool funcReturnsVoid, BasicBlock* newExitBlock); 
		static void createNewReturnInstruction(BasicBlock* newExitBlock, bool funcReturnsVoid, PHINode* phiInstruction);
static void pushIfInstructionIsReturn(Instruction* I, std::vector<ReturnInst*> &vec);
};
