// Test PHI nodes
int test_phi_simple(int x, int cond) {
  int z;
  if (cond)
    z = x;
  else
    z = x;
  return z;  // Should optimize to just x (both branches same)
}

int test_phi_const(int cond) {
  int z;
  if (cond)
    z = 5;
  else
    z = 5;
  return z;  // Should optimize to constant 5
}

int test_phi_merge(int x, int y, int cond) {
  int z;
  if (cond)
    z = x + 1;
  else
    z = y + 1;
  return z - 1;  // Might find optimizations
}
