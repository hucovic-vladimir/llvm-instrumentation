#include "llvm/IR/PassManager.h"

using namespace llvm;

class InstructionCount : public llvm::PassInfoMixin<InstructionCount> {
    private:
        FunctionCallee profInit;
        FunctionCallee bbEnter;
        Constant* moduleNameStr;
        Twine moduleName;
        void instrumentBasicBlock(BasicBlock &bb);

	public:
        llvm::PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

