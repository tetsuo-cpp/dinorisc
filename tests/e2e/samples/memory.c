// Simple memory operations program for testing RISC-V instruction decoding
// Expected instructions: load/store operations, pointer arithmetic

int main() {
  int array[5] = {1, 2, 3, 4, 5};
  int sum = 0;

  for (int i = 0; i < 5; i++) {
    sum += array[i];
  }

  return sum;
}
