#include "../headers/InstrumentationFunctions.h"
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>

using namespace llvm;

InstrumentationFunctions::InstrumentationFunctions(LLVMContext &context) {
	profInitFuncType = FunctionType::get(Type::getVoidTy(context), false);
	profExportFuncType = FunctionType::get(Type::getVoidTy(context), false);
	profExportFuncType2 = FunctionType::get(Type::getVoidTy(context), false);
	bbEnterFuncType = FunctionType::get(Type::getVoidTy(context), {Type::getInt64Ty(context)}, false);
}


FunctionCallee* InstrumentationFunctions::getInitFunctionCallee(Module& module) {
	profInitFunc = module.getOrInsertFunction("__prof_init", profInitFuncType);
	return &profInitFunc;
}

FunctionCallee* InstrumentationFunctions::getExportFunctionCallee(Module& module) {
	profExportFunc = module.getOrInsertFunction("__prof_export", profExportFuncType);
	return &profExportFunc;
}

FunctionCallee* InstrumentationFunctions::getExportFunctionCallee2(Module& module) {
	profExportFunc2 = module.getOrInsertFunction("__prof_export2", profExportFuncType2);
	return &profExportFunc2;
}

FunctionCallee* InstrumentationFunctions::getBBEnterFunctionCallee(Module& module) {
	bbEnterFunc = module.getOrInsertFunction("__bb_enter", bbEnterFuncType);
	return &bbEnterFunc;
}


void InstrumentationFunctions::insertProfInitCall(Module &module, Instruction* insertBefore) {
	IRBuilder<> builder(insertBefore);
	FunctionCallee* profInitFunc = getInitFunctionCallee(module);
	builder.CreateCall(*profInitFunc);
}

void InstrumentationFunctions::insertProfExportCall(Module &module, Instruction* insertBefore) {
	IRBuilder<> builder(insertBefore);
	FunctionCallee* profExportFunc2 = getExportFunctionCallee2(module);
	builder.CreateCall(*profExportFunc2);
}


void InstrumentationFunctions::insertBBEnterCall(Module &module, Instruction* insertBefore, Value* basicBlockId) {
	IRBuilder<> builder(insertBefore);
	FunctionCallee* bbEnterFunc = getBBEnterFunctionCallee(module);
	builder.CreateCall(*bbEnterFunc, basicBlockId);
}
