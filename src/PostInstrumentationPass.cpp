/// TODO refactor and add comments

#include <iostream>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <fstream>
#include <algorithm>
using namespace llvm;

class PostInstrumentationPass : public PassInfoMixin<PostInstrumentationPass>  {
public:

	struct exportFunctions {
		Function* exportFunction;
		Function* exportArrayFunction;
	};

	struct ModuleInfo {
		std::string moduleName;
		std::string arrayName;
		unsigned long arraySize;
		GlobalVariable* array = nullptr;
	};

	std::vector<ModuleInfo> getModulesArraysFromFile(Module &M) {
		std::vector<ModuleInfo> modules;
		std::ifstream modulesFile("modules.tmp", std::ios::in);
		if (!modulesFile.is_open()) {
			std::cerr << "Error: Could not open modules.tmp" << std::endl;
			exit(1);
		}
		std::string line;
		while (std::getline(modulesFile, line)) {
			/// separate by coma
			std::string moduleName = line.substr(0, line.find(","));
			line.erase(0, moduleName.size() + 1);
			std::cerr << "line: " << line << std::endl;
			std::string arrayName = line.substr(0, line.find(","));
			line.erase(0, arrayName.size() + 1);
			std::cerr << "line: " << line << std::endl;
			unsigned long arraySize = std::stoul(line);

			ArrayType* arrayType = ArrayType::get(Type::getInt64Ty(M.getContext()), arraySize);
			M.getOrInsertGlobal(arrayName, arrayType);
			GlobalVariable* array = M.getGlobalVariable(arrayName);
			if (array == nullptr) {
				std::cerr << "Error: Could not create and find global variable " << arrayName << std::endl;
				exit(1);
			}
			array->setInitializer(ConstantAggregateZero::get(arrayType));
			modules.push_back({moduleName, arrayName, arraySize, array});
		}
		return modules;
	}

	exportFunctions getExportFunctions(Module &M) {
		Function* exportFunction = M.getFunction("__prof_export2");
		if (exportFunction == nullptr) {
			std::cerr << "Error: __prof_export2 not found in the module."
				<< "This pass should only be run on the instrumentation code." << std::endl;
			exit(1);
		}

		Function* exportArrayFunction = M.getFunction("__export_array");
		if (exportArrayFunction == nullptr) {
			std::cerr << "Error: __export_array not found in the module."
				<< "This pass should only be run on the instrumentation code." << std::endl;
			exit(1);
		}
		return {exportFunction, exportArrayFunction};
	}

	void insertArrayExportCalls(Module &M, std::vector<ModuleInfo> modules) {
		auto [exportFunction, exportArrayFunction] = getExportFunctions(M);
		/// insert calls to __export_array into __prof_export function
		Instruction* insertionPoint = exportFunction->getEntryBlock().getTerminator();
		IRBuilder<> builder(insertionPoint);
		for (auto [moduleName, arrayName, size, array] : modules) {
			/// insert call to __export_array and all the necessary arguments
			Constant* moduleNameValue = builder.CreateGlobalStringPtr(moduleName);
			Value* sizeValue = ConstantInt::get(Type::getInt64Ty(M.getContext()), size);
			Value* args[] = {moduleNameValue, array, sizeValue};
			builder.CreateCall(exportArrayFunction, args);
		}
	}

	PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
		std::vector<ModuleInfo> modules = getModulesArraysFromFile(M);
		std::cerr << modules.size() << " modules found." << std::endl;
		insertArrayExportCalls(M, modules);
		return PreservedAnalyses::none();
	}
};


PassPluginLibraryInfo getPostInstrumentationPassPluginInfo() {
	const auto callback = [](PassBuilder &PB) {
		PB.registerOptimizerLastEPCallback(
				[](ModulePassManager &MPM, OptimizationLevel Level) {
				MPM.addPass(PostInstrumentationPass());
				}
				);
	};
	return {LLVM_PLUGIN_API_VERSION, "PostInstrumentationPass", "v0.1", callback};
}


extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
	return getPostInstrumentationPassPluginInfo();
}
