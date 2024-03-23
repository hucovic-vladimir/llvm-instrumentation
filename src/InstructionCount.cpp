// TODO fix ugly formatting

#include "../headers/InstructionCount.h"
#include "../headers/PassUtilities.h"
#include "../headers/InstrumentationFunctions.h"
#include <algorithm>
#include <bits/node_handle.h>
#include <llvm/ADT/ilist_node_options.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/PassManager.h>
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

void dominatorAnalysis(Function &F, LoopInfo &LI) {
	std::vector<BasicBlock*> coveredBlocks;
	blockCovers blockCoversVector;
	std::vector<BasicBlock*> blockQueue;

	auto loops = LI.getLoopsInPreorder();
	SmallVector<BasicBlock*> loopHeaders;
	std::map<BasicBlock*, std::set<BasicBlock*>> loopExitMap;
	for(Loop* loop : loops) {
		SmallVector<BasicBlock*> loopExits;
		BasicBlock* loopHeader = loop->getHeader();
		loopHeaders.push_back(loopHeader);
		loop->getExitBlocks(loopExits);
		loopExitMap[loopHeader] = std::set<BasicBlock*>(loopExits.begin(), loopExits.end());
		errs() << "Loop header: " << loopHeader->getName() << "\n";
		for(BasicBlock* loopExit : loopExits) {
			errs() << "\tLoop exit: " << loopExit->getName() << "\n";
		}
	}

	BasicBlock* entryBlock = &F.getEntryBlock();
	blockQueue.push_back(entryBlock);
	while(!blockQueue.empty()) {
		BasicBlock* currentBlock = blockQueue.front();
		errs() << "Processing block " << currentBlock->getName() << "\n";
		blockQueue.erase(blockQueue.begin());
		if(std::find(loopHeaders.begin(), loopHeaders.end(), currentBlock) != loopHeaders.end()){
			errs() << "Block " << currentBlock->getName() << " is a loop header, skipping past the loop\n";
			for(BasicBlock* loopExit : loopExitMap[currentBlock]) {
				blockQueue.push_back(loopExit);
				errs() << "\tAdding loop exit " << loopExit->getName() << " to the queue\n";
			}
			continue;
		}
		if(!currentBlock) { errs() << "Current block null, exiting.\n"; return;}
		if(currentBlock->getName().str() == "") { errs() << "Current block has no name, exiting.\n"; return; }
		// 0 Predecessors - it is the entry block
		if(pred_size(currentBlock) == 0) { 
			enqueueSuccessors(currentBlock, blockQueue);
			errs() << "Block " << currentBlock->getName() << " is an entry block and will be covered by any exit block\n";
			coveredBlocks.push_back(currentBlock);
			continue; 
		}
		// 0 successors - it is an exit block, TODO all the exit blocks should cover the entry block
		if(succ_size(currentBlock) == 0) { errs() << "Block " << currentBlock->getName() << " is an exit block, returning back\n"; continue; }
		// More than 1 successors, the function branches at this block
		else if(succ_size(currentBlock) > 1) {
			errs() << "Block " << currentBlock->getName() << " is a branch block\n";
		}
		enqueueSuccessors(currentBlock, blockQueue);
		// 1 predecessor - This block can cover the predecessor
		if(pred_size(currentBlock) == 1) {
			BasicBlock* pred = *pred_begin(currentBlock);
			errs() << "Block " << currentBlock->getName() << " has one predecessor: " << pred->getName() << "\n";
			coveredBlocks.push_back(currentBlock);
		}
		// Multiple predecessors - This block cannot cover all predecessors
		else if(pred_size(currentBlock) > 1) {
			BasicBlock* succ = *succ_begin(currentBlock);
			blockCoversVector[succ].insert(currentBlock);
			coveredBlocks.push_back(currentBlock);
			errs() << "Block " << currentBlock->getName() << " has multiple predecessors, marking it as covered (by itself)\n";
		}
	}

	std::vector<BasicBlock*> allBlocks;
	for(BasicBlock &bb : F) {
		allBlocks.push_back(&bb);
	}
	std::vector<BasicBlock*> uncoveredBlocks = checkAllBlocksCovered(coveredBlocks, allBlocks);
	if(uncoveredBlocks.empty()) {
		errs() << "All blocks covered\n";
	}
	else {
		errs() << "Not all blocks covered\n";
		for(BasicBlock* bb : uncoveredBlocks) {
			errs() << "\tUncovered block: " << bb->getName() << "\n";
		}
	}
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

/// @todo Move elsewhere
/// @brief Describes the type of optimization that can be performed on the loop header instrumentation point
enum class HeaderOptimizationType {
	None,
	Preheader,
	ExitBlocks
};

/// The instrumentation point can be removed from the header of a loop
/// in 2 scenarios:
/// 1. The loop has a preheader, define as the only block that is entering the loop and has a single successor, which is the header.
/// In this case, the number of executions of the loop header is equal to the number of executions of the preheader
/// 2. ALL of the loop exits have only one predecessor, which is the header. In this case, the sum of the execution counts of the loop exits
/// is equal to the execution count of the loop header
/// Additionally, all latches of the loop (blocks that have an edge to the header) must have only one successors, being the header.
/// Otherwise the header cannot be optimized
///
/// The sum of the executions of all loop exits are equal to the number of exectuions of the preheader.
/// The number of executions of the header of the loop is equal to the number of executions of the latch
/// plus the number of executions of the exit blocks. Therefore, the latfh if the loop needs to be instrumented and all exit
/// blocks have to be instrumented. This assumes, that the loop is not in some stupid form which would break this logic,
/// e.g. a loop that has a latch that has entries that are not the header, or something like that. 
/// If a loop latch is also a loop exit, it does not influence this at all, since the backedge of the latch
/// will be executed once per every iteration of the loop and the exit edge will be executed exactly once.
/// For loops that somehow violate this form, either we do not optimize the loop at all, or we can run the
/// loop-simplify pass before this pass to ensure that the loop is in a form that can be optimized.
/// The loop-simplify pass can fail apparantly, so in case that happens I guess we would just not optimize the loop.
HeaderOptimizationType canHeaderBeOptimized(Loop* loop) {
	SmallVector<BasicBlock*> loopLatches;
	loop->getLoopLatches(loopLatches);
	for(BasicBlock* latch : loopLatches) {
		if(succ_size(latch) > 1) {
			errs() << "Loop latch " << latch->getName() << " has multiple successors and cannot be used to calculate the number of executions of the header\n";
			return HeaderOptimizationType::None;
		}
	}
	BasicBlock* preheader = loop->getLoopPreheader();
	if(preheader) {
		errs() << "Loop has a preheader\n";
		return HeaderOptimizationType::Preheader;
	}
	SmallVector<BasicBlock*> loopExits;
	loop->getExitBlocks(loopExits);
	for(BasicBlock* exitBlock : loopExits) {
		if(pred_size(exitBlock) != 1) {
			for(auto predecessor : predecessors(exitBlock)) {
				if(!loop->contains(predecessor))
					errs() << "Loop exit " << exitBlock->getName() << " has predecessor " << predecessor->getName() << " that is outside the loop." << "\n";
			}
			return HeaderOptimizationType::None;
		}
	}
	errs() << "The preheader does not exit, but all loop exits have only one predecessor\n";
	return HeaderOptimizationType::ExitBlocks;
}

struct LatchHeaderPair {
	SmallVector<BasicBlock*> latches;
	BasicBlock* header;
};

SmallVector<LatchHeaderPair> optimizeLoopHeaders(LoopInfo &LI) {
	SmallVector<LatchHeaderPair> pairs;
	for(Loop* loop : LI.getLoopsInPreorder()){
		LatchHeaderPair pair = {};
		BasicBlock* preheader = nullptr;
		HeaderOptimizationType optimizationType = canHeaderBeOptimized(loop);
		switch(optimizationType) {
			case HeaderOptimizationType::None:
				errs() << "Loop header cannot be optimized\n";
				break;
			case HeaderOptimizationType::Preheader:
				errs() << "Optimizing loop header with preheader\n";
				preheader = loop->getLoopPreheader();
				loop->getLoopLatches(pair.latches);
				pair.header = loop->getHeader();
				pairs.push_back(pair);
				break;
			case HeaderOptimizationType::ExitBlocks:
				break;
		}
	}
	return pairs;
}


bool doesFunctionContainLoops(LoopInfo &LI) {
	auto loops = LI.getLoopsInPreorder();
	return !loops.empty();
}




blockCovers getOptimizedMapForNoLoopFunction(Function &F) {
}

// Recursive function to expand the coverage of a block to include indirect coverage
void expandCoverage(BasicBlock* block, blockCovers& bc, std::set<BasicBlock*>& visited) {
    if (visited.find(block) != visited.end()) {
        // Block already processed, avoid infinite recursion
        return;
    }
    visited.insert(block);

    std::vector<BasicBlock*> toAdd;
    for (auto coveredBlock : bc[block]) {
        // Add all blocks covered by the coveredBlock
        for (auto indirectlyCovered : bc[coveredBlock]) {
            if (bc[block].find(indirectlyCovered) == bc[block].end()) {
                // Only add if not already covered directly
                toAdd.push_back(indirectlyCovered);
            }
        }
        // Recursively expand coverage for the coveredBlock
        expandCoverage(coveredBlock, bc, visited);
    }

    // Add the collected blocks to the coverage
    for (auto addBlock : toAdd) {
        bc[block].insert(addBlock);
    }
}

blockCovers getMinimalCoverMap(blockCovers bc) {
    // First, expand coverage for each block to include indirect coverages
    for (auto& [block, _] : bc) {
        std::set<BasicBlock*> visited;
        expandCoverage(block, bc, visited);
    }

    // Identify blocks to remove: those that are covered by others
    std::set<BasicBlock*> toRemove;
    for (auto& [block, covered] : bc) {
        for (auto& c : covered) {
            toRemove.insert(c);
        }
    }

    // Remove the identified blocks
    for (auto c : toRemove) {
        bc.erase(c);
    }

    return bc;
}

/// @brief Run the pass
/// @param M The module to run the pass on
/// @param MAM The module analysis manager
/// @return The preserved analyses (IR is modified, so none to be safe)
PreservedAnalyses InstructionCount::run(Module &M, ModuleAnalysisManager &MAM){

	unsigned long bbCount = 0;
	LLVMContext& CTX = M.getContext();

	InstrumentationFunctions IF = InstrumentationFunctions(CTX);

	/// insert the instrumentation calls
	for(auto &F : M){
		if(F.isDeclaration()) continue;
		SmallVector<BasicBlock*> coveredBlocks;
		FunctionPassManager FPM;
		FPM.addPass(RequireAnalysisPass<LoopAnalysis, Function>());
		FPM.addPass(LoopSimplifyPass());
		FunctionAnalysisManager &FAM = MAM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
		FPM.run(F, FAM);
		LoopInfo &LI = FAM.getResult<LoopAnalysis>(F);

		blockCovers blockCoversVector;
		if(!doesFunctionContainLoops(LI)) {	
			blockCoversVector = getOptimizedMapForNoLoopFunction(F);
			blockCoversVector = getMinimalCoverMap(blockCoversVector);
			for(auto [block, covers] : blockCoversVector) {
				errs() << "Block " << block->getName() << " covers: ";
				for(BasicBlock* covered : covers) {
					errs() << "\t" << covered->getName() << " ";
				}
				errs() << "\n";
			}
		}

		loopsAnalysis(LI, F);
		SmallVector<LatchHeaderPair> pairs = optimizeLoopHeaders(LI);

		for(LatchHeaderPair pair : pairs) {
			errs() << "Optimized loop header: " << pair.header->getName() << "\n";
			for(BasicBlock* latch : pair.latches) {
				errs() << "\tLatch: " << latch->getName() << "\n";
			}
		}

		for(auto &BB : F){
			Instruction* insertionPoint = &*BB.getFirstInsertionPt();
			std::string bbInfo = getBBInfo(BB, bbCount);
			writeBBInfoToFile(bbInfo, "./bbinfo.csv");

			incrementCounter(M, insertionPoint, bbCount);
			bbCount++;

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
	std::error_code EC;
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



