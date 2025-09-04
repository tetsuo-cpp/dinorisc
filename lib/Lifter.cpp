#include "Lifter.h"
#include <stdexcept>
#include <string>

namespace dinorisc {

Lifter::Lifter() : nextValueId(1), zeroConstant(0) {
  // Initialize all registers to zero constants
  for (size_t i = 0; i < 32; ++i) {
    registerValues[i] = 0; // x0 is always zero, others start as zero
  }
}

ir::BasicBlock
Lifter::liftBasicBlock(const std::vector<riscv::Instruction> &instructions) {
  currentInstructions.clear();

  // Create zero constant once per basic block for x0 register
  zeroConstant = createConstant(ir::Type::i64, 0);

  ir::BasicBlock block;

  for (size_t i = 0; i < instructions.size(); ++i) {
    const auto &inst = instructions[i];

    if (isTerminator(inst)) {
      // This instruction terminates the block - don't lift it as a regular
      // instruction
      uint64_t fallThroughAddress = (i + 1 < instructions.size())
                                        ? instructions[i + 1].address
                                        : inst.address + 4;
      block.terminator = liftTerminator(inst, fallThroughAddress);
      break;
    } else {
      // Regular instruction
      liftSingleInstruction(inst);
    }
  }

  // If we didn't find a terminator, create a fall-through branch
  if (instructions.empty() || !isTerminator(instructions.back())) {
    uint64_t nextAddress =
        instructions.empty() ? 0 : instructions.back().address + 4;
    block.terminator = ir::Terminator{ir::Branch{nextAddress}};
  }

  block.instructions = currentInstructions;
  return block;
}

void Lifter::liftSingleInstruction(const riscv::Instruction &inst) {
  switch (inst.opcode) {
  // Arithmetic instructions
  case riscv::Instruction::Opcode::ADD:
    liftRTypeBinaryOp(inst, ir::BinaryOpcode::Add);
    break;
  case riscv::Instruction::Opcode::ADDI:
    liftITypeBinaryOp(inst, ir::BinaryOpcode::Add);
    break;
  case riscv::Instruction::Opcode::SUB:
    liftRTypeBinaryOp(inst, ir::BinaryOpcode::Sub);
    break;
  case riscv::Instruction::Opcode::ADDW:
    liftWTypeBinaryOp(inst, ir::BinaryOpcode::Add);
    break;
  case riscv::Instruction::Opcode::ADDIW:
    liftWTypeImmOp(inst, ir::BinaryOpcode::Add);
    break;

  // Bitwise operations
  case riscv::Instruction::Opcode::AND:
    liftRTypeBinaryOp(inst, ir::BinaryOpcode::And);
    break;
  case riscv::Instruction::Opcode::ANDI:
    liftITypeBinaryOp(inst, ir::BinaryOpcode::And);
    break;
  case riscv::Instruction::Opcode::OR:
    liftRTypeBinaryOp(inst, ir::BinaryOpcode::Or);
    break;
  case riscv::Instruction::Opcode::ORI:
    liftITypeBinaryOp(inst, ir::BinaryOpcode::Or);
    break;
  case riscv::Instruction::Opcode::XOR:
    liftRTypeBinaryOp(inst, ir::BinaryOpcode::Xor);
    break;
  case riscv::Instruction::Opcode::XORI:
    liftITypeBinaryOp(inst, ir::BinaryOpcode::Xor);
    break;

  // Shift operations
  case riscv::Instruction::Opcode::SLL:
    liftRTypeBinaryOp(inst, ir::BinaryOpcode::Shl);
    break;
  case riscv::Instruction::Opcode::SLLI:
    liftITypeBinaryOp(inst, ir::BinaryOpcode::Shl);
    break;
  case riscv::Instruction::Opcode::SRL:
    liftRTypeBinaryOp(inst, ir::BinaryOpcode::Shr);
    break;
  case riscv::Instruction::Opcode::SRLI:
    liftITypeBinaryOp(inst, ir::BinaryOpcode::Shr);
    break;
  case riscv::Instruction::Opcode::SRA:
    liftRTypeBinaryOp(inst, ir::BinaryOpcode::Sar);
    break;
  case riscv::Instruction::Opcode::SRAI:
    liftITypeBinaryOp(inst, ir::BinaryOpcode::Sar);
    break;
  case riscv::Instruction::Opcode::SLLIW:
    liftWTypeImmOp(inst, ir::BinaryOpcode::Shl);
    break;

  // Comparison instructions
  case riscv::Instruction::Opcode::SLT:
    liftRTypeBinaryOp(inst, ir::BinaryOpcode::Lt);
    break;
  case riscv::Instruction::Opcode::SLTU:
    liftRTypeBinaryOp(inst, ir::BinaryOpcode::LtU);
    break;

  // Load instructions
  case riscv::Instruction::Opcode::LB:
    liftLoadInstruction(inst, ir::Type::i8, true);
    break;
  case riscv::Instruction::Opcode::LH:
    liftLoadInstruction(inst, ir::Type::i16, true);
    break;
  case riscv::Instruction::Opcode::LD:
    liftLoadInstruction(inst, ir::Type::i64, false);
    break;
  case riscv::Instruction::Opcode::LW:
    liftLoadInstruction(inst, ir::Type::i32, true);
    break;
  case riscv::Instruction::Opcode::LWU:
    liftLoadInstruction(inst, ir::Type::i32, false);
    break;

  // Store instructions
  case riscv::Instruction::Opcode::SD:
    liftStoreInstruction(inst, ir::Type::i64);
    break;
  case riscv::Instruction::Opcode::SW:
    liftStoreInstruction(inst, ir::Type::i32);
    break;

  // Upper immediate instructions
  case riscv::Instruction::Opcode::LUI: {
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(1)) << 12);
    setRegisterValue(inst.getRegister(0), imm);
    break;
  }
  case riscv::Instruction::Opcode::AUIPC: {
    ir::ValueId pc = createConstant(ir::Type::i64, inst.address);
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(1)) << 12);
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, pc, imm);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }

  default:
    throw std::runtime_error("Unsupported RISC-V instruction in lifter: " +
                             inst.toString());
  }
}

