#include "InstructionSelector.h"

namespace dinorisc {
namespace lowering {

InstructionSelector::InstructionSelector() : nextVirtualReg(0) {}

std::vector<arm64::Instruction>
InstructionSelector::selectInstructions(const ir::BasicBlock &block) {
  std::vector<arm64::Instruction> result;

  // Select instructions for each IR instruction
  for (const auto &inst : block.instructions) {
    auto selected = selectInstruction(inst);
    result.insert(result.end(), selected.begin(), selected.end());
  }

  // Select instructions for the terminator
  auto termSelected = selectTerminator(block.terminator);
  result.insert(result.end(), termSelected.begin(), termSelected.end());

  return result;
}

VirtualRegister
InstructionSelector::getVirtualRegister(ir::ValueId valueId) const {
  auto it = irToVReg.find(valueId);
  if (it != irToVReg.end()) {
    return it->second;
  }
  return 0; // Invalid register
}

VirtualRegister
InstructionSelector::assignVirtualRegister(ir::ValueId valueId) {
  auto it = irToVReg.find(valueId);
  if (it != irToVReg.end()) {
    return it->second;
  }

  VirtualRegister vreg = nextVirtualReg++;
  irToVReg[valueId] = vreg;
  vregToIR[vreg] = valueId;
  return vreg;
}

void InstructionSelector::recordValueType(ir::ValueId valueId, ir::Type type) {
  valueTypes[valueId] = type;
}

ir::Type InstructionSelector::getValueType(ir::ValueId valueId) const {
  auto it = valueTypes.find(valueId);
  if (it != valueTypes.end()) {
    return it->second;
  }
  return ir::Type::i64; // Default fallback
}

std::vector<arm64::Instruction>
InstructionSelector::selectInstruction(const ir::Instruction &inst) {
  std::vector<arm64::Instruction> result;

  std::visit(
      [&](const auto &instKind) {
        using T = std::decay_t<decltype(instKind)>;

        if constexpr (std::is_same_v<T, ir::BinaryOp>) {
          recordValueType(inst.valueId, instKind.type);
          result.push_back(selectBinaryOp(instKind, inst.valueId));
        } else if constexpr (std::is_same_v<T, ir::Load>) {
          recordValueType(inst.valueId, instKind.type);
          result.push_back(selectLoad(instKind, inst.valueId));
        } else if constexpr (std::is_same_v<T, ir::Store>) {
          result.push_back(selectStore(instKind));
        } else if constexpr (std::is_same_v<T, ir::Const>) {
          recordValueType(inst.valueId, instKind.type);
          result.push_back(selectConst(instKind, inst.valueId));
        } else if constexpr (std::is_same_v<T, ir::Sext>) {
          recordValueType(inst.valueId, instKind.toType);
          result.push_back(selectSext(instKind, inst.valueId));
        } else if constexpr (std::is_same_v<T, ir::Zext>) {
          recordValueType(inst.valueId, instKind.toType);
          result.push_back(selectZext(instKind, inst.valueId));
        } else if constexpr (std::is_same_v<T, ir::Trunc>) {
          recordValueType(inst.valueId, instKind.toType);
          result.push_back(selectTrunc(instKind, inst.valueId));
        }
      },
      inst.kind);

  return result;
}

std::vector<arm64::Instruction>
InstructionSelector::selectTerminator(const ir::Terminator &term) {
  std::vector<arm64::Instruction> result;

  std::visit(
      [&](const auto &termKind) {
        using T = std::decay_t<decltype(termKind)>;

        if constexpr (std::is_same_v<T, ir::Branch>) {
          arm64::Instruction branch;
          branch.kind =
              arm64::BranchInst{arm64::Opcode::B, termKind.targetBlock};
          result.push_back(branch);
        } else if constexpr (std::is_same_v<T, ir::CondBranch>) {
          // Generate compare and conditional branch
          VirtualRegister condReg = getVirtualRegister(termKind.condition);

          // Compare against zero
          arm64::Instruction cmp;
          cmp.kind = arm64::ThreeOperandInst{
              arm64::Opcode::CMP, arm64::DataSize::X,
              arm64::VirtualReg{condReg},
              arm64::VirtualReg{condReg}, // Compare reg with itself for now
              arm64::Immediate{0}};
          result.push_back(cmp);

          // Branch if not equal (true case)
          arm64::Instruction branchTrue;
          branchTrue.kind =
              arm64::BranchInst{arm64::Opcode::B_NE, termKind.trueBlock};
          result.push_back(branchTrue);

          // Fall through to false block (or explicit branch)
          arm64::Instruction branchFalse;
          branchFalse.kind =
              arm64::BranchInst{arm64::Opcode::B, termKind.falseBlock};
          result.push_back(branchFalse);
        } else if constexpr (std::is_same_v<T, ir::Return>) {
          if (termKind.value) {
            // Move return value to x0
            VirtualRegister retReg = getVirtualRegister(*termKind.value);
            arm64::Instruction mov;
            mov.kind = arm64::TwoOperandInst{
                arm64::Opcode::MOV, arm64::DataSize::X, arm64::Register::X0,
                arm64::VirtualReg{retReg}};
            result.push_back(mov);
          }

          arm64::Instruction ret;
          ret.kind = arm64::TwoOperandInst{
              arm64::Opcode::RET, arm64::DataSize::X,
              arm64::Register::X30, // Link register
              arm64::Register::X30 // Not used for RET but required by structure
          };
          result.push_back(ret);
        }
      },
      term.kind);

  return result;
}

arm64::Instruction
InstructionSelector::selectBinaryOp(const ir::BinaryOp &binOp,
                                    ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);
  VirtualRegister lhsReg = getVirtualRegister(binOp.lhs);
  VirtualRegister rhsReg = getVirtualRegister(binOp.rhs);

  arm64::Instruction inst;
  inst.kind = arm64::ThreeOperandInst{
      irBinaryOpToARM64(binOp.opcode), irTypeToDataSize(binOp.type),
      arm64::VirtualReg{destReg}, arm64::VirtualReg{lhsReg},
      arm64::VirtualReg{rhsReg}};

  return inst;
}

arm64::Instruction InstructionSelector::selectLoad(const ir::Load &load,
                                                   ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);
  VirtualRegister addrReg = getVirtualRegister(load.address);

  arm64::Instruction inst;
  inst.kind = arm64::MemoryInst{
      arm64::Opcode::LDR, irTypeToDataSize(load.type),
      arm64::VirtualReg{destReg}, arm64::VirtualReg{addrReg},
      0 // No offset for now
  };

  return inst;
}

arm64::Instruction InstructionSelector::selectStore(const ir::Store &store) {
  VirtualRegister valueReg = getVirtualRegister(store.value);
  VirtualRegister addrReg = getVirtualRegister(store.address);

  ir::Type valueType = getValueType(store.value);

  arm64::Instruction inst;
  inst.kind = arm64::MemoryInst{
      arm64::Opcode::STR, irTypeToDataSize(valueType),
      arm64::VirtualReg{valueReg}, arm64::VirtualReg{addrReg},
      0 // No offset for now
  };

  return inst;
}

arm64::Instruction InstructionSelector::selectConst(const ir::Const &constInst,
                                                    ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);

  arm64::Instruction inst;
  inst.kind = arm64::TwoOperandInst{
      arm64::Opcode::MOV, irTypeToDataSize(constInst.type),
      arm64::VirtualReg{destReg}, arm64::Immediate{constInst.value}};

  return inst;
}

