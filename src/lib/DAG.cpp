#include "DAG.h"
#include "CFGAnalysis.h"
#include "llvm/IR/CFG.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/FileSystem.h"
#include <sstream>
#include <map>

DAG::DAG(Function &F, FunctionAnalysisManager &FAM) : entry(nullptr), exit(nullptr), function(&F) {
	BasicBlock* entryBlock = &F.getEntryBlock();
	BasicBlock* exitBlock = CFGAnalysis::getSingleExit(F);
	map<BasicBlock*, BasicBlock*> backEdges = CFGAnalysis::getBackEdges(F, FAM);
	map<BasicBlock*, bool> hasDummyEdgeFromEntry;
	map<BasicBlock*, bool> hasDummyEdgeToExit;

	assert(exitBlock != nullptr && ("Exit block not found in " + F.getName().str()).c_str());
	entry = new GraphNode(entryBlock);
	exit = new GraphNode(exitBlock);

	std::map<BasicBlock*, GraphNode*> blockToNodeMap;

	blockToNodeMap[entry->getBlock()] = entry;
	blockToNodeMap[exit->getBlock()] = exit;

	nodes.push_back(entry);

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
			if(backEdges.find(&BB) != backEdges.end() && backEdges[&BB] == succ) {
				// If this is a back edge, add the dummy edge entry -> target of backEdge
				if(!hasDummyEdgeFromEntry[succ]) {
					edges.push_back(new GraphEdge(entry, dstNode));
					hasDummyEdgeFromEntry[succ] = true;
				}
				// And the dummy edge source -> exit
				if(!hasDummyEdgeToExit[&BB]) {
					edges.push_back(new GraphEdge(srcNode, exit));
					hasDummyEdgeToExit[&BB] = true;
				}
				GraphEdge* backedge = new GraphEdge(srcNode, dstNode);
				backedge->isBackedge = true;
				this->backEdges.push_back(backedge);
				continue;
			}
			edges.push_back(new GraphEdge(srcNode, dstNode));
		}
	}

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

string DAG::toStr() {
	stringstream ss;
	unordered_map<GraphNode*, vector<GraphNode*>> edgeMap;
	for (auto edge : edges) {
		edgeMap[edge->getSrc()].push_back(edge->getDst());
	}
	ss << "Function: " << function->getName().str() << "\n";
	ss << "Entry: " << entry->getName() << "\n";
	ss << "Exit: "  << exit->getName() << "\n";
	ss << "Edges :\n";
	for(auto node : nodes) {
		if (edgeMap.find(node) != edgeMap.end()) {
			for (auto& succ : edgeMap[node]) {
				ss << "\t" << node->getName() << " -> ";
				ss << succ->getName() << ", val: " << findEdge(*node, *succ)->getValue() << "\n";
			}
		}
		else {
			ss << "\t" << node->getName() <<  " -> END\n";
		}
	}
	for(auto node : nodes) {
		ss << node->toStr();
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

	for (auto edge : edges) {
		if (edge->getSrc() == node) {
			GraphNode* succ = edge->getDst();
			dfsTopologicalSort(succ, visited, result);
		}
	}    
	result.push_back(node);
}

void DAG::assignEdgeValues() {
	edges.push_back(new GraphEdge(exit, entry)); 
	vector<GraphNode*> reverseTopoOrder = getReverseTopologicalOrder();
	map<GraphNode*, int> numPaths;
	for(auto* node : reverseTopoOrder) {
		if(node == exit) {
			numPaths[node] = 1;
		}
		else {
			numPaths[node] = 0;
			for(auto edge : edges) {
				GraphNode* src = edge->getSrc();
				if(src == node) {
					edge->assignValue(numPaths[src]);
					numPaths[edge->getSrc()] += numPaths[edge->getDst()];
				}
			}
		}
	}
}

void DAG::printEdgeValues() {
	errs() << "Function: " << entry->getBlock()->getParent()->getName().str() << "\n";
	for(auto& edge : edges) {
		errs() << "\tEdge " << edge->getSrc()->getName() << " -> " << edge->getDst()->getName() << ": " << edge->getValue() << "\n";
	}
}

DAG* DAG::createFromFunction(Function &F, FunctionAnalysisManager &FAM) {
	return new DAG(F, FAM); 
}

void DAG::exportNodesToJson(string filename) {
	json::Array nodesArray;
	// Read existing file if it exists
	sys::fs::file_status status;
	error_code EC = sys::fs::status(filename, status);
	if (!EC && sys::fs::exists(status)) {
		ErrorOr<unique_ptr<MemoryBuffer>> fileBuffer = MemoryBuffer::getFile(filename);
		if (fileBuffer) {
			Expected<json::Value> parsedJson = json::parse(fileBuffer.get()->getBuffer());
			if (parsedJson) {
				if (auto *rootObj = parsedJson->getAsObject()) {
					if (auto *existingNodes = rootObj->getArray("nodes")) {
						nodesArray = *existingNodes;
					}
				}
			} else {
				errs() << "Error parsing existing JSON: " << toString(parsedJson.takeError()) << "\n";
			}
		}
	}

	for (auto& node : nodes) {
		Expected<json::Value> nodeJson = json::parse(node->toJson(this, 0)); // Get raw JSON string
		if (nodeJson) {
			nodesArray.push_back(std::move(*nodeJson));
		}
	}

	json::Object root;
	root["nodes"] = std::move(nodesArray);

	// Write to file
	raw_fd_ostream jsonFile(filename, EC);
	if(EC) {
		errs() << "Error opening file: " << filename << ": " << EC.message() << "\n";
		return;
	}

	jsonFile << formatv("{0:2}", json::Value(std::move(root)));
}

void DAG::eachEdge(
		std::function<void(GraphEdge*)> func) {
	for (auto edge : edges) {
		func(edge);
	}
}

void DAG::eachNode(
		std::function<void(GraphNode*)> func) {
	for (auto& node : nodes) {
		func(node);
	}
}

GraphEdge* DAG::findEdge(BasicBlock* src, BasicBlock* dst) {
	for(auto edge : edges) {
		if(edge->getSrc()->getBlock() == src && edge->getDst()->getBlock() == dst) {
			return edge;
		}
	}
	return nullptr;
}

GraphEdge* DAG::findBackedge(BasicBlock* src, BasicBlock* dst) {
	for(auto edge : backEdges) {
		if(edge->getSrc()->getBlock() == src && edge->getDst()->getBlock() == dst) {
			return edge;
		}
	}
	return nullptr;
}

void DAG::determineInstrumentedChords() {

}


