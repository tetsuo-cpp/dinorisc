#include "ExecutionEngine.h"
#include <cstring>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

namespace dinorisc {

ExecutionEngine::ExecutionEngine() { setupMemoryProtection(); }

ExecutionEngine::~ExecutionEngine() {
  // Clean up all allocated memory regions
  for (const auto &region : allocatedRegions) {
    freeExecutableMemory(region.address, region.size);
  }
  allocatedRegions.clear();
}

uint64_t ExecutionEngine::executeBlock(const std::vector<uint8_t> &machineCode,
                                       GuestState *guestState) {
  if (machineCode.empty()) {
    std::cerr << "ExecutionEngine: Empty machine code block" << std::endl;
    return 0;
  }

  // Allocate executable memory and copy machine code
  void *execMemory = allocateExecutableMemory(machineCode);
  if (!execMemory) {
    std::cerr << "ExecutionEngine: Failed to allocate executable memory"
              << std::endl;
    return 0;
  }

  // Cast to function pointer and execute
  // The compiled code should follow the calling convention:
  // - x0: pointer to GuestState
  // - return value in x0: next PC
  typedef uint64_t (*CompiledBlockFunctionPtr)(GuestState *);
  CompiledBlockFunctionPtr func =
      reinterpret_cast<CompiledBlockFunctionPtr>(execMemory);

  std::cout << "ExecutionEngine: Executing " << machineCode.size()
            << " bytes of ARM64 code at PC=0x" << std::hex << guestState->pc
            << std::dec << std::endl;

  try {
    uint64_t nextPC = func(guestState);
    std::cout << "ExecutionEngine: Block executed successfully, next PC=0x"
              << std::hex << nextPC << std::dec << std::endl;
    return nextPC;
  } catch (...) {
    std::cerr << "ExecutionEngine: Exception during block execution"
              << std::endl;
    return 0;
  }
}

void *ExecutionEngine::allocateExecutableMemory(
    const std::vector<uint8_t> &machineCode) {
  // Round up to page size
  size_t pageSize = getpagesize();
  size_t allocSize =
      ((machineCode.size() + pageSize - 1) / pageSize) * pageSize;

  void *memory = allocateExecutablePage(allocSize);
  if (!memory) {
    return nullptr;
  }

  // Copy machine code to executable memory
  std::memcpy(memory, machineCode.data(), machineCode.size());

  // Change permissions to read+execute (remove write)
  if (mprotect(memory, allocSize, PROT_READ | PROT_EXEC) != 0) {
    std::cerr << "ExecutionEngine: Failed to set execute permissions"
              << std::endl;
    munmap(memory, allocSize);
    return nullptr;
  }

  // Track the allocation
  allocatedRegions.push_back({memory, allocSize});

  return memory;
}

void ExecutionEngine::freeExecutableMemory(void *memory, size_t size) {
  if (memory) {
    munmap(memory, size);
  }
}

void ExecutionEngine::setupMemoryProtection() {
  // Nothing special needed for setup on ARM64 macOS
  // mmap will handle the memory protection flags
}

void *ExecutionEngine::allocateExecutablePage(size_t size) {
  // Allocate memory with read and write permissions first
  void *memory = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (memory == MAP_FAILED) {
    std::cerr << "ExecutionEngine: mmap failed for writable memory"
              << std::endl;
    return nullptr;
  }

  return memory;
}

} // namespace dinorisc