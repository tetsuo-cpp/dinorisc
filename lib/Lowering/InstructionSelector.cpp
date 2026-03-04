#include "InstructionSelector.h"
#include "../Error.h"
#include "../GuestState.h"
#include <cstddef>

namespace dinorisc {
namespace lowering {

namespace {

std::optional<arm64::Condition> comparisonCondition(ir::BinaryOpcode opcode) {
  switch (opcode) {
  case ir::BinaryOpcode::Eq:
    return arm64::Condition::EQ;
  case ir::BinaryOpcode::Ne:
    return arm64::Condition::NE;
  case ir::BinaryOpcode::Lt:
    return arm64::Condition::LT;
  case ir::BinaryOpcode::Le:
    return arm64::Condition::LE;
  case ir::BinaryOpcode::Gt:
    return arm64::Condition::GT;
  case ir::BinaryOpcode::Ge:
    return arm64::Condition::GE;
  case ir::BinaryOpcode::LtU:
    return arm64::Condition::CC;
  case ir::BinaryOpcode::LeU:
    return arm64::Condition::LS;
  case ir::BinaryOpcode::GtU:
    return arm64::Condition::HI;
  case ir::BinaryOpcode::GeU:
    return arm64::Condition::CS;
  default:
    return std::nullopt;
  }
}

} // namespace

InstructionSelector::InstructionSelector() : nextVirtualReg(0) {}

std::vector<arm64::Instruction>
InstructionSelector::selectInstructions(const ir::BasicBlock &block) {
  std::vector<arm64::Instruction> result;

  for (const auto &inst : block.instructions) {
    auto selected = selectInstruction(inst);
    result.insert(result.end(), selected.begin(), selected.end());
  }

  auto termSelected = selectTerminator(block.terminator);
  result.insert(result.end(), termSelected.begin(), termSelected.end());

  return result;
}

std::optional<VirtualRegister>
InstructionSelector::getVirtualRegister(ir::ValueId valueId) const {
  auto it = irToVReg.find(valueId);
  if (it != irToVReg.end()) {
    return it->second;
  }
  return std::nullopt;
}

VirtualRegister
InstructionSelector::getVirtualRegisterOrThrow(ir::ValueId valueId) const {
  auto vreg = getVirtualRegister(valueId);
  if (vreg.has_value()) {
    return vreg.value();
  }
  throw LoweringError("No virtual register assigned to IR value " +
                      std::to_string(valueId));
}

VirtualRegister
InstructionSelector::assignVirtualRegister(ir::ValueId valueId) {
  auto existing = getVirtualRegister(valueId);
  if (existing.has_value()) {
    return existing.value();
  }

  VirtualRegister vreg = nextVirtualReg++;
  irToVReg[valueId] = vreg;
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
          auto binOpInsts = selectBinaryOp(instKind, inst.valueId);
          result.insert(result.end(), binOpInsts.begin(), binOpInsts.end());
        } else if constexpr (std::is_same_v<T, ir::Load>) {
          recordValueType(inst.valueId, instKind.type);
          auto loadInsts = selectLoad(instKind, inst.valueId);
          result.insert(result.end(), loadInsts.begin(), loadInsts.end());
        } else if constexpr (std::is_same_v<T, ir::Store>) {
          auto storeInsts = selectStore(instKind);
          result.insert(result.end(), storeInsts.begin(), storeInsts.end());
        } else if constexpr (std::is_same_v<T, ir::Const>) {
          recordValueType(inst.valueId, instKind.type);
          auto constInsts = selectConst(instKind, inst.valueId);
          result.insert(result.end(), constInsts.begin(), constInsts.end());
        } else if constexpr (std::is_same_v<T, ir::Sext>) {
          recordValueType(inst.valueId, instKind.toType);
          result.push_back(selectSext(instKind, inst.valueId));
        } else if constexpr (std::is_same_v<T, ir::Zext>) {
          recordValueType(inst.valueId, instKind.toType);
          result.push_back(selectZext(instKind, inst.valueId));
        } else if constexpr (std::is_same_v<T, ir::Trunc>) {
          recordValueType(inst.valueId, instKind.toType);
          result.push_back(selectTrunc(instKind, inst.valueId));
        } else if constexpr (std::is_same_v<T, ir::RegRead>) {
          recordValueType(inst.valueId, ir::Type::i64);
          result.push_back(selectRegRead(instKind, inst.valueId));
        } else if constexpr (std::is_same_v<T, ir::RegWrite>) {
          result.push_back(selectRegWrite(instKind));
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
          ir::Const targetConst{ir::Type::i64,
                                static_cast<int64_t>(termKind.targetBlock)};
          auto constInsts =
              selectConstIntoRegister(targetConst, arm64::Register::X0);
          result.insert(result.end(), constInsts.begin(), constInsts.end());

          arm64::Instruction ret;
          ret.kind =
              arm64::TwoOperandInst{arm64::Opcode::RET, arm64::DataSize::X,
                                    arm64::Register::X30, // Link register
                                    arm64::Register::X30};
          result.push_back(ret);
        } else if constexpr (std::is_same_v<T, ir::CondBranch>) {
          auto condBranchInsts = selectCondBranch(termKind);
          result.insert(result.end(), condBranchInsts.begin(),
                        condBranchInsts.end());
        } else if constexpr (std::is_same_v<T, ir::Return>) {
          if (termKind.value.has_value()) {
            VirtualRegister srcReg =
                getVirtualRegisterOrThrow(termKind.value.value());
            arm64::Instruction moveReturn;
            moveReturn.kind =
                arm64::TwoOperandInst{arm64::Opcode::MOV, arm64::DataSize::X,
                                      arm64::Register::X0, srcReg};
            result.push_back(moveReturn);
          } else {
            // next-PC = 0 so the dispatcher stops
            ir::Const zero{ir::Type::i64, 0};
            auto zeroInsts = selectConstIntoRegister(zero, arm64::Register::X0);
            result.insert(result.end(), zeroInsts.begin(), zeroInsts.end());
          }

          arm64::Instruction ret;
          ret.kind =
              arm64::TwoOperandInst{arm64::Opcode::RET, arm64::DataSize::X,
                                    arm64::Register::X30, // Link register
                                    arm64::Register::X30};
          result.push_back(ret);
        }
      },
      term.kind);

  return result;
}

