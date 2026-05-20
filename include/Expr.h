#pragma once
#include <vector>
#include <string>
#include <memory>
#include "llvm/ADT/APInt.h"

namespace llvm {
  class raw_ostream;
}

namespace mini {
struct Expr;

struct Block {
  std::string name;
  unsigned Preds;
  unsigned Number;
  unsigned ConcretePred = -1;
  std::vector<Expr*> PredVars;
};

struct Expr {
  enum Op {
    Const, Var, Phi,
    Add, Sub, Mul, UDiv, SDiv, URem, SRem,
    And, Or, Xor,
    Shl, LShr, AShr,
    Eq, Ne, Ult, Slt, Ule, Sle,
    ZExt, SExt, Trunc,
    Select,
    Bswap, Cttz, Ctlz, Freeze
  } op;
  
  unsigned Width;
  int64_t val;
  std::string name;
  std::vector<Expr*> operands;
  Block* block;
  
  // Dataflow facts
  llvm::APInt KnownZeros;
  llvm::APInt KnownOnes;
  bool NonZero = false;
  bool NonNegative = false;
  
  Expr(Op o, unsigned w) : op(o), Width(w), val(0), KnownZeros(w, 0), KnownOnes(w, 0) {}
  Expr(int64_t v, unsigned w) : op(Const), Width(w), val(v), KnownZeros(w, 0), KnownOnes(w, 0) {}
  Expr(std::string n, unsigned w) : op(Var), Width(w), val(0), name(n), KnownZeros(w, 0), KnownOnes(w, 0) {}
};

struct InstMapping {
  Expr *LHS, *RHS;
  InstMapping(Expr *L, Expr *R) : LHS(L), RHS(R) {}
};

void printExpr(Expr* e, llvm::raw_ostream& os);
int cost(Expr* e);

} // namespace mini
