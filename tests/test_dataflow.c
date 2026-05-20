// Test dataflow facts (KnownBits)
int test_known_bits(int x) {
  x = x & 0xFF;  // x is now 0-255, lower 8 bits known
  return x | 0xFF;  // Should optimize to 0xFF since lower 8 bits already set
}

int test_nonzero(int x) {
  if (x == 0) return 0;
  return x * 1;  // x is known non-zero here, x*1 => x
}

int test_and_same(int x) {
  return x & x;  // Should optimize to x
}

int test_or_same(int x) {
  return x | x;  // Should optimize to x
}

int test_xor_same(int x) {
  return x ^ x;  // Should optimize to 0
}

int test_sub_same(int x) {
  return x - x;  // Should optimize to 0
}
