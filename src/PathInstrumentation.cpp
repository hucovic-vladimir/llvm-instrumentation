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
#include "llvm/IR/Dominators.h"
#include "lib/DAG.h"
#include "lib/SpanningTree.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/IR/Verifier.h"
#include "lib/PathRecorder.h"
#include "../headers/InstrumentationFunctions.h"
#include <filesystem>

using namespace llvm;
using namespace std;

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

void createDirectories(const std::string &path) {
	if(!std::filesystem::exists(path)) {
		sys::fs::create_directories(path, false);
	}
}

void createOutputDirectories(Module &M) {
	std::error_code EC;
	string moduleSource = std::filesystem::path(M.getSourceFileName()).parent_path();
	if(moduleSource.size()) {
		createDirectories("/tmp/graphs/" + moduleSource);
		createDirectories("/tmp/llfiles/" + moduleSource);
	}

	if(EC) {
		errs() << "Error creating directory: " << EC.message() << "\n";
	}
}

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

void addPathCounterToJSONArray(json::Array &array, Function* f, size_t size) {
	json::Object pathCounterObject;
	string moduleName = f->getParent()->getName().str();
	pathCounterObject["counter"] = moduleName + "__paths_" + f->getName().str();
	pathCounterObject["size"] = size;
	array.push_back(std::move(pathCounterObject));
}

void writeJSON(json::Object &&jsonObj, const string &filename) {
	std::error_code EC;
	raw_fd_ostream file(filename, EC, sys::fs::OF_Text);
	if (EC) {
		errs() << "Error opening file " << filename << ": " << EC.message() << "\n";
		return;
	}

	file << formatv("{0:2}", json::Value(std::move(jsonObj)));
}

PreservedAnalyses PathInstrumentation::run(Module &M, ModuleAnalysisManager &MAM) {
	createDirectories(".pathinst/path_counters/");
	json::Object pathCountersJSON;
	json::Array pathCountersArray;
	createOutputDirectories(M);
	string moduleSourceFilename = filesystem::path(M.getSourceFileName()).filename().string();
	string moduleFullPath = filesystem::path(M.getSourceFileName()).string(); 
	FunctionAnalysisManager &FAM = 
		MAM.getResult<FunctionAnalysisManagerModuleProxy>(M)
		.getManager();
		std::string IRFilename = "/tmp/llfiles/" + M.getName().str() + ".ll";
		std::string IRFilename2 = "/tmp/llfiles/" + M.getName().str() + "post_transformation" + ".ll";
		PathRecorder pr;
	for(Function &F : M) {
		if(F.isDeclaration() || F.isIntrinsic()) continue;

		CFGTransformer::transformToSingleExit(F);

		std::string Filename = "/tmp/graphs/" + F.getName().str() + ".dot";
		std::error_code EC;
		raw_fd_ostream File(Filename, EC, sys::fs::OF_Text);
		GraphNode::resetLastId();

		if(F.size() == 1) {
			pr.addCounterForSingleBlockFunction(&F);
			addPathCounterToJSONArray(pathCountersArray, &F, 1);
		}

		else {
			const auto dag = DAG::createFromFunction(F, FAM);
			if(dag) {
				dag->assignEdgeValues();
				// dag->eventCountingDFS();
				AllocaInst* counter = insertPathCounter(F);
				CFGTransformer::addInstrumentedEdges(dag, counter);
				// CFGTransformer::instrumentChords(dag, counter);
				BasicBlock* exitBlock = dag->getExit()->getBlock();
				CFGTransformer::insertPrintOfCounter(F, *exitBlock, counter);
				dumpNodesToJson(dag, M);
				pr.addPathArray(dag);

				addPathCounterToJSONArray(pathCountersArray, &F, dag->getNumberUniquePaths());
			}
		}
		WriteGraph(File, &F, false);
	}
	dumpModuleIR(M, IRFilename);

	pathCountersJSON["module"] = moduleFullPath;
	pathCountersJSON["counters"] = std::move(pathCountersArray);

	writeJSON(std::move(pathCountersJSON), ".pathinst/path_counters/" + moduleSourceFilename + ".json");
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
		PB.registerAnalysisRegistrationCallback(
				[](FunctionAnalysisManager &FAM) {
					FAM.registerPass([&] { return DominatorTreeAnalysis(); });
				});
	};
	return {LLVM_PLUGIN_API_VERSION, "path-instrumentation", "v0.1", callback};
}

/// Dynamic entry point
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
	return getPathInstrumentationPluginInfo();
}
