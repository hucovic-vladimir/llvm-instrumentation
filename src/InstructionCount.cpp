// TODO fix ugly formatting
#include "../headers/BasicBlockWrapper.h"
#include "../headers/InstructionCount.h"
#include "../headers/PassUtilities.h"
#include "../headers/InstrumentationFunctions.h"
#include "../headers/DiamondPattern.h"
#include "../headers/HalfDiamondPattern.h"
#include "../headers/UnconditionalJumpPattern.h"
#include "../headers/FunctionPatterns.h"
#include "../headers/SumOfExitsPattern.h"
#include <algorithm>
#include <llvm/ADT/ilist_node_options.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Passes/PassBuilder.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Demangle/Demangle.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/CFGPrinter.h>
#include <llvm/Support/GraphWriter.h>
#include <llvm/Analysis/CallGraph.h>
#include <filesystem>

namespace fs = std::filesystem;

using blockWrapperMap = std::unordered_map<BasicBlock*, BasicBlockWrapper*>;
using blockCovers = std::map<BasicBlock*, std::set<BasicBlock*>>;

blockWrapperMap wrappers;

const std::string InstructionCount::getLocalArrayName(Module &M) {
	std::string arrayName = "__basicblocks_arr_" + PassUtilities::getFileName(M.getSourceFileName());
	arrayName.erase(std::remove(arrayName.begin(), arrayName.end(), '.'), arrayName.end());
	return arrayName;
}

GlobalVariable* InstructionCount::getOrCreateCounter(Module &M) {
	std::string arrayName = getLocalArrayName(M); 
	GlobalVariable* counter = M.getGlobalVariable(arrayName);
	if(counter) return counter;
	else {
		LLVMContext& CTX = M.getContext();
		counter = new GlobalVariable(M, Type::getInt64PtrTy(CTX), false, GlobalValue::ExternalLinkage, nullptr, arrayName);
	}
	return counter;
}

void InstructionCount::incrementCounter(Module &M, Instruction* insertionPoint, unsigned long bbIndex) {
	LLVMContext& CTX = M.getContext();
	GlobalVariable* counter =	getOrCreateCounter(M);
	IRBuilder<> builder(insertionPoint);
	Value* offset = ConstantInt::get(Type::getInt64Ty(CTX), bbIndex);
	Value* addr = builder.CreateGEP(Type::getInt64Ty(CTX), counter, offset);
	Value* counterValue = builder.CreateLoad(Type::getInt64Ty(CTX), addr);
	Value* newCounterValue = builder.CreateAdd(counterValue, ConstantInt::get(Type::getInt64Ty(CTX), 1));
	builder.CreateStore(newCounterValue, addr);
}

