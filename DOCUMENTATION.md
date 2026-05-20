# Mini-Souper: Educational Superoptimizer Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Core Components](#core-components)
4. [How It Works](#how-it-works)
5. [Optimization Pipeline](#optimization-pipeline)
6. [Key Algorithms](#key-algorithms)
7. [Building and Running](#building-and-running)
8. [Examples](#examples)

---

## Overview

Mini-Souper is a simplified, educational implementation of a superoptimizer for LLVM IR. It demonstrates the core concepts of program synthesis and superoptimization in a compact, understandable codebase (~1000 lines).

### What is Superoptimization?

Superoptimization is the process of finding the optimal (shortest/fastest) equivalent program for a given input program. Unlike traditional compilers that apply predefined optimization patterns, superoptimizers:
1. **Generate** candidate optimized programs
2. **Verify** equivalence using formal methods (SMT solvers)
3. **Select** the best candidate based on a cost model

### Mini-Souper vs Full Souper

| Feature | Mini-Souper | Full Souper |
|---------|-------------|-------------|
| Lines of Code | ~1000 | ~50,000+ |
| Purpose | Educational | Production |
| Synthesis | Enumerative | Enumerative + CEGIS |
| Dataflow | Basic | Advanced |
| Path Conditions | Basic | Full SSA |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        Mini-Souper                          │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐      ┌──────────────┐                   │
│  │  LLVM IR     │─────▶│ ExprBuilder  │                   │
│  │  Input       │      │              │                   │
│  └──────────────┘      └──────┬───────┘                   │
│                               │                            │
│                               ▼                            │
│                        ┌──────────────┐                    │
│                        │  Expr Tree   │                    │
│                        │  (Internal   │                    │
│                        │   IR)        │                    │
│                        └──────┬───────┘                    │
│                               │                            │
│                               ▼                            │
│                   ┌───────────────────────┐                │
│                   │ Enumerative           │                │
│                   │ Synthesizer           │                │
│                   │ (Generate Candidates) │                │
│                   └───────────┬───────────┘                │
│                               │                            │
│                               ▼                            │
│                        ┌──────────────┐                    │
│                        │  Candidates  │                    │
│                        │  [e1,e2,...] │                    │
│                        └──────┬───────┘                    │
│                               │                            │
│                               ▼                            │
│                        ┌──────────────┐                    │
│                        │  Z3 Solver   │                    │
│                        │  (Verify)    │                    │
│                        └──────┬───────┘                    │
│                               │                            │
│                               ▼                            │
│                        ┌──────────────┐                    │
│                        │ Optimizations│                    │
│                        │   Found!     │                    │
│                        └──────────────┘                    │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

---

## Core Components

### 1. Expression Representation (`Expr.h`, `Expr.cpp`)

The `Expr` structure is mini-souper's internal representation of computations.

```cpp
struct Expr {
  enum Op {
    // Values
    Const, Var, Phi,
    // Arithmetic
    Add, Sub, Mul, UDiv, SDiv, URem, SRem,
    // Bitwise
    And, Or, Xor, Shl, LShr, AShr,
    // Comparisons
    Eq, Ne, Ult, Slt, Ule, Sle,
    // Conversions
    ZExt, SExt, Trunc,
    // Control
    Select
  } op;
  
  unsigned Width;              // Bit width (e.g., 32 for i32)
  int64_t val;                 // For constants
  std::string name;            // For variables
  std::vector<Expr*> operands; // Child expressions
  
  // Dataflow facts (harvested from LLVM)
  llvm::APInt KnownZeros;      // Bits known to be 0
  llvm::APInt KnownOnes;       // Bits known to be 1
  bool NonZero;                // Value cannot be 0
  bool NonNegative;            // Value >= 0
};
```

**Key Features:**
- **Tree structure**: Each expression is a node with operands as children
- **Typed**: Each expression has a bit width
- **Dataflow annotations**: Carries facts about value ranges and bit patterns

**Example:**
```
LLVM IR:  %3 = add i32 %1, %2
Expr:     Add(Var("1", 32), Var("2", 32))
```

### 2. Expression Builder (`ExprBuilder.cpp`)

Converts LLVM IR instructions into mini-souper's `Expr` representation.

**Key Responsibilities:**
1. **Translation**: LLVM Instruction → Expr tree
2. **Dataflow Harvesting**: Extract facts using LLVM's ValueTracking
3. **Path Condition Tracking**: Record branch conditions

**Translation Examples:**

| LLVM IR | Expr Tree |
|---------|-----------|
| `%3 = add i32 %1, %2` | `Add(Var("1"), Var("2"))` |
| `%4 = mul i32 %3, 4` | `Mul(Add(...), Const(4))` |
| `%5 = icmp eq i32 %3, 0` | `Eq(Add(...), Const(0))` |
| `%6 = select i1 %5, i32 %1, i32 %2` | `Select(Eq(...), Var("1"), Var("2"))` |

**Dataflow Harvesting:**
```cpp
// For variable %x, harvest facts:
llvm::KnownBits Known = computeKnownBits(%x);
expr->KnownZeros = Known.Zero;  // e.g., 0x00FF (low byte unknown)
expr->KnownOnes = Known.One;    // e.g., 0x0100 (bit 8 is 1)
expr->NonZero = isKnownNonZero(%x);
expr->NonNegative = isKnownNonNegative(%x);
```

**Path Conditions:**
```cpp
// If we have: if (x > 0) { y = x + 1; }
// When processing the then-block:
addPathCondition(BB);
// Records: (x > 0) == true
```

### 3. Enumerative Synthesizer (`EnumerativeSynthesizer.cpp`)

Generates candidate optimized expressions using enumerative synthesis.

**Algorithm:**
```
enumerate(expr, maxCost):
  candidates = []
  
  // 1. Try constants
  for c in {0, 1, -1, 2, 4, 8, 16}:
    candidates.add(Const(c))
  
  // 2. Try operands directly
  for op in expr.operands:
    candidates.add(op)
  
  // 3. Try simple transformations
  for op in expr.operands:
    for c in {0, 1, -1, 2, 4, 8}:
      candidates.add(op + c)
      candidates.add(op - c)
      candidates.add(op * c)
      candidates.add(op << c)
      candidates.add(op & c)
      // ... etc
  
  // 4. Try combining operands
  for op1 in expr.operands:
    for op2 in expr.operands:
      if op1 != op2:
        candidates.add(op1 + op2)
        candidates.add(op1 - op2)
        // ... etc
  
  return candidates
```

**Example:**
```
Input:  (x + 1) - 1
Candidates generated:
  - 0, 1, -1, 2, ...     (constants)
  - x                    (operand)
  - x + 0, x + 1, ...    (operand + const)
  - x - 0, x - 1, ...    (operand - const)
  - ...
```

### 4. Z3 Solver (`Z3Solver.cpp`)

Verifies equivalence between original and candidate expressions using the Z3 SMT solver.

**Core Verification:**
```cpp
bool verify(Expr* lhs, Expr* rhs) {
  // Convert to Z3 expressions
  z3::expr lhsZ3 = toZ3Expr(lhs);
  z3::expr rhsZ3 = toZ3Expr(rhs);
  
  // Check if (lhs != rhs) is UNSAT
  // If UNSAT, then lhs == rhs for all inputs
  solver.add(lhsZ3 != rhsZ3);
  return (solver.check() == z3::unsat);
}
```

**With Path Conditions:**
```cpp
bool verifyWithPCs(Expr* lhs, Expr* rhs, PathConditions PCs) {
  // Add path conditions as assumptions
  for (pc in PCs):
    solver.add(pc.lhs == pc.rhs);
  
  // Check if (PCs) => (lhs == rhs)
  solver.add(lhs != rhs);
  return (solver.check() == z3::unsat);
}
```

**Dataflow Integration:**
```cpp
// When creating Z3 variable, add constraints:
z3::expr var = c.bv_const("x", 32);

// If KnownZeros = 0x00FF (low byte unknown)
solver.add((var & 0x00FF) == 0);

// If NonZero = true
solver.add(var != 0);

// If NonNegative = true (MSB = 0)
solver.add((var >> 31) == 0);
```

---

## How It Works

### Step-by-Step Execution

```
1. Parse LLVM IR
   ├─ Load .ll file
   └─ Create LLVM Module

2. For each Function:
   ├─ Build DominatorTree
   ├─ Build LoopInfo
   └─ Create ExprBuilder

3. For each BasicBlock:
   ├─ Extract path conditions
   └─ For each Instruction:
      │
      ├─ Convert to Expr tree
      │  └─ Harvest dataflow facts
      │
      ├─ Enumerate candidates
      │  ├─ Generate constants
      │  ├─ Generate simple ops
      │  └─ Generate combinations
      │
      ├─ For each candidate:
      │  ├─ Check cost < original
      │  ├─ Verify with Z3
      │  └─ If valid: FOUND OPTIMIZATION!
      │
      └─ Report results

4. Print Summary
```

### Example Walkthrough

**Input C Code:**
```c
int foo(int x) {
  return (x * 8) / 4;
}
```

**LLVM IR:**
```llvm
define i32 @foo(i32 %x) {
  %1 = mul i32 %x, 8
  %2 = udiv i32 %1, 4
  ret i32 %2
}
```

**Mini-Souper Processing:**

1. **Convert `%1 = mul i32 %x, 8`:**
   ```
   Expr: Mul(Var("x", 32), Const(8, 32))
   Cost: 4 (multiplication is expensive)
   ```

2. **Enumerate candidates for `%1`:**
   ```
   - x << 3        (cost: 1)
   - x * 8         (cost: 4)
   - x + x + ...   (cost: 7)
   ```

3. **Verify `x * 8 ≡ x << 3`:**
   ```
   Z3 Query: (x * 8) != (x << 3)
   Result: UNSAT ✓
   Optimization: x * 8 → x << 3
   ```

4. **Convert `%2 = udiv i32 %1, 4`:**
   ```
   Expr: UDiv(Mul(Var("x"), Const(8)), Const(4))
   Cost: 8
   ```

5. **Enumerate candidates for `%2`:**
   ```
   - x << 1        (cost: 1)
   - x * 2         (cost: 4)
   - (x << 3) >> 2 (cost: 2)
   ```

6. **Verify `(x * 8) / 4 ≡ x << 1`:**
   ```
   Z3 Query: ((x * 8) / 4) != (x << 1)
   Result: UNSAT ✓
   Optimization: (x * 8) / 4 → x << 1
   ```

**Output:**
```
Optimization found:
  Original:  ((x * 8) / 4) (cost: 8)
  Optimized: (x << 1)      (cost: 1)
```

---

## Optimization Pipeline

### 1. Strength Reduction

**Pattern:** Expensive operations → Cheaper equivalents

```
x * 2^n  →  x << n
x / 2^n  →  x >> n
x % 2^n  →  x & (2^n - 1)
```

**Example:**
```c
int f(int x) { return x * 16; }
// Optimized: x << 4
```

### 2. Algebraic Simplification

**Pattern:** Complex expressions → Simpler forms

```
x + 0    →  x
x * 1    →  x
x - x    →  0
x ^ x    →  0
x & x    →  x
x | 0    →  x
```

**Example:**
```c
int f(int x) { return (x + 5) - 5; }
// Optimized: x
```

### 3. Constant Folding

**Pattern:** Operations on constants → Single constant

```
(x + 3) + 5  →  x + 8
(x << 2) << 1  →  x << 3
```

### 4. Dataflow-Guided Optimization

**Pattern:** Use known facts to simplify

```
// If x is known to be non-zero:
(x == 0) ? a : b  →  b

// If x's low 2 bits are known to be 0:
x % 4  →  0
```

**Example:**
```c
int f(unsigned x) {
  if (x > 0) {
    return (x == 0) ? 1 : 2;  // Optimized: return 2
  }
}
```

### 5. Path-Sensitive Optimization

**Pattern:** Use branch conditions to optimize

```c
if (x > 10) {
  y = (x > 5) ? a : b;  // Optimized: y = a
}
```

---

## Key Algorithms

### Cost Model

```cpp
int cost(Expr* e) {
  switch (e->op) {
    case Const:
    case Var:
      return 0;
    
    case Mul:
    case UDiv:
    case SDiv:
    case URem:
    case SRem:
      return 4 + cost(operands);
    
    default:  // Add, Sub, And, Or, Xor, Shl, etc.
      return 1 + cost(operands);
  }
}
```

**Rationale:**
- Constants/variables are free (already computed)
- Multiplication/division are expensive (4 cycles)
- Other operations are cheap (1 cycle)

### Equivalence Checking

```
verify(lhs, rhs):
  1. Convert lhs and rhs to Z3 bit-vectors
  2. Add dataflow constraints for all variables
  3. Assert: lhs != rhs
  4. Check satisfiability:
     - UNSAT → lhs ≡ rhs (always equal)
     - SAT   → lhs ≢ rhs (counterexample exists)
```

**Example:**
```
Verify: (x * 2) ≡ (x << 1)

Z3 Query:
  (declare-const x (_ BitVec 32))
  (assert (not (= (bvmul x #x00000002)
                  (bvshl x #x00000001))))
  (check-sat)

Result: unsat ✓
```

### Enumerative Synthesis

```
synthesize(expr):
  worklist = [constants, operands]
  
  while worklist not empty:
    candidate = worklist.pop()
    
    if cost(candidate) < cost(expr):
      if verify(expr, candidate):
        return candidate
    
    if cost(candidate) < maxCost:
      worklist.add(apply_ops(candidate))
  
  return expr  // No optimization found
```

---

## Building and Running

### Prerequisites

```bash
# Mini-souper uses Souper's dependencies
cd /path/to/souper
./build_deps.sh Release
```

### Build

```bash
cd mini_souper
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Run

```bash
# Compile C to LLVM IR
clang -O0 -emit-llvm -S test.c -o test.ll

# Run mini-souper
./mini_souper test.ll
```

### Example Output

```
=== Mini-Souper: ENUMERATIVE Synthesis ===
Using enumerative synthesis like real Souper!

Function: foo
  Trying 45 candidates for: ((x * 8) / 4)
  Optimization found:
    Original:  ((x * 8) / 4) (cost: 8)
    Optimized: (x << 1)      (cost: 1)
    
  Optimizations in this function: 1
---

=== Summary ===
Total optimizations found: 1
```

---

## Examples

### Example 1: Strength Reduction

**Input:**
```c
int multiply_by_power_of_two(int x) {
  return x * 32;
}
```

**Optimization:**
```
Original:  (x * 32)  [cost: 4]
Optimized: (x << 5)  [cost: 1]
```

### Example 2: Algebraic Simplification

**Input:**
```c
int redundant_ops(int x) {
  return (x + 10) - 10;
}
```

**Optimization:**
```
Original:  ((x + 10) - 10)  [cost: 2]
Optimized: x                [cost: 0]
```

### Example 3: Path-Sensitive

**Input:**
```c
int conditional(int x) {
  if (x > 0) {
    return (x == 0) ? 1 : 2;
  }
  return 0;
}
```

**Optimization:**
```
Under conditions: (x > 0) == 1
Original:  select((x == 0), 1, 2)  [cost: 2]
Optimized: 2                       [cost: 0]
```

### Example 4: Dataflow-Guided

**Input:**
```c
unsigned int mask_redundant(unsigned int x) {
  x = x & 0xFFFFFF00;  // Clear low byte
  return x % 256;       // Get low byte
}
```

**Optimization:**
```
With dataflow: x.KnownZeros = 0xFF (low byte is 0)
Original:  (x % 256)  [cost: 4]
Optimized: 0          [cost: 0]
```

---

## Limitations

1. **Enumerative Only**: No CEGIS (Counter-Example Guided Inductive Synthesis)
2. **Simple Dataflow**: Basic facts only, no advanced analysis
3. **No Caching**: Each query is independent
4. **Limited Operators**: Subset of LLVM operations
5. **No Loop Handling**: Loop-carried dependencies simplified to variables

---

## Comparison with Full Souper

| Feature | Mini-Souper | Full Souper |
|---------|-------------|-------------|
| **Synthesis** | Enumerative (breadth-first) | Enumerative + CEGIS |
| **Dataflow** | KnownBits, NonZero, NonNegative | Full ValueTracking + custom |
| **Path Conditions** | Single predecessor branches | Full SSA with block predicates |
| **Caching** | None | Redis-based query cache |
| **Precision** | Basic | Production-grade |
| **Performance** | Educational | Optimized for speed |

---

## Further Reading

- [Souper Paper](https://arxiv.org/pdf/1711.04422.pdf) - Original research paper
- [Z3 Tutorial](https://rise4fun.com/z3/tutorial) - SMT solver basics
- [LLVM ValueTracking](https://llvm.org/doxygen/ValueTracking_8h.html) - Dataflow analysis
- [Program Synthesis](https://people.csail.mit.edu/asolar/SynthesisCourse/) - General synthesis techniques

---

## Conclusion

Mini-Souper demonstrates that superoptimization, while complex, can be understood through a clean implementation. The key insights are:

1. **Internal Representation**: Convert to a simple, analyzable form (Expr trees)
2. **Synthesis**: Generate candidates systematically (enumerative)
3. **Verification**: Use SMT solvers for formal correctness (Z3)
4. **Dataflow**: Leverage compiler analysis to guide optimization
5. **Cost Model**: Prefer simpler, faster operations

This foundation scales to production superoptimizers like full Souper, which add sophisticated synthesis strategies, caching, and integration with compiler pipelines.
