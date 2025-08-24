// Simple conditional branch program for testing RISC-V instruction decoding
// Expected instructions: conditional branches, comparisons

int main() {
  int x = 7;
  int result;

  if (x > 5) {
    result = x * 2;
  } else {
    result = x + 1;
  }

  return result;
}
