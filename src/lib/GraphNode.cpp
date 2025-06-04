#include "DAG.h"
#include "../../headers/PassUtilities.h"
#include <sstream>

using namespace std;

int GraphNode::lastNodeId = 0;

void GraphNode::assignSuccessors(DAG* graph)  {
	for(auto edge : graph->getEdges()) {
		if(edge->getSrc()->id == this->id) {
			successors.push_back(edge->getDst());
		}
	}
}

std::string GraphNode::toJson(DAG* dag, unsigned depth) const {
	auto tabs = PassUtilities::getTabs; 
	std::stringstream ss;
	ss << tabs(depth) << "{\n";
	ss << tabs(depth+1) << "\"function\": \"" << block->getParent()->getName().str() << "\",\n";
	ss << tabs(depth+1) << "\"id\": " << id << ",\n";
	ss << tabs(depth+1) << "\"name\": \"" << block->getName().str() << "\",\n";
	ss << tabs(depth+1) << "\"successors\": [";
	for (size_t i = 0; i < successors.size(); ++i) {
		GraphEdge* edge = dag->findEdge(*this, *successors[i]);
		ss << "{ "; 
		ss << "\"id\": " << successors[i]->getId() << ", ";
		ss << "\"edgeValue\": \"" << edge->getValue() << "\"";
		ss << " }";
		if (i < successors.size() - 1) ss << ", ";
	}
	ss << "]\n";
	ss << tabs(depth) << "}\n";
	return ss.str();
}


void GraphNode::printSuccessors() const {
	errs() << "Successors of " << block->getName() << ": ";
	for (GraphNode* succ : successors) {
		errs() << succ->getName() << " ptr: " << succ << " id: " << succ->getId() << ", ";
	}
	errs() << "\n";
}
