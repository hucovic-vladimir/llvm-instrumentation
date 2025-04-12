#include "llvm/Pass.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"

using namespace llvm;

class PathInstrumentation : public PassInfoMixin<PathInstrumentation> {
	public:
		PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};
