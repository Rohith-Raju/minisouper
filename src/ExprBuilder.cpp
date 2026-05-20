#include "../include/Expr.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Support/KnownBits.h"
#include <map>

namespace mini {

class ExprBuilder {
  std::map<llvm::Value*, Expr*> cache;
  std::vector<std::unique_ptr<Expr>> storage;
  std::vector<std::unique_ptr<Block>> blocks;
  const llvm::DataLayout& DL;
  llvm::LoopInfo* LI;
  std::vector<InstMapping> PCs;
  
  Expr* make(Expr::Op op, unsigned width) {
    storage.push_back(std::make_unique<Expr>(op, width));
    return storage.back().get();
  }
  
  Expr* makeConst(int64_t val, unsigned width) {
    storage.push_back(std::make_unique<Expr>(val, width));
    return storage.back().get();
  }
  
  Expr* makeVar(std::string name, unsigned width) {
    storage.push_back(std::make_unique<Expr>(name, width));
    return storage.back().get();
  }
  
  Block* createBlock(unsigned preds) {
    blocks.push_back(std::make_unique<Block>());
    Block* b = blocks.back().get();
    b->Preds = preds;
    b->Number = blocks.size() - 1;
    return b;
  }

public:
  ExprBuilder(const llvm::DataLayout& dl, llvm::LoopInfo* li = nullptr) 
    : DL(dl), LI(li) {}
  
  Expr* convert(llvm::Value* V) {
    if (cache.count(V))
      return cache[V];
    
    unsigned width = V->getType()->isIntegerTy() ? 
                     V->getType()->getIntegerBitWidth() : 64;
    
    // Handle constants
    if (auto* CI = llvm::dyn_cast<llvm::ConstantInt>(V)) {
      Expr* e = makeConst(CI->getSExtValue(), width);
      cache[V] = e;
      return e;
    }
    
    // Handle PHI nodes
    if (auto* Phi = llvm::dyn_cast<llvm::PHINode>(V)) {
      // Skip loop headers
      if (LI && LI->isLoopHeader(Phi->getParent())) {
        Expr* e = makeVar(V->hasName() ? V->getName().str() : "phi_loop", width);
        cache[V] = e;
        return e;
      }
      
      Block* B = createBlock(Phi->getNumIncomingValues());
      Expr* e = make(Expr::Phi, width);
      e->block = B;
      
      for (unsigned i = 0; i < Phi->getNumIncomingValues(); i++) {
        e->operands.push_back(convert(Phi->getIncomingValue(i)));
      }
      
      cache[V] = e;
      return e;
    }
    
    // Handle casts
    if (auto* Cast = llvm::dyn_cast<llvm::CastInst>(V)) {
      Expr* op = convert(Cast->getOperand(0));
      Expr* e = nullptr;
      
      switch (Cast->getOpcode()) {
        case llvm::Instruction::ZExt:
          e = make(Expr::ZExt, width);
          break;
        case llvm::Instruction::SExt:
          e = make(Expr::SExt, width);
          break;
        case llvm::Instruction::Trunc:
          e = make(Expr::Trunc, width);
          break;
        default:
          e = makeVar("cast", width);
          cache[V] = e;
          return e;
      }
      
      e->operands = {op};
      cache[V] = e;
      return e;
    }
    
    // Handle select
    if (auto* Sel = llvm::dyn_cast<llvm::SelectInst>(V)) {
      Expr* cond = convert(Sel->getCondition());
      Expr* tval = convert(Sel->getTrueValue());
      Expr* fval = convert(Sel->getFalseValue());
      
      Expr* e = make(Expr::Select, width);
      e->operands = {cond, tval, fval};
      cache[V] = e;
      return e;
    }
    
    // Handle comparisons
    if (auto* Cmp = llvm::dyn_cast<llvm::ICmpInst>(V)) {
      Expr* lhs = convert(Cmp->getOperand(0));
      Expr* rhs = convert(Cmp->getOperand(1));
      
      Expr* e = nullptr;
      switch (Cmp->getPredicate()) {
        case llvm::CmpInst::ICMP_EQ:
          e = make(Expr::Eq, 1);
          e->operands = {lhs, rhs};
          break;
        case llvm::CmpInst::ICMP_NE:
          e = make(Expr::Ne, 1);
          e->operands = {lhs, rhs};
          break;
        case llvm::CmpInst::ICMP_ULT:
          e = make(Expr::Ult, 1);
          e->operands = {lhs, rhs};
          break;
        case llvm::CmpInst::ICMP_UGT:
          e = make(Expr::Ult, 1);  // a > b  =>  b < a
          e->operands = {rhs, lhs};
          break;
        case llvm::CmpInst::ICMP_SLT:
          e = make(Expr::Slt, 1);
          e->operands = {lhs, rhs};
          break;
        case llvm::CmpInst::ICMP_SGT:
          e = make(Expr::Slt, 1);  // a > b  =>  b < a
          e->operands = {rhs, lhs};
          break;
        case llvm::CmpInst::ICMP_ULE:
          e = make(Expr::Ule, 1);
          e->operands = {lhs, rhs};
          break;
        case llvm::CmpInst::ICMP_SLE:
          e = make(Expr::Sle, 1);
          e->operands = {lhs, rhs};
          break;
        default:
          e = makeVar("cmp", 1);
          cache[V] = e;
          return e;
      }
      
      cache[V] = e;
      return e;
    }
    
    // Handle binary operations
    if (auto* BO = llvm::dyn_cast<llvm::BinaryOperator>(V)) {
      Expr* lhs = convert(BO->getOperand(0));
      Expr* rhs = convert(BO->getOperand(1));
      
      Expr* e = nullptr;
      switch (BO->getOpcode()) {
        case llvm::Instruction::Add:
          e = make(Expr::Add, width);
          break;
        case llvm::Instruction::Sub:
          e = make(Expr::Sub, width);
          break;
        case llvm::Instruction::Mul:
          e = make(Expr::Mul, width);
          break;
        case llvm::Instruction::UDiv:
          e = make(Expr::UDiv, width);
          break;
        case llvm::Instruction::SDiv:
          e = make(Expr::SDiv, width);
          break;
        case llvm::Instruction::URem:
          e = make(Expr::URem, width);
          break;
        case llvm::Instruction::SRem:
          e = make(Expr::SRem, width);
          break;
        case llvm::Instruction::And:
          e = make(Expr::And, width);
          break;
        case llvm::Instruction::Or:
          e = make(Expr::Or, width);
          break;
        case llvm::Instruction::Xor:
          e = make(Expr::Xor, width);
          break;
        case llvm::Instruction::Shl:
          e = make(Expr::Shl, width);
          break;
        case llvm::Instruction::LShr:
          e = make(Expr::LShr, width);
          break;
        case llvm::Instruction::AShr:
          e = make(Expr::AShr, width);
          break;
        default:
          e = makeVar("binop", width);
          cache[V] = e;
          return e;
      }
      
      e->operands = {lhs, rhs};
      cache[V] = e;
      return e;
    }
    
    // Create variable and harvest dataflow facts (only for non-i1 types)
    std::string name = V->hasName() ? V->getName().str() : "tmp";
    Expr* e = makeVar(name, width);
    
    // Only harvest dataflow facts for non-boolean types
    if (width > 1) {
      // Harvest KnownBits
      llvm::KnownBits Known(width);
      llvm::computeKnownBits(V, Known, DL);
      e->KnownZeros = Known.Zero;
      e->KnownOnes = Known.One;
      
      // Harvest NonZero
      e->NonZero = llvm::isKnownNonZero(V, DL);
      
      // Harvest NonNegative
      e->NonNegative = llvm::isKnownNonNegative(V, DL);
    }
    
    cache[V] = e;
    return e;
  }
  
  void addPathCondition(llvm::BasicBlock* BB) {
    if (auto* Pred = BB->getSinglePredecessor()) {
      if (auto* Branch = llvm::dyn_cast<llvm::BranchInst>(Pred->getTerminator())) {
        if (Branch->isConditional()) {
          Expr* Cond = convert(Branch->getCondition());
          bool TakenBranch = (Branch->getSuccessor(0) == BB);
          Expr* ConstVal = makeConst(TakenBranch ? 1 : 0, 1);
          PCs.push_back(InstMapping(Cond, ConstVal));
        }
      }
    }
  }
  
  const std::vector<InstMapping>& getPathConditions() const {
    return PCs;
  }
};

} // namespace mini
