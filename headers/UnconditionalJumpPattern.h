#include "../headers/OptimizationPattern.h"
#include <llvm/IR/CFG.h>
#include <sstream>

class UnconditionalJumpPattern : public OptimizationPattern {
	private:
		static unsigned long patternCount;
		unsigned long id;
		BasicBlockWrapper* start = nullptr;
		BasicBlockWrapper* jumpDestination = nullptr;

	public:
		UnconditionalJumpPattern(BasicBlockWrapper* start, BasicBlockWrapper* jumpDestination) :
			id(OptimizationPattern::getPatternCount()), start(start), jumpDestination(jumpDestination)
	{ OptimizationPattern::getPatternCount()++; }

		virtual BasicBlock* getPatternExitBlock() override { return jumpDestination->getBB(); }

		virtual std::string toJson(unsigned depth = 0) override {
			auto tabs = PassUtilities::getTabs;
			std::stringstream ss;
			ss << tabs(depth) << "{\n";
			ss << tabs(depth+1) << "\"id\": " << id << ",\n";
			ss << tabs(depth+1) << "\"type\": \"unconditionalJump\",\n";
			ss << tabs(depth+1) << "\"start\": " << start->getId() << ",\n";
			ss << tabs(depth+1) << "\"jumpDestination\": " << jumpDestination->getId() << "\n";
			ss << tabs(depth) << "}";
			return ss.str();
		}

		virtual std::vector<BasicBlockWrapper*> getNonInstrumentedBlocks() override { return {start}; }

		static UnconditionalJumpPattern* checkForPattern(bbWrapperMap map, BasicBlockWrapper* start, std::vector<BasicBlock*>& containedBlocks ,unsigned depth = 1) { 
			BasicBlock* startBlock = start->getBB();
			if(startBlock->getTerminator()->getNumSuccessors() != 1) { return nullptr; }
			BasicBlock* next = startBlock->getTerminator()->getSuccessor(0);
			if(pred_size(next) != 1) { return nullptr; }
			containedBlocks.push_back(next);
			containedBlocks.push_back(startBlock);
			return new UnconditionalJumpPattern(map[startBlock], map[next]);
		}
};
