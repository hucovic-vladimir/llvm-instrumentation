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
		GraphEdge findEdge(GraphNode src, GraphNode dst) {
			for(auto& edge : edges) {
				if(*edge.getSrc() == src && *edge.getDst() == dst) {
					return edge;
				}
			}
			return GraphEdge((GraphNode*)nullptr, (GraphNode*)nullptr);
		}
		GraphEdge findEdge(BasicBlock* src, BasicBlock* dst) {
			for(auto& edge : edges) {
				if(edge.getSrc()->getBlock() == src && edge.getDst()->getBlock() == dst) {
					return edge;
				}
			}
			return GraphEdge((GraphNode*)nullptr, (GraphNode*)nullptr);
		}

		GraphNode* getEntry() const { return entry; }
		GraphNode* getExit() const { return exit; }

		long assignNodeIds(long& lastAssignedId) {
			for(auto& node : nodes) {
				node->assignId(lastAssignedId++);
			}
			return lastAssignedId;
		}

		vector<GraphEdge> getEdges() const {
			return edges;
		}

		void eachNode(
				std::function<void(GraphNode*)> func) {
			for (auto& node : nodes) {
				func(node);
			}
		}

		void eachEdge(
				std::function<void(GraphEdge)> func) {
			for (auto edge : edges) {
				func(edge);
			}
		}

		void exportNodesToJson(string filename) {
			error_code EC;
			raw_fd_ostream jsonFile(filename, EC);
			if(EC) {
				errs() << "Error opening file: " << filename << ": " << EC.message() << "\n";
				return;
			}
			jsonFile << "{\n";
			jsonFile << "\t\"nodes\": [\n";
			for (size_t i = 0; i < nodes.size(); ++i) {
				jsonFile << nodes[i]->toJson(2);
				if (i < nodes.size() - 1) jsonFile << "\t\t,\n";
			}
			jsonFile << "\n\t]\n";
			jsonFile << "}\n";
			errs() << "Exported nodes to " << filename << "\n";
		}

	private:
		GraphNode* entry;
		GraphNode* exit;
		Function* function;
		vector<GraphEdge> edges;
		vector<GraphNode*> nodes;

		DAG(Function &F, FunctionAnalysisManager &FAM);
		void addDummyEdge();
		void dfsTopologicalSort(GraphNode* node,
				unordered_set<GraphNode*>& visited,
				vector<GraphNode*>& result);
		void pushEdgesToReachableBlocks(Function& F);
		bool hasUnreachable(BasicBlock& BB);
};

