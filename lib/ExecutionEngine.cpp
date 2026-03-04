#include "ExecutionEngine.h"
#include <cstring>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

namespace dinorisc {

uint64_t ExecutionEngine::executeBlock(const std::vector<uint8_t> &machineCode,
                                       GuestState *guestState) {
  if (machineCode.empty()) {
    return 0;
  }

  size_t pageSize = getpagesize();
  size_t allocSize =
      ((machineCode.size() + pageSize - 1) / pageSize) * pageSize;

  void *memory = mmap(nullptr, allocSize, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (memory == MAP_FAILED) {
    std::cerr << "ExecutionEngine: mmap failed" << std::endl;
    return 0;
  }

  std::memcpy(memory, machineCode.data(), machineCode.size());

  if (mprotect(memory, allocSize, PROT_READ | PROT_EXEC) != 0) {
    std::cerr << "ExecutionEngine: Failed to set execute permissions"
              << std::endl;
    munmap(memory, allocSize);
    return 0;
  }

  typedef uint64_t (*CompiledBlockFunctionPtr)(GuestState *);
  auto func = reinterpret_cast<CompiledBlockFunctionPtr>(memory);
  uint64_t nextPC = func(guestState);

  munmap(memory, allocSize);
  return nextPC;
}

} // namespace dinorisc
