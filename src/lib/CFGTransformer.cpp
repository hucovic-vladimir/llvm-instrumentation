#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "CFGTransformer.h"
#include "CFGAnalysis.h"
#include "../../headers/InstrumentationFunctions.h"

using namespace llvm;


void CFGTransformer::transformToSingleExit(Function &F) {
	if(F.size() == 1) {
		return;
	}	
	std::vector<ReturnInst*> returnInstructions;
	getReturnInstructions(F, returnInstructions);
	if(returnInstructions.size() == 1) {
		return;
	}
	
	BasicBlock* newExitBlock = BasicBlock::Create(F.getContext(), "unified_exit", &F);
	IRBuilder<> builder(newExitBlock);
	Type* returnType = F.getReturnType();
	bool returnsVoid = returnType->isVoidTy();

	PHINode* retValuePhi = returnsVoid ?
		nullptr : builder.CreatePHI(returnType, 0, "unified_exit.return.val");
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


void CFGTransformer::insertPathCounterIncrement(BasicBlock& edge, int edgeValue, AllocaInst* pathCounterVar) {
	IRBuilder<> builder(&edge);
	Value* currentValue = builder.CreateLoad(builder.getInt32Ty(), pathCounterVar, "current_value");
	Value* newValue = builder.CreateAdd(currentValue, builder.getInt32(edgeValue), "new_value");
	builder.CreateStore(newValue, pathCounterVar);
}

void CFGTransformer::resetCounterAlongBackedge(AllocaInst* counter, BasicBlock* backEdge) {
	IRBuilder<> builder(backEdge);
	builder.SetInsertPoint(backEdge->getTerminator());
	builder.CreateStore(builder.getInt32(0), counter);
} 

void CFGTransformer::insertPrintOfCounter(Function &F, BasicBlock& exit, AllocaInst* counter) {
    InstrumentationFunctions IF(F.getContext());
    Module &M = *F.getParent();
    
    Instruction* term = exit.getTerminator(); 
    IRBuilder<> builder(term);
    
    Value* loadedCounter = builder.CreateLoad(builder.getInt32Ty(), counter, "loaded_counter"); 
    IF.insertPrintfCall(M, term, "Counter: \%d\n", {loadedCounter});
}

void CFGTransformer::addInstrumentedEdges(BasicBlock& start, DAG& dag, AllocaInst* pathCounterVar) {
	// Make a copy of successors
	std::vector<BasicBlock*> successorBlocks;
	for (BasicBlock* succ : successors(&start)) {
		successorBlocks.push_back(succ);
	}

	for (BasicBlock* succ : successorBlocks) {
		if (&start == succ) continue;

		// Get the edge value
		GraphEdge* edge = dag.findEdge(&start, succ);
		if(edge == nullptr) {
			edge = dag.findBackedge(&start, succ);
			assert(edge != nullptr && "Back edge not found");
		}
		int edgeValue = edge->getValue();
		if (edgeValue != 0 || edge->isBackedge) {
			// Create the new block
			BasicBlock* instrumentedBasicBlock = BasicBlock::Create(
					start.getContext(), 
					"EDGE +" + std::to_string(edgeValue) + "_" +start.getName(),
					start.getParent(), 
					succ
					);

			if(edgeValue != 0) {
				insertPathCounterIncrement(*instrumentedBasicBlock, edgeValue, pathCounterVar);
			}

			// TODO: Handle other possible terminator instructions

			IRBuilder<> builderNewBlock(instrumentedBasicBlock);
			builderNewBlock.CreateBr(succ);

			updatePHINodes(start, succ, instrumentedBasicBlock);
			redirectTerminatorOperands(start, succ, instrumentedBasicBlock);

			if(edge->isBackedge) {
				insertPrintOfCounter(*start.getParent(), *instrumentedBasicBlock, pathCounterVar);
				// errs() << "Instr. block: \n";
				// instrumentedBasicBlock->print(errs());
			}
		}
	}
}

void CFGTransformer::addInstrumentedEdges(DAG& dag, AllocaInst* pathCounterVar) {
    Function* F = dag.getFunction();
    if (F->empty() || F->size() == 1) {
        return;
    }
    
    std::vector<BasicBlock*> blocks;
    for (BasicBlock& BB : *F) {
        blocks.push_back(&BB);
    }
    
    for (BasicBlock* BB : blocks) {
        if (!succ_empty(BB)) {
            addInstrumentedEdges(*BB, dag, pathCounterVar);
        }
    }
}