std::vector<arm64::Instruction>
InstructionSelector::selectCondBranch(const ir::CondBranch &condBranch) {
  std::vector<arm64::Instruction> result;

  // Use conditional select to choose target PC
  VirtualRegister condReg = getVirtualRegisterOrThrow(condBranch.condition);

  // Compare condition against zero
  arm64::Instruction cmp;
  cmp.kind = arm64::TwoOperandInst{arm64::Opcode::CMP, arm64::DataSize::X,
                                   condReg, arm64::Immediate{0}};
  result.push_back(cmp);

  // Load true target into temporary register
  // Create a unique IR value ID for the true target constant
  ir::ValueId trueValueId = nextVirtualReg++; // Use virtual reg as unique ID
  VirtualRegister trueTempReg = assignVirtualRegister(trueValueId);
  ir::Const trueConst{ir::Type::i64,
                      static_cast<int64_t>(condBranch.trueBlock)};
  auto trueInsts = selectConst(trueConst, trueValueId);
  result.insert(result.end(), trueInsts.begin(), trueInsts.end());

  // Load false target into temporary register
  // Create a unique IR value ID for the false target constant
  ir::ValueId falseValueId = nextVirtualReg++; // Use virtual reg as unique ID
  VirtualRegister falseTempReg = assignVirtualRegister(falseValueId);
  ir::Const falseConst{ir::Type::i64,
                       static_cast<int64_t>(condBranch.falseBlock)};
  auto falseInsts = selectConst(falseConst, falseValueId);
  result.insert(result.end(), falseInsts.begin(), falseInsts.end());

  // Conditional select: choose true target if condition != 0 (NE)
  VirtualRegister targetReg = nextVirtualReg++;
  arm64::Instruction csel;
  csel.kind = arm64::ConditionalSelectInst{
      arm64::Opcode::CSEL, arm64::DataSize::X, targetReg,
      trueTempReg,         falseTempReg,       arm64::Condition::NE};
  result.push_back(csel);

  // Move selected target to X0 for branch
  arm64::Instruction movTarget;
  movTarget.kind = arm64::TwoOperandInst{arm64::Opcode::MOV, arm64::DataSize::X,
                                         arm64::Register::X0, targetReg};
  result.push_back(movTarget);

  arm64::Instruction ret;
  ret.kind = arm64::TwoOperandInst{arm64::Opcode::RET, arm64::DataSize::X,
                                   arm64::Register::X30, // Link register
                                   arm64::Register::X30};
  result.push_back(ret);

  return result;
}

