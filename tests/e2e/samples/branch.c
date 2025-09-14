// Simple conditional branch program for testing RISC-V instruction decoding
// Expected instructions: conditional branches, comparisons

int conditional_calc(int x, int threshold) {
  int result;

  if (x > threshold) {
    result = x * 2;
  } else {
    result = x + 1;
  }

  return result;
}
