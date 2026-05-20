#include "../include/Z3Solver.h"
#include "z3++.h"
#include <map>

namespace mini {

Z3Solver::Z3Solver() {
  ctx = new z3::context();
  solver = new z3::solver(*static_cast<z3::context*>(ctx));
}

Z3Solver::~Z3Solver() {
  delete static_cast<z3::solver*>(solver);
  delete static_cast<z3::context*>(ctx);
}

bool Z3Solver::verify(Expr* lhs, Expr* rhs) {
  z3::context& c = *static_cast<z3::context*>(ctx);
  z3::solver& s = *static_cast<z3::solver*>(solver);
  
  s.reset();
  
  // Skip if widths don't match
  if (lhs->Width != rhs->Width) {
    return false;
  }
  
  std::map<std::string, z3::expr> varCache;
  z3::expr lhsZ3 = toZ3Expr(lhs, c, varCache);
  z3::expr rhsZ3 = toZ3Expr(rhs, c, varCache);
  
  // Check if LHS != RHS is UNSAT (meaning LHS == RHS always)
  s.add(lhsZ3 != rhsZ3);
  bool equivalent = (s.check() == z3::unsat);
  
  return equivalent;
}

bool Z3Solver::verifyWithPCs(Expr* lhs, Expr* rhs, const std::vector<InstMapping>& PCs) {
  z3::context& c = *static_cast<z3::context*>(ctx);
  z3::solver& s = *static_cast<z3::solver*>(solver);
  
  s.reset();
  
  std::map<std::string, z3::expr> varCache;
  
  // Add path conditions
  for (const auto& pc : PCs) {
    z3::expr pcLHS = toZ3Expr(pc.LHS, c, varCache);
    z3::expr pcRHS = toZ3Expr(pc.RHS, c, varCache);
    s.add(pcLHS == pcRHS);
  }
  
  z3::expr lhsZ3 = toZ3Expr(lhs, c, varCache);
  z3::expr rhsZ3 = toZ3Expr(rhs, c, varCache);
  
  // Check if (PCs) && (LHS != RHS) is UNSAT
  // This means: under PCs, LHS == RHS always holds
  s.add(lhsZ3 != rhsZ3);
  bool equivalent = (s.check() == z3::unsat);
  
  return equivalent;
}

z3::expr Z3Solver::toZ3Expr(Expr* e, z3::context& c, std::map<std::string, z3::expr>& varCache) {
  if (!e) {
    return c.bv_val(0, 32);
  }
  
  if (e->op == Expr::Const) {
    return c.bv_val(e->val, e->Width);
  }
  
  if (e->op == Expr::Var) {
    auto it = varCache.find(e->name);
    if (it == varCache.end()) {
      z3::expr var = c.bv_const(e->name.c_str(), e->Width);
      
      // Add dataflow constraints
      z3::solver& s = *static_cast<z3::solver*>(solver);
      
      // KnownZeros constraint: (var & KnownZeros) == 0
      if (!e->KnownZeros.isZero()) {
        s.add((var & c.bv_val(e->KnownZeros.getZExtValue(), e->Width)) == c.bv_val(0, e->Width));
      }
      
      // KnownOnes constraint: (var & KnownOnes) == KnownOnes
      if (!e->KnownOnes.isZero()) {
        uint64_t onesVal = e->KnownOnes.getZExtValue();
        s.add((var & c.bv_val(onesVal, e->Width)) == c.bv_val(onesVal, e->Width));
      }
      
      // NonZero constraint
      if (e->NonZero) {
        s.add(var != c.bv_val(0, e->Width));
      }
      
      // NonNegative constraint (MSB == 0)
      if (e->NonNegative) {
        z3::expr msb = z3::lshr(var, c.bv_val(e->Width - 1, e->Width));
        s.add(msb == c.bv_val(0, e->Width));
      }
      
      varCache.insert({e->name, var});
      return var;
    }
    return it->second;
  }
  
  if (e->op == Expr::Select && e->operands.size() == 3) {
    z3::expr cond = toZ3Expr(e->operands[0], c, varCache);
    z3::expr tval = toZ3Expr(e->operands[1], c, varCache);
    z3::expr fval = toZ3Expr(e->operands[2], c, varCache);
    // Condition is already a 1-bit value from comparison
    z3::expr condBool = (cond == c.bv_val(1, 1));
    return z3::ite(condBool, tval, fval);
  }
  
  if (e->op == Expr::ZExt && e->operands.size() == 1) {
    z3::expr op = toZ3Expr(e->operands[0], c, varCache);
    return z3::zext(op, e->Width - e->operands[0]->Width);
  }
  
  if (e->op == Expr::SExt && e->operands.size() == 1) {
    z3::expr op = toZ3Expr(e->operands[0], c, varCache);
    return z3::sext(op, e->Width - e->operands[0]->Width);
  }
  
  if (e->op == Expr::Trunc && e->operands.size() == 1) {
    z3::expr op = toZ3Expr(e->operands[0], c, varCache);
    return op.extract(e->Width - 1, 0);
  }
  
  if (e->op == Expr::Phi && e->operands.size() >= 1) {
    // PHI: all operands must have same width as result
    // If widths don't match, extend/truncate to match PHI width
    z3::expr result = toZ3Expr(e->operands[0], c, varCache);
    
    // Extend or truncate first operand to match PHI width if needed
    if (e->operands[0]->Width < e->Width) {
      result = z3::zext(result, e->Width - e->operands[0]->Width);
    } else if (e->operands[0]->Width > e->Width) {
      result = result.extract(e->Width - 1, 0);
    }
    
    // For simplicity, just return first operand
    // Proper handling would need block predicates
    return result;
  }
  
  if (e->operands.size() < 2) {
    return c.bv_const("unknown", e->Width);
  }
  
  z3::expr lhs = toZ3Expr(e->operands[0], c, varCache);
  z3::expr rhs = toZ3Expr(e->operands[1], c, varCache);
  
  switch (e->op) {
    case Expr::Add:
      return lhs + rhs;
    case Expr::Sub:
      return lhs - rhs;
    case Expr::Mul:
      return lhs * rhs;
    case Expr::UDiv:
      return z3::udiv(lhs, rhs);
    case Expr::SDiv:
      return lhs / rhs;
    case Expr::URem:
      return z3::urem(lhs, rhs);
    case Expr::SRem:
      return z3::srem(lhs, rhs);
    case Expr::And:
      return lhs & rhs;
    case Expr::Or:
      return lhs | rhs;
    case Expr::Xor:
      return lhs ^ rhs;
    case Expr::Shl:
      return z3::shl(lhs, rhs);
    case Expr::LShr:
      return z3::lshr(lhs, rhs);
    case Expr::AShr:
      return z3::ashr(lhs, rhs);
    case Expr::Eq:
      return z3::ite(lhs == rhs, c.bv_val(1, 1), c.bv_val(0, 1));
    case Expr::Ne:
      return z3::ite(lhs != rhs, c.bv_val(1, 1), c.bv_val(0, 1));
    case Expr::Ult:
      return z3::ite(z3::ult(lhs, rhs), c.bv_val(1, 1), c.bv_val(0, 1));
    case Expr::Slt:
      return z3::ite(lhs < rhs, c.bv_val(1, 1), c.bv_val(0, 1));
    case Expr::Ule:
      return z3::ite(z3::ule(lhs, rhs), c.bv_val(1, 1), c.bv_val(0, 1));
    case Expr::Sle:
      return z3::ite(lhs <= rhs, c.bv_val(1, 1), c.bv_val(0, 1));
    default:
      return c.bv_const("unknown", e->Width);
  }
}

void* Z3Solver::toZ3(Expr* e) {
  z3::context& c = *static_cast<z3::context*>(ctx);
  std::map<std::string, z3::expr> varCache;
  return new z3::expr(toZ3Expr(e, c, varCache));
}

std::vector<Expr*> Z3Solver::synthesize(Expr* original, int maxCost) {
  std::vector<Expr*> results;
  // Synthesis is done by Synthesizer class, this is just for verification
  return results;
}

} // namespace mini
