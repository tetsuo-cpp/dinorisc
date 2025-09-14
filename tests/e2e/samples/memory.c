// Simple memory operations program for testing RISC-V instruction decoding
// Expected instructions: load/store operations, pointer arithmetic

int array_sum(int n) {
  int array[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  int sum = 0;

  for (int i = 0; i < n && i < 10; i++) {
    sum += array[i];
  }

  return sum;
}
