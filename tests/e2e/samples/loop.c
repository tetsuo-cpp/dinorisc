// Simple loop program for testing RISC-V instruction decoding
// Expected instructions: branch, compare, increment operations

int main() {
  int sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += i;
  }
  return sum;
}
