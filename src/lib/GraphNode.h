#pragma once
#include "llvm/IR/Function.h"
#include <vector>
#include <sstream>
#include "DAG.h"

using namespace std; using namespace llvm;

class DAG;

class GraphNode {
	public:
		GraphNode(BasicBlock* block) : block(block) {
			this->assignId();
		}
		BasicBlock* getBlock() const { return block; }
		string getName() const { return block->getName().str(); }
		string getNodeName() const { return name; }
		void setNodeName(string n) { name = n; }

		bool operator==(const GraphNode& other) const { return block == other.block;}
		bool operator!=(const GraphNode& other) const { return !(*this == other);}
		bool operator<(const GraphNode& other) const { return block < other.block; }
		bool operator>(const GraphNode& other) const { return block > other.block;}

		friend raw_ostream& operator<<(raw_ostream& OS, const GraphNode& node) {
			OS << "GraphNode: " << node.getName() << "Id: " << node.id << "\n";
			return OS;
		}

		string toStr() {
			stringstream ss;
			ss << "GraphNode: " << this->getName() << " Id: " << this->id << "\n";
			return ss.str();
		}

		void assignSuccessors(DAG* graph);
		void printSuccessors() const;
		std::string toJson(DAG* dag, unsigned depth = 0) const;
		long getId() const { return id; }
		void assignId() {	this->id = lastNodeId++; }
		static void resetLastId() { lastNodeId = 0; }

	private:
		int id = -1;
		BasicBlock* block;
		vector<GraphNode*> successors;
		static int lastNodeId;
		string name;
};
