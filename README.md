# Mini-Souper

A compact, educational superoptimizer for LLVM IR. Mini-Souper demonstrates how production tools like [Souper](https://github.com/google/souper) find equivalent, lower-cost programs using **enumerative synthesis** and **SMT-based verification** (Z3).

The codebase is intentionally small (~750 lines of C++) so you can read the full pipeline end to end: LLVM IR in, candidate expressions out, Z3 proves equivalence, cheapest valid rewrite wins.

For algorithm-level detail, dataflow semantics, and walkthroughs, see [DOCUMENTATION.md](DOCUMENTATION.md).

---

## Table of Contents

- [What is a superoptimizer?](#what-is-a-superoptimizer)
- [Features](#features)
- [How it works](#how-it-works)
- [Requirements](#requirements)
- [Installation](#installation)
- [Usage](#usage)
- [Project layout](#project-layout)
- [Test suite](#test-suite)
- [Optimization examples](#optimization-examples)
- [Cost model](#cost-model)
- [Mini-Souper vs Souper](#mini-souper-vs-souper)
- [Limitations](#limitations)
- [Further reading](#further-reading)

---

## What is a superoptimizer?

Traditional compilers apply fixed rewrite rules (`x * 2` → `x << 1`). A **superoptimizer** instead:

1. **Builds** an internal representation of each instruction
2. **Synthesizes** many candidate equivalent expressions
3. **Verifies** equivalence with an SMT solver (unsat on `lhs ≠ rhs` means they match for all inputs)
4. **Selects** the cheapest verified candidate

Mini-Souper implements this loop in a readable form suitable for learning, not for shipping in a production compiler pipeline.

---

## Features

| Capability | Description |
|------------|-------------|
| **LLVM IR input** | Parses `.ll` files via LLVM's IR reader |
| **Expr IR** | Tree-shaped internal ops (add, mul, shifts, icmp, select, phi, …) |
| **Enumerative synthesis** | Generates constants, operand reuse, unary/binary combinations |
| **Z3 verification** | Bit-vector equivalence checking with optional path conditions |
| **Dataflow facts** | Harvests `KnownBits`, non-zero, and non-negative from LLVM `ValueTracking` |
| **Path conditions** | Uses branch predicates from dominator predecessors |
| **Cost model** | Prefers cheaper ops (e.g. shift over multiply) |

Supported optimization families include strength reduction, algebraic identities, constant folding patterns, dataflow-guided simplification, and path-sensitive rewrites.

---

## How it works

```
  .ll file          Expr trees           Candidates          Z3 verify
 ┌─────────┐      ┌────────────┐      ┌──────────────┐      ┌───────────┐
 │ LLVM IR │ ───▶ │ ExprBuilder│ ───▶ │ Enumerative  │ ───▶ │ Z3Solver  │
 │ Module  │      │ + dataflow │      │ Synthesizer  │      │ (UNSAT?)  │
 └─────────┘      └────────────┘      └──────────────┘      └─────┬─────┘
                                                                  │
                                                                  ▼
                                                          Cheaper equivalent
                                                          (if cost lower)
```

**Per-instruction pipeline** (`src/main.cpp`):

1. Walk each function’s basic blocks (with dominator/loop info for analysis).
2. Record path conditions at block entries.
3. Convert integer instructions to `Expr` trees (skip constants and bare variables).
4. Enumerate synthesis candidates up to a cost bound.
5. For each candidate with **strictly lower** cost than the original, ask Z3 whether `original ≠ candidate` is satisfiable.
6. On first UNSAT result, report the optimization and move to the next instruction.

Equivalence without path conditions: `verify(lhs, rhs)` asserts `lhs ≠ rhs` and checks UNSAT.

With path conditions: `verifyWithPCs` assumes each recorded branch fact before checking equivalence.

---

## Requirements

Mini-Souper does **not** vendor LLVM or Z3. It expects the same dependency layout as [Souper](https://github.com/google/souper) after running `build_deps.sh`:

| Dependency | Expected path (relative to repo root) |
|------------|----------------------------------------|
| LLVM + Clang | `../third_party/llvm-Release-install/` |
| Z3 | `../third_party/z3-install/` |

**Important:** CMake resolves `../third_party` from the **parent directory of this repository**, not from inside the repo. That means one of these layouts must be true:

### Option A — Inside the Souper tree (recommended)

```text
souper/
├── third_party/          # created by ./build_deps.sh
│   ├── llvm-Release-install/
│   └── z3-install/
└── minisouper/           # this repository
    ├── CMakeLists.txt
    └── src/
```

Here `minisouper/../third_party` correctly points at `souper/third_party`.

### Option B — Standalone clone with a symlink

If you keep the repo at e.g. `~/code/minisouper`, create a sibling `third_party` that points at Souper’s deps:

```bash
ln -s /path/to/souper/third_party /path/to/parent-of-minisouper/third_party
# e.g. ln -s ~/code/souper/third_party ~/code/third_party
```

### Build tools

- **CMake** ≥ 3.13
- **C++17** compiler (the project configures Clang from the LLVM install)
- **Linux** is the primary target (Souper’s `build_deps.sh`); macOS may require adjusting the Z3 library path in `CMakeLists.txt` (`lib64` vs `lib`)

---

## Installation

### 1. Build Souper dependencies (one-time)

From a Souper checkout:

```bash
git clone https://github.com/google/souper.git
cd souper
./build_deps.sh Release
```

This downloads and builds LLVM 18 (Souper fork) and Z3 into `third_party/`. It can take a long time and significant disk space.

### 2. Place or link Mini-Souper

Clone this repo as `souper/minisouper`, or use the symlink approach above so `../third_party` exists.

### 3. Build Mini-Souper

```bash
cd minisouper   # or your clone path
mkdir -p build && cd build
cmake ..
make -j$(nproc)   # or: make -j$(sysctl -n hw.ncpu) on macOS
```

The binary is `build/mini_souper`.

### 4. Run all tests (optional)

From the repository root:

```bash
./build_and_test.sh
```

This configures, builds, compiles every `tests/test_*.c` to LLVM IR with `-O0`, and runs `mini_souper` on each `.ll` file.

---

## Usage

### Compile C to LLVM IR

Use the Clang from Souper’s LLVM install (paths adjust if you used Option B):

```bash
/path/to/souper/third_party/llvm-Release-install/bin/clang \
  -O0 -emit-llvm -S myfunc.c -o myfunc.ll
```

`-O0` keeps redundant arithmetic visible so the superoptimizer has something to improve. For tests, `compile_tests.sh` also passes `-Xclang -disable-O0-optnone`.

### Run the superoptimizer

```bash
./build/mini_souper myfunc.ll
```

### Example session

Input C:

```c
int foo(int x) {
  return (x * 8) / 4;
}
```

Typical output:

```text
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

### CLI

```text
mini_souper <input.ll>
```

Only one argument is accepted today: a path to LLVM IR (`.ll`).

---

## Project layout

```text
minisouper/
├── CMakeLists.txt          # Build: LLVM + Z3 from ../third_party
├── build_and_test.sh       # Full build + test driver
├── DOCUMENTATION.md        # In-depth design and algorithms
├── README.md               # This file
├── include/
│   ├── Expr.h              # Internal expression AST + cost/print
│   └── Z3Solver.h          # SMT equivalence checking API
├── src/
│   ├── main.cpp            # Driver: parse IR, synthesize, verify, report
│   ├── Expr.cpp            # Printing and cost model
│   ├── ExprBuilder.cpp     # LLVM Instruction → Expr (+ dataflow, PCs)
│   ├── EnumerativeSynthesizer.cpp  # Candidate generation
│   └── Z3Solver.cpp        # Expr ↔ Z3 bit-vectors, verify / verifyWithPCs
└── tests/
    ├── test_strength.c     # mul/div/shift, identities
    ├── test_dataflow.c     # KnownBits, x&x, x^x, etc.
    ├── test_pc.c           # Branch-sensitive optimizations
    ├── test_phi.c          # PHI merge cases
    ├── test_*.ll           # Pre-generated IR (optional)
    └── compile_tests.sh    # C → .ll into build/
```

`ExprBuilder.cpp` and `EnumerativeSynthesizer.cpp` are `#include`d from `main.cpp` (single translation unit for simplicity).

---

## Test suite

| Test file | What it exercises |
|-----------|-------------------|
| `test_strength.c` | `x * 8` → shifts, `x / 16`, `x * 1`, `x + 0`, `x & 0`, chained mul/div |
| `test_dataflow.c` | Known bits, `x & x`, `x ^ x`, `x - x`, non-zero paths |
| `test_pc.c` | Comparisons and branches (`x > 10`, `x == 5`, `x != 0`) |
| `test_phi.c` | PHI nodes with identical or constant arms |

Compile tests only:

```bash
cd tests && ./compile_tests.sh
```

Run a single IR file:

```bash
./build/mini_souper tests/test_strength.ll
```

---

## Optimization examples

### Strength reduction

```c
return x * 32;   // → x << 5
return x / 16;   // → x >> 4  (unsigned)
```

### Algebraic simplification

```c
return (x + 10) - 10;   // → x
return x ^ x;           // → 0
return x & x;           // → x
```

### Path-sensitive

```c
if (x > 0) {
  return (x == 0) ? 1 : 2;   // under PC (x > 0): → 2
}
```

### Dataflow-guided

```c
x = x & 0xFFFFFF00;
return x % 256;   // low byte known zero → 0
```

More examples and Z3 query intuition are in [DOCUMENTATION.md](DOCUMENTATION.md#examples).

---

## Cost model

Defined in `src/Expr.cpp`:

| Expression kind | Cost |
|-----------------|------|
| `Const`, `Var` | 0 |
| `Mul`, `UDiv`, `SDiv`, `URem`, `SRem` | 4 + sum of operands |
| Other ops (`Add`, `Sub`, shifts, bitwise, icmp, `Select`, …) | 1 + sum of operands |

Candidates must beat the original’s cost before Z3 is invoked. The first valid cheaper rewrite is reported (not necessarily globally optimal across all possible programs).

---

## Mini-Souper vs Souper

| | Mini-Souper | [Souper](https://github.com/google/souper) |
|---|-------------|-------------------------------------------|
| **Size** | ~750 LOC | Tens of thousands of LOC |
| **Goal** | Teach core ideas | Production LLVM integration |
| **Synthesis** | Enumerative only | Enumerative + CEGIS + pruning |
| **Verification** | Z3 directly | Souper IR + Alive2 + caching |
| **Dataflow** | Basic LLVM facts | Extensive custom analysis |
| **Path conditions** | Predecessor branch facts | Full block predicates / SSA |
| **Caching** | None | Redis-backed query cache |
| **Output** | stdout report | LLVM pass / candidate maps |

Mini-Souper is a **subset** of Souper’s ideas, not a drop-in replacement.

---

## Limitations

- **Enumerative only** — no counterexample-guided inductive synthesis (CEGIS)
- **Bounded search** — small constant set and shallow combinations; misses many rewrites
- **Integer IR only** — non-integer instructions are skipped
- **First match wins** — not exhaustive global search for the true minimum-cost program
- **Loops** — no full loop-aware reasoning; loop-carried values are treated simplistically
- **No LLVM rewrite** — reports optimizations only; does not emit modified bitcode
- **Dependency coupling** — requires Souper’s LLVM/Z3 build tree layout

---

## Further reading

- [DOCUMENTATION.md](DOCUMENTATION.md) — architecture, components, algorithms, step-by-step walkthrough
- [Souper (OSDI 2018)](https://arxiv.org/pdf/1711.04422.pdf) — research background
- [Z3 tutorial](https://rise4fun.com/z3/tutorial) — SMT and bit-vectors
- [LLVM ValueTracking](https://llvm.org/doxygen/ValueTracking_8h.html) — dataflow facts used in `ExprBuilder`

---

## License

Mini-Souper is an educational companion to Souper. If you distribute or modify it, follow the licensing terms of Souper, LLVM, and Z3 for the respective components you link against.
