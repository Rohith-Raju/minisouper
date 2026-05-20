#include "../include/Expr.h"
#include <vector>
#include <set>

namespace mini {

class EnumerativeSynthesizer {
  std::vector<std::unique_ptr<Expr>> storage;
  
  Expr* makeConst(int64_t val, unsigned width) {
    storage.push_back(std::make_unique<Expr>(val, width));
    return storage.back().get();
  }
  
  Expr* make(Expr::Op op, unsigned width, const std::vector<Expr*>& ops) {
    storage.push_back(std::make_unique<Expr>(op, width));
    Expr* e = storage.back().get();
    e->operands = ops;
    return e;
  }

public:
  std::vector<Expr*> enumerate(Expr* root, int maxCost) {
    std::vector<Expr*> candidates;
    std::set<Expr*> seen;
    
    for (int64_t c : {0, 1, -1, 2, -2, 4, 8, 16}) {
      candidates.push_back(makeConst(c, root->Width));
    }
    for (auto* op : root->operands) {
      if (!seen.count(op)) {
        candidates.push_back(op);
        seen.insert(op);
      }
    }
    
    if (maxCost < 1) return candidates;
    
    for (auto* op : root->operands) {
      for (int64_t c : {0, 1, -1, 2, 4, 8}) {
        Expr* constExpr = makeConst(c, root->Width);
        candidates.push_back(make(Expr::Add, root->Width, {op, constExpr}));
        candidates.push_back(make(Expr::Sub, root->Width, {op, constExpr}));
        candidates.push_back(make(Expr::Mul, root->Width, {op, constExpr}));
        candidates.push_back(make(Expr::Shl, root->Width, {op, constExpr}));
        candidates.push_back(make(Expr::LShr, root->Width, {op, constExpr}));
        candidates.push_back(make(Expr::And, root->Width, {op, constExpr}));
        candidates.push_back(make(Expr::Or, root->Width, {op, constExpr}));
        candidates.push_back(make(Expr::Xor, root->Width, {op, constExpr}));
      }
      
      for (auto* op2 : root->operands) {
        if (op != op2) {
          candidates.push_back(make(Expr::Add, root->Width, {op, op2}));
          candidates.push_back(make(Expr::Sub, root->Width, {op, op2}));
          candidates.push_back(make(Expr::Mul, root->Width, {op, op2}));
          candidates.push_back(make(Expr::And, root->Width, {op, op2}));
          candidates.push_back(make(Expr::Or, root->Width, {op, op2}));
          candidates.push_back(make(Expr::Xor, root->Width, {op, op2}));
        }
      }
    }
    
    return candidates;
  }
};

} // namespace mini
