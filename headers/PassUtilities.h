// TODO rework this class 

#include <stdlib.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>

using namespace llvm;

/// @class PassUtilities
/// @brief A class to hold utility functions for passes
/// @todo rework this clas
class PassUtilities {
    public:
				/// @brief Get the start and end line numbers of a basic block
				/// @param bb The basic block to get the start and end lines from
				/// @return A pair of the start and end line numbers of the basic block
				/// @todo try to think of a better solution to mapping basic blocks to source code 
        static std::pair<unsigned long, unsigned long> getBasicBlockStartEndLines(BasicBlock& bb);
				/// @brief Insert a call to setStartTime at the given instruction
				/// @param i The instruction to insert the call to setStartTime at
        static void insertSetStartTime(Instruction &i);
				/// @brief Insert the module name as a global string
				/// @param m The module to insert the module name as a global string into
				/// @return The global string containing the module name
        static Constant* insertModuleNameAsCharPtr(Module* m);
				
				/// @todo maybe remove this or rework it to cover all the functions
				/// that we dont want to instrument
        static bool isStdFunction(Function& f) { return f.getName().startswith("__cxx"); }
};
