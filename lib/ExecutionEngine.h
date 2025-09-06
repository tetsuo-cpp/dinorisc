#pragma once

#include "GuestState.h"
#include <cstdint>
#include <functional>
#include <vector>

namespace dinorisc {

// Function type for executing translated basic blocks
// Takes guest state pointer and returns the next PC
using CompiledBlockFunction = std::function<uint64_t(GuestState *)>;

class ExecutionEngine {
public:
  ExecutionEngine();
  ~ExecutionEngine();

  // Execute raw ARM64 machine code
  // Returns the next PC to execute, or 0 if execution should stop
  uint64_t executeBlock(const std::vector<uint8_t> &machineCode,
                        GuestState *guestState);

  // Create an executable memory region from machine code
  void *allocateExecutableMemory(const std::vector<uint8_t> &machineCode);

  // Free executable memory
  void freeExecutableMemory(void *memory, size_t size);

private:
  // Platform-specific implementation helpers
  void setupMemoryProtection();
  void *allocateExecutablePage(size_t size);

  // Track allocated memory for cleanup
  struct MemoryRegion {
    void *address;
    size_t size;
  };
  std::vector<MemoryRegion> allocatedRegions;
};

} // namespace dinorisc