#include "OptimizationPattern.h"
#include <llvm/IR/CFG.h>
#include <sstream>

class SumOfExitsPattern : public OptimizationPattern {
	private:
		static unsigned long patternCount;
		unsigned long id;
		std::vector<BasicBlockWrapper*> exits;
		BasicBlockWrapper* entry;

	public:
		SumOfExitsPattern(std::vector<BasicBlockWrapper*> exits, BasicBlockWrapper* entry) :
			id(OptimizationPattern::getPatternCount()), exits(exits), entry(entry)
	{ OptimizationPattern::getPatternCount()++; } 

		virtual BasicBlock* getPatternExitBlock() override { return entry->getBB(); }

		virtual std::string toJson(unsigned depth = 0) override {
			auto tabs = PassUtilities::getTabs;
			std::stringstream ss;
			ss << tabs(depth) << "{\n";
			ss << tabs(depth+1) << "\"id\": " << id << ",\n";
			ss << tabs(depth+1) << "\"type\": \"sumOfExits\",\n";
			ss << tabs(depth+1) << "\"entry\": " << entry->getId() << ",\n";
			ss << tabs(depth+1) << "\"exitBlocks\": [";
			for(auto exitBlock : exits) {
				ss << exitBlock->getId() << "";
				if(exitBlock != exits.back()) { ss << ", "; }
			}
			ss << "]\n";
			ss << tabs(depth) << "}";
			return ss.str();
		}

		virtual std::vector<BasicBlockWrapper*> getNonInstrumentedBlocks() override { return {entry}; }

		static SumOfExitsPattern* checkForPattern(bbWrapperMap map, BasicBlockWrapper* start, std::vector<BasicBlock*>& containedBlocks, unsigned depth = 1) {
			if(!start->getBB()->isEntryBlock() || start->getBB()->getParent()->size() == 1) {
				return nullptr;
			}
			BasicBlockWrapper* entry = start;
			std::vector<BasicBlockWrapper*> exits;
			for(auto& bb : *entry->getBB()->getParent()) {
				if(succ_size(&bb) == 0) {
					exits.push_back(map[&bb]);
				}
			}
			return new SumOfExitsPattern(exits, entry);
		}
};

