#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h" 
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "../headers/PathInstrumentation.h"
#include "lib/CFGTransformer.h"

using namespace llvm;
using namespace std;

PreservedAnalyses PathInstrumentation::run(Module &M, ModuleAnalysisManager &MAM) {
	errs() << "Hello from path inst" << "\n";
	for(Function &F : M) {
		if(F.isDeclaration() || F.isIntrinsic()) continue;

		CFGTransformer::transformToSingleExit(F);
		std::string Filename = "/tmp/graphs/" + F.getName().str() + ".dot";
		std::error_code EC;
		raw_fd_ostream File(Filename, EC, sys::fs::OF_Text);
		WriteGraph(File, &F);
	}
	return PreservedAnalyses::none();
}

/// Static entry point
PassPluginLibraryInfo getPathInstrumentationPluginInfo() {
	const auto callback = [](PassBuilder &PB) {
		PB.registerPipelineParsingCallback(
				[](StringRef Name, ModulePassManager &MPM,
					ArrayRef<PassBuilder::PipelineElement>) {
				if (Name == "path-instrumentation") {
				MPM.addPass(PathInstrumentation());
					return true;
				}
					return false;
		});

		PB.registerPipelineEarlySimplificationEPCallback(
				[](ModulePassManager &MPM, OptimizationLevel Level) {
				MPM.addPass(PathInstrumentation());
				}
		);
	};
	return {LLVM_PLUGIN_API_VERSION, "path-instrumentation", "v0.1", callback};
}

/// Dynamic entry point
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
	return getPathInstrumentationPluginInfo();
}