std::vector<arm64::Instruction>
InstructionSelector::selectBinaryOp(const ir::BinaryOp &binOp,
                                    ir::ValueId resultId) {
  std::vector<arm64::Instruction> result;
  VirtualRegister destReg = assignVirtualRegister(resultId);
  VirtualRegister lhsReg = getVirtualRegisterOrThrow(binOp.lhs);
  VirtualRegister rhsReg = getVirtualRegisterOrThrow(binOp.rhs);

  if (auto condition = comparisonCondition(binOp.opcode)) {
    arm64::Instruction cmp;
    cmp.kind = arm64::TwoOperandInst{arm64::Opcode::CMP, arm64::DataSize::X,
                                     lhsReg, rhsReg};
    result.push_back(cmp);

    arm64::Instruction cset;
    cset.kind = arm64::ConditionalInst{arm64::Opcode::CSET, arm64::DataSize::X,
                                       destReg, *condition};
    result.push_back(cset);
  } else {
    arm64::Instruction inst;
    inst.kind = arm64::ThreeOperandInst{irBinaryOpToARM64(binOp.opcode),
                                        irTypeToDataSize(binOp.type), destReg,
                                        lhsReg, rhsReg};
    result.push_back(inst);
  }

  return result;
}

std::vector<arm64::Instruction>
InstructionSelector::selectLoad(const ir::Load &load, ir::ValueId resultId) {
  std::vector<arm64::Instruction> result;

  VirtualRegister destReg = assignVirtualRegister(resultId);
  VirtualRegister guestAddrReg = getVirtualRegisterOrThrow(load.address);

  // Perform address translation
  VirtualRegister hostAddrReg;
  auto addrTranslation = generateAddressTranslation(guestAddrReg, hostAddrReg);
  result.insert(result.end(), addrTranslation.begin(), addrTranslation.end());

  // LDR dest, [hostAddrReg]
  arm64::Instruction ldr;
  ldr.kind = arm64::MemoryInst{arm64::Opcode::LDR, irTypeToDataSize(load.type),
                               destReg, hostAddrReg, 0};
  result.push_back(ldr);

  return result;
}

std::vector<arm64::Instruction>
InstructionSelector::selectStore(const ir::Store &store) {
  std::vector<arm64::Instruction> result;

  VirtualRegister valueReg = getVirtualRegisterOrThrow(store.value);
  VirtualRegister guestAddrReg = getVirtualRegisterOrThrow(store.address);
  ir::Type valueType = getValueType(store.value);

  // Perform address translation
  VirtualRegister hostAddrReg;
  auto addrTranslation = generateAddressTranslation(guestAddrReg, hostAddrReg);
  result.insert(result.end(), addrTranslation.begin(), addrTranslation.end());

  // STR value, [hostAddrReg]
  arm64::Instruction str;
  str.kind = arm64::MemoryInst{arm64::Opcode::STR, irTypeToDataSize(valueType),
                               valueReg, hostAddrReg, 0};
  result.push_back(str);

  return result;
}

std::vector<arm64::Instruction>
InstructionSelector::generateAddressTranslation(VirtualRegister guestAddrReg,
                                                VirtualRegister &hostAddrReg) {
  std::vector<arm64::Instruction> result;

  // Allocate temporary register for address translation
  hostAddrReg = nextVirtualReg++;

  // Address translation sequence:
  // Load guest base from GuestState (X0 is GuestState pointer)
  // Load shadow base from GuestState
  // temp = guest_addr - guest_base
  // host_addr = temp + shadow_base

  VirtualRegister guestBaseReg = nextVirtualReg++;
  VirtualRegister shadowBaseReg = nextVirtualReg++;

  // LDR guestBaseReg, [x0, #guestMemoryBase_offset]
  arm64::Instruction loadGuestBase;
  loadGuestBase.kind = arm64::MemoryInst{
      arm64::Opcode::LDR, arm64::DataSize::X, guestBaseReg, arm64::Register::X0,
      static_cast<int32_t>(offsetof(GuestState, guestMemoryBase))};
  result.push_back(loadGuestBase);

  // LDR shadowBaseReg, [x0, #shadowMemory_offset]
  arm64::Instruction loadShadowBase;
  loadShadowBase.kind = arm64::MemoryInst{
      arm64::Opcode::LDR, arm64::DataSize::X, shadowBaseReg,
      arm64::Register::X0,
      static_cast<int32_t>(offsetof(GuestState, shadowMemory))};
  result.push_back(loadShadowBase);

  // SUB temp, guest_addr, guestBaseReg
  arm64::Instruction sub;
  sub.kind = arm64::ThreeOperandInst{arm64::Opcode::SUB, arm64::DataSize::X,
                                     hostAddrReg, guestAddrReg, guestBaseReg};
  result.push_back(sub);

  // ADD temp, temp, shadowBaseReg
  arm64::Instruction add;
  add.kind = arm64::ThreeOperandInst{arm64::Opcode::ADD, arm64::DataSize::X,
                                     hostAddrReg, hostAddrReg, shadowBaseReg};
  result.push_back(add);

  return result;
}

