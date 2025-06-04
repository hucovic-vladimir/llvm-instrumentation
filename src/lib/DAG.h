#pragma once
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include <vector>
#include <unordered_set>
#include "GraphNode.h"
#include "GraphEdge.h"

using namespace llvm;
using namespace std;

class GraphNode;
class GraphEdge;

class DAG {
	public:
		static DAG* createFromFunction(Function &F, FunctionAnalysisManager &FAM);
		string toStr();
		vector<GraphNode*> getReverseTopologicalOrder();
		void assignEdgeValues();
		void printEdgeValues();
		Function* getFunction() const { return function; }

		GraphEdge* findEdge(BasicBlock* src, BasicBlock* dst);
		GraphEdge* findBackedge(BasicBlock* src, BasicBlock* dst);
		GraphEdge* findEdge(GraphNode src, GraphNode dst) {
			return findEdge(src.getBlock(), dst.getBlock());
		}
		GraphEdge* findBackedge(GraphNode src, GraphNode dst) {
			return findBackedge(src.getBlock(), dst.getBlock());
		}
		GraphNode* getEntry() const { return entry; }
		GraphNode* getExit() const { return exit; }

		long assignNodeIds(long& lastAssignedId);

		vector<GraphEdge*> getEdges() const { return edges; }
		vector<GraphNode*> getNodes() const { return nodes; }

		void eachNode(std::function<void(GraphNode*)> func);
		void eachEdge(std::function<void(GraphEdge*)> func);

		void exportNodesToJson(string filename);

		void determineInstrumentedChords();


	private:
		GraphNode* entry;
		GraphNode* exit;
		Function* function;
		vector<GraphEdge*> edges;
		vector<GraphNode*> nodes;
		vector<GraphEdge*> backEdges;

		DAG(Function &F, FunctionAnalysisManager &FAM);
		void addDummyEdge();
		void dfsTopologicalSort(GraphNode* node,
				unordered_set<GraphNode*>& visited,
				vector<GraphNode*>& result);
		void pushEdgesToReachableBlocks(Function& F);
		bool hasUnreachable(BasicBlock& BB);
};

