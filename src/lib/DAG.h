#pragma once
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include <vector>
#include <unordered_set>

using namespace llvm;
using namespace std;

class GraphNode {
	public:
		GraphNode(BasicBlock* block) : block(block) {}
		BasicBlock* getBlock() const { return block; }
		string getName() const { return block->getName().str(); }

		bool operator==(const GraphNode& other) const {
			return block == other.block;
		}
		bool operator!=(const GraphNode& other) const {
			return !(*this == other);
		}
		bool operator<(const GraphNode& other) const {
			return block < other.block;
		}
		bool operator>(const GraphNode& other) const {
			return block > other.block;
		}
		friend raw_ostream& operator<<(raw_ostream& OS, const GraphNode& node) {
			OS << "GraphNode: " << node.getName();
			return OS;
		}

	private:
		BasicBlock* block;
};

class GraphEdge {
	public:
		GraphEdge(BasicBlock* src, BasicBlock* dst) : src(src), dst(dst) {}
		GraphEdge(GraphNode src, GraphNode dst) : src(src.getBlock()), dst(dst.getBlock()) {}
		GraphNode getSrc() const { return src; }
		GraphNode getDst() const { return dst; }
		void assignValue(int value) { this->value = value; }
		int getValue() const { return value; }
		friend raw_ostream& operator<<(raw_ostream& OS, const GraphEdge& edge) {
			OS << "GraphEdge: " << edge.getSrc().getName() << " -> " << edge.getDst().getName();
			return OS;
		}

	private:
		GraphNode src;
		GraphNode dst;
		int value = 0;
};

namespace std {
	template <>
	struct hash<GraphNode> {
		size_t operator()(const GraphNode& node) const {
			return hash<BasicBlock*>()(node.getBlock());
		}
	};
}

class DAG {
	public:
		static DAG* createFromFunction(Function &F, FunctionAnalysisManager &FAM);
		string toStr();
		vector<GraphNode> getReverseTopologicalOrder();
		void assignEdgeValues();
		void printEdgeValues();
		Function* getFunction() const { return function; }
		GraphEdge findEdge(GraphNode src, GraphNode dst) {
			for(auto& edge : edges) {
				if(edge.getSrc() == src && edge.getDst() == dst) {
					return edge;
				}
			}
			return GraphEdge(nullptr, nullptr);
		}
		GraphEdge findEdge(BasicBlock* src, BasicBlock* dst) {
			for(auto& edge : edges) {
				if(edge.getSrc().getBlock() == src && edge.getDst().getBlock() == dst) {
					return edge;
				}
			}
			return GraphEdge(nullptr, nullptr);
		}

	private:
		Function* function;
		GraphNode entry;
		GraphNode exit;
		vector<GraphEdge> edges;

		DAG(Function &F, FunctionAnalysisManager &FAM);
		void addDummyEdge();
		void dfsTopologicalSort(GraphNode node,
				unordered_set<GraphNode>& visited,
				vector<GraphNode>& result);
		void pushEdgesToReachableBlocks(Function& F);
};
