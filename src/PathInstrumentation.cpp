#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h" 
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "../headers/PathInstrumentation.h"
#include "lib/CFGTransformer.h"
#include "llvm/Analysis/CycleAnalysis.h"
#include "lib/DAG.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/IR/Verifier.h"

AllocaInst* insertPathCounter(Function &F) {
	BasicBlock& entry = F.getEntryBlock();
	Instruction* firstInst = entry.getFirstNonPHI();
	IRBuilder<> builder(firstInst);
	LLVMContext &context = F.getContext();
	AllocaInst* pathCounterVar = builder.CreateAlloca(Type::getInt32Ty(context), nullptr, "__path_counter");
	builder.CreateStore(builder.getInt32(0), pathCounterVar);
	return pathCounterVar;
}


void dumpModuleIR(llvm::Module &M, const std::string &Filename) {
  std::error_code EC;
  llvm::raw_fd_ostream OS(Filename, EC);
  
  if (EC) {
    errs() << "Error opening file " << Filename << ": " << EC.message() << "\n";
    return;
  }
  
  // Dump the module IR to the output stream
  M.print(OS, nullptr);
}


using namespace llvm;
using namespace std;

template<>
struct DOTGraphTraits<Function*> : public DefaultDOTGraphTraits {
	DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple) {}
	
	std::string getNodeLabel(const BasicBlock *BB, const Function *) {
		if (BB->hasName())
			return BB->getName().str();
		else
			return "BB" + Twine(reinterpret_cast<uintptr_t>(BB)).str();
	}
};

PreservedAnalyses PathInstrumentation::run(Module &M, ModuleAnalysisManager &MAM) {
	FunctionAnalysisManager &FAM = 
		MAM.getResult<FunctionAnalysisManagerModuleProxy>(M)
		.getManager();
		std::string IRFilename = "/tmp/llfiles/" + M.getName().str() + ".ll";
		std::string IRFilename2 = "/tmp/llfiles/" + M.getName().str() + "post_transformation" + ".ll";
	for(Function &F : M) {
		if(F.isDeclaration() || F.isIntrinsic()) continue;

		CFGTransformer::transformToSingleExit(F);

		std::string Filename = "/tmp/graphs/" + F.getName().str() + ".dot";
		std::error_code EC;
		raw_fd_ostream File(Filename, EC, sys::fs::OF_Text);

		bool isSimple = false;  // Set to false to include more details
		/* WriteGraph(File, &F, isSimple); */

		const auto dag = DAG::createFromFunction(F, FAM);
		if(dag) {
			dag->assignEdgeValues();
			dag->printEdgeValues();
			AllocaInst* counter = insertPathCounter(F);
			errs() << "Path counter inserted in function " + F.getName() + "\n";
			CFGTransformer::addInstrumentedEdges(*dag, counter);
		}
		WriteGraph(File, &F, isSimple);
	}
	dumpModuleIR(M, IRFilename);

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
				});
		PB.registerAnalysisRegistrationCallback(
				[](FunctionAnalysisManager &FAM) {
					FAM.registerPass([&] { return CycleAnalysis(); });
				});
	};
	return {LLVM_PLUGIN_API_VERSION, "path-instrumentation", "v0.1", callback};
}

/// Dynamic entry point
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
	return getPathInstrumentationPluginInfo();
}
