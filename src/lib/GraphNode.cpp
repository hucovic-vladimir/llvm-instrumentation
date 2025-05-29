#include "DAG.h"
#include "../../headers/PassUtilities.h"
#include <sstream>

using namespace std;

void GraphNode::assignSuccessors(DAG* graph)  {
	for(auto& edge : graph->getEdges()) {
		if(*edge.getSrc() == *this) {
			successors.push_back(edge.getDst());
		}
	}
}

std::string GraphNode::toJson(unsigned depth) const {
	auto tabs = PassUtilities::getTabs; 
	std::stringstream ss;
	ss << tabs(depth) << "{\n";
	ss << tabs(depth+1) << "\"function\": \"" << block->getParent()->getName().str() << "\",\n";
	ss << tabs(depth+1) << "\"id\": " << id << ",\n";
	ss << tabs(depth+1) << "\"name\": \"" << block->getName().str() << "\",\n";
	ss << tabs(depth+1) << "\"successors\": [";
	for (size_t i = 0; i < successors.size(); ++i) {
		ss << "{ "; 
		ss << "\"id\": " << successors[i]->getId() << ", ";
		ss << "\"edgeValue\": \"" << successors[i]->getId() << "\"";
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
