#!/bin/bash

CLANG="../../third_party/llvm-Release-install/bin/clang"
OUTPUT_DIR="../build"

for c_file in *.c; do
  if [ -f "$c_file" ]; then
    ll_file="${c_file%.c}.ll"
    echo "Compiling $c_file -> $OUTPUT_DIR/$ll_file"
    $CLANG -S -emit-llvm -O0 -Xclang -disable-O0-optnone "$c_file" -o "$OUTPUT_DIR/$ll_file"
  fi
done

echo "Done!"
