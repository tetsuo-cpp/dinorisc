#pragma once

#include <cstddef>
#include <cstdint>

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

  // Constructor that initializes state
  GuestState()
      : pc(0), shadowMemory(nullptr), shadowMemorySize(0), guestMemoryBase(0) {
    // Initialize all registers to 0
    // x0 is hardwired to zero in RISC-V
    for (int i = 0; i < 32; ++i) {
      x[i] = 0;
    }
  }

  // Ensure x0 always returns 0 (RISC-V specification)
  uint64_t readRegister(uint32_t reg) const {
    if (reg >= 32)
      return 0;
    if (reg == 0)
      return 0; // x0 is always zero
    return x[reg];
  }

  // Write to register (x0 writes are ignored)
  void writeRegister(uint32_t reg, uint64_t value) {
    if (reg >= 32 || reg == 0)
      return; // x0 is read-only
    x[reg] = value;
  }
};

} // namespace dinorisc