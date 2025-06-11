#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "CFGTransformer.h"
#include "SSACounterManager.h"
#include <set>
#include "CFGAnalysis.h"
#include "../../headers/InstrumentationFunctions.h"
#include <regex>

using namespace llvm;

std::string removeBracketsAndNewlinesRegex(const std::string& input) {
	std::regex pattern(R"(\n?\s*\[.*?\]\n?)");
	return std::regex_replace(input, pattern, "");
	return input;
}

void initializePathRegister(BasicBlock* where, AllocaInst* pathCounterVar, int initValue) {
	IRBuilder<> builder(where->getTerminator());

	Value* init = ConstantInt::get(Type::getInt64Ty(where->getContext()), initValue);
	builder.CreateStore(init, pathCounterVar);

	string oldName = where->getName().str();
	string newName = oldName + "\n [ r = " + std::to_string(initValue) + " ] ";
	where->setName(newName);
}

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
	Value* currentValue = builder.CreateLoad(builder.getInt64Ty(), pathCounterVar, "current_value");
	Value* newValue = builder.CreateAdd(currentValue, builder.getInt64(edgeValue), "new_value");
	builder.CreateStore(newValue, pathCounterVar);
}

void CFGTransformer::insertPrintOfCounter(Function &F, BasicBlock& exit, AllocaInst* counter) {
	InstrumentationFunctions IF(F.getContext());
	Module &M = *F.getParent();

	Instruction* term = exit.getTerminator(); 
	IRBuilder<> builder(term);

	Value* loadedCounter = builder.CreateLoad(builder.getInt64Ty(), counter, "loaded_counter"); 
	IF.insertPrintfCall(M, term, "Counter: \%d\n", {loadedCounter});
}

void CFGTransformer::incrementPathCounter(GlobalVariable* pathCounterArr, AllocaInst* pathCounterVar, BasicBlock* exit) {
	LLVMContext& context = exit->getContext();
	IRBuilder<> builder(exit->getTerminator()); // Insert before the terminator

	// Load the path counter value (i32) - this is our index
	Value* pathIndex = builder.CreateLoad(Type::getInt64Ty(context), pathCounterVar, "path_index");

	// Convert i32 index to i64 for GEP
	Value* pathIndex64 = builder.CreateZExt(pathIndex, Type::getInt64Ty(context), "path_index_64");

	// Get pointer to the array element at index pathIndex
	Value* elementPtr = builder.CreateInBoundsGEP(
			pathCounterArr->getValueType(),  // Array type
			pathCounterArr,                  // Base pointer (the global array)
			{builder.getInt64(0), pathIndex64}, // Indices: [0][pathIndex]
			"path_element_ptr"
			);

	// Load current value at that index
	Value* currentCount = builder.CreateLoad(Type::getInt64Ty(context), elementPtr, "current_count");

	// Increment by 1
	Value* newCount = builder.CreateAdd(currentCount, builder.getInt64(1), "new_count");

	// Store the incremented value back
	builder.CreateStore(newCount, elementPtr);

	string oldName = exit->getName().str();
	string newName = oldName + "\n" + "[ paths(r)++; ]" ;
	exit->setName(newName);
}

void CFGTransformer::incrementPathCounter(GlobalVariable* pathCounterArr, int constantIndex, BasicBlock* exit) {
	LLVMContext& context = exit->getContext();
	IRBuilder<> builder(exit->getTerminator());

	// Use the constant index directly
	Value* pathIndex = builder.getInt64(constantIndex);

	// Get pointer to the array element at index pathIndex
	Value* elementPtr = builder.CreateInBoundsGEP(
			pathCounterArr->getValueType(),  
			pathCounterArr,                  
			{builder.getInt64(0), pathIndex}, 
			"path_element_ptr"
			);

	// Load current value at that index
	Value* currentCount = builder.CreateLoad(Type::getInt64Ty(context), elementPtr, "current_count");

	// Increment by 1
	Value* newCount = builder.CreateAdd(currentCount, builder.getInt64(1), "new_count");

	// Store the incremented value back
	builder.CreateStore(newCount, elementPtr);

	string oldName = exit->getName().str();
	string newName = oldName + "\n" + "[ paths(" + std::to_string(constantIndex) +  ")++; ]" ;
	exit->setName(newName);
}


