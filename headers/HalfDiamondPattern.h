#include "../headers/OptimizationPattern.h"
#include <llvm/IR/CFG.h>
#include <sstream>

class HalfDiamondPattern : public OptimizationPattern {
	private:
		static unsigned long patternCount;
		unsigned long id;
		BasicBlockWrapper* condBlock = nullptr;
		BasicBlockWrapper* branchBlock =	nullptr;
		BasicBlockWrapper* joinBlock = nullptr;

	public:
		HalfDiamondPattern(BasicBlockWrapper* condBlock, BasicBlockWrapper* branchBlock, BasicBlockWrapper* joinBlock) :
			id(OptimizationPattern::getPatternCount()), condBlock(condBlock), branchBlock(branchBlock), joinBlock(joinBlock) { OptimizationPattern::getPatternCount()++; }

		virtual BasicBlock* getPatternExitBlock() override { return joinBlock->getBB(); }

		virtual std::string toJson(unsigned depth = 0) override {
			auto tabs = PassUtilities::getTabs;
			std::stringstream ss;
			ss << tabs(depth) << "{\n";
			ss << tabs(depth+1) << "\"id\": " << id << ",\n";
			ss << tabs(depth+1) << "\"type\": \"halfDiamond\",\n";
			ss << tabs(depth+1) << "\"condBlock\": " << condBlock->getId() << ",\n";
			ss << tabs(depth+1) << "\"joinBlock\": " << joinBlock->getId() << ",\n";
			ss << tabs(depth+1) << "\"branchBlocks\": [\n";
			ss << tabs(depth+2) << "" << branchBlock->getId() << "";
			ss << "\n" << tabs(depth+1) << "]\n";
			ss << tabs(depth) << "}";
			return ss.str();
		}

		virtual std::vector<BasicBlockWrapper*> getNonInstrumentedBlocks() override { return {condBlock}; }

		static HalfDiamondPattern* checkForPattern(bbWrapperMap map, BasicBlockWrapper* start, std::vector<BasicBlock*>& containedBlocks, unsigned depth = 1) {
			BasicBlock* startBlock = start->getBB();
			if(startBlock->getTerminator()->getNumSuccessors() != 2) { return nullptr; }
			BasicBlock* succ1 = startBlock->getTerminator()->getSuccessor(0);
			BasicBlock* succ2 = startBlock->getTerminator()->getSuccessor(1);
			BasicBlock* potentialJoinBlock = succ1;
			BasicBlock* potentialBranchBlock = succ2;
			if(potentialBranchBlock->getTerminator()->getNumSuccessors() != 1) {
				goto second;
			}
			if(potentialBranchBlock->getTerminator()->getSuccessor(0) == potentialJoinBlock) {
				if(pred_size(potentialJoinBlock) != 2) {
					goto second;
				}
				if(potentialBranchBlock->getTerminator()->getNumSuccessors() != 1) {
					goto second;
				}
				if(pred_size(potentialBranchBlock) != 1) {
					goto second;
				}
				containedBlocks.push_back(startBlock);
				containedBlocks.push_back(succ2);
				containedBlocks.push_back(potentialJoinBlock);
				return new HalfDiamondPattern(map[startBlock], map[potentialBranchBlock], map[potentialJoinBlock]);
			}
second:
			potentialJoinBlock = succ2;
			potentialBranchBlock = succ1;
			if(potentialBranchBlock->getTerminator()->getNumSuccessors() != 1) {
				return nullptr;
			}
			if(succ1->getTerminator()->getSuccessor(0) == potentialJoinBlock) {
				if(pred_size(potentialJoinBlock) != 2) {
					return nullptr; 
				}
				if(potentialBranchBlock->getTerminator()->getNumSuccessors() != 1) {
					return nullptr;
				}
				if(pred_size(potentialBranchBlock) != 1) {
					return nullptr;
				}
				containedBlocks.push_back(startBlock);
				containedBlocks.push_back(succ1);
				containedBlocks.push_back(potentialJoinBlock);
				return new HalfDiamondPattern(map[startBlock], map[potentialBranchBlock], map[potentialJoinBlock]);
			}
			return nullptr;
		}
};


