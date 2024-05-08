
#include "../headers/PassUtilities.h"
#include <climits>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/DebugLoc.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/IRBuilder.h>
#include <filesystem>

namespace fs = std::filesystem;

std::pair<unsigned long, unsigned long> PassUtilities::getBasicBlockStartEndLines(BasicBlock &bb){
   long min = LONG_MAX;
   long max = 0;
   for(Instruction &inst : bb){
       DebugLoc debugNode = inst.getDebugLoc();
       if(debugNode){
           long line = debugNode.getLine();
           if(line <= 0) continue;
           min = line < min ? line : min;
           max = line > max ? line : max;
       }       
    }
   return {min, max};
}

void PassUtilities::insertSetStartTime(Instruction &i){
   IRBuilder<> builder(i.getContext());
}

Constant* PassUtilities::insertModuleNameAsCharPtr(Module* m){
    IRBuilder<> builder(m->getContext());
    builder.SetInsertPoint(m->functions().begin()->getEntryBlock().getFirstNonPHI());
    Constant* moduleNameStr = builder.CreateGlobalString(m->getName());
    return moduleNameStr;
}


const std::vector<ReturnInst*> PassUtilities::getReturnInstructionsFromFunction(Function &F){
	std::vector<ReturnInst*> returnInstructions;
	for(auto &BB : F){
		for(auto &I : BB){
			if(auto* ret = dyn_cast<ReturnInst>(&I)){
				returnInstructions.push_back(ret);
			}
		}
	}
	return returnInstructions;
}
const std::string PassUtilities::getFileName(const std::string& path) {
	return fs::path(path).filename().string();
}
