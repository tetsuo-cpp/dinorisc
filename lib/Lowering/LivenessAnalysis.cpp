#include "LivenessAnalysis.h"

namespace dinorisc {
namespace lowering {

LivenessAnalysis::LivenessAnalysis(
    const std::vector<arm64::Instruction> &instructions)
    : instructions(instructions) {}

void LivenessAnalysis::computeUseDef() {
  defSites.clear();
  useSites.clear();

  // Process instructions in order
  for (size_t i = 0; i < instructions.size(); ++i) {
    const auto &inst = instructions[i];

    // Record definition sites
    auto definedVRegs = getDefinedVirtualRegisters(inst);
    for (uint32_t vreg : definedVRegs) {
      defSites[vreg] = i;
    }

    // Record use sites
    auto usedVRegs = getUsedVirtualRegisters(inst);
    for (uint32_t vreg : usedVRegs) {
      useSites[vreg].push_back(i);
    }
  }
}

std::vector<LiveInterval> LivenessAnalysis::computeLiveIntervals() {
  computeUseDef();

  std::vector<LiveInterval> intervals;

  // Create intervals for each virtual register
  for (const auto &[vreg, defSite] : defSites) {
    LiveInterval interval;
    interval.virtualRegister = vreg;
    interval.start = defSite;

    // Find the last use of this virtual register
    interval.end = defSite; // At minimum, live at definition

    auto useIt = useSites.find(vreg);
    if (useIt != useSites.end() && !useIt->second.empty()) {
      // Extend to the last use
      interval.end =
          *std::max_element(useIt->second.begin(), useIt->second.end());
    }

    intervals.push_back(interval);
  }

  // Sort intervals by start position for linear scan
  std::sort(intervals.begin(), intervals.end(),
            [](const LiveInterval &a, const LiveInterval &b) {
              return a.start < b.start;
            });

  return intervals;
}

std::set<uint32_t> LivenessAnalysis::getLiveAtIndex(size_t index) const {
  std::set<uint32_t> liveVRegs;

  // A virtual register is live at index if it's defined before or at index
  // and used at or after index
  for (const auto &[vreg, defSite] : defSites) {
    if (defSite <= index) {
      // Check if it's used after this index
      auto useIt = useSites.find(vreg);
      if (useIt != useSites.end()) {
        for (size_t useSite : useIt->second) {
          if (useSite >= index) {
            liveVRegs.insert(vreg);
            break;
          }
        }
      }
    }
  }

  return liveVRegs;
}

std::set<uint32_t> LivenessAnalysis::getUsedVirtualRegisters(
    const arm64::Instruction &inst) const {
  std::set<uint32_t> usedVRegs;

  std::visit(
      [&](const auto &instKind) {
        using T = std::decay_t<decltype(instKind)>;

        if constexpr (std::is_same_v<T, arm64::ThreeOperandInst>) {
          if (auto vreg = getVirtualRegisterFromOperand(instKind.src1)) {
            usedVRegs.insert(*vreg);
          }
          if (auto vreg = getVirtualRegisterFromOperand(instKind.src2)) {
            usedVRegs.insert(*vreg);
          }
        } else if constexpr (std::is_same_v<T, arm64::TwoOperandInst>) {
          if (auto vreg = getVirtualRegisterFromOperand(instKind.src)) {
            usedVRegs.insert(*vreg);
          }
        } else if constexpr (std::is_same_v<T, arm64::MemoryInst>) {
          if (auto vreg = getVirtualRegisterFromOperand(instKind.baseReg)) {
            usedVRegs.insert(*vreg);
          }
          // For store instructions, the reg operand is a source (value being
          // stored)
          if (instKind.opcode == arm64::Opcode::STR) {
            if (auto vreg = getVirtualRegisterFromOperand(instKind.reg)) {
              usedVRegs.insert(*vreg);
            }
          }
        }
        // BranchInst doesn't use virtual registers
      },
      inst.kind);

  return usedVRegs;
}

std::set<uint32_t> LivenessAnalysis::getDefinedVirtualRegisters(
    const arm64::Instruction &inst) const {
  std::set<uint32_t> definedVRegs;

  std::visit(
      [&](const auto &instKind) {
        using T = std::decay_t<decltype(instKind)>;

        if constexpr (std::is_same_v<T, arm64::ThreeOperandInst>) {
          if (auto vreg = getVirtualRegisterFromOperand(instKind.dest)) {
            definedVRegs.insert(*vreg);
          }
        } else if constexpr (std::is_same_v<T, arm64::TwoOperandInst>) {
          if (auto vreg = getVirtualRegisterFromOperand(instKind.dest)) {
            definedVRegs.insert(*vreg);
          }
        } else if constexpr (std::is_same_v<T, arm64::MemoryInst>) {
          // For load instructions, the reg operand is a destination
          if (instKind.opcode == arm64::Opcode::LDR) {
            if (auto vreg = getVirtualRegisterFromOperand(instKind.reg)) {
              definedVRegs.insert(*vreg);
            }
          }
        }
        // BranchInst doesn't define virtual registers
      },
      inst.kind);

  return definedVRegs;
}

std::optional<uint32_t> LivenessAnalysis::getVirtualRegisterFromOperand(
    const arm64::Operand &operand) const {
  if (std::holds_alternative<arm64::VirtualReg>(operand)) {
    return std::get<arm64::VirtualReg>(operand).id;
  }
  return std::nullopt;
}

} // namespace lowering
} // namespace dinorisc