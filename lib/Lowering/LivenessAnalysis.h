#pragma once

#include "../IR/IR.h"
#include <set>
#include <unordered_map>
#include <vector>

namespace dinorisc {
namespace lowering {

struct LiveInterval {
  ir::ValueId valueId;
  size_t start;
  size_t end;

  bool overlaps(const LiveInterval &other) const {
    return !(end < other.start || start > other.end);
  }
};

class LivenessAnalysis {
public:
  explicit LivenessAnalysis(const ir::BasicBlock &block);

  // Compute use-def information for all values in the block
  void computeUseDef();

  // Calculate live intervals for each value
  std::vector<LiveInterval> computeLiveIntervals();

  // Get the set of values that are live at a specific instruction index
  std::set<ir::ValueId> getLiveAtIndex(size_t index) const;

private:
  const ir::BasicBlock &block;

  // Use-def information
  std::unordered_map<ir::ValueId, size_t>
      defSites; // Where each value is defined
  std::unordered_map<ir::ValueId, std::vector<size_t>>
      useSites; // Where each value is used

  // Collect all values used by an instruction
  std::set<ir::ValueId> getUsedValues(const ir::Instruction &inst) const;

  // Check if a value is used by the terminator
  bool isUsedByTerminator(ir::ValueId valueId) const;
};

} // namespace lowering
} // namespace dinorisc