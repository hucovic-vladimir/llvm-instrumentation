#pragma once

#include "DAG.h"
#include "GraphNode.h"
#include <llvm/Support/raw_ostream.h>

using namespace std; using namespace llvm;

class GraphEdge {
	public:
		GraphEdge(BasicBlock* src, BasicBlock* dst) : src(new GraphNode(src)), dst(new GraphNode(dst)) {}
		GraphEdge(GraphNode* src, GraphNode* dst) : src(src), dst(dst) {}
		GraphNode* getSrc() { return src; }
		GraphNode* getDst() { return dst; }
		void assignValue(int value) { this->value = value; }
		int getValue() const { return value; }
		friend raw_ostream& operator<<(raw_ostream& OS, GraphEdge& edge) {
			OS << "GraphEdge: " << edge.getSrc()->getName() << " -> " << edge.getDst()->getName() << ", value: " << edge.value;
			return OS;
		}
		bool isBackedge = false;

	private:
		GraphNode* src;
		GraphNode* dst;
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
