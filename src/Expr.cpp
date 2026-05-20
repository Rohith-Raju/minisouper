#include "../include/Expr.h"
#include "llvm/Support/raw_ostream.h"
#include <set>

namespace mini {

void printExpr(Expr* e, llvm::raw_ostream& os) {
  if (!e) return;
  
  if (e->op == Expr::Const) {
    os << e->val;
  } else if (e->op == Expr::Var) {
    os << e->name;
  } else if (e->op == Expr::Phi) {
    os << "phi(";
    for (size_t i = 0; i < e->operands.size(); i++) {
      if (i > 0) os << ", ";
      printExpr(e->operands[i], os);
    }
    os << ")";
  } else if (e->op == Expr::Select) {
    os << "select(";
    printExpr(e->operands[0], os);
    os << ", ";
    printExpr(e->operands[1], os);
    os << ", ";
    printExpr(e->operands[2], os);
    os << ")";
  } else if (e->op == Expr::ZExt || e->op == Expr::SExt || e->op == Expr::Trunc) {
    const char* name = e->op == Expr::ZExt ? "zext" : e->op == Expr::SExt ? "sext" : "trunc";
    os << name << "(";
    printExpr(e->operands[0], os);
    os << ")";
  } else {
    os << "(";
    printExpr(e->operands[0], os);
    
    switch (e->op) {
      case Expr::Add: os << " + "; break;
      case Expr::Sub: os << " - "; break;
      case Expr::Mul: os << " * "; break;
      case Expr::UDiv: os << " / "; break;
      case Expr::SDiv: os << " s/ "; break;
      case Expr::URem: os << " % "; break;
      case Expr::SRem: os << " s% "; break;
      case Expr::And: os << " & "; break;
      case Expr::Or: os << " | "; break;
      case Expr::Xor: os << " ^ "; break;
      case Expr::Shl: os << " << "; break;
      case Expr::LShr: os << " >> "; break;
      case Expr::AShr: os << " s>> "; break;
      case Expr::Eq: os << " == "; break;
      case Expr::Ne: os << " != "; break;
      case Expr::Ult: os << " < "; break;
      case Expr::Slt: os << " s< "; break;
      case Expr::Ule: os << " <= "; break;
      case Expr::Sle: os << " s<= "; break;
      default: os << " ? "; break;
    }
    
    if (e->operands.size() > 1)
      printExpr(e->operands[1], os);
    os << ")";
  }
}

int costHelper(Expr* e, std::set<Expr*>& visited) {
  if (!e || visited.count(e)) return 0;
  visited.insert(e);
  
  int c = 0;
  if (e->op == Expr::Const || e->op == Expr::Var) {
    c = 0;
  } else if (e->op == Expr::Mul || e->op == Expr::UDiv || e->op == Expr::SDiv) {
    c = 4;
  } else if (e->op == Expr::URem || e->op == Expr::SRem) {
    c = 4;
  } else {
    c = 1;
  }
  
  for (auto* op : e->operands)
    c += costHelper(op, visited);
  
  return c;
}

int cost(Expr* e) {
  std::set<Expr*> visited;
  return costHelper(e, visited);
}

} // namespace mini
