// Test strength reduction and cost-based synthesis
int test_mul_power2(int x) {
  return x * 8;  // Should optimize to x << 3
}

int test_div_power2(unsigned x) {
  return x / 16;  // Should optimize to x >> 4
}

int test_mul_by_one(int x) {
  return x * 1;  // Should optimize to x
}

int test_add_zero(int x) {
  return x + 0;  // Should optimize to x
}

int test_shift_zero(int x) {
  return x << 0;  // Should optimize to x
}

int test_and_zero(int x) {
  return x & 0;  // Should optimize to 0
}

int test_or_zero(int x) {
  return x | 0;  // Should optimize to x
}

int test_complex(int x) {
  int y = x * 4;  // x << 2
  int z = y / 2;  // (x << 2) >> 1 = x << 1 = x * 2
  return z;
}
