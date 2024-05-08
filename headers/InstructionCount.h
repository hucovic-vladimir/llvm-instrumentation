#include "llvm/IR/PassManager.h"
#include "OptimizationPattern.h"

using namespace llvm;

class InstructionCount : public llvm::PassInfoMixin<InstructionCount> {
    private:
			/// @brief get the name of the execution count array for a module
			/// @param M the module
			const std::string getLocalArrayName(Module &M);

			/// @brief get or insert the global variable that will hold the execution counts
			/// @param M the module
			GlobalVariable* getOrCreateCounter(Module &M);

			/// @brief insert instructions that increment the execution count of the basic block at the specified index
			/// @param M the module
			/// @param insertionPoint the instruction before which the increment instructions will be inserted
			/// @param bbIndex the index of the basic block
			void incrementCounter(Module &M, Instruction* insertionPoint,
					unsigned long bbIndex);
			std::vector<OptimizationPattern*> getOptimizationPatterns(Function &F);

	public:
				/// @brief run the pass
				/// @param M the module
				/// @param MAM the module analysis manager
        llvm::PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

