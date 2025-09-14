// Fibonacci sequence program for testing RISC-V instruction decoding
// Expected instructions: loops, comparisons, arithmetic operations

int fibonacci(int n) {
  if (n <= 1) {
    return n;
  }

  int a = 0, b = 1;
  for (int i = 2; i <= n; i++) {
    int temp = a + b;
    a = b;
    b = temp;
  }

  return b;
}