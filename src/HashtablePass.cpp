#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Passes/PassPlugin.h>
#include <llvm/Passes/PassBuilder.h>

using namespace llvm;
class HashtablePass : public PassInfoMixin<HashtablePass> {


	public:
		PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM) {
			if(M.getName().contains("instrumentationCode")) 
				return PreservedAnalyses::none();
			FunctionCallee f = M.getOrInsertFunction("__bb_enter", FunctionType::get(Type::getVoidTy(M.getContext()), {IntegerType::getInt8PtrTy(M.getContext())}, false));
			FunctionCallee f2 = M.getOrInsertFunction("__prof_export", FunctionType::get(Type::getVoidTy(M.getContext()), false));
			IRBuilder builder(M.getContext());
			for(auto &F : M) {
				if(F.isDeclaration())
					continue;
				for(auto &BB : F) {
					BB.getFirstInsertionPt();
					builder.SetInsertPoint(&BB, BB.getFirstInsertionPt());
					std::string bbName = M.getName().str() + "_" + F.getName().str() + "_" + BB.getName().str();
					builder.CreateCall(f, {builder.CreateGlobalStringPtr(bbName)});
					for(auto &I : BB) {
						if(isa<CallInst>(I)) {
							CallInst *CI = cast<CallInst>(&I);
							Function *Callee = CI->getCalledFunction();
							if(Callee) {
								if(Callee->getName() == "exit") {
									builder.SetInsertPoint(CI);
									builder.CreateCall(f2);
								}
							}
						}
					}
				}
				if(F.getName() == "main") { // Export the profile at the end of the main function
					for(auto &BB : F) {
						if(isa<ReturnInst>(BB.getTerminator())) {
							builder.SetInsertPoint(BB.getTerminator());
							builder.CreateCall(f2);
						}
					}
				}
			}
			return PreservedAnalyses::none();
		}
};


/// @brief Get the plugin info for the pass
PassPluginLibraryInfo getHashtablePassPluginInfo(){
	const auto callback = [](PassBuilder &PB) {
		PB.registerOptimizerLastEPCallback(
				[](ModulePassManager &MPM, OptimizationLevel Level){
					MPM.addPass(HashtablePass());
				}
				);
	};
	return {LLVM_PLUGIN_API_VERSION, "Hashtable", "v0.1", callback};
}

/// @brief Register the pass with the pass manager
extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo(){
	return getHashtablePassPluginInfo();
}








