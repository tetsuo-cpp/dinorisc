#pragma once

#include "../ARM64/Instruction.h"
#include "LivenessAnalysis.h"
#include <unordered_map>
#include <vector>

namespace dinorisc {
namespace lowering {

using VirtualRegister = arm64::VirtualRegister;

class RegisterAllocator {
public:
  RegisterAllocator();

  // Perform linear scan register allocation on ARM64 instructions
  // Returns true on success, false if we run out of registers
  bool allocateRegisters(std::vector<arm64::Instruction> &instructions,
                         const std::vector<LiveInterval> &liveIntervals);

  // Get the physical register assigned to a virtual register
  arm64::Register getPhysicalRegister(VirtualRegister vreg) const;

private:
  // Available ARM64 general-purpose registers for allocation
  static const std::vector<arm64::Register> availableRegisters;

  // Mapping from virtual registers to physical registers
  std::unordered_map<VirtualRegister, arm64::Register> allocation;

  // Active intervals during linear scan
  struct ActiveInterval {
    LiveInterval interval;
    arm64::Register physicalReg;
  };
  std::vector<ActiveInterval> activeIntervals;

  // Get next available physical register
  arm64::Register getNextAvailableRegister();

  // Check if a physical register is available at the given point
  bool isRegisterAvailable(arm64::Register reg, size_t point) const;

  // Expire old intervals that are no longer active
  void expireOldIntervals(size_t currentPoint);

  // Replace virtual registers in instruction with physical registers
  void replaceVirtualRegisters(arm64::Instruction &inst);

  // Helper to replace register in operand
  arm64::Operand replaceOperandRegister(const arm64::Operand &operand);
};

} // namespace lowering
} // namespace dinorisc