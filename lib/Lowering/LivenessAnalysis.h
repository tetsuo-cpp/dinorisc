#pragma once

#include "../ARM64/Instruction.h"
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

namespace dinorisc {
namespace lowering {

struct LiveInterval {
  uint32_t virtualRegister;
  size_t start;
  size_t end;

  bool overlaps(const LiveInterval &other) const {
    return !(end < other.start || start > other.end);
  }
};

class LivenessAnalysis {
public:
  explicit LivenessAnalysis(
      const std::vector<arm64::Instruction> &instructions);

  // Compute use-def information for all virtual registers
  void computeUseDef();

  // Calculate live intervals for each virtual register
  std::vector<LiveInterval> computeLiveIntervals();

  // Get the set of virtual registers that are live at a specific instruction
  // index
  std::set<uint32_t> getLiveAtIndex(size_t index) const;

private:
  const std::vector<arm64::Instruction> &instructions;

  // Use-def information
  std::unordered_map<uint32_t, size_t>
      defSites; // Where each virtual register is defined
  std::unordered_map<uint32_t, std::vector<size_t>>
      useSites; // Where each virtual register is used

  // Collect all virtual registers used by an instruction
  std::set<uint32_t>
  getUsedVirtualRegisters(const arm64::Instruction &inst) const;

  // Collect all virtual registers defined by an instruction
  std::set<uint32_t>
  getDefinedVirtualRegisters(const arm64::Instruction &inst) const;

  // Extract virtual register from operand if it is one
  std::optional<uint32_t>
  getVirtualRegisterFromOperand(const arm64::Operand &operand) const;
};

} // namespace lowering
} // namespace dinorisc