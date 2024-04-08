// TODO fix ugly formatting
#include "../headers/BasicBlockWrapper.h"
#include "../headers/InstructionCount.h"
#include "../headers/PassUtilities.h"
#include "../headers/InstrumentationFunctions.h"
#include "../headers/DiamondPattern.h"
#include "../headers/HalfDiamondPattern.h"
#include "../headers/UnconditionalJumpPattern.h"
#include "../headers/FunctionPatterns.h"
#include <algorithm>
#include <bits/node_handle.h>
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



using blockWrapperMap = std::unordered_map<BasicBlock*, BasicBlockWrapper*>;
blockWrapperMap wrappers;

/// @todo Move elsewhere
/// @todo rename
using blockCovers = std::map<BasicBlock*, std::set<BasicBlock*>>;

const std::string getFileName(const std::string& path);

void enqueueSuccessors(BasicBlock* currentBlock, std::vector<BasicBlock*>& blockQueue) {
	for(BasicBlock* succ : successors(currentBlock)) {
		if(std::find(blockQueue.begin(), blockQueue.end(), succ) == blockQueue.end()) {
			blockQueue.push_back(succ);
		}
	}
}

void markBranchBlockCovered(BasicBlock* branchingBlock, blockCovers &bc) {
	for(BasicBlock* succ : successors(branchingBlock)) {
		bc[succ].insert(branchingBlock);
	}
}


std::vector<BasicBlock*> checkAllBlocksCovered(std::vector<BasicBlock*> covered, std::vector<BasicBlock*> allBlock) {
	std::vector<BasicBlock*> uncovered;
	for(BasicBlock* block : allBlock) {
		if(std::find(covered.begin(), covered.end(), block) == covered.end()) {
			uncovered.push_back(block);
		}
	}
	return uncovered;
}

void doNothing() {
	return;
}


void loopsAnalysis(LoopInfo &LI, Function &F) {
	PostDominatorTree PDT(F);
	PDT.recalculate(F);

	auto root = PDT.getRoot();
	std::vector<BasicBlock*> nodes;
	nodes.push_back(root);

	errs() << "All basic blocks in function " << F.getName() << "\n";
	for(BasicBlock &bb : F) {
		errs() << "\tBasic block: " << bb.getName() << "\n";
	} 
	auto loops = LI.getLoopsInPreorder();
	if(loops.empty()) {
		errs() << "No loops found in function " << F.getName() << "\n"; 
	}
	for(Loop* loop : loops) {
		errs() << "Loop header: " << loop->getHeader()->getName() << "\n";
		SmallVector<BasicBlock*> loopExits;
		loop->getLoopLatches(loopExits);
		for(BasicBlock* loopExit : loopExits) {
			errs() << "\tLoop latch: " << loopExit->getName() << "\n";
		}
		/// Run the loop simplify pass
		/// This pass will ensure that the loop is in a form that can be optimized
	}
	F.viewCFG();
}

const std::string getLocalArrayName(Module &M) {
	std::string arrayName = "__basicblocks_arr_" + getFileName(M.getSourceFileName());
	arrayName.erase(std::remove(arrayName.begin(), arrayName.end(), '.'), arrayName.end());
	return arrayName;
}

GlobalVariable* getOrCreateCounter(Module &M) {
	std::string arrayName = getLocalArrayName(M); 
	GlobalVariable* counter = M.getGlobalVariable(arrayName);
	if(counter) return counter;
	else {
		LLVMContext& CTX = M.getContext();
		counter = new GlobalVariable(M, Type::getInt64PtrTy(CTX), false, GlobalValue::ExternalLinkage, nullptr, arrayName);
	}
	return counter;
}

/// @todo move elsewhere
void incrementCounter(Module &M, Instruction* insertionPoint, unsigned long bbIndex) {
	LLVMContext& CTX = M.getContext();
	GlobalVariable* counter =	getOrCreateCounter(M);
	IRBuilder<> builder(insertionPoint);
	Value* offset = ConstantInt::get(Type::getInt64Ty(CTX), bbIndex);
	Value* addr = builder.CreateGEP(Type::getInt64Ty(CTX), counter, offset);
	Value* counterValue = builder.CreateLoad(Type::getInt64Ty(CTX), addr);
	Value* newCounterValue = builder.CreateAdd(counterValue, ConstantInt::get(Type::getInt64Ty(CTX), 1));
	builder.CreateStore(newCounterValue, addr);
}


/// @todo Move elsewhere
/// @brief Get the file name from a path
/// @param path The path to get the file name from
/// @return The file name
const std::string getFileName(const std::string& path) {
	size_t pos = path.find_last_of("/\\");
	if (pos != std::string::npos) {
		return path.substr(pos + 1);
	}
	return path;
}

/// @todo Move elsewhere
/// @brief Get all return instructions from a function
/// @param F The function to get the return instructions from
/// @return A vector of all return instructions in the function
const std::vector<ReturnInst*> getReturnInstructionsFromFunction(Function &F){
	std::vector<ReturnInst*> returnInstructions;
	for(auto &BB : F){
		for(auto &I : BB){
			if(auto* ret = dyn_cast<ReturnInst>(&I)){
				returnInstructions.push_back(ret);
			}
		}
	}
	return returnInstructions;
}


/// @todo Move elsewhere
/// @brief Return the demangled named of a function which is a parent of the basic block BB
/// If the parent is not found, return "NO_PARENT_FUNCTION_FOUND"
/// @param BB The basic block to get the parent function name from
/// @return The demangled name of the parent function
const std::string getBasicBlockDemangledFunctionName(BasicBlock& BB){
	Function* parent = BB.getParent();
	if(!parent) {
		return "NO_PARENT_FUNCTION_FOUND";
	}
	return demangle(parent->getName().str());
}

/// @todo Move elsewhere
/// @brief Return the module name (original source code name) of a basic block
/// If the parent module/function is not found, return "NO_PARENT_MODULE_FOUND" or "NO_PARENT FUNCTION_FOUND"
/// @param BB The basic block to get the module name from
/// @return The module name of the basic block
const std::string getBasicBlockModuleName(BasicBlock& BB) {
	Function* parent = BB.getParent();
	if(!parent) {
		return "NO_PARENT_FUNCTION_FOUND";
	}
	Module* module = parent->getParent();
	if(!module) {
		return "NO_PARENT_MODULE_FOUND";
	}
	return module->getSourceFileName();
}

/// @todo move elsewhere
/// @brief Get the basic block information to be exported from the pass
/// @param BB The basic block to get the information from
/// @param bbIndex The ID of the basic block
const std::string getBBInfo(BasicBlock& BB, unsigned long bbIndex){
	unsigned long id = bbIndex;
	std::string moduleName = getBasicBlockModuleName(BB);
	std::string functionName = getBasicBlockDemangledFunctionName(BB);
	std::string label = BB.getName().str() != "" ? BB.getName().str() : "NO_LABEL";
	size_t size = BB.sizeWithoutDebug();
	std::pair<int, int> startEndLines = PassUtilities::getBasicBlockStartEndLines(BB);

	std::stringstream ss;
	ss << id << "," << moduleName <<  "," << functionName << "," << label << "," 
		 << size  << "," << startEndLines.first << "," << startEndLines.second << "\n";

	return ss.str();
}

///	@todo Move elsewhere
///	@brief Write the basic block information to a file
///	@param bbInfo The string containing the basic block info
///	@param filename The name of the file to write to
///	@throws std::runtime_error if the file cannot be opened for writing
///	@note If the file does not exist, it will be created
///	@note If the file does exist, the basic block info will be appended to the file
void writeBBInfoToFile(const std::string& bbInfo, std::string filename){
	std::fstream bbInfoFile(filename, std::ios::app);
	if(!bbInfoFile.is_open()) {
		std::cerr << "Failed to open file " << filename << " for writing" << std::endl;
		throw std::runtime_error("Could not open " + filename + " for writing");
	}
	else {
		if(bbInfoFile.tellg() == 0) {
			// write the header
			bbInfoFile << "ID,MODULE,FUNCTION,LABEL,INSTRCOUNT,START_LINE,END_LINE\n";
		}
	} // else

	bbInfoFile << bbInfo;
	bbInfoFile.flush();
	bbInfoFile.close();
}


bool doesFunctionContainLoops(LoopInfo &LI) {
	auto loops = LI.getLoopsInPreorder();
	return !loops.empty();
}

std::vector<OptimizationPattern*> getOptimizedMapForNoLoopFunction(Function &F) {
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
		DiamondPattern* diamond = DiamondPattern::checkForPattern(wrappers, wrappers[bb], processed);
		if(diamond) {
			errs() << "Diamond pattern found in " << F.getName() << "\n";
			patterns.push_back(diamond);
			BasicBlock* patternExitBlock = diamond->getPatternExitBlock();
			for(BasicBlock* succ : successors(patternExitBlock)) {
				if(std::find(processed.begin(), processed.end(), succ) == processed.end() && std::find(blockQueue.begin(), blockQueue.end(), succ) == blockQueue.end())
					blockQueue.push_back(succ);
			}
			continue;
		}
		HalfDiamondPattern* halfDiamond = HalfDiamondPattern::checkForPattern(wrappers, wrappers[bb], processed);
		if(halfDiamond) {
			errs() << "Half Diamond pattern found in " << F.getName() << "\n";
			patterns.push_back(halfDiamond);
			BasicBlock* patternExitBlock = halfDiamond->getPatternExitBlock();
			for(BasicBlock* succ : successors(patternExitBlock)) {
				if(std::find(processed.begin(), processed.end(), succ) == processed.end() && std::find(blockQueue.begin(), blockQueue.end(), succ) == blockQueue.end())
					blockQueue.push_back(succ);
			}
			continue;
		}
		UnconditionalJumpPattern* unconditionalJump = UnconditionalJumpPattern::checkForPattern(wrappers, wrappers[bb], processed);
		if(unconditionalJump) {
			errs() << "Unconditional jump pattern found in " << F.getName() << "\n";
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

	std::error_code EC;
	raw_fd_ostream bbFile(".basicblocks/" + getFileName(M.getName().str()) + ".json", EC);
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
		SmallVector<BasicBlock*> coveredBlocks;
		FunctionPassManager FPM;
		FPM.addPass(RequireAnalysisPass<LoopAnalysis, Function>());
		FunctionAnalysisManager &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
		FPM.run(F, FAM);
		/* LoopInfo &LI = FAM.getResult<LoopAnalysis>(F); */

		/* F.viewCFG(); */
		std::vector<OptimizationPattern*> funcPatterns = getOptimizedMapForNoLoopFunction(F);
	

		std::vector<BasicBlock*> nonInstrumentedBlocks;
		for(auto& pattern : funcPatterns) {
			auto patternNonInstrumentedBlocks = pattern->getNonInstrumentedBlocks();
			for(auto& block : patternNonInstrumentedBlocks) {
				nonInstrumentedBlocks.push_back(block->getBB());
			}
		}

		FunctionPatterns* funcPatternsObj = new FunctionPatterns(&F, funcPatterns);
		if(funcPatternsObj->getPatternCount() > 0)
			patterns.push_back(funcPatternsObj);

		errs() << "THE FUNCTION " << F.getName() << " HAS " << funcPatternsObj->getPatternCount() << " PATTERNS\n";

		for(auto &BB : F){
			Instruction* insertionPoint = &*BB.getFirstInsertionPt();
			std::string bbInfo = getBBInfo(BB, bbCount);
			/* writeBBInfoToFile(bbInfo, "./bbinfo.csv"); */

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
			std::vector<ReturnInst*> returnInstructions = getReturnInstructionsFromFunction(F);
			for(Instruction* I : returnInstructions){
				IF.insertProfExportCall(M, I);
			}
		}
	} // for F


	/// dump the instrumented module to a file in the .llfiles directory
	/// mostly for debugging purposes - could be turned on or off in compilation
	raw_fd_ostream llFileStream(".llfiles/" + getFileName(M.getName().str()) + ".ll", EC);
	if(EC){
		std::cerr << "Failed to open " << M.getName().str() << " for writing" << "\n";
		exit(1);
	}
	M.print(llFileStream, nullptr);

	std::fstream arraysFile("./modules.tmp", std::ios::out | std::ios::app);	
	if(!arraysFile.is_open()){
		std::cerr << "Failed to open moduleArrays for writing" << "\n";
		exit(1);
	}
	arraysFile << M.getSourceFileName() << "," << getLocalArrayName(M) << "," << bbCount << "\n";


	if(patterns.size() > 0){
		raw_fd_ostream patternFile(".patterns/" + getFileName(M.getName().str()) + ".json", EC);
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


	/* CallGraph CG(M); */
	/* for (const auto &I : CG) { */
	/* 	const llvm::CallGraphNode *N = I.second.get(); */
	/* 	const llvm::Function *F = N->getFunction(); */

	/* 	if (F) { */
	/* 		llvm::errs() << "Function: " << F->getName() << "\n"; */
	/* 		for (const auto &CI : *N) { */
	/* 			if (const llvm::Function *Callee = CI.second->getFunction()) { */
	/* 				const Module* m = Callee->getParent(); */
	/* 				llvm::errs() << "  calls function: " << Callee->getName() << " in module " << Callee << " in module" << m->getName() << "\n"; */
	/* 			} */
	/* 		} */
	/* 	} */
	/* } */


	return PreservedAnalyses::none();
}
/// @brief Get the plugin info for the pass
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



/// @brief Register the pass with the pass manager
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo(){
	return getInstructionCountPluginInfo();
}



