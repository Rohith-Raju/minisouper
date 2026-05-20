#!/bin/bash
# Build and test Mini-Souper

set -e

echo "=== Building Mini-Souper ==="
cd "$(dirname "$0")"

# Create build directory
mkdir -p build
cd build

# Configure and build
cmake ..
make -j$(nproc)

echo ""
echo "=== Build Complete ==="
echo ""

# Test if we have clang
CLANG="../third_party/llvm-Release-install/bin/clang"
if [ ! -f "$CLANG" ]; then
    CLANG="clang"
fi

echo "=== Compiling Test Cases ==="
cd ../tests

# Compile test cases to LLVM IR
for test in test_*.c; do
    base=$(basename "$test" .c)
    echo "Compiling $test..."
    $CLANG -O0 -emit-llvm -S "$test" -o "${base}.ll"
done

echo ""
echo "=== Running Mini-Souper on Test Cases ==="
echo ""

# Run mini-souper on each test
for test in test_*.ll; do
    echo "=========================================="
    echo "Testing: $test"
    echo "=========================================="
    ../build/mini_souper "$test" 2>&1 || echo "(Test had issues, continuing...)"
    echo ""
done

echo ""
echo "=== All Tests Complete ==="
echo ""

