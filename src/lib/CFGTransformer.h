#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "DAG.h"

using namespace llvm;

class CFGTransformer {
	public:

static void instrumentChords(DAG* dag, AllocaInst* pathCounterVar);
		static void transformToSingleExit(Function &F);
		static void instrumentEdge(BasicBlock* edge, AllocaInst* counter, int edgeIncrement);
		static BasicBlock* insertEdgeBlockBetween(BasicBlock* src, BasicBlock* dst);
		static void addInstrumentedEdges(DAG* dag, AllocaInst* pathCounterVar);
		static void addInstrumentedEdges(BasicBlock& start, DAG* dag, AllocaInst* pathCounterVar);
		static void insertPathCounterIncrement(BasicBlock& edge, int edgeValue, AllocaInst* pathCounterVar);
		static void resetCounterAlongBackedge(AllocaInst* counter, BasicBlock* backEdge);
		static void insertPrintOfCounter(Function &F, BasicBlock& exit, AllocaInst* counter);
	private:
		static void getReturnInstructions(Function &F, std::vector<ReturnInst*> &returnInstructions);
		static void redirectReturns(std::vector<ReturnInst*> &returnInstructions, PHINode* phiInstruction, bool funcReturnsVoid, BasicBlock* newExitBlock); 
		static void createNewReturnInstruction(BasicBlock* newExitBlock, bool funcReturnsVoid, PHINode* phiInstruction);
		static void pushIfInstructionIsReturn(Instruction* I, std::vector<ReturnInst*> &vec);
		static unsigned countSuccessors(BasicBlock& BB);
		static vector<BasicBlock*> copyBlockSuccessors(BasicBlock& BB) {
			std::vector<BasicBlock*> successorBlocks;
			for(BasicBlock* succ : successors(&BB)) {
				successorBlocks.push_back(succ);
			}
			return successorBlocks;
		}
		static void updatePHINodes(BasicBlock& BB, BasicBlock* succ, BasicBlock* instrumentedBasicBlock) {
			for (PHINode &Phi : succ->phis()) {
				Value* V = Phi.getIncomingValueForBlock(&BB);
				Phi.removeIncomingValue(&BB, false);
				Phi.addIncoming(V, instrumentedBasicBlock);
			}
		}
		static void redirectTerminatorOperands(BasicBlock& BB, BasicBlock* succ, BasicBlock* instrumentedBasicBlock) {
			Instruction* terminator = BB.getTerminator();
			if(terminator) {
				for (unsigned i = 0; i < terminator->getNumSuccessors(); ++i) {
					if (terminator->getSuccessor(i) == succ) {
						terminator->setSuccessor(i, instrumentedBasicBlock);
					}
				}
			}
		}
};
