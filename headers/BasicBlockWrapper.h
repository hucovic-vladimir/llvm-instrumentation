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
		std::vector<unsigned long> successorsIds;



		std::string getDebugInformation(unsigned depth) {
			auto tabs = PassUtilities::getTabs;
			std::map<unsigned, std::vector<unsigned>> lineToColumns;

			for (Instruction& i : *bb) {
				auto debug = i.getDebugLoc();
				if (debug) {
					if(debug.getLine() != 0) {
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

	public:
		BasicBlockWrapper(unsigned long id, BasicBlock* bb) : id(id), bb(bb) {}

		BasicBlock* getBB() { return bb; }
		unsigned long getId() { return id; }
		std::string toJson(unsigned depth = 0) {
			auto tabs = PassUtilities::getTabs; 
			std::stringstream ss;
			ss << tabs(depth) << "{\n";
			ss << tabs(depth+1) << "\"id\": " << id << ",\n";
			ss << tabs(depth+1) << "\"name\": \"" << bb->getName().str() << "\",\n";
			ss << tabs(depth+1) << "\"debug\": " << getDebugInformation(depth+1);
			ss << tabs(depth+1) << "\"successors\": [";
			for (size_t i = 0; i < successorsIds.size(); ++i) {
				ss << successorsIds[i];
				if (i < successorsIds.size() - 1) ss << ", ";
			}
			ss << "]\n";
			ss << tabs(depth) << "}";
			return ss.str();
		}
		
		std::vector<unsigned long> getSuccessors(std::unordered_map<BasicBlock*, BasicBlockWrapper*> bbMap) {
			successorsIds.clear();
			for (BasicBlock* succ : successors(bb)) {
				successorsIds.push_back(bbMap[succ]->getId());
			}
			return successorsIds;
		}
};

