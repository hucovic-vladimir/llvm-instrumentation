#include "DAG.h"
#include "CFGAnalysis.h"
#include "SpanningTree.h"
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
			GraphEdge* backedge = nullptr;
			GraphEdge* dummyEdge = nullptr;
			if(backEdges.find(&BB) != backEdges.end() && backEdges[&BB] == succ) {
				// Record a backedge
				backedge = new GraphEdge(srcNode, dstNode);
				backedge->setEdgeType(EdgeType::Backedge);
				this->backEdges.push_back(backedge);
				// If this is a back edge, add the dummy edge entry -> target of backEdge
				if(!hasDummyEdgeFromEntry[succ]) {
					dummyEdge = new GraphEdge(entry, dstNode);
					edges.push_back(dummyEdge);
					hasDummyEdgeFromEntry[succ] = true;
					dummyEdgeToBackedgeMap[dummyEdge] = backedge;
					dummyEdge->setEdgeType(EdgeType::DummyEdgeFromEntry);
				}
				// And the dummy edge source -> exit
				if(!hasDummyEdgeToExit[&BB]) {
					dummyEdge = new GraphEdge(srcNode, exit);
					edges.push_back(dummyEdge);
					hasDummyEdgeToExit[&BB] = true;
					dummyEdgeToBackedgeMap[dummyEdge] = backedge;
					dummyEdge->setEdgeType(EdgeType::DummyEdgeToExit);
				}
				continue;
			}
			edges.push_back(new GraphEdge(srcNode, dstNode));
		}
	}

	for (auto& node : nodes) {
		node->assignSuccessors(this);
	}
	if(exitBlock != entryBlock) {
		nodes.push_back(exit);
		GraphEdge* dummyEdge = new GraphEdge(exit, entry);
		dummyEdge->setEdgeType(EdgeType::DummyEdgeExitToEntry);
		edges.push_back(dummyEdge);
	}
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
	ss << "Function: " << function->getName().str() << "\n";
	ss << "Entry: " << entry->getName() << "\n";
	ss << "Exit: "  << exit->getName() << "\n";
	ss << "Edges :\n";

	vector<GraphNode*> revTopoOrder = getReverseTopologicalOrder();
	vector<GraphNode*> topoOrder(revTopoOrder.rbegin(), revTopoOrder.rend());

	// Create a map from node to its topological position
	unordered_map<GraphNode*, int> topoPosition;
	for(int i = 0; i < topoOrder.size(); i++) {
		topoPosition[topoOrder[i]] = i;
	}

	// Sort edges by topological order of source node, then by destination node
	vector<GraphEdge*> sortedEdges = edges;
	std::sort(sortedEdges.begin(), sortedEdges.end(), [&](GraphEdge* a, GraphEdge* b) {
			int srcPosA = topoPosition[a->getSrc()];
			int srcPosB = topoPosition[b->getSrc()];
			if(srcPosA != srcPosB) {
			return srcPosA < srcPosB; // Earlier in topo order comes first
			}
			// If same source, sort by destination
			return topoPosition[a->getDst()] < topoPosition[b->getDst()];
			});

	// Print sorted edges
	for(auto edge : sortedEdges) {
		ss << "\t" << edge->getSrc()->getName() << " -> ";
		ss << edge->getDst()->getName() << ", val: " << edge->getValue() << "\n";
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
	vector<GraphNode*> reverseTopoOrder = getReverseTopologicalOrder();
	errs() << "Reverse Topological Order:\n";
	for (auto node : reverseTopoOrder) {
		errs() << "\t" << node->getName() << "\n";
	}
	map<GraphNode*, int> numPaths;
	for(auto* node : reverseTopoOrder) {
		if(node == exit) {
			numPaths[node] = 1;
		}
		else {
			numPaths[node] = 0;
			for(auto edge : edges) {
				if(edge->isDummyEdgeExitToEntry()) {
					continue;
				}
				GraphNode* src = edge->getSrc();
				if(src == node) {
				errs() << "Processing edge: " << edge->getSrc()->getName() << " -> " << edge->getDst()->getName() << "\n";
					errs() << "Num paths from " << src->getName() << " to exit: " << numPaths[exit] << "\n";
					errs() << "Assigning value " << numPaths[src] << " to edge: " << edge->getSrc()->getName() << " -> " << edge->getDst()->getName() << "\n";
					errs() << "Edge address: " << edge << "\n";
					edge->assignValue(numPaths[src]);
					numPaths[edge->getSrc()] += numPaths[edge->getDst()];
					errs() << "Updated num paths for " << edge->getSrc()->getName() << ": " << numPaths[edge->getSrc()] << "\n";
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
	for (auto node : nodes) {
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

GraphEdge* DAG::findChord(BasicBlock* src, BasicBlock* dst) {
	for(auto edge : chords) {
		if(edge->getSrc()->getBlock() == src && edge->getDst()->getBlock() == dst) {
			return edge;
		}
	}
	return nullptr;
}

int dir(GraphEdge* e, GraphEdge* f) {
	if(e == nullptr)
		return 1;
	bool cond = (e->getSrc() == f->getSrc() || e->getSrc() == f->getDst() || e->getDst() == f->getSrc() || e->getDst() == f->getDst());
	assert(cond && "Edges are not connected, cannot determine direction");
	if(e->getSrc() == f->getDst() || e->getDst() == f->getSrc())
		return 1;
	 return -1;
}

void DAG::eventCountingDFS() {
	getChordsAndSpanningTree();
	for(GraphEdge* e : chords) {
		e->setIncrementValue(0);
	}
	eventCountingDFS(0, this->entry, nullptr);
	for(GraphEdge* e : chords) {
		e->setIncrementValue(e->getIncrementValue() + e->getValue());
	}
}

void DAG::eventCountingDFS(int events, GraphNode* node, GraphEdge* edge) {
	for(GraphEdge* f : spanningTree) {
		if(f->getDst() == node && edge != f) {
			eventCountingDFS(dir(edge, f) * events + f->getValue(), f->getSrc(), f);
		}
		else if(f->getSrc() == node && edge != f) {
			eventCountingDFS(dir(edge, f) * events + f->getValue(), f->getDst(), f);
		}
	}
	for(GraphEdge* c : chords) {
		if(c->getSrc() == node || c->getDst() == node) {
			c->setIncrementValue(c->getIncrementValue() + dir(edge, c) * events);
		}
	}
}

void DAG::getChordsAndSpanningTree() {
	vector<GraphEdge*> spanningTree = SpanningTree::kruskalMaxSpanningTree(this);
	vector<GraphEdge*> chords;
	for(GraphEdge* e : this->edges) {
		if(std::find(spanningTree.begin(), spanningTree.end(), e) == spanningTree.end()) {
			chords.push_back(e);
		}
	}
	this->spanningTree = spanningTree;
	this->chords = chords;
}

