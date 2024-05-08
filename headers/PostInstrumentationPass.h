#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/IR/Function.h>

using namespace llvm;
class PostInstrumentationPass : public PassInfoMixin<PostInstrumentationPass>  {
	public:
		/// Struct to hold the functions which export the execution count arrays
		struct exportFunctions {
			Function* exportFunction;
			Function* exportArrayFunction;
			Function* exportModulesFunction;
		};

		/// Struct to hold some information about a module
		struct ModuleInfo {
			std::string moduleName;
			std::string arrayName;
			unsigned long arraySize;
			GlobalVariable* array = nullptr;
		};

		/// @brief Run the pass
		/// @param M The module to run the pass on
		/// @param AM The analysis manager
		/// @return no analysis preserved
		PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);

	private:
		/// @brief Extract ModuleInfo structs from the modules.tmp file
		/// @param M The module to get the arrays from
		/// @return structs with the information about the modules
		std::vector<ModuleInfo> getModulesArraysFromFile(Module &M);
		/// @brief Get the export functions as LLVM Function objects
		/// @param M The module to get the functions from
		/// @return The functions
		exportFunctions getExportFunctions(Module &M);
		/// @brief Insert calls to the export functions
		/// @param M The module to insert the calls in
		/// @param modules The modules to export
		void insertArrayExportCalls(Module &M, std::vector<ModuleInfo> modules);
};