std::vector<arm64::Instruction>
InstructionSelector::selectConst(const ir::Const &constInst,
                                 ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);
  return selectConstIntoRegister(constInst, destReg);
}

std::vector<arm64::Instruction>
InstructionSelector::selectConstIntoRegister(const ir::Const &constInst,
                                             arm64::Operand targetOperand) {
  std::vector<arm64::Instruction> result;
  arm64::DataSize dataSize = irTypeToDataSize(constInst.type);

  // Check if this is a small negative value that can be encoded with MOVN
  if (constInst.value < 0 && constInst.value >= -65536) {
    // Use MOVN: Result = ~imm16, so imm16 = ~value
    uint64_t movnImm = static_cast<uint64_t>(~constInst.value);
    arm64::Instruction inst;
    inst.kind = arm64::TwoOperandInst{arm64::Opcode::MOVN, dataSize,
                                      targetOperand, arm64::Immediate{movnImm}};
    result.push_back(inst);
  } else if (constInst.value >= 0 && constInst.value <= 0xFFFF) {
    // Use MOV for small positive values
    arm64::Instruction inst;
    inst.kind = arm64::TwoOperandInst{
        arm64::Opcode::MOV, dataSize, targetOperand,
        arm64::Immediate{static_cast<uint64_t>(constInst.value)}};
    result.push_back(inst);
  } else {
    uint64_t value = static_cast<uint64_t>(constInst.value);
    bool firstChunk = true;

    for (int shift = 0; shift < 64; shift += 16) {
      uint16_t chunk = static_cast<uint16_t>((value >> shift) & 0xFFFF);
      if (chunk == 0)
        continue;

      auto opcode = firstChunk ? arm64::Opcode::MOVZ : arm64::Opcode::MOVK;
      arm64::Instruction inst;
      inst.kind = arm64::MoveWideInst{opcode, dataSize, targetOperand, chunk,
                                      static_cast<uint8_t>(shift)};
      result.push_back(inst);
      firstChunk = false;
    }

    if (firstChunk) {
      arm64::Instruction inst;
      inst.kind = arm64::MoveWideInst{arm64::Opcode::MOVZ, dataSize,
                                      targetOperand, 0, 0};
      result.push_back(inst);
    }
  }

  return result;
}

arm64::Instruction InstructionSelector::selectSext(const ir::Sext &sext,
                                                   ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);
  VirtualRegister srcReg = getVirtualRegisterOrThrow(sext.operand);

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
                                    destReg, srcReg};

  return inst;
}

arm64::Instruction InstructionSelector::selectZext(const ir::Zext &zext,
                                                   ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);
  VirtualRegister srcReg = getVirtualRegisterOrThrow(zext.operand);

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
                                    destReg, srcReg};

  return inst;
}

arm64::Instruction InstructionSelector::selectTrunc(const ir::Trunc &trunc,
                                                    ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);
  VirtualRegister srcReg = getVirtualRegisterOrThrow(trunc.operand);

  arm64::Instruction inst;
  inst.kind = arm64::TwoOperandInst{
      arm64::Opcode::MOV, irTypeToDataSize(trunc.toType), destReg, srcReg};

  return inst;
}

arm64::Instruction
InstructionSelector::selectRegRead(const ir::RegRead &regRead,
                                   ir::ValueId resultId) {
  VirtualRegister destReg = assignVirtualRegister(resultId);

  int32_t offset =
      static_cast<int32_t>(regRead.regNumber * REGISTER_SIZE_BYTES);

  arm64::Instruction inst;
  inst.kind = arm64::MemoryInst{arm64::Opcode::LDR, arm64::DataSize::X, destReg,
                                arm64::Register::X0, offset};

  return inst;
}

arm64::Instruction
InstructionSelector::selectRegWrite(const ir::RegWrite &regWrite) {
  VirtualRegister valueReg = getVirtualRegisterOrThrow(regWrite.value);

  int32_t offset =
      static_cast<int32_t>(regWrite.regNumber * REGISTER_SIZE_BYTES);

  arm64::Instruction inst;
  inst.kind = arm64::MemoryInst{arm64::Opcode::STR, arm64::DataSize::X,
                                valueReg, arm64::Register::X0, offset};

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
    throw LoweringError("Unexpected opcode in irBinaryOpToARM64");
  }
}

} // namespace lowering
} // namespace dinorisc