std::vector<OptimizationPattern*> InstructionCount::getOptimizationPatterns(Function &F) {
	std::vector<OptimizationPattern*> patterns;
	// return if the function has only 1 block
	if(F.size() == 1) {
		return patterns;
	}
	std::vector<BasicBlock*> blockQueue;
	std::vector<BasicBlock*> processed;
	blockQueue.push_back(&F.getEntryBlock());
	while(!blockQueue.empty()) {
		BasicBlock* bb = blockQueue.front();
		blockQueue.erase(blockQueue.begin());
		if(processed.size() > 0) {
			if(std::find(processed.begin(), processed.end(), bb) != processed.end()) {
				continue;
			}
		}
		processed.push_back(bb);

		/// check for diamond pattern
		DiamondPattern* diamond = DiamondPattern::checkForPattern(wrappers, wrappers[bb], processed);
		if(diamond) {
			patterns.push_back(diamond);
			BasicBlock* patternExitBlock = diamond->getPatternExitBlock();
			for(BasicBlock* succ : successors(patternExitBlock)) {
				if(std::find(processed.begin(), processed.end(), succ) == processed.end() && std::find(blockQueue.begin(), blockQueue.end(), succ) == blockQueue.end())
					blockQueue.push_back(succ);
			}
			continue;
		}

		/// check for half diamond pattern
		HalfDiamondPattern* halfDiamond = HalfDiamondPattern::checkForPattern(wrappers, wrappers[bb], processed);
		if(halfDiamond) {
			patterns.push_back(halfDiamond);
			BasicBlock* patternExitBlock = halfDiamond->getPatternExitBlock();
			for(BasicBlock* succ : successors(patternExitBlock)) {
				if(std::find(processed.begin(), processed.end(), succ) == processed.end() && std::find(blockQueue.begin(), blockQueue.end(), succ) == blockQueue.end())
					blockQueue.push_back(succ);
			}
			continue;
		}

		/// check for unconditional jump pattern
		UnconditionalJumpPattern* unconditionalJump = UnconditionalJumpPattern::checkForPattern(wrappers, wrappers[bb], processed);
		if(unconditionalJump) {
			patterns.push_back(unconditionalJump);
			BasicBlock* patternExitBlock = unconditionalJump->getPatternExitBlock();
			for(BasicBlock* succ : successors(patternExitBlock)) {
				if(std::find(processed.begin(), processed.end(), succ) == processed.end() && std::find(blockQueue.begin(), blockQueue.end(), succ) == blockQueue.end())
					blockQueue.push_back(succ);
			}
			continue;
		}
		for(BasicBlock* succ : successors(bb)) {
			if(std::find(processed.begin(), processed.end(), succ) == processed.end() && std::find(blockQueue.begin(), blockQueue.end(), succ) == blockQueue.end())
				blockQueue.push_back(succ);
		}
	}
	return patterns;
}

