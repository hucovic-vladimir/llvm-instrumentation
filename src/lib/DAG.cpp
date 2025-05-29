#include "DAG.h"
#include "CFGAnalysis.h"
#include "llvm/IR/CFG.h"
#include <sstream>
#include <map>

DAG::DAG(Function &F, FunctionAnalysisManager &FAM) : entry(nullptr), exit(nullptr), function(&F) {
	// 1. First set entry and exit points
	BasicBlock* entryBlock = &F.getEntryBlock();
	BasicBlock* exitBlock = CFGAnalysis::getSingleExit(F);
	assert(exitBlock != nullptr && ("Exit block not found in " + F.getName().str()).c_str());
	entry = new GraphNode(entryBlock);
	exit = new GraphNode(exitBlock);

	// 2. Create a single pass to build the node map (using a map instead of searching a vector repeatedly)
	std::map<BasicBlock*, GraphNode*> blockToNodeMap;

	// Add entry and exit nodes to the map
	blockToNodeMap[entry->getBlock()] = entry;
	blockToNodeMap[exit->getBlock()] = exit;

	nodes.push_back(entry);

	// 3. First create all nodes
	for (BasicBlock& BB : F) {
		// Skip unreachable blocks
		if (hasUnreachable(BB)) continue;

		// Create node if it doesn't exist
		if (blockToNodeMap.find(&BB) == blockToNodeMap.end()) {
			GraphNode* node = new GraphNode(&BB);
			blockToNodeMap[&BB] = node;
			nodes.push_back(node);
		}
	}

	// 4. Then create all edges using the map (O(1) lookup instead of O(n) search)
	for (BasicBlock& BB : F) {
		if (hasUnreachable(BB)) continue;

		GraphNode* srcNode = blockToNodeMap[&BB];

		for (BasicBlock* succ : successors(&BB)) {
			if (hasUnreachable(*succ)) continue;

			// Get or create successor node
			GraphNode* dstNode;
			auto it = blockToNodeMap.find(succ);
			if (it == blockToNodeMap.end()) {
				dstNode = new GraphNode(succ);
				blockToNodeMap[succ] = dstNode;
				nodes.push_back(dstNode);
			} else {
				dstNode = it->second;
			}

			// Create the edge
			edges.push_back(GraphEdge(srcNode, dstNode));
		}
	}

	// 5. Assign successors to all nodes
	for (auto& node : nodes) {
		node->assignSuccessors(this);
	}
	if(exitBlock != entryBlock)
		nodes.push_back(exit);
}

bool DAG::hasUnreachable(BasicBlock& BB) {
	for (auto& I : BB) {
		if (StringRef(I.getOpcodeName()).contains("unreachable")) {
			return true;
		}
	}
	return false;
}

std::string DAG::toStr() {
	std::stringstream ss;
	ss << "Function: " << function->getName().str() << "\n";
	ss << "Entry: " << entry->getName() << "\n";
	ss << "Exit: "  << exit->getName() << "\n";
	ss << "Edges :\n";
	for(auto edge : edges) {
		ss << "\t Edge " << 
			edge.getSrc()->getName() << 
			" -> " <<
			edge.getDst()->getName() << "\n";
	}
	for(auto node : nodes) {
		ss << "\t Node " << node->getName() << "\n";
	}
	return ss.str();
}

vector<GraphNode*> DAG::getReverseTopologicalOrder() {
    vector<GraphNode*> result;
    unordered_set<GraphNode*> visited;
    
    dfsTopologicalSort(entry, visited, result);
    
    return result;
}

void DAG::dfsTopologicalSort(GraphNode* node, unordered_set<GraphNode*>& visited, vector<GraphNode*>& result) {
    if (visited.count(node) > 0)
        return;

    visited.insert(node);

    for (auto& edge : edges) {
        if (edge.getSrc() == node) {
            GraphNode* succ = edge.getDst();
            dfsTopologicalSort(succ, visited, result);
        }
    }    
    result.push_back(node);
}

void DAG::assignEdgeValues() {
	vector<GraphNode*> reverseTopoOrder = getReverseTopologicalOrder();
	map<GraphNode*, int> numPaths;
	for(auto* node : reverseTopoOrder) {
		if(node == exit) {
			numPaths[node] = 1;
		}
		else {
			numPaths[node] = 0;
			for(auto& edge : edges) {
				GraphNode* src = edge.getSrc();
				if(src == node) {
					edge.assignValue(numPaths[src]);
					numPaths[edge.getSrc()] += numPaths[edge.getDst()];
				}
			}
		}
	}
}

void DAG::printEdgeValues() {
	errs() << "Function: " << entry->getBlock()->getParent()->getName().str() << "\n";
	for(auto& edge : edges) {
		errs() << "\tEdge " << edge.getSrc()->getName() << " -> " << edge.getDst()->getName() << ": " << edge.getValue() << "\n";
	}
}

DAG* DAG::createFromFunction(Function &F, FunctionAnalysisManager &FAM) {
    if(CFGAnalysis::isAcyclic(F, FAM)) {
        return new DAG(F, FAM); 
    }
    return nullptr;
}
