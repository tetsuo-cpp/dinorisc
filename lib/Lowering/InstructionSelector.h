#pragma once

#include "../ARM64/Instruction.h"
#include "../IR/IR.h"
#include <unordered_map>
#include <vector>

namespace dinorisc {
namespace lowering {

using VirtualRegister = arm64::VirtualRegister;

class InstructionSelector {
public:
  explicit InstructionSelector();

  // Select ARM64 instructions for an IR basic block
  std::vector<arm64::Instruction>
  selectInstructions(const ir::BasicBlock &block);

  // Get the virtual register assigned to an IR value
  VirtualRegister getVirtualRegister(ir::ValueId valueId) const;

  // Get mapping from virtual registers back to IR values (computed on demand)
  std::unordered_map<VirtualRegister, ir::ValueId> getVRegToIRMapping() const;

private:
  VirtualRegister nextVirtualReg;
  std::unordered_map<ir::ValueId, VirtualRegister> irToVReg;
  std::unordered_map<ir::ValueId, ir::Type> valueTypes;

  // Assign a virtual register to an IR value
  VirtualRegister assignVirtualRegister(ir::ValueId valueId);

  // Track and retrieve value types
  void recordValueType(ir::ValueId valueId, ir::Type type);
  ir::Type getValueType(ir::ValueId valueId) const;

  // Convert IR instruction to ARM64 instructions
  std::vector<arm64::Instruction>
  selectInstruction(const ir::Instruction &inst);

  // Convert IR terminator to ARM64 instructions
  std::vector<arm64::Instruction> selectTerminator(const ir::Terminator &term);

  // Helper functions for specific instruction types
  arm64::Instruction selectBinaryOp(const ir::BinaryOp &binOp,
                                    ir::ValueId resultId);
  arm64::Instruction selectLoad(const ir::Load &load, ir::ValueId resultId);
  arm64::Instruction selectStore(const ir::Store &store);
  arm64::Instruction selectConst(const ir::Const &constInst,
                                 ir::ValueId resultId);
  arm64::Instruction selectSext(const ir::Sext &sext, ir::ValueId resultId);
  arm64::Instruction selectZext(const ir::Zext &zext, ir::ValueId resultId);
  arm64::Instruction selectTrunc(const ir::Trunc &trunc, ir::ValueId resultId);
  arm64::Instruction selectRegRead(const ir::RegRead &regRead,
                                   ir::ValueId resultId);
  arm64::Instruction selectRegWrite(const ir::RegWrite &regWrite);

  // Convert IR types to ARM64 data sizes
  arm64::DataSize irTypeToDataSize(ir::Type type) const;

  // Convert binary opcodes
  arm64::Opcode irBinaryOpToARM64(ir::BinaryOpcode opcode) const;
};

} // namespace lowering
} // namespace dinorisc