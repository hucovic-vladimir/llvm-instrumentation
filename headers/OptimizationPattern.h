#pragma once
#include "BasicBlockWrapper.h"
#include "../headers/PassUtilities.h"
#include "llvm/IR/BasicBlock.h"

using bbWrapperMap = std::unordered_map<BasicBlock*, BasicBlockWrapper*>;
using namespace llvm;
class OptimizationPattern {
	private:
		static unsigned long patternCount;
	public:
		virtual std::string toJson(unsigned depth = 0) = 0;
		static OptimizationPattern* checkForPattern(bbWrapperMap map, BasicBlockWrapper* start, std::vector<BasicBlock*>& containedBlocks, unsigned depth = 1) { return nullptr; };
		static inline unsigned long& getPatternCount() { return patternCount; }
		virtual std::vector<BasicBlockWrapper*> getNonInstrumentedBlocks() = 0;
		virtual BasicBlock* getPatternExitBlock() = 0;
};

unsigned long OptimizationPattern::patternCount = 0;
