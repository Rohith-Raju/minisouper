#include "../include/Expr.h"
#include "../include/Z3Solver.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/Dominators.h"

#include "ExprBuilder.cpp"
#include "EnumerativeSynthesizer.cpp"

using namespace llvm;
using namespace mini;

void printOptimization(Expr* LHS, Expr* RHS, const std::vector<InstMapping>& PCs) {
  outs() << "  Optimization found:\n";
  outs() << "    Original:  ";
  printExpr(LHS, outs());
  outs() << " (cost: " << cost(LHS) << ")\n";
  outs() << "    Optimized: ";
  printExpr(RHS, outs());
  outs() << " (cost: " << cost(RHS) << ")\n";
  
  if (!PCs.empty()) {
    outs() << "    Under conditions:\n";
    for (const auto& PC : PCs) {
      outs() << "      ";
      printExpr(PC.LHS, outs());
      outs() << " == ";
      printExpr(PC.RHS, outs());
      outs() << "\n";
    }
  }
  outs() << "\n";
}

int main(int argc, char** argv) {
  if (argc < 2) {
    errs() << "Usage: " << argv[0] << " <input.ll>\n";
    return 1;
  }
  
  LLVMContext Context;
  SMDiagnostic Err;

  std::unique_ptr<Module> M = parseIRFile(argv[1], Err, Context);
  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }
  
  outs() << "=== Mini-Souper: ENUMERATIVE Synthesis ===\n";
  outs() << "Using enumerative synthesis like real Souper!\n\n";
  
  int total_optimizations = 0;
  Z3Solver solver;
  EnumerativeSynthesizer synth;
  
  for (Function& F : *M) {
    if (F.isDeclaration()) continue;
    
    outs() << "Function: " << F.getName() << "\n";
    
    DominatorTree DT(F);
    LoopInfo LI(DT);
    
    const DataLayout& DL = M->getDataLayout();
    ExprBuilder builder(DL, &LI);
    int func_opts = 0;
    
    for (BasicBlock& BB : F) {
      builder.addPathCondition(&BB);
      
      for (Instruction& I : BB) {
        if (!I.getType()->isIntegerTy()) continue;
        if (I.use_empty()) continue;
        
        Expr* e = builder.convert(&I);
        
        if (e->op == Expr::Const || e->op == Expr::Var)
          continue;
        
        // ENUMERATIVE: Try ALL possible expressions
        auto candidates = synth.enumerate(e, 1);
        
        outs() << "  Trying " << candidates.size() << " candidates for: ";
        printExpr(e, outs());
        outs() << "\n";
        
        for (auto* cand : candidates) {
          if (cost(cand) >= cost(e))
            continue;
          
          bool valid = false;
          const auto& PCs = builder.getPathConditions();
          
          if (PCs.empty()) {
            valid = solver.verify(e, cand);
          } else {
            valid = solver.verifyWithPCs(e, cand, PCs);
          }
          
          if (valid) {
            func_opts++;
            printOptimization(e, cand, PCs);
            break;
          }
        }
      }
    }
    
    outs() << "  Optimizations in this function: " << func_opts << "\n";
    outs() << "---\n\n";
    total_optimizations += func_opts;
  }
  
  outs() << "\n=== Summary ===\n";
  outs() << "Total optimizations found: " << total_optimizations << "\n";
  return 0;
}
