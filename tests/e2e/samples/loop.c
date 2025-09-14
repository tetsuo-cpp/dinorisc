// Simple loop program for testing RISC-V instruction decoding
// Expected instructions: branch, compare, increment operations

int sum_to_n(int n) {
  int sum = 0;
  for (int i = 0; i < n; i++) {
    sum += i;
  }
  return sum;
}