ir::ValueId Lifter::getRegisterValue(uint32_t regNum) {
  if (regNum == 0) {
    // x0 is always zero - return cached constant
    return zeroConstant;
  }
  if (registerValues[regNum] == 0) {
    // Register not yet set, create zero constant
    registerValues[regNum] = createConstant(ir::Type::i64, 0);
  }
  return registerValues[regNum];
}

void Lifter::setRegisterValue(uint32_t regNum, ir::ValueId valueId) {
  if (regNum != 0) { // x0 is always zero, cannot be set
    registerValues[regNum] = valueId;
  }
}

ir::ValueId Lifter::createConstant(ir::Type type, uint64_t value) {
  return addInstruction(ir::Const{type, value});
}

ir::ValueId Lifter::createBinaryOp(ir::BinaryOpcode opcode, ir::Type type,
                                   ir::ValueId lhs, ir::ValueId rhs) {
  return addInstruction(ir::BinaryOp{opcode, type, lhs, rhs});
}

ir::ValueId Lifter::createLoad(ir::Type type, ir::ValueId address) {
  return addInstruction(ir::Load{type, address});
}

ir::ValueId Lifter::createStore(ir::ValueId value, ir::ValueId address) {
  return addInstruction(ir::Store{value, address});
}

ir::ValueId Lifter::createSext(ir::Type toType, ir::ValueId operand) {
  return addInstruction(ir::Sext{toType, operand});
}

ir::ValueId Lifter::createZext(ir::Type toType, ir::ValueId operand) {
  return addInstruction(ir::Zext{toType, operand});
}

ir::ValueId Lifter::createTrunc(ir::Type toType, ir::ValueId operand) {
  return addInstruction(ir::Trunc{toType, operand});
}

ir::ValueId Lifter::addInstruction(ir::InstructionKind kind) {
  ir::ValueId valueId = nextValueId++;
  currentInstructions.push_back({valueId, kind});
  return valueId;
}

void Lifter::liftRTypeBinaryOp(const riscv::Instruction &inst,
                               ir::BinaryOpcode opcode, ir::Type type) {
  ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
  ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
  ir::ValueId result = createBinaryOp(opcode, type, rs1, rs2);
  setRegisterValue(inst.getRegister(0), result);
}

void Lifter::liftITypeBinaryOp(const riscv::Instruction &inst,
                               ir::BinaryOpcode opcode, ir::Type type) {
  ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
  ir::ValueId imm = createConstant(type, inst.getImmediate(2));
  ir::ValueId result = createBinaryOp(opcode, type, rs1, imm);
  setRegisterValue(inst.getRegister(0), result);
}

void Lifter::liftWTypeBinaryOp(const riscv::Instruction &inst,
                               ir::BinaryOpcode opcode) {
  ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
  ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
  ir::ValueId rs1_trunc = createTrunc(ir::Type::i32, rs1);
  ir::ValueId rs2_trunc = createTrunc(ir::Type::i32, rs2);
  ir::ValueId result_32 =
      createBinaryOp(opcode, ir::Type::i32, rs1_trunc, rs2_trunc);
  ir::ValueId result = createSext(ir::Type::i64, result_32);
  setRegisterValue(inst.getRegister(0), result);
}

void Lifter::liftWTypeImmOp(const riscv::Instruction &inst,
                            ir::BinaryOpcode opcode) {
  ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
  ir::ValueId imm = createConstant(ir::Type::i32, inst.getImmediate(2));
  ir::ValueId rs1_trunc = createTrunc(ir::Type::i32, rs1);
  ir::ValueId result_32 = createBinaryOp(opcode, ir::Type::i32, rs1_trunc, imm);
  ir::ValueId result = createSext(ir::Type::i64, result_32);
  setRegisterValue(inst.getRegister(0), result);
}

