#pragma once
#include "OptimizationPattern.h"
#include <llvm/IR/Function.h>
#include <vector>
#include <sstream>

class FunctionPatterns {
	private:
		Function* function;
		std::vector<OptimizationPattern*> patterns;

	public:
		FunctionPatterns(Function* function, std::vector<OptimizationPattern*> patterns) : function(function), patterns(patterns) {};
		FunctionPatterns(Function* function) : function(function) {};
		virtual std::string toJson(unsigned depth = 0) {
			auto tabs = PassUtilities::getTabs;
			std::stringstream ss;
			ss << tabs(depth) << "{\n";
			ss << tabs(depth+1) << "\"functionName\": \"" << function->getName().str() << "\",\n";
			ss << tabs(depth+1) << "\"patterns\": [\n";
			for(auto p : patterns) {
				ss << p->toJson(depth+2);
				if(p != patterns.back()) {
					ss << ",\n";
				}
			}
			ss << "\n" << tabs(depth+1) << "]\n";
			ss << tabs(depth) << "}";
			return ss.str();
		}
		unsigned long getPatternCount() { return patterns.size(); }
};