arm64::Instruction InstructionSelector::selectSext(const ir::Sext &sext,
                                                   ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);
  VirtualRegister srcReg = getVirtualRegister(sext.operand);

  arm64::Opcode opcode;

  // Select appropriate sign extension instruction based on source type
  ir::Type fromType = getValueType(sext.operand);
  switch (fromType) {
  case ir::Type::i8:
    opcode = arm64::Opcode::SXTB;
    break;
  case ir::Type::i16:
    opcode = arm64::Opcode::SXTH;
    break;
  case ir::Type::i32:
    opcode = arm64::Opcode::SXTW;
    break;
  default:
    // For i1 or other cases, use MOV as fallback
    opcode = arm64::Opcode::MOV;
    break;
  }

  arm64::Instruction inst;
  inst.kind = arm64::TwoOperandInst{opcode, irTypeToDataSize(sext.toType),
                                    arm64::VirtualReg{destReg},
                                    arm64::VirtualReg{srcReg}};

  return inst;
}

arm64::Instruction InstructionSelector::selectZext(const ir::Zext &zext,
                                                   ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);
  VirtualRegister srcReg = getVirtualRegister(zext.operand);

  arm64::Opcode opcode;

  // Select appropriate zero extension instruction based on source type
  ir::Type fromType = getValueType(zext.operand);
  switch (fromType) {
  case ir::Type::i8:
    opcode = arm64::Opcode::UXTB;
    break;
  case ir::Type::i16:
    opcode = arm64::Opcode::UXTH;
    break;
  case ir::Type::i32:
    // For i32 -> i64, MOV with W register automatically zero-extends
    opcode = arm64::Opcode::MOV;
    break;
  default:
    // For i1 or other cases, use MOV as fallback
    opcode = arm64::Opcode::MOV;
    break;
  }

  arm64::Instruction inst;
  inst.kind = arm64::TwoOperandInst{opcode, irTypeToDataSize(zext.toType),
                                    arm64::VirtualReg{destReg},
                                    arm64::VirtualReg{srcReg}};

  return inst;
}

arm64::Instruction InstructionSelector::selectTrunc(const ir::Trunc &trunc,
                                                    ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);
  VirtualRegister srcReg = getVirtualRegister(trunc.operand);

  arm64::Instruction inst;
  inst.kind = arm64::TwoOperandInst{
      arm64::Opcode::MOV, irTypeToDataSize(trunc.toType),
      arm64::VirtualReg{destReg}, arm64::VirtualReg{srcReg}};

  return inst;
}

arm64::DataSize InstructionSelector::irTypeToDataSize(ir::Type type) const {
  switch (type) {
  case ir::Type::i1:
  case ir::Type::i8:
    return arm64::DataSize::B;
  case ir::Type::i16:
    return arm64::DataSize::H;
  case ir::Type::i32:
    return arm64::DataSize::W;
  case ir::Type::i64:
    return arm64::DataSize::X;
  }
  return arm64::DataSize::X;
}

arm64::Opcode
InstructionSelector::irBinaryOpToARM64(ir::BinaryOpcode opcode) const {
  switch (opcode) {
  case ir::BinaryOpcode::Add:
    return arm64::Opcode::ADD;
  case ir::BinaryOpcode::Sub:
    return arm64::Opcode::SUB;
  case ir::BinaryOpcode::Mul:
    return arm64::Opcode::MUL;
  case ir::BinaryOpcode::DivU:
    return arm64::Opcode::UDIV;
  case ir::BinaryOpcode::Div:
    return arm64::Opcode::SDIV;
  case ir::BinaryOpcode::And:
    return arm64::Opcode::AND;
  case ir::BinaryOpcode::Or:
    return arm64::Opcode::ORR;
  case ir::BinaryOpcode::Xor:
    return arm64::Opcode::EOR;
  case ir::BinaryOpcode::Shl:
    return arm64::Opcode::LSL;
  case ir::BinaryOpcode::Shr:
    return arm64::Opcode::LSR;
  case ir::BinaryOpcode::Sar:
    return arm64::Opcode::ASR;
  default:
    // For comparison operations, we'll need to handle them differently
    return arm64::Opcode::ADD; // Placeholder
  }
}

} // namespace lowering
} // namespace dinorisc