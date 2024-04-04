#include "OptimizationPattern.h"
#include <llvm/IR/CFG.h>
#include <sstream>

class DiamondPattern : public OptimizationPattern {
	private:
		static unsigned long patternCount;
		unsigned long id;
		std::vector<BasicBlockWrapper*> branchBlocks;
		BasicBlockWrapper* condBlock = nullptr;
		BasicBlockWrapper* joinBlock = nullptr;

	public:
		DiamondPattern(std::vector<BasicBlockWrapper*> branchBlocks, BasicBlockWrapper* condBlock, BasicBlockWrapper* joinBlock) :
			id(OptimizationPattern::getPatternCount()), branchBlocks(branchBlocks), condBlock(condBlock), joinBlock(joinBlock) 
	{ OptimizationPattern::getPatternCount()++; }

		virtual BasicBlock* getPatternExitBlock() override { return joinBlock->getBB(); }

		virtual std::string toJson(unsigned depth = 0) override {
			auto tabs = PassUtilities::getTabs;
			std::stringstream ss;
			ss << tabs(depth) << "{\n";
			ss << tabs(depth+1) << "\"id\": " << id << ",\n";
			ss << tabs(depth+1) << "\"type\": \"diamond\",\n";
			ss << tabs(depth+1) << "\"condBlock\": " << condBlock->getId() << ",\n";
			ss << tabs(depth+1) << "\"joinBlock\": " << joinBlock->getId() << ",\n";
			ss << tabs(depth+1) << "\"branchBlocks\": [\n";
			for(auto branchBlock : branchBlocks) {
				ss << tabs(depth+2) << "" << branchBlock->getId() << "";
				if(branchBlock != branchBlocks.back()) { ss << ",\n"; }
			}
			ss << "\n" << tabs(depth+1) << "]\n";
			ss << tabs(depth) << "}";
			return ss.str();
		}

		virtual std::vector<BasicBlockWrapper*> getNonInstrumentedBlocks() override { return {condBlock, joinBlock}; }

		static DiamondPattern* checkForPattern(bbWrapperMap map, BasicBlockWrapper* start, std::vector<BasicBlock*>& containedBlocks, unsigned depth = 1) {
			BasicBlock* startBlock = start->getBB();
			if(startBlock->getTerminator()->getNumSuccessors() <= 1) { return nullptr; }
			if(startBlock->getTerminator()->getSuccessor(0)->getTerminator()->getNumSuccessors() != 1) { return nullptr; }
			if(startBlock->getTerminator()->getSuccessor(1)->getTerminator()->getNumSuccessors() != 1) { return nullptr; }
			if(pred_size(startBlock->getTerminator()->getSuccessor(0)) != 1) { return nullptr; }
			if(pred_size(startBlock->getTerminator()->getSuccessor(1)) != 1) { return nullptr; }
			SmallVector<BasicBlock*> startSuccessors;
			BasicBlock* potentialJoinBlock = startBlock->getTerminator()->getSuccessor(0)->getTerminator()->getSuccessor(0);
			bool allJoin = true;
			for(auto succ : successors(startBlock)) {
				if(!(succ_size(succ) == 1 && succ->getUniqueSuccessor() == potentialJoinBlock)) {
					allJoin = false;
					break;
				}; 
			}
			BasicBlock* joinBlock = nullptr;
			BasicBlock* condBlock = nullptr;
			std::vector<BasicBlockWrapper*> branchBlocks;
			if(allJoin) {
				if(pred_size(potentialJoinBlock) != 2) { return nullptr; }
				joinBlock = potentialJoinBlock;
				condBlock = startBlock;
				for(auto succ : successors(startBlock)) {
					branchBlocks.push_back(map[succ]);
					containedBlocks.push_back(succ);
				}
				containedBlocks.push_back(joinBlock);
				containedBlocks.push_back(condBlock);
				return new DiamondPattern(branchBlocks, map[condBlock], map[joinBlock]);
			}
			return nullptr;
		}
};
