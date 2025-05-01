#include "DAG.h"
#include "CFGAnalysis.h"
#include "llvm/IR/CFG.h"
#include <sstream>
#include <map>

DAG::DAG(Function &F, FunctionAnalysisManager &FAM) : entry(&F.getEntryBlock()), exit(nullptr) {
	const string assertionMessage = "Exit block not found in " + F.getName().str() + "\n"; 
	this->exit = CFGAnalysis::getSingleExit(F);
	assert(this->exit != nullptr && assertionMessage.c_str()); 

	for(BasicBlock& BB : F) {
		for(BasicBlock* succ : successors(&BB)) {
		bool shouldBeSkipped = false;
			for(auto& I : *succ) {
				if(StringRef(I.getOpcodeName()).contains("unreachable")) {
					shouldBeSkipped = true;
				}
			}
			if(!shouldBeSkipped) {
				GraphEdge edge(&BB, succ);
				this->edges.push_back(edge);
			}
		}
	}

	this->function = &F;
}

DAG* DAG::createFromFunction(Function &F, FunctionAnalysisManager &FAM) {
	if(CFGAnalysis::isAcyclic(F, FAM)) {
		return new DAG(F, FAM); 
	}
	return nullptr;
}

void DAG::addDummyEdge() {
	GraphEdge dummyEdge(entry.getBlock(), exit.getBlock());
	this->edges.push_back(dummyEdge);
}

void DAG::pushEdgesToReachableBlocks(Function& F) {
	bool shouldBeSkipped = false;
	for(BasicBlock& BB : F) {
		for(BasicBlock* succ : successors(&BB)) {
			for(auto& I : *succ) {
				if(StringRef(I.getOpcodeName()).contains("unreachable")) {
					shouldBeSkipped = true;
				}
			}
			if(!shouldBeSkipped) {
				GraphNode bbNode(&BB);
				GraphNode succNode(succ);
				this->edges.push_back(GraphEdge(bbNode, succNode));
			}
		}
	}
}

std::string DAG::toStr() {
	std::stringstream ss;
	ss << "Function: " << function->getName().str() << "\n";
	ss << "Entry: " << entry.getName() << "\n";
	ss << "Exit: "  << exit.getName() << "\n";
	ss << "Edges :\n";
	for(auto edge : edges) {
		ss << "\t Edge " << 
			edge.getSrc().getName() << 
			" -> " <<
			edge.getDst().getName() << "\n";
	}
	return ss.str();
}

vector<GraphNode> DAG::getReverseTopologicalOrder() {
    vector<GraphNode> result;
    unordered_set<GraphNode> visited;
    
    dfsTopologicalSort(entry, visited, result);
    
    return result;
}

void DAG::dfsTopologicalSort(GraphNode node, unordered_set<GraphNode>& visited, vector<GraphNode>& result) {
    if (visited.count(node) > 0)
        return;

    visited.insert(node);

    for (const auto& edge : edges) {
        if (edge.getSrc() == node) {
            GraphNode succ = edge.getDst();
            dfsTopologicalSort(succ, visited, result);
        }
    }    
    result.push_back(node);
}

void DAG::assignEdgeValues() {
	vector<GraphNode> reverseTopoOrder = getReverseTopologicalOrder();
	map<GraphNode, int> numPaths;
	for(auto node : reverseTopoOrder) {
		if(node == exit) {
			numPaths[node] = 1;
		}
		else {
			numPaths[node] = 0;
			for(auto& edge : edges) {
				GraphNode src = edge.getSrc();
				if(src == node) {
					edge.assignValue(numPaths[src]);
					numPaths[edge.getSrc()] += numPaths[edge.getDst()];
				}
			}
		}
	}
}

void DAG::printEdgeValues() {
	errs() << "Function: " << entry.getBlock()->getParent()->getName().str() << "\n";
	for(auto& edge : edges) {
		errs() << "\tEdge " << edge.getSrc().getName() << " -> " << edge.getDst().getName() << ": " << edge.getValue() << "\n";
	}
}