void Lifter::liftLoadInstruction(const riscv::Instruction &inst,
                                 ir::Type loadType, bool signExtend) {
  ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
  ir::ValueId imm = createConstant(ir::Type::i64, inst.getImmediate(2));
  ir::ValueId addr =
      createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, imm);
  ir::ValueId loaded = createLoad(loadType, addr);

  ir::ValueId result = loaded;
  if (loadType != ir::Type::i64) {
    result = signExtend ? createSext(ir::Type::i64, loaded)
                        : createZext(ir::Type::i64, loaded);
  }
  setRegisterValue(inst.getRegister(0), result);
}

void Lifter::liftStoreInstruction(const riscv::Instruction &inst,
                                  ir::Type storeType) {
  ir::ValueId rs1 = getRegisterValue(inst.getRegister(1)); // base address
  ir::ValueId rs2 = getRegisterValue(inst.getRegister(0)); // value to store
  ir::ValueId imm =
      createConstant(ir::Type::i64, inst.getImmediate(2)); // offset
  ir::ValueId addr =
      createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, imm);

  ir::ValueId val = rs2;
  if (storeType != ir::Type::i64) {
    val = createTrunc(storeType, rs2);
  }
  createStore(val, addr);
}

bool Lifter::isTerminator(const riscv::Instruction &inst) {
  switch (inst.opcode) {
  case riscv::Instruction::Opcode::BEQ:
  case riscv::Instruction::Opcode::BNE:
  case riscv::Instruction::Opcode::BLT:
  case riscv::Instruction::Opcode::BGE:
  case riscv::Instruction::Opcode::BLTU:
  case riscv::Instruction::Opcode::BGEU:
  case riscv::Instruction::Opcode::JAL:
  case riscv::Instruction::Opcode::JALR:
    return true;
  default:
    return false;
  }
}

ir::Terminator Lifter::liftTerminator(const riscv::Instruction &inst,
                                      uint64_t fallThroughAddress) {
  switch (inst.opcode) {
  // Conditional branches
  case riscv::Instruction::Opcode::BEQ:
    return createConditionalBranch(ir::BinaryOpcode::Eq, inst,
                                   fallThroughAddress);
  case riscv::Instruction::Opcode::BNE:
    return createConditionalBranch(ir::BinaryOpcode::Ne, inst,
                                   fallThroughAddress);
  case riscv::Instruction::Opcode::BLT:
    return createConditionalBranch(ir::BinaryOpcode::Lt, inst,
                                   fallThroughAddress);
  case riscv::Instruction::Opcode::BGE:
    return createConditionalBranch(ir::BinaryOpcode::Ge, inst,
                                   fallThroughAddress);
  case riscv::Instruction::Opcode::BLTU:
    return createConditionalBranch(ir::BinaryOpcode::LtU, inst,
                                   fallThroughAddress);
  case riscv::Instruction::Opcode::BGEU:
    return createConditionalBranch(ir::BinaryOpcode::GeU, inst,
                                   fallThroughAddress);

  // Unconditional jumps
  case riscv::Instruction::Opcode::JAL: {
    // JAL rd, imm: rd = pc + 4, pc = pc + imm
    ir::ValueId returnAddr = createConstant(ir::Type::i64, inst.address + 4);
    setRegisterValue(inst.getRegister(0), returnAddr);
    uint64_t target = inst.address + inst.getImmediate(1);
    return ir::Terminator{ir::Branch{target}};
  }
  case riscv::Instruction::Opcode::JALR: {
    // JALR rd, rs1, imm: rd = pc + 4, pc = (rs1 + imm) & ~1
    ir::ValueId returnAddr = createConstant(ir::Type::i64, inst.address + 4);
    setRegisterValue(inst.getRegister(0), returnAddr);

    // For now, we'll treat JALR as a return (can be enhanced later for indirect
    // jumps)
    ir::ValueId retVal = createConstant(ir::Type::i64, 0);
    return ir::Terminator{ir::Return{retVal}};
  }

  default:
    throw std::runtime_error(
        "Invalid terminator instruction in liftTerminator");
  }
}

ir::Terminator Lifter::createConditionalBranch(ir::BinaryOpcode compareOp,
                                               const riscv::Instruction &inst,
                                               uint64_t fallThroughAddress) {
  ir::ValueId rs1 = getRegisterValue(inst.getRegister(0));
  ir::ValueId rs2 = getRegisterValue(inst.getRegister(1));
  ir::ValueId condition = createBinaryOp(compareOp, ir::Type::i1, rs1, rs2);
  uint64_t target = calculateBranchTarget(inst);
  return ir::Terminator{ir::CondBranch{condition, target, fallThroughAddress}};
}

uint64_t Lifter::calculateBranchTarget(const riscv::Instruction &inst) {
  return inst.address + inst.getImmediate(2);
}

} // namespace dinorisc