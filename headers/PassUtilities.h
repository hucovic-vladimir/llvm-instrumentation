// TODO rework this class 
#pragma once
#include <stdlib.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

using namespace llvm;

/// @class PassUtilities
/// @brief A class to hold utility functions for passes
/// @todo rework this clas
class PassUtilities {
    public:
			/// @todo Move elsewhere
			/// @brief Get the file name from a path
			/// @param path The path to get the file name from
			/// @return The file name
			static const std::string getFileName(const std::string& path);

			/// @brief Get all return instructions from a function
			/// @param F The function to get the return instructions from
			/// @return A vector of all return instructions in the function
			static const std::vector<ReturnInst*> getReturnInstructionsFromFunction(Function &F);

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

				/// @brief Get the tabs for a given depth
				/// @param depth The depth to get the tabs for
				/// @return A string of tabs
				static std::string getTabs(unsigned depth) {
					std::string tabs = "";
					for(unsigned i = 0; i < depth; i++) {
						tabs += "\t";
					}
					return tabs;
				}
};
