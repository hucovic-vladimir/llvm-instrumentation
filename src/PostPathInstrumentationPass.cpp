#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/IR/Function.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/FileSystem.h>
#include <filesystem>
#include "PostPathInstrumentationPass.h"
#include <iostream>

using namespace llvm;
using namespace std;

PreservedAnalyses PostPathInstrumentationPass::run(Module &M, ModuleAnalysisManager &AM) {
	std::vector<ModuleInfo> modules = getModulesArraysFromFile(M);
	std::cerr << modules.size() << " modules found." << std::endl;
	return PreservedAnalyses::none();
}

/// Static entry point
PassPluginLibraryInfo getPostPathInstrumentationPassPluginInfo() {
	const auto callback = [](PassBuilder &PB) {
		PB.registerPipelineEarlySimplificationEPCallback(
				[](ModulePassManager &MPM, OptimizationLevel Level) {
				MPM.addPass(PostPathInstrumentationPass());
				}
				);
	};
	return {LLVM_PLUGIN_API_VERSION, "PostPathInstrumentationPass", "v0.1", callback};
}

/// Dynamic entry point
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo llvmGetPassPluginInfo() {
	return getPostPathInstrumentationPassPluginInfo();
}


std::vector<PostPathInstrumentationPass::ModuleInfo> 
PostPathInstrumentationPass::getAllPathArrays() {
	std::vector<ModuleInfo> modules;
	std::string counterDir = ".pathinst/path_counters/";

	// Check if directory exists
	if (!std::filesystem::exists(counterDir)) {
		errs() << "Directory does not exist: " << counterDir << "\n";
		return modules;
	}

	// Iterate through all files in the directory
	for (auto& entry : std::filesystem::directory_iterator(counterDir)) {
		if (!entry.is_regular_file()) continue;

		std::string filename = entry.path().string();

		// Check if it's a JSON file
		if (entry.path().extension() != ".json") continue;

		errs() << "Processing file: " << filename << "\n";

		// Parse the JSON file into ModuleInfo
		auto moduleInfoResult = parseModuleInfoFromJSON(filename);
		if (!moduleInfoResult) {
			errs() << "Error parsing " << filename << ": " 
				<< toString(moduleInfoResult.takeError()) << "\n";
			continue;
		}

		modules.push_back(std::move(*moduleInfoResult));
	}

	return modules;
}

// New function that parses a single JSON file into ModuleInfo
Expected<PostPathInstrumentationPass::ModuleInfo> 
PostPathInstrumentationPass::parseModuleInfoFromJSON(const std::string& filename) {
	// Read the file
	string errorMsg = "Failed to read file: " + filename;
	auto fileBuffer = MemoryBuffer::getFile(filename);
	if (!fileBuffer) {
		return createStringError(std::errc::invalid_argument, 
				errorMsg.c_str());
	}

	// Parse JSON
	Expected<json::Value> parsed = json::parse(fileBuffer.get()->getBuffer());
	if (!parsed) {
		return parsed.takeError();
	}

	// Get root object
	json::Object *root = parsed->getAsObject();
	if (!root) {
		return createStringError(std::errc::invalid_argument, 
				"Root is not a JSON object");
	}

	ModuleInfo moduleInfo;

	// Parse module name
	const json::Value *moduleValue = root->get("module");
	if (!moduleValue) {
		return createStringError(std::errc::invalid_argument, 
				"Missing 'module' field");
	}

	auto moduleStr = moduleValue->getAsString();
	if (!moduleStr) {
		return createStringError(std::errc::invalid_argument, 
				"'module' field is not a string");
	}
	moduleInfo.moduleName = moduleStr->str();

	// Parse counters array
	const json::Value *countersValue = root->get("counters");
	if (!countersValue) {
		return createStringError(std::errc::invalid_argument, 
				"Missing 'counters' field");
	}

	const json::Array *countersArray = countersValue->getAsArray();
	if (!countersArray) {
		return createStringError(std::errc::invalid_argument, 
				"'counters' field is not an array");
	}

	// Parse each counter object into FunctionInfo
	for (const json::Value& counterValue : *countersArray) {
		const json::Object* counterObj = counterValue.getAsObject();
		if (!counterObj) {
			return createStringError(std::errc::invalid_argument, 
					"Counter element is not an object");
		}

		FunctionInfo funcInfo;

		// Parse counter name (full path array name)
		const json::Value* counterNameValue = counterObj->get("counter");
		if (!counterNameValue) {
			return createStringError(std::errc::invalid_argument, 
					"Missing 'counter' field in counter object");
		}

		auto counterNameStr = counterNameValue->getAsString();
		if (!counterNameStr) {
			return createStringError(std::errc::invalid_argument, 
					"'counter' field is not a string");
		}

		funcInfo.pathArrayName = counterNameStr->str();
		funcInfo.functionName = extractFunctionName(funcInfo.pathArrayName);

		// Parse size
		const json::Value *sizeValue = counterObj->get("size");
		if (!sizeValue) {
			return createStringError(std::errc::invalid_argument, 
					"Missing 'size' field in counter object");
		}

		auto sizeInt = sizeValue->getAsInteger();
		if (!sizeInt) {
			return createStringError(std::errc::invalid_argument, 
					"'size' field is not an integer");
		}
		funcInfo.arraySize = static_cast<size_t>(*sizeInt);

		// pathArray will be set later when we find the actual GlobalVariable
		funcInfo.pathArray = nullptr;

		moduleInfo.functions.push_back(funcInfo);
	}

	return moduleInfo;
}
