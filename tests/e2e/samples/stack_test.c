// Stack operations test program for testing shadow stack implementation
// Tests basic stack variable allocation and access

int stack_add(int a, int b) {
  int local_a = a;
  int local_b = b;
  int sum = local_a + local_b;
  return sum;
}