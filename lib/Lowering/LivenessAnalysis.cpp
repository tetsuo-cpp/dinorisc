#include "LivenessAnalysis.h"

namespace dinorisc {
namespace lowering {

LivenessAnalysis::LivenessAnalysis(const ir::BasicBlock &block)
    : block(block) {}

void LivenessAnalysis::computeUseDef() {
  defSites.clear();
  useSites.clear();

  // Process instructions in order
  for (size_t i = 0; i < block.instructions.size(); ++i) {
    const auto &inst = block.instructions[i];

    // Record definition site
    defSites[inst.valueId] = i;

    // Collect used values and record use sites
    auto usedValues = getUsedValues(inst);
    for (ir::ValueId valueId : usedValues) {
      useSites[valueId].push_back(i);
    }
  }

  // Check terminator for additional uses
  std::visit(
      [&](const auto &term) {
        using T = std::decay_t<decltype(term)>;

        if constexpr (std::is_same_v<T, ir::CondBranch>) {
          useSites[term.condition].push_back(block.instructions.size());
        } else if constexpr (std::is_same_v<T, ir::Return>) {
          if (term.hasValue) {
            useSites[term.value].push_back(block.instructions.size());
          }
        }
      },
      block.terminator.kind);
}

std::vector<LiveInterval> LivenessAnalysis::computeLiveIntervals() {
  computeUseDef();

  std::vector<LiveInterval> intervals;

  // Create intervals for each value
  for (const auto &[valueId, defSite] : defSites) {
    LiveInterval interval;
    interval.valueId = valueId;
    interval.start = defSite;

    // Find the last use of this value
    interval.end = defSite; // At minimum, live at definition

    auto useIt = useSites.find(valueId);
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

std::set<ir::ValueId> LivenessAnalysis::getLiveAtIndex(size_t index) const {
  std::set<ir::ValueId> liveValues;

  // A value is live at index if it's defined before or at index
  // and used at or after index
  for (const auto &[valueId, defSite] : defSites) {
    if (defSite <= index) {
      // Check if it's used after this index
      auto useIt = useSites.find(valueId);
      if (useIt != useSites.end()) {
        for (size_t useSite : useIt->second) {
          if (useSite >= index) {
            liveValues.insert(valueId);
            break;
          }
        }
      }
    }
  }

  return liveValues;
}

std::set<ir::ValueId>
LivenessAnalysis::getUsedValues(const ir::Instruction &inst) const {
  std::set<ir::ValueId> usedValues;

  std::visit(
      [&](const auto &instKind) {
        using T = std::decay_t<decltype(instKind)>;

        if constexpr (std::is_same_v<T, ir::BinaryOp>) {
          usedValues.insert(instKind.lhs);
          usedValues.insert(instKind.rhs);
        } else if constexpr (std::is_same_v<T, ir::Sext>) {
          usedValues.insert(instKind.operand);
        } else if constexpr (std::is_same_v<T, ir::Zext>) {
          usedValues.insert(instKind.operand);
        } else if constexpr (std::is_same_v<T, ir::Trunc>) {
          usedValues.insert(instKind.operand);
        } else if constexpr (std::is_same_v<T, ir::Load>) {
          usedValues.insert(instKind.address);
        } else if constexpr (std::is_same_v<T, ir::Store>) {
          usedValues.insert(instKind.value);
          usedValues.insert(instKind.address);
        }
        // Const doesn't use any values
      },
      inst.kind);

  return usedValues;
}

bool LivenessAnalysis::isUsedByTerminator(ir::ValueId valueId) const {
  bool used = false;

  std::visit(
      [&](const auto &term) {
        using T = std::decay_t<decltype(term)>;

        if constexpr (std::is_same_v<T, ir::CondBranch>) {
          if (term.condition == valueId) {
            used = true;
          }
        } else if constexpr (std::is_same_v<T, ir::Return>) {
          if (term.hasValue && term.value == valueId) {
            used = true;
          }
        }
      },
      block.terminator.kind);

  return used;
}

} // namespace lowering
} // namespace dinorisc