#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>

/// @class InstrumentationFunctions
/// @brief Stores the function types and function callees for the instrumentation functions,
/// and provides methods to insert calls to these functions into the IR
/// @details The instrumentation functions are used to instrument the IR to count the number of times
/// each basic block is executed. The instrumentation functions are defined in the instrumentation code
/// which is currently being linked statically with the profiled program.
class InstrumentationFunctions {
	private:
		/// @brief The function type for the profile initialization function
		/// @details The profile initialization function is called at the start of the program.
		/// Not used for anything at the moment
		llvm::FunctionType* profInitFuncType;

		/// @brief The function type for the function that exports the profiling information 
		/// @details It should be inserted at every point where the program can end 
		/// so that the information is not lost.
		llvm::FunctionType* profExportFuncType;
		llvm::FunctionType* profExportFuncType2;

		/// @brief The function type for the function that performs the basic block execution count increment
		llvm::FunctionType* bbEnterFuncType;

		/// @brief Function Callee for profiling initialization function 
		llvm::FunctionCallee profInitFunc;

		/// @brief Function Callee for export function
		llvm::FunctionCallee profExportFunc;
		llvm::FunctionCallee profExportFunc2;

		/// @brief Function Callee for basic block enter function
		llvm::FunctionCallee bbEnterFunc;

	public:
		/// @brief constructs this class from the provided global LLVM context
		InstrumentationFunctions(llvm::LLVMContext &context); 

		/// getters
		llvm::FunctionType getProfInitFuncType();
		llvm::FunctionType getProfExportFuncType();
		llvm::FunctionType getBBEnterFuncType();
		llvm::FunctionCallee* getInitFunctionCallee(llvm::Module& module);
		llvm::FunctionCallee* getExportFunctionCallee(llvm::Module& module);
		llvm::FunctionCallee* getExportFunctionCallee2(llvm::Module& module);
		llvm::FunctionCallee* getBBEnterFunctionCallee(llvm::Module& module);

		/// @brief inserts the __prof_init() function into the provided Module and
		/// inserts a call to it before the provided Instruction
		void insertProfInitCall(llvm::Module &module, llvm::Instruction* insertBefore);
		/// @brief inserts the __prof_export() function into the provided Module and
		/// inserts a call to it before the provided Instruction
		void insertProfExportCall(llvm::Module &module, llvm::Instruction* insertBefore);
		/// @brief inserts the __bb_enter(basicBlockId) function into the provided Module and
		/// inserts a call to it before the provided Instruction
		/// @todo maybe this function should also handle creating the Value* and therefore
		/// an integer type should be passed here
		void insertBBEnterCall(llvm::Module &module, llvm::Instruction* insertBefore, llvm::Value* basicBlockId);
};
