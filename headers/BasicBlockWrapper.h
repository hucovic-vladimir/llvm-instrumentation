#pragma once
#include "PassUtilities.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Instructions.h>
#include <sstream>
#include <map>


using namespace llvm;

class BasicBlockWrapper {
	private:
		unsigned long id;
		BasicBlock* bb;
		std::vector<BasicBlockWrapper*> successorWrappers;

		std::string getDebugInformation(unsigned depth) {
			auto tabs = PassUtilities::getTabs;
			std::map<unsigned, std::vector<unsigned>> lineToColumns;

			for (Instruction& i : *bb) {
				auto debug = i.getDebugLoc();
				if (debug) {
					if(debug.getLine() != 0) {
						// Add if column not already in list
						if(std::find(lineToColumns[debug.getLine()].begin(), lineToColumns[debug.getLine()].end(), debug.getCol()) == lineToColumns[debug.getLine()].end())
							lineToColumns[debug.getLine()].push_back(debug.getCol());
					}
				}
			}

			std::stringstream ss;
			ss << "[\n";
			for (auto& lineColumnPair : lineToColumns) {
				ss << tabs(depth + 1) << "{\n";
				ss << tabs(depth + 2) << "\"line\": " << lineColumnPair.first << ", \"columns\": [";
				for (size_t i = 0; i < lineColumnPair.second.size(); ++i) {
					ss << lineColumnPair.second[i];
					if (i < lineColumnPair.second.size() - 1) ss << ", ";
				}
				ss << "]\n";
				if(lineColumnPair.first != lineToColumns.rbegin()->first)
					ss << tabs(depth + 1) << "},\n";
				else
					ss << tabs(depth + 1) << "}\n";
			}
			ss << tabs(depth) << "],\n";
			return ss.str();
		}

		std::vector<Instruction*> getNonDebugInstructions() {
			std::vector<Instruction*> nonDebugInstructions;
			for (Instruction& i : *bb) {
				if (!i.isDebugOrPseudoInst()) {
					nonDebugInstructions.push_back(&i);
				}
			}
			return nonDebugInstructions;
		}

	public:
		BasicBlockWrapper(unsigned long id, BasicBlock* bb) : id(id), bb(bb) {}

		BasicBlock* getBB() { return bb; }
		unsigned long getId() { return id; }
		std::string toJson(unsigned depth = 0) {
			auto tabs = PassUtilities::getTabs; 
			std::stringstream ss;
			ss << tabs(depth) << "{\n";
			ss << tabs(depth+1) << "\"function\": \"" << bb->getParent()->getName().str() << "\",\n";
			ss << tabs(depth+1) << "\"id\": " << id << ",\n";
			ss << tabs(depth+1) << "\"name\": \"" << bb->getName().str() << "\",\n";
			ss << tabs(depth+1) << "\"debug\": " << getDebugInformation(depth+1);
			ss << tabs(depth+1) << "\"successors\": [";
			for (size_t i = 0; i < successorWrappers.size(); ++i) {
				ss << successorWrappers[i]->getId();
				if (i < successorWrappers.size() - 1) ss << ", ";
			}
			ss << "],\n";
			ss << tabs(depth+1) << "\"ir\": [\n";
			std::vector<Instruction*> nonDebugInstructions = getNonDebugInstructions();
			for(Instruction* i : nonDebugInstructions) {
				std::string instructionStr;
				raw_string_ostream rso(instructionStr);
				i->print(rso);
				while(instructionStr.find("\n") != std::string::npos)
					instructionStr.replace(instructionStr.find("\n"), 1, "\\n");
				ss << tabs(depth+2) << "\"" << instructionStr.erase(0, 2) << "\"";
				if(i != nonDebugInstructions.back())
					ss << ",";
				ss << "\n";
			}
			ss << tabs(depth+1) << "]\n";
			ss << tabs(depth) << "}";
			return ss.str();
		}
		
		std::vector<BasicBlockWrapper*> getSuccessors(std::unordered_map<BasicBlock*, BasicBlockWrapper*> bbMap) {
			successorWrappers.clear();
			for (BasicBlock* succ : successors(bb)) {
				successorWrappers.push_back(bbMap[succ]);
			}
			return successorWrappers;
		}
};

