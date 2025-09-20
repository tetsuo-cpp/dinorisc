#include "RegisterAllocator.h"
#include <algorithm>
#include <cassert>
#include <set>

namespace dinorisc {
namespace lowering {

const std::vector<arm64::Register> RegisterAllocator::availableRegisters = {
    arm64::Register::X1,  arm64::Register::X2,  arm64::Register::X3,
    arm64::Register::X4,  arm64::Register::X5,  arm64::Register::X6,
    arm64::Register::X7,  arm64::Register::X8,  arm64::Register::X9,
    arm64::Register::X10, arm64::Register::X11, arm64::Register::X12,
    arm64::Register::X13, arm64::Register::X14, arm64::Register::X15,
    arm64::Register::X16, arm64::Register::X17, arm64::Register::X18,
    arm64::Register::X19, arm64::Register::X20, arm64::Register::X21,
    arm64::Register::X22, arm64::Register::X23, arm64::Register::X24,
    arm64::Register::X25, arm64::Register::X26, arm64::Register::X27,
    arm64::Register::X28
    // Note: X0 (GuestState pointer), X29 (frame pointer), X30 (link register),
    // and SP are reserved
};

RegisterAllocator::RegisterAllocator() {}

bool RegisterAllocator::allocateRegisters(
    std::vector<arm64::Instruction> &instructions,
    const std::vector<LiveInterval> &liveIntervals) {
  allocation.clear();
  activeIntervals.clear();

  // Debug assertion to verify input is sorted
  assert(std::is_sorted(liveIntervals.begin(), liveIntervals.end(),
                        [](const LiveInterval &a, const LiveInterval &b) {
                          return a.start < b.start;
                        }));

  // Linear scan algorithm using pre-sorted intervals
  for (const auto &interval : liveIntervals) {
    // Expire old intervals
    expireOldIntervals(interval.start);

    // Try to find an available register
    auto physReg = getNextAvailableRegister();
    if (!physReg.has_value()) { // No register available
      // For first pass, just fail - no spilling
      return false;
    }

    // Assign the register directly to the virtual register
    VirtualRegister vreg = interval.virtualRegister;
    allocation[vreg] = physReg.value();
    activeIntervals.push_back({interval, physReg.value()});
  }

  // Replace virtual registers in all instructions
  for (auto &inst : instructions) {
    replaceVirtualRegisters(inst);
  }

  return true;
}

std::optional<arm64::Register>
RegisterAllocator::getPhysicalRegister(VirtualRegister vreg) const {
  auto it = allocation.find(vreg);
  if (it != allocation.end()) {
    return it->second;
  }
  return std::nullopt;
}

arm64::Register
RegisterAllocator::getPhysicalRegisterOrThrow(VirtualRegister vreg) const {
  auto reg = getPhysicalRegister(vreg);
  if (reg.has_value()) {
    return reg.value();
  }
  throw std::runtime_error(
      "No physical register assigned to virtual register " +
      std::to_string(vreg));
}

std::optional<arm64::Register> RegisterAllocator::getNextAvailableRegister() {
  std::set<arm64::Register> inUse;
  for (const auto &active : activeIntervals) {
    inUse.insert(active.physicalReg);
  }

  for (arm64::Register reg : availableRegisters) {
    if (inUse.find(reg) == inUse.end()) {
      return reg;
    }
  }

  return std::nullopt; // No available register
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
          instKind.dest = replaceOperandRegister(instKind.dest);
          instKind.src1 = replaceOperandRegister(instKind.src1);
          instKind.src2 = replaceOperandRegister(instKind.src2);
        } else if constexpr (std::is_same_v<T, arm64::TwoOperandInst>) {
          instKind.dest = replaceOperandRegister(instKind.dest);
          instKind.src = replaceOperandRegister(instKind.src);
        } else if constexpr (std::is_same_v<T, arm64::MemoryInst>) {
          instKind.reg = replaceOperandRegister(instKind.reg);
          instKind.baseReg = replaceOperandRegister(instKind.baseReg);
        } else if constexpr (std::is_same_v<T, arm64::MoveWideInst>) {
          instKind.dest = replaceOperandRegister(instKind.dest);
        } else if constexpr (std::is_same_v<T, arm64::BranchInst>) {
          // No register operands to replace
        } else if constexpr (std::is_same_v<T, arm64::ConditionalInst>) {
          instKind.dest = replaceOperandRegister(instKind.dest);
        } else if constexpr (std::is_same_v<T, arm64::ConditionalSelectInst>) {
          instKind.dest = replaceOperandRegister(instKind.dest);
          instKind.src1 = replaceOperandRegister(instKind.src1);
          instKind.src2 = replaceOperandRegister(instKind.src2);
        }
      },
      inst.kind);
}

arm64::Operand
RegisterAllocator::replaceOperandRegister(const arm64::Operand &operand) {
  if (std::holds_alternative<arm64::Register>(operand)) {
    // Physical register - return as-is
    return operand;
  } else if (std::holds_alternative<VirtualRegister>(operand)) {
    // Virtual register - map to physical register
    VirtualRegister vreg = std::get<VirtualRegister>(operand);
    arm64::Register physReg = getPhysicalRegisterOrThrow(vreg);
    return physReg;
  }

  // For immediate operands, return as-is
  return operand;
}

} // namespace lowering
} // namespace dinorisc
