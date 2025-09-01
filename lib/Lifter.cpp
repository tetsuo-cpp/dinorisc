#include "Lifter.h"
#include <stdexcept>
#include <string>

namespace dinorisc {

Lifter::Lifter() : nextValueId(1) {
  // Initialize all registers to zero constants
  for (size_t i = 0; i < 32; ++i) {
    registerValues[i] = 0; // x0 is always zero, others start as zero
  }
}

ir::BasicBlock
Lifter::liftBasicBlock(const std::vector<riscv::Instruction> &instructions) {
  currentInstructions.clear();
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
  case riscv::Instruction::Opcode::ADD: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, rs2);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::ADDI: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, imm);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::SUB: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Sub, ir::Type::i64, rs1, rs2);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::ADDW: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId rs1_trunc = createTrunc(ir::Type::i64, ir::Type::i32, rs1);
    ir::ValueId rs2_trunc = createTrunc(ir::Type::i64, ir::Type::i32, rs2);
    ir::ValueId result_32 = createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i32,
                                           rs1_trunc, rs2_trunc);
    ir::ValueId result = createSext(ir::Type::i32, ir::Type::i64, result_32);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::ADDIW: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i32, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId rs1_trunc = createTrunc(ir::Type::i64, ir::Type::i32, rs1);
    ir::ValueId result_32 =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i32, rs1_trunc, imm);
    ir::ValueId result = createSext(ir::Type::i32, ir::Type::i64, result_32);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }

  // Bitwise operations
  case riscv::Instruction::Opcode::AND: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::And, ir::Type::i64, rs1, rs2);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::ANDI: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::And, ir::Type::i64, rs1, imm);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::OR: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Or, ir::Type::i64, rs1, rs2);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::ORI: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Or, ir::Type::i64, rs1, imm);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::XOR: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Xor, ir::Type::i64, rs1, rs2);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::XORI: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Xor, ir::Type::i64, rs1, imm);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }

  // Shift operations
  case riscv::Instruction::Opcode::SLL: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Shl, ir::Type::i64, rs1, rs2);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::SLLI: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Shl, ir::Type::i64, rs1, imm);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::SRL: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Shr, ir::Type::i64, rs1, rs2);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::SRLI: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Shr, ir::Type::i64, rs1, imm);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::SRA: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Sar, ir::Type::i64, rs1, rs2);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::SRAI: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Sar, ir::Type::i64, rs1, imm);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::SLLIW: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i32, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId rs1_trunc = createTrunc(ir::Type::i64, ir::Type::i32, rs1);
    ir::ValueId result_32 =
        createBinaryOp(ir::BinaryOpcode::Shl, ir::Type::i32, rs1_trunc, imm);
    ir::ValueId result = createSext(ir::Type::i32, ir::Type::i64, result_32);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }

  // Comparison instructions
  case riscv::Instruction::Opcode::SLT: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::Lt, ir::Type::i64, rs1, rs2);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::SLTU: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(2));
    ir::ValueId result =
        createBinaryOp(ir::BinaryOpcode::LtU, ir::Type::i64, rs1, rs2);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }

  // Load instructions
  case riscv::Instruction::Opcode::LB: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId addr =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, imm);
    ir::ValueId load8 = createLoad(ir::Type::i8, addr);
    ir::ValueId result = createSext(ir::Type::i8, ir::Type::i64, load8);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::LH: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId addr =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, imm);
    ir::ValueId load16 = createLoad(ir::Type::i16, addr);
    ir::ValueId result = createSext(ir::Type::i16, ir::Type::i64, load16);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::LD: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId addr =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, imm);
    ir::ValueId result = createLoad(ir::Type::i64, addr);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::LW: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId addr =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, imm);
    ir::ValueId load32 = createLoad(ir::Type::i32, addr);
    ir::ValueId result = createSext(ir::Type::i32, ir::Type::i64, load32);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }
  case riscv::Instruction::Opcode::LWU: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId addr =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, imm);
    ir::ValueId load32 = createLoad(ir::Type::i32, addr);
    ir::ValueId result = createZext(ir::Type::i32, ir::Type::i64, load32);
    setRegisterValue(inst.getRegister(0), result);
    break;
  }

  // Store instructions
  case riscv::Instruction::Opcode::SD: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(0));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId addr =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, imm);
    createStore(rs2, addr);
    break;
  }
  case riscv::Instruction::Opcode::SW: {
    ir::ValueId rs1 = getRegisterValue(inst.getRegister(1));
    ir::ValueId rs2 = getRegisterValue(inst.getRegister(0));
    ir::ValueId imm = createConstant(
        ir::Type::i64, static_cast<uint64_t>(inst.getImmediate(2)));
    ir::ValueId addr =
        createBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, rs1, imm);
    ir::ValueId val32 = createTrunc(ir::Type::i64, ir::Type::i32, rs2);
    createStore(val32, addr);
    break;
  }

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
    // x0 is always zero
    return createConstant(ir::Type::i64, 0);
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

ir::ValueId Lifter::createSext(ir::Type fromType, ir::Type toType,
                               ir::ValueId operand) {
  return addInstruction(ir::Sext{fromType, toType, operand});
}

ir::ValueId Lifter::createZext(ir::Type fromType, ir::Type toType,
                               ir::ValueId operand) {
  return addInstruction(ir::Zext{fromType, toType, operand});
}

ir::ValueId Lifter::createTrunc(ir::Type fromType, ir::Type toType,
                                ir::ValueId operand) {
  return addInstruction(ir::Trunc{fromType, toType, operand});
}

ir::ValueId Lifter::addInstruction(ir::InstructionKind kind) {
  ir::ValueId valueId = nextValueId++;
  currentInstructions.push_back({valueId, kind});
  return valueId;
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
    return ir::Terminator{ir::Return{retVal, true}};
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