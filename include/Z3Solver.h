#pragma once
#include "Expr.h"
#include "z3++.h"
#include <vector>
#include <map>

namespace mini {

class Z3Solver {
public:
  Z3Solver();
  ~Z3Solver();
  
  // Verify if lhs and rhs are equivalent (LHS != RHS is UNSAT)
  bool verify(Expr* lhs, Expr* rhs);
  
  // Verify with path conditions: (PCs) => (LHS == RHS)
  bool verifyWithPCs(Expr* lhs, Expr* rhs, const std::vector<InstMapping>& PCs);
  
  // Find simpler equivalent expressions using Z3
  std::vector<Expr*> synthesize(Expr* original, int maxCost = 3);
  
private:
  void* ctx;  // z3::context*
  void* solver; // z3::solver*
  
  z3::expr toZ3Expr(Expr* e, z3::context& c, std::map<std::string, z3::expr>& varCache);
  void* toZ3(Expr* e);  // Convert Expr to z3::expr
  Expr* fromZ3(void* z3expr);  // Convert z3::expr to Expr
};

} // namespace mini
