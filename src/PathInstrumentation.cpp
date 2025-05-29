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
#include "../headers/InstrumentationFunctions.h"
#include <filesystem>

AllocaInst* insertPathCounter(Function &F) {
	BasicBlock& entry = F.getEntryBlock();
	Instruction* firstInst = entry.getFirstNonPHI();
	IRBuilder<> builder(firstInst);
	LLVMContext &context = F.getContext();
	AllocaInst* pathCounterVar = builder.CreateAlloca(Type::getInt32Ty(context), nullptr, "__path_counter");
	builder.CreateStore(builder.getInt32(0), pathCounterVar);
	return pathCounterVar;
}


void insertPrintOfCounter(Function &F, BasicBlock& exit, AllocaInst* counter) {
	InstrumentationFunctions IF(F.getContext());
	Module &M = *F.getParent();

	Instruction* terminator = exit.getTerminator(); 
	IRBuilder<> builder(terminator);
	Value* loadedCounter = builder.CreateLoad(builder.getInt32Ty(), counter, "loaded_counter"); 
  IF.insertPrintfCall(M, terminator, "Counter: \%d\n", {loadedCounter});
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

void createOutputDirectories(Module &M) {
	std::error_code EC;
	string moduleSource = std::filesystem::path(M.getSourceFileName()).parent_path();
	if(moduleSource.size()) {
		sys::fs::create_directories("/tmp/graphs/" + moduleSource);
		sys::fs::create_directories("/tmp/llfiles/" + moduleSource);
	}
	else {
		sys::fs::create_directories("/tmp/graphs/");
		sys::fs::create_directories("/tmp/llfiles/");
	}


	if(EC) {
		errs() << "Error creating directory: " << EC.message() << "\n";
	}
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

void dumpNodesToJson(DAG* dag, Module& M) {
	error_code EC;
	string moduleSourceFilename = filesystem::path(M.getSourceFileName()).filename().string();
	string moduleSourceDirname = filesystem::path(M.getSourceFileName()).parent_path().string();
	sys::fs::create_directories(".nodes/" + moduleSourceDirname);
	if(EC) {
		errs() << "Error creating directory: " << EC.message() << "\n";
	}
	string jsonFilename = ".nodes/" + moduleSourceDirname + "/" + moduleSourceFilename + ".json";
	dag->exportNodesToJson(jsonFilename);
}

PreservedAnalyses PathInstrumentation::run(Module &M, ModuleAnalysisManager &MAM) {
	createOutputDirectories(M);
	string moduleSourceFilename = filesystem::path(M.getSourceFileName()).filename().string();
	FunctionAnalysisManager &FAM = 
		MAM.getResult<FunctionAnalysisManagerModuleProxy>(M)
		.getManager();
		std::string IRFilename = "/tmp/llfiles/" + M.getName().str() + ".ll";
		std::string IRFilename2 = "/tmp/llfiles/" + M.getName().str() + "post_transformation" + ".ll";
		long lastAssignedId = 0;
	for(Function &F : M) {
		if(F.isDeclaration() || F.isIntrinsic()) continue;

		CFGTransformer::transformToSingleExit(F);

		std::string Filename = "/tmp/graphs/" + F.getName().str() + ".dot";
		std::error_code EC;
		raw_fd_ostream File(Filename, EC, sys::fs::OF_Text);


		const auto dag = DAG::createFromFunction(F, FAM);
		if(dag) {
			dag->assignEdgeValues();
			/* dag->printEdgeValues(); */
			AllocaInst* counter = insertPathCounter(F);
			CFGTransformer::addInstrumentedEdges(*dag, counter);
			BasicBlock* exitBlock = dag->getExit()->getBlock();
			insertPrintOfCounter(F, *exitBlock, counter);
			dag->assignNodeIds(lastAssignedId);
			dumpNodesToJson(dag, M);
		}
		WriteGraph(File, &F, false);
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