void CFGTransformer::instrumentChords(DAG* dag, AllocaInst* pathCounterVar, GlobalVariable* pathCounterArr) {
	BasicBlock* entryBlock = dag->getEntry()->getBlock();
	BasicBlock* exit = dag->getExit()->getBlock();
	initializePathRegister(entryBlock, pathCounterVar, 0);
	incrementPathCounter(pathCounterArr, pathCounterVar, exit);
	for(GraphEdge* chord : dag->getChords()) {
		if(chord->isDummyEdgeFromEntry()) {
			GraphEdge* actualBackedge = dag->getBackedgeFromDummyEdge(chord);
			BasicBlock* backedgeSrc = actualBackedge->getSrc()->getBlock();
			BasicBlock* backedgeDst = actualBackedge->getDst()->getBlock();
			assert(actualBackedge && "Actual backedge exists for dummy edge\n");	
			BasicBlock* edgeBlock = insertEdgeBlockBetween(backedgeSrc, backedgeDst);
			incrementPathCounter(pathCounterArr, pathCounterVar, edgeBlock);
			initializePathRegister(edgeBlock, pathCounterVar, chord->getIncrementValue());
		}
		else if(chord->isDummyEdgeToExit() && chord->getIncrementValue() != 0) {
			GraphEdge* actualBackedge = dag->getBackedgeFromDummyEdge(chord);
			BasicBlock* backedgeSrc = actualBackedge->getSrc()->getBlock();
			instrumentEdge(backedgeSrc, pathCounterVar, chord->getIncrementValue());
		}
		else if(chord->getIncrementValue() != 0) {
			BasicBlock* src = chord->getSrc()->getBlock();
			BasicBlock* dst = chord->getDst()->getBlock();
			BasicBlock* edgeBlock = insertEdgeBlockBetween(src, dst);
			instrumentEdge(edgeBlock, pathCounterVar, chord->getIncrementValue());
		}
	}
}

BasicBlock* CFGTransformer::insertEdgeBlockBetween(BasicBlock* src, BasicBlock* dst) {
	// Get the function context
	Function* function = src->getParent();
	LLVMContext& context = function->getContext();

	// Create a new basic block
	BasicBlock* newBlock = BasicBlock::Create(context, "EDGE " + removeBracketsAndNewlinesRegex(src->getName().str()) + " -> " + removeBracketsAndNewlinesRegex(dst->getName().str()) , function);

	// Get the terminator instruction of the source block
	Instruction* srcTerminator = src->getTerminator();

	// Handle different types of terminators
	if (BranchInst* branchInst = dyn_cast<BranchInst>(srcTerminator)) {
		if (branchInst->isUnconditional()) {
			// Unconditional branch: src -> dst becomes src -> newBlock -> dst
			assert(branchInst->getSuccessor(0) == dst && "Unconditional branch target mismatch");

			// Update the branch to point to newBlock
			branchInst->setSuccessor(0, newBlock);
		} else {
			// Conditional branch: update the appropriate successor
			for (unsigned i = 0; i < branchInst->getNumSuccessors(); ++i) {
				if (branchInst->getSuccessor(i) == dst) {
					branchInst->setSuccessor(i, newBlock);
					break;
				}
			}
		}
	} else if (SwitchInst* switchInst = dyn_cast<SwitchInst>(srcTerminator)) {
		// Handle switch instruction
		// Update default case if it points to dst
		if (switchInst->getDefaultDest() == dst) {
			switchInst->setDefaultDest(newBlock);
		}

		// Update any case that points to dst
		for (auto& switchCase : switchInst->cases()) {
			if (switchCase.getCaseSuccessor() == dst) {
				switchCase.setSuccessor(newBlock);
			}
		}
	} else if (InvokeInst* invokeInst = dyn_cast<InvokeInst>(srcTerminator)) {
		// Handle invoke instruction (has normal and unwind destinations)
		if (invokeInst->getNormalDest() == dst) {
			invokeInst->setNormalDest(newBlock);
		} else if (invokeInst->getUnwindDest() == dst) {
			invokeInst->setUnwindDest(newBlock);
		}
	}

	// Update PHI nodes in dst to reference newBlock instead of src
	for (Instruction& inst : *dst) {
		if (PHINode* phi = dyn_cast<PHINode>(&inst)) {
			// Find incoming values from src and update them to come from newBlock
			for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
				if (phi->getIncomingBlock(i) == src) {
					phi->setIncomingBlock(i, newBlock);
				}
			}
		} else {
			// PHI nodes are always at the beginning of a block
			break;
		}
	}

	// Create an unconditional branch from newBlock to dst
	BranchInst::Create(dst, newBlock);

	return newBlock;
}

