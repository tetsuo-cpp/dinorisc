#pragma once

#include "IR/IR.h"
#include "RISCV/Instruction.h"
#include <array>
#include <vector>

namespace dinorisc {

class Lifter {
public:
  Lifter();

  // Lift a basic block of RISC-V instructions to IR basic block
  ir::BasicBlock
  liftBasicBlock(const std::vector<riscv::Instruction> &instructions);

  // Get the current IR value for a RISC-V register
  ir::ValueId getRegisterValue(uint32_t regNum);

  // Check if an instruction is a control flow terminator
  bool isTerminator(const riscv::Instruction &inst);

private:
  // SSA value ID counter
  ir::ValueId nextValueId;

  // Map RISC-V registers (x0-x31) to current IR value IDs
  std::array<ir::ValueId, 32> registerValues;

  // Helper methods for creating IR instructions
  ir::ValueId createConstant(ir::Type type, uint64_t value);
  ir::ValueId createBinaryOp(ir::BinaryOpcode opcode, ir::Type type,
                             ir::ValueId lhs, ir::ValueId rhs);
  ir::ValueId createLoad(ir::Type type, ir::ValueId address);
  ir::ValueId createStore(ir::ValueId value, ir::ValueId address);
  ir::ValueId createSext(ir::Type fromType, ir::Type toType,
                         ir::ValueId operand);
  ir::ValueId createZext(ir::Type fromType, ir::Type toType,
                         ir::ValueId operand);
  ir::ValueId createTrunc(ir::Type fromType, ir::Type toType,
                          ir::ValueId operand);

  // Set the IR value for a RISC-V register
  void setRegisterValue(uint32_t regNum, ir::ValueId valueId);

  // Current instruction list being built
  std::vector<ir::Instruction> currentInstructions;

  // Helper method to add instruction to current list and return its value ID
  ir::ValueId addInstruction(ir::InstructionKind kind);

  // Control flow helpers
  ir::Terminator liftTerminator(const riscv::Instruction &inst,
                                uint64_t fallThroughAddress);
  void liftSingleInstruction(const riscv::Instruction &inst);

  // Terminator creation helpers
  ir::Terminator createConditionalBranch(ir::BinaryOpcode compareOp,
                                         const riscv::Instruction &inst,
                                         uint64_t fallThroughAddress);
  uint64_t calculateBranchTarget(const riscv::Instruction &inst);
};

} // namespace dinorisc