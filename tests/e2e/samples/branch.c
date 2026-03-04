// Simple conditional branch program for testing RISC-V instruction decoding
// Expected instructions: conditional branches, comparisons

int conditional_calc(int x, int threshold) {
  if (x > threshold)
    return x * 2;
  return x + 1;
}
