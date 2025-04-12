#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "CFGTransformer.h"

using namespace llvm;

void CFGTransformer::transformToSingleExit(Function &F) {
	if(F.size() == 1) {
		/* errs() << "Function has 1 block, skipping" << "\n"; */
		return;
	}	
	std::vector<ReturnInst*> returnInstructions;
	getReturnInstructions(F, returnInstructions);
	if(returnInstructions.size() == 1) {
		/* errs() << "Function has 1 return instruction, skipping" << "\n"; */
		return;
	}

	BasicBlock* newExitBlock = BasicBlock::Create(F.getContext(), "unified_exit", &F);
	IRBuilder<> builder(newExitBlock);
	Type* returnType = F.getReturnType();
	bool returnsVoid = returnType->isVoidTy();

	PHINode* retValuePhi = returnsVoid ?
		nullptr : builder.CreatePHI(returnType, 0, "unified_exit.return.val");

	errs() << "Redirection happened in function " + F.getName() + "\n";
	redirectReturns(returnInstructions, retValuePhi, returnsVoid, newExitBlock);
	createNewReturnInstruction(newExitBlock, returnsVoid, retValuePhi);
}

void CFGTransformer::getReturnInstructions(Function &F, std::vector<ReturnInst*> &returnInstructions) {
	returnInstructions.reserve(F.size());
	for(BasicBlock& BB : F) {
		auto* terminator = BB.getTerminator();
		pushIfInstructionIsReturn(terminator, returnInstructions);
	}
}

void CFGTransformer::pushIfInstructionIsReturn(Instruction* I, std::vector<ReturnInst*> &vec) {
	if(ReturnInst* RI = dyn_cast<ReturnInst>(I)) {
		vec.push_back(RI);
	}
}

void CFGTransformer::redirectReturns(std::vector<ReturnInst*> &returnInstructions, PHINode* phiInstruction, bool funcReturnsVoid, BasicBlock* newExitBlock) {
	for(ReturnInst* RI : returnInstructions) {
		BasicBlock* returnBlock	= RI->getParent();
		Value* returnVal = nullptr;
		if(!funcReturnsVoid) {
			returnVal = RI->getReturnValue();
			phiInstruction->addIncoming(returnVal, returnBlock);
		}
		IRBuilder<> builder(returnBlock);
		builder.SetInsertPoint(RI);
		builder.CreateBr(newExitBlock);
		RI->eraseFromParent();
	}
}

void CFGTransformer::createNewReturnInstruction(BasicBlock* newExitBlock, bool funcReturnsVoid, PHINode* phiInstruction) {
	IRBuilder<> builder(newExitBlock);
	if(funcReturnsVoid)
		builder.CreateRetVoid();
	else
		builder.CreateRet(phiInstruction);
}
