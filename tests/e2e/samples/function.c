// Simple function call program for testing RISC-V instruction decoding
// Expected instructions: function call/return, stack operations

int add_numbers(int a, int b) { return a + b; }

int main() {
  int result = add_numbers(3, 7);
  return result;
}