/// @brief Run the pass
/// @param M The module to run the pass on
/// @param MAM The module analysis manager
/// @return The preserved analyses (IR is modified, so none to be safe)
PreservedAnalyses InstructionCount::run(Module &M, ModuleAnalysisManager &MAM){

	unsigned long bbCount = 0;
	LLVMContext& CTX = M.getContext();

	InstrumentationFunctions IF = InstrumentationFunctions(CTX);

	std::vector<FunctionPatterns*> patterns;

	std::vector<BasicBlockWrapper*> wrappersVec;
	for(auto &F : M) {
		for(BasicBlock &BB : F) {
			BasicBlockWrapper* wrapper = new BasicBlockWrapper(bbCount++, &BB);
			wrappersVec.push_back(wrapper);
			wrappers[&BB] = wrapper;
		}
	}

	fs::create_directory(".basicblocks");
	// could be removed later
	/* fs::create_directory(".llfiles"); */
	fs::create_directory(".patterns");

	std::error_code EC;
	std::string sourceFileName = PassUtilities::getFileName(M.getSourceFileName());
	std::string directories = fs::path(M.getSourceFileName()).parent_path().string();
	if(directories.size())
		fs::create_directories(".basicblocks/" + directories);
	raw_fd_ostream bbFile(".basicblocks/" + M.getSourceFileName() + ".json", EC);
	bbFile << "{\n";
	bbFile << PassUtilities::getTabs(1) << "\"blocks\": [\n";
	for(auto& wrapper : wrappersVec) {
		wrapper->getSuccessors(wrappers);
		bbFile << wrapper->toJson(2);
		if(!(wrapper == wrappersVec.back())) {
			bbFile << ",\n";
		}
		else {
			bbFile << "\n";
		}
	}
	bbFile << PassUtilities::getTabs(1) << "]\n";
	bbFile << "}\n";

	for(auto &F : M){
		if(F.isDeclaration()) continue;

		/// get the basic block patterns in this function and add them to the patterns vector
		/// use the nonInstrumentedBlocks to specify which blocks should not be instrumented
		std::vector<OptimizationPattern*> funcPatterns = getOptimizationPatterns(F);
		std::vector<BasicBlock*> nonInstrumentedBlocks;
		for(auto& pattern : funcPatterns) {
			auto patternNonInstrumentedBlocks = pattern->getNonInstrumentedBlocks();
			for(auto& block : patternNonInstrumentedBlocks) {
				nonInstrumentedBlocks.push_back(block->getBB());
			}
		}

		/// If the entry block is not part of any pattern, check for the sum of exits pattern
		if(std::find(nonInstrumentedBlocks.begin(), nonInstrumentedBlocks.end(), &F.getEntryBlock()) == nonInstrumentedBlocks.end()) {
			SumOfExitsPattern* sumOfExits = SumOfExitsPattern::checkForPattern(wrappers, wrappers[&F.getEntryBlock()], nonInstrumentedBlocks);
			if(sumOfExits) {
				funcPatterns.push_back(sumOfExits);
				nonInstrumentedBlocks.push_back(&F.getEntryBlock());
			}
		}

		FunctionPatterns* funcPatternsObj = new FunctionPatterns(&F, funcPatterns);
		if(funcPatternsObj->getPatternCount() > 0)
			patterns.push_back(funcPatternsObj);

		for(auto &BB : F){
			Instruction* insertionPoint = &*BB.getFirstInsertionPt();

			/// instrument the basic block
			if(std::find(nonInstrumentedBlocks.begin(), nonInstrumentedBlocks.end(), &BB) == nonInstrumentedBlocks.end()) {
				incrementCounter(M, insertionPoint, wrappers[&BB]->getId());
			}

			// Ensure that the information is properly exported
			// when the progaram terminates in other ways
			// than returning from the main function
			// todo find a better way to cover all cases
			for(Instruction& I : BB) {
				CallInst* callInst = dyn_cast<CallInst>(&I);
				if(callInst){
					Function* calledFunc = callInst->getCalledFunction();
					if(calledFunc && calledFunc->getName().str() == "exit") {
						IF.insertProfExportCall(M, callInst);
					}
				}
			} // for I
		} // for BB

		/// Insert export call at the return points of the main function
		if(F.getName().str() == "main") {
			Instruction* insertionPoint = &*F.getEntryBlock().getFirstInsertionPt();
			IF.insertProfInitCall(M, insertionPoint);
			std::vector<ReturnInst*> returnInstructions = PassUtilities::getReturnInstructionsFromFunction(F);
			for(Instruction* I : returnInstructions){
				IF.insertProfExportCall(M, I);
			}
		}
	} // for F


	/// write the name of the module, the name of the array and the number of basic blocks to modules.tmp
	std::fstream arraysFile("./modules.tmp", std::ios::out | std::ios::app);	
	if(!arraysFile.is_open()){
		std::cerr << "Failed to open modules.tmp for writing" << "\n";
		exit(1);
	}
	arraysFile << M.getSourceFileName() << "," << getLocalArrayName(M) << "," << bbCount << "\n";


	/// export patterns
	if(patterns.size() > 0){
		std::string sourceFileName = PassUtilities::getFileName(M.getSourceFileName());
		std::string directories = fs::path(M.getSourceFileName()).parent_path().string();
		if(directories.size())
			fs::create_directories(".patterns/" + directories);
		raw_fd_ostream patternFile(".patterns/" + directories + "/" + sourceFileName + ".json", EC);
		patternFile << "{\n";
		patternFile << PassUtilities::getTabs(1) << "\"functions\": [\n";
		for(auto p : patterns) {
			if(p->getPatternCount() == 0) continue;
			patternFile << p->toJson(2);
			if(p == patterns.back()) { patternFile << "\n"; }
			else { patternFile << ",\n"; }
		}
		patternFile << PassUtilities::getTabs(1) << "]\n";
		patternFile << "}\n";
	}

	return PreservedAnalyses::none();
}

/// @brief Static entry point
PassPluginLibraryInfo getInstructionCountPluginInfo(){
	const auto callback = [](PassBuilder &PB) {
		PB.registerOptimizerLastEPCallback(
				[](ModulePassManager &MPM, OptimizationLevel Level){
					MPM.addPass(InstructionCount());
				}
				);
	};
	return {LLVM_PLUGIN_API_VERSION, "InstructionCount", "v0.1", callback};
}

/// @brief Dynamic entry point
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo(){
	return getInstructionCountPluginInfo();
}



