#pragma once

#include "Instruction.h"
#include <cstdint>
#include <vector>

namespace dinorisc {
namespace arm64 {

class Encoder {
public:
  std::vector<uint8_t> encodeInstruction(const Instruction &inst);

private:
  uint32_t encodeThreeOperandInst(const ThreeOperandInst &inst);
  uint32_t encodeTwoOperandInst(const TwoOperandInst &inst);
  uint32_t encodeMemoryInst(const MemoryInst &inst);
  uint32_t encodeMoveWideInst(const MoveWideInst &inst);
  uint32_t encodeBranchInst(const BranchInst &inst);
  uint32_t encodeConditionalInst(const ConditionalInst &inst);
  uint32_t encodeConditionalSelectInst(const ConditionalSelectInst &inst);

  uint32_t encodeRegister(const Operand &operand);
  bool isImmediate(const Operand &operand);

  uint32_t getSfBit(DataSize size);
  uint32_t getConditionCode(Opcode opcode);
};

} // namespace arm64
} // namespace dinorisc