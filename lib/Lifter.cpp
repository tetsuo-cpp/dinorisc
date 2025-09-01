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

std::vector<ir::Instruction>
Lifter::liftInstruction(const riscv::Instruction &inst) {
  currentInstructions.clear();

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

  // Load instructions
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
                             std::to_string(static_cast<int>(inst.opcode)));
  }

  return currentInstructions;
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

} // namespace dinorisc