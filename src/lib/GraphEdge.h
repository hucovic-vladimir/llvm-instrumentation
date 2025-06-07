#pragma once

#include "DAG.h"
#include "GraphNode.h"
#include <llvm/Support/raw_ostream.h>

using namespace std; using namespace llvm;

enum class EdgeType {
	Normal,
	Backedge,
	DummyEdgeToExit,
	DummyEdgeFromEntry,
	DummyEdgeExitToEntry,
};

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
		int getIncrementValue() const { return incrementValue; };
		void setIncrementValue(int value) { incrementValue = value; }
		bool isBackedge() const { return edgeType == EdgeType::Backedge; }
		bool isDummyEdgeToExit() const { return edgeType == EdgeType::DummyEdgeToExit; }
		bool isDummyEdgeFromEntry() const { return edgeType == EdgeType::DummyEdgeFromEntry; }
		bool isDummyEdgeExitToEntry() const { return edgeType == EdgeType::DummyEdgeExitToEntry; }
		bool isNormal() const { return edgeType == EdgeType::Normal; }
		void setEdgeType(EdgeType type) { edgeType = type; }
		bool isChord() const { return chord; }

	private:
		GraphNode* src;
		GraphNode* dst;
		int value = 0;
		int incrementValue = 0;
		EdgeType edgeType = EdgeType::Normal;
		bool chord = false;
};

namespace std {
	template <>
	struct hash<GraphNode> {
		size_t operator()(const GraphNode& node) const {
			return hash<BasicBlock*>()(node.getBlock());
		}
	};
}
