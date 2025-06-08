#include <map>
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/Type.h"
#include "DAG.h"
using namespace std;
using namespace llvm;

class PathRecorder {
	public:
		void addPathArray(DAG* dag) {
			Function* func = dag->getFunction();
			if(pathsMap.find(func) != pathsMap.end()) {
				return;
			}
			Module* mod = func->getParent();
			string arrayName = func->getParent()->getName().str() + "__paths_" + func->getName().str();
			GlobalVariable* pathArray = createCounterArrayDeclaration(mod, dag->getNumberUniquePaths(), arrayName);
			pathsMap[func] = pathArray;
		}

		void addCounterForSingleBlockFunction(Function* func) {
			if(pathsMap.find(func) != pathsMap.end()) {
				return;
			}
			errs() << "Single block func called\n";
			assert(func->size() == 1 && "Can only be used for single-block function.");
			Module* mod = func->getParent();
			string arrayName = func->getParent()->getName().str() + "__paths_" + func->getName().str();
			GlobalVariable* pathArray = createCounterArrayDeclaration(mod, 1, arrayName);
			pathsMap[func] = pathArray;
		}

		GlobalVariable* getPathArray(Function* f) { 
			return pathsMap[f];
		}

	private:
		map<Function*, GlobalVariable*> pathsMap;

		GlobalVariable* createCounterArrayDeclaration(Module* mod, size_t size, string name) {
			GlobalVariable* existingArray = mod->getGlobalVariable(name);
			if (existingArray) {
				errs() << "Found existing array declaration: " << name << "\n";
				return existingArray;
			}

			LLVMContext& context = mod->getContext();
			ArrayType* arrayType = ArrayType::get(Type::getInt32Ty(context), size);

			// Create declaration only (no initializer)
			GlobalVariable* pathArray = new GlobalVariable (
					*mod, 
					arrayType, 
					false,                          // isConstant = false
					GlobalValue::ExternalLinkage,   // Linkage = External
					nullptr,                        // Initializer = nullptr (declaration only)
					name
					); 

			return pathArray;
		}
};
