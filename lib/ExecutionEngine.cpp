#include "ExecutionEngine.h"
#include "Error.h"
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

namespace dinorisc {

uint64_t ExecutionEngine::executeBlock(const std::vector<uint8_t> &machineCode,
                                       GuestState *guestState) {
  size_t pageSize = getpagesize();
  size_t allocSize =
      ((machineCode.size() + pageSize - 1) / pageSize) * pageSize;

  void *memory = mmap(nullptr, allocSize, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (memory == MAP_FAILED) {
    throw RuntimeError("ExecutionEngine: mmap failed");
  }

  std::memcpy(memory, machineCode.data(), machineCode.size());

  if (mprotect(memory, allocSize, PROT_READ | PROT_EXEC) != 0) {
    munmap(memory, allocSize);
    throw RuntimeError("ExecutionEngine: Failed to set execute permissions");
  }

  typedef uint64_t (*CompiledBlockFunctionPtr)(GuestState *);
  auto func = reinterpret_cast<CompiledBlockFunctionPtr>(memory);
  uint64_t nextPC = func(guestState);

  munmap(memory, allocSize);
  return nextPC;
}

} // namespace dinorisc
