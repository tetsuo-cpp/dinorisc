#include "RegisterAllocator.h"
#include <algorithm>

namespace dinorisc {
namespace lowering {

const std::vector<arm64::Register> RegisterAllocator::availableRegisters = {
    arm64::Register::X0,  arm64::Register::X1,  arm64::Register::X2,
    arm64::Register::X3,  arm64::Register::X4,  arm64::Register::X5,
    arm64::Register::X6,  arm64::Register::X7,  arm64::Register::X8,
    arm64::Register::X9,  arm64::Register::X10, arm64::Register::X11,
    arm64::Register::X12, arm64::Register::X13, arm64::Register::X14,
    arm64::Register::X15, arm64::Register::X16, arm64::Register::X17,
    arm64::Register::X18, arm64::Register::X19, arm64::Register::X20,
    arm64::Register::X21, arm64::Register::X22, arm64::Register::X23,
    arm64::Register::X24, arm64::Register::X25, arm64::Register::X26,
    arm64::Register::X27, arm64::Register::X28
    // Note: X29 (frame pointer), X30 (link register), and SP are reserved
};

RegisterAllocator::RegisterAllocator() {}

bool RegisterAllocator::allocateRegisters(
    std::vector<arm64::Instruction> &instructions,
    const std::vector<LiveInterval> &liveIntervals,
    const InstructionSelector &selector) {
  allocation.clear();
  activeIntervals.clear();

  // Sort intervals by start point (should already be sorted from liveness
  // analysis)
  auto sortedIntervals = liveIntervals;
  std::sort(sortedIntervals.begin(), sortedIntervals.end(),
            [](const LiveInterval &a, const LiveInterval &b) {
              return a.start < b.start;
            });

  // Linear scan algorithm
  for (const auto &interval : sortedIntervals) {
    // Expire old intervals
    expireOldIntervals(interval.start);

    // Try to find an available register
    arm64::Register physReg = getNextAvailableRegister();
    if (physReg == arm64::Register::XSP) { // No register available
      // For first pass, just fail - no spilling
      return false;
    }

    // Assign the register
    VirtualRegister vreg = selector.getVirtualRegister(interval.valueId);
    if (vreg == 0) {
      // This shouldn't happen if selector is working correctly
      continue;
    }

    allocation[vreg] = physReg;
    activeIntervals.push_back({interval, physReg});
  }

  // Replace virtual registers in all instructions
  for (auto &inst : instructions) {
    replaceVirtualRegisters(inst);
  }

  return true;
}

arm64::Register
RegisterAllocator::getPhysicalRegister(VirtualRegister vreg) const {
  auto it = allocation.find(vreg);
  if (it != allocation.end()) {
    return it->second;
  }
  return arm64::Register::XSP; // Invalid/unallocated
}

arm64::Register RegisterAllocator::getNextAvailableRegister() {
  // Find first register not currently in use
  for (arm64::Register reg : availableRegisters) {
    bool inUse = false;
    for (const auto &active : activeIntervals) {
      if (active.physicalReg == reg) {
        inUse = true;
        break;
      }
    }
    if (!inUse) {
      return reg;
    }
  }

  return arm64::Register::XSP; // No available register
}

bool RegisterAllocator::isRegisterAvailable(arm64::Register reg,
                                            size_t point) const {
  for (const auto &active : activeIntervals) {
    if (active.physicalReg == reg && active.interval.start <= point &&
        point <= active.interval.end) {
      return false;
    }
  }
  return true;
}

void RegisterAllocator::expireOldIntervals(size_t currentPoint) {
  // Remove intervals that have ended
  activeIntervals.erase(
      std::remove_if(activeIntervals.begin(), activeIntervals.end(),
                     [currentPoint](const ActiveInterval &active) {
                       return active.interval.end < currentPoint;
                     }),
      activeIntervals.end());
}

void RegisterAllocator::replaceVirtualRegisters(arm64::Instruction &inst) {
  std::visit(
      [&](auto &instKind) {
        using T = std::decay_t<decltype(instKind)>;

        if constexpr (std::is_same_v<T, arm64::ThreeOperandInst>) {
          instKind.src1 = replaceOperandRegister(instKind.src1);
          instKind.src2 = replaceOperandRegister(instKind.src2);
          // dest is already a physical register placeholder, but we should map
          // it properly For now, assume dest register mapping is handled
          // separately
        } else if constexpr (std::is_same_v<T, arm64::TwoOperandInst>) {
          instKind.src = replaceOperandRegister(instKind.src);
          // dest mapping handled separately
        } else if constexpr (std::is_same_v<T, arm64::MemoryInst>) {
          // baseReg mapping - for now assume it's already physical
          // reg mapping handled separately
        } else if constexpr (std::is_same_v<T, arm64::BranchInst>) {
          // No register operands to replace
        }
      },
      inst.kind);
}

arm64::Operand
RegisterAllocator::replaceOperandRegister(const arm64::Operand &operand) {
  if (std::holds_alternative<arm64::Register>(operand)) {
    arm64::Register reg = std::get<arm64::Register>(operand);

    // If this is a virtual register (represented as a physical reg
    // placeholder), we need a better way to identify and map virtual registers.
    // For now, just return the operand as-is since our instruction selector
    // already maps virtual registers to placeholder physical registers.
    return operand;
  }

  // For immediate operands, return as-is
  return operand;
}

} // namespace lowering
} // namespace dinorisc