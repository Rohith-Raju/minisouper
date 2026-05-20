// Test path conditions
int test_path_condition(int x) {
  if (x > 10) {
    // Here we know x > 10
    int y = x - 5;  // y > 5
    return y;
  }
  return 0;
}

int test_branch_optimization(int x) {
  if (x == 5) {
    // Here we know x == 5
    return x * 2;  // Should be optimized knowing x == 5
  }
  return x;
}

int test_nonzero_branch(int x) {
  if (x != 0) {
    // x is known non-zero
    return x / x;  // Should optimize to 1
  }
  return 0;
}
