#pragma once
#include "llvm/IR/Function.h"
#include <vector>
#include "DAG.h"

using namespace std; using namespace llvm;

class DAG;

class GraphNode {
	public:
		GraphNode(BasicBlock* block) : block(block) {}
		BasicBlock* getBlock() const { return block; }
		string getName() const { return block->getName().str(); }

		bool operator==(const GraphNode& other) const { return block == other.block;}
		bool operator!=(const GraphNode& other) const { return !(*this == other);}
		bool operator<(const GraphNode& other) const { return block < other.block; }
		bool operator>(const GraphNode& other) const { return block > other.block;}

		friend raw_ostream& operator<<(raw_ostream& OS, const GraphNode& node) {
			OS << "GraphNode: " << node.getName();
			return OS;
		}

		void assignSuccessors(DAG* graph);
		void printSuccessors() const;
		std::string toJson(unsigned depth = 0) const;
		long getId() const { return id; }
		void assignId(long id) {	this->id = id; }

	private:
		long id = -1;
		BasicBlock* block;
		vector<GraphNode*> successors;
};