void CFGTransformer::instrumentEdge(BasicBlock* edge, AllocaInst* counter, int edgeIncrement) {
	// Create an IR builder for the edge block
	IRBuilder<> builder(edge->getTerminator());

	// Load the current value of the counter
	Value* currentValue = builder.CreateLoad(builder.getInt64Ty(), counter, "current_value");

	// Increment the counter by the edge increment value
	Value* newValue = builder.CreateAdd(currentValue, builder.getInt64(edgeIncrement), "new_value");

	// Store the new value back to the counter
	builder.CreateStore(newValue, counter);
	string oldName = edge->getName().str();
	string newName = oldName + "\n" + (edgeIncrement >= 0 ? ("[ r = r +" + std::to_string(edgeIncrement) + " ]") : ("[ r = r - " + std::to_string(-edgeIncrement) + " ]"));
	edge->setName(newName);
}


void CFGTransformer::addInstrumentedEdges(DAG* dag, AllocaInst* pathCounterVar, GlobalVariable* pathCounterArr) {
	BasicBlock* entryBlock = dag->getEntry()->getBlock();
	BasicBlock* exit = dag->getExit()->getBlock();
	initializePathRegister(entryBlock, pathCounterVar, 0);
	incrementPathCounter(pathCounterArr, pathCounterVar, exit);
	for(GraphEdge* edge : dag->getEdges()) {
		if(edge->isNormal() && edge->getValue() != 0) {
			BasicBlock* src = edge->getSrc()->getBlock();
			BasicBlock* dst = edge->getDst()->getBlock();
			BasicBlock* edgeBlock = insertEdgeBlockBetween(src, dst);
			instrumentEdge(edgeBlock, pathCounterVar, edge->getValue());
		}
		else if(edge->isDummyEdgeFromEntry()) {
			GraphEdge* actualBackedge = dag->getBackedgeFromDummyEdge(edge);
			BasicBlock* backedgeSrc = actualBackedge->getSrc()->getBlock();
			BasicBlock* backedgeDst = actualBackedge->getDst()->getBlock();
			assert(actualBackedge && "Actual backedge exists for dummy edge\n");	
			BasicBlock* edgeBlock = insertEdgeBlockBetween(backedgeSrc, backedgeDst);
			incrementPathCounter(pathCounterArr, pathCounterVar, edgeBlock);
			initializePathRegister(edgeBlock, pathCounterVar, edge->getValue());
		}
		else if(edge->isDummyEdgeToExit() && edge->getValue() != 0) {
			GraphEdge* actualBackedge = dag->getBackedgeFromDummyEdge(edge);
			BasicBlock* backedgeSrc = actualBackedge->getSrc()->getBlock();
			instrumentEdge(backedgeSrc, pathCounterVar, edge->getValue());
		}
	}
}
