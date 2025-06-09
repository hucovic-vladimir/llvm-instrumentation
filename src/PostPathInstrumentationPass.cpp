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

void createPathArrayDefinitions(Module &M, vector<PostPathInstrumentationPass::ModuleInfo>& moduleInfo) {
	LLVMContext& context = M.getContext();

	for(auto& module : moduleInfo) {
		for(auto& func : module.functions) {
			GlobalVariable* existingArray = M.getGlobalVariable(func.pathArrayName);
			if (existingArray) {
				errs() << "Found existing path array: " << func.pathArrayName << "\n";
				func.pathArray = existingArray;
				continue;
			}

			ArrayType* arrayType = ArrayType::get(Type::getInt64Ty(context), func.arraySize);

			Constant* zeroInit = Constant::getNullValue(arrayType);

			GlobalVariable* pathArray = new GlobalVariable(
					M, 
					arrayType,
					false,
					GlobalValue::ExternalLinkage,   
					zeroInit,                       
					func.pathArrayName
					); 

			func.pathArray = pathArray;
			errs() << "Created new path array: " << func.pathArrayName << "\n";
		}
	}
}



void insertPathArrayExportCalls(Module &M, vector<PostPathInstrumentationPass::ModuleInfo>& moduleInfo) {
	LLVMContext& context = M.getContext();

	// Find or create the __export_path_arrays function
	Function* exportPathArraysFunc = M.getFunction("__export_path_arrays");
	if (!exportPathArraysFunc) {
		// Create function type: void __export_path_arrays()
		FunctionType* funcType = FunctionType::get(Type::getVoidTy(context), false);
		exportPathArraysFunc = Function::Create(
				funcType,
				GlobalValue::ExternalLinkage,
				"__export_path_arrays",
				M
				);
	}

	// Clear existing basic blocks if any
	exportPathArraysFunc->deleteBody();

	// Create entry basic block
	BasicBlock* entryBB = BasicBlock::Create(context, "entry", exportPathArraysFunc);
	IRBuilder<> builder(entryBB);

	// Get or declare the __export_path_array function
	// void __export_path_array(const char* moduleName, const char* funcName, unsigned long* arr, unsigned long len, bool lastModule)
	std::vector<Type*> paramTypes = {
		Type::getInt8PtrTy(context),    // const char* moduleName
		Type::getInt8PtrTy(context),    // const char* funcName  
		Type::getInt64PtrTy(context),   // unsigned long* arr
		Type::getInt64Ty(context),      // unsigned long len
		Type::getInt1Ty(context)        // bool lastModule
	};
	FunctionType* exportFuncType = FunctionType::get(Type::getVoidTy(context), paramTypes, false);
	FunctionCallee exportPathArrayFunc = M.getOrInsertFunction("__export_path_array", exportFuncType);

	// Count total number of functions across all modules
	size_t totalFunctions = 0;
	for (const auto& module : moduleInfo) {
		totalFunctions += module.functions.size();
	}

	size_t currentFunctionIndex = 0;

	// Generate calls for each module and function
	for (const auto& module : moduleInfo) {
		// Create module name string constant
		Constant* moduleNameStr = builder.CreateGlobalStringPtr(module.moduleName, "module_name_" + module.moduleName);

		for (const auto& func : module.functions) {
			currentFunctionIndex++;
			bool isLastFunction = (currentFunctionIndex == totalFunctions);

			// Create function name string constant
			Constant* funcNameStr = builder.CreateGlobalStringPtr(func.functionName, "func_name_" + func.functionName);

			// Get pointer to the path array
			if (!func.pathArray) {
				errs() << "Warning: pathArray is null for function " << func.functionName << "\n";
				continue;
			}

			// Cast array to unsigned long* (i64*)
			Value* arrayPtr = builder.CreateBitCast(func.pathArray, Type::getInt64PtrTy(context));

			// Create array size constant
			Value* arraySize = builder.getInt64(func.arraySize);

			// Create lastModule boolean
			Value* lastModule = builder.getInt1(isLastFunction);

			// Create the function call
			builder.CreateCall(exportPathArrayFunc, {
					moduleNameStr,
					funcNameStr,
					arrayPtr,
					arraySize,
					lastModule
					});

			errs() << "Inserted call for: " << module.moduleName << "::" << func.functionName 
				<< " (size: " << func.arraySize << ")" << (isLastFunction ? " [LAST]" : "") << "\n";
		}
	}

	// Add return statement
	builder.CreateRetVoid();

	errs() << "Created __export_path_arrays function with " << totalFunctions << " export calls\n";
}




PreservedAnalyses PostPathInstrumentationPass::run(Module &M, ModuleAnalysisManager &AM) {
	vector<ModuleInfo> modules = getAllPathArrays();
	errs() << "Found " << modules.size() << " modules with path arrays.\n";

	createPathArrayDefinitions(M, modules);
	insertPathArrayExportCalls(M, modules);

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


// Implementation of function name extraction
std::string PostPathInstrumentationPass::extractFunctionName(const std::string& counterName) {
	const std::string prefix = "__paths_";
	size_t pos = counterName.find(prefix);

	if (pos != std::string::npos) {
		// Return everything after "__paths_"
		return counterName.substr(pos + prefix.length());
	}

	// If "__paths_" is not found, return the original name
	return counterName;
}
