// TODO fix ugly formatting

#include "../headers/InstructionCount.h"
#include "../headers/PassUtilities.h"
#include "../headers/InstrumentationFunctions.h"
#include <algorithm>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Passes/PassBuilder.h>
#include <fstream>
#include <iostream>
#include <string>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Demangle/Demangle.h>

/// @todo Move elsewhere
/// @brief Get the file name from a path
/// @param path The path to get the file name from
/// @return The file name
std::string getFileName(const std::string& path) {
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
std::vector<ReturnInst*> getReturnInstructionsFromFunction(Function &F){
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

/// @todo move elsewhere and finish
/// @brief Get the basic block information to be exported from the pass
/// @param BB The basic block to get the information from
void getBBInfo(BasicBlock& BB){
	size_t size = BB.sizeWithoutDebug();
	std::string label = BB.getName().str();
	std::pair<int, int> startEndLines = PassUtilities::getBasicBlockStartEndLines(BB);
	std::string moduleName = BB.getParent()->getParent()->getSourceFileName();
	std::string functionName = demangle(BB.getParent()->getName().str());
}

/// @brief Run the pass
/// @param M The module to run the pass on
/// @param MAM The module analysis manager
/// @return The preserved analyses (IR is modified, so none to be safe)
PreservedAnalyses InstructionCount::run(Module &M, ModuleAnalysisManager &MAM){
	unsigned long bbCount = 0;
	LLVMContext& CTX = M.getContext();

	InstrumentationFunctions IF = InstrumentationFunctions(CTX);

	// open bbcount for reading
	// TODO think about how to handle this better
	// to allow for parallel compilation
	std::fstream bbCountFileRead("./bbcount.tmp", std::ios::in);
	if(bbCountFileRead.is_open()){
		bbCountFileRead >> bbCount;
	}
	bbCountFileRead.close();

	/// insert the instrumentation calls
	for(auto &F : M){
		for(auto &BB : F){
			Instruction* insertionPoint = &*BB.getFirstInsertionPt();
			// Create th constant in the IR
			Value* bbCountValue = ConstantInt::get(Type::getInt64Ty(CTX), bbCount);
			IF.insertBBEnterCall(M, insertionPoint, bbCountValue);
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


	/// write bbcount to file
	std::fstream bbCountFile("./bbcount.tmp", std::ios::out | std::ios::trunc);
	if(!bbCountFile.is_open()){
		std::cerr << "Failed to open bbcount.tmp for writing" << "\n";
		exit(1);
	}

	/// write the new bbcount to the file
	/// to be read in the next run
	/// TODO maybe there is a better way to 
	bbCountFile << bbCount << std::endl;
	bbCountFile.flush();
	bbCountFile.close();

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
