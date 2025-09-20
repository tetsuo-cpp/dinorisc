#pragma once

#include <cstddef>
#include <cstdint>
#include <sys/mman.h>

namespace dinorisc {

struct GuestState {
  // RISC-V 64-bit general-purpose registers (x0-x31)
  uint64_t x[32];

  // Program counter
  uint64_t pc;

  // Shadow memory for guest program's stack and data
  void *shadowMemory;
  size_t shadowMemorySize;
  uint64_t guestMemoryBase;

  GuestState()
      : x{}, pc(0), shadowMemory(nullptr), shadowMemorySize(0),
        guestMemoryBase(0) {}

  ~GuestState() {
    if (shadowMemory && shadowMemorySize > 0) {
      munmap(shadowMemory, shadowMemorySize);
    }
  }

  // Write to register (x0 writes are ignored per RISC-V specification)
  void writeRegister(uint8_t reg, uint64_t value) {
    if (reg >= 32 || reg == 0)
      return;
    x[reg] = value;
  }
};

} // namespace dinorisc