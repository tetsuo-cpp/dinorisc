#include "Decoder.h"

namespace dinorisc {
namespace riscv {

Instruction Decoder::decode(uint32_t rawInstruction, uint64_t pc) const {
  // Extract basic fields
  DecodedFields fields = extractFields(rawInstruction);

  // Determine the specific opcode
  Instruction::Opcode opcode = determineOpcode(fields, rawInstruction);

  // Extract operands based on the opcode
  std::vector<Instruction::Operand> operands =
      extractOperands(fields, opcode, rawInstruction);

  return Instruction(opcode, std::move(operands), rawInstruction, pc);
}

uint32_t Decoder::readInstruction(const uint8_t *data, size_t offset) {
  return static_cast<uint32_t>(data[offset]) |
         (static_cast<uint32_t>(data[offset + 1]) << 8) |
         (static_cast<uint32_t>(data[offset + 2]) << 16) |
         (static_cast<uint32_t>(data[offset + 3]) << 24);
}

Decoder::DecodedFields Decoder::extractFields(uint32_t raw) const {
  DecodedFields fields{};

  fields.opcode = raw & 0x7F;
  fields.rd = (raw >> 7) & 0x1F;
  fields.funct3 = (raw >> 12) & 0x7;
  fields.rs1 = (raw >> 15) & 0x1F;
  fields.rs2 = (raw >> 20) & 0x1F;
  fields.funct7 = (raw >> 25) & 0x7F;

  return fields;
}

int32_t Decoder::signExtend(uint32_t value, int bits) const {
  uint32_t signBit = 1U << (bits - 1);
  if (value & signBit) {
    return static_cast<int32_t>(value | (~0U << bits));
  }
  return static_cast<int32_t>(value);
}

Instruction::Opcode Decoder::determineOpcode(const DecodedFields &fields,
                                             uint32_t raw) const {
  switch (fields.opcode) {
  case 0x33: // OP
    switch (fields.funct3) {
    case 0x0:
      return (fields.funct7 == 0x20) ? Instruction::Opcode::SUB
                                     : Instruction::Opcode::ADD;
    case 0x1:
      return Instruction::Opcode::SLL;
    case 0x2:
      return Instruction::Opcode::SLT;
    case 0x3:
      return Instruction::Opcode::SLTU;
    case 0x4:
      return Instruction::Opcode::XOR;
    case 0x5:
      return (fields.funct7 == 0x20) ? Instruction::Opcode::SRA
                                     : Instruction::Opcode::SRL;
    case 0x6:
      return Instruction::Opcode::OR;
    case 0x7:
      return Instruction::Opcode::AND;
    }
    break;

  case 0x3B: // OP_32
    switch (fields.funct3) {
    case 0x0:
      return (fields.funct7 == 0x20) ? Instruction::Opcode::SUBW
                                     : Instruction::Opcode::ADDW;
    case 0x1:
      return Instruction::Opcode::SLLW;
    case 0x5:
      return (fields.funct7 == 0x20) ? Instruction::Opcode::SRAW
                                     : Instruction::Opcode::SRLW;
    }
    break;

  case 0x13: // OP_IMM
    switch (fields.funct3) {
    case 0x0:
      return Instruction::Opcode::ADDI;
    case 0x1:
      return Instruction::Opcode::SLLI;
    case 0x2:
      return Instruction::Opcode::SLTI;
    case 0x3:
      return Instruction::Opcode::SLTIU;
    case 0x4:
      return Instruction::Opcode::XORI;
    case 0x5:
      return (fields.funct7 == 0x20) ? Instruction::Opcode::SRAI
                                     : Instruction::Opcode::SRLI;
    case 0x6:
      return Instruction::Opcode::ORI;
    case 0x7:
      return Instruction::Opcode::ANDI;
    }
    break;

  case 0x1B: // OP_IMM_32
    switch (fields.funct3) {
    case 0x0:
      return Instruction::Opcode::ADDIW;
    case 0x1:
      return Instruction::Opcode::SLLIW;
    case 0x5:
      return (fields.funct7 == 0x20) ? Instruction::Opcode::SRAIW
                                     : Instruction::Opcode::SRLIW;
    }
    break;

  case 0x03: // LOAD
    switch (fields.funct3) {
    case 0x0:
      return Instruction::Opcode::LB;
    case 0x1:
      return Instruction::Opcode::LH;
    case 0x2:
      return Instruction::Opcode::LW;
    case 0x3:
      return Instruction::Opcode::LD;
    case 0x4:
      return Instruction::Opcode::LBU;
    case 0x5:
      return Instruction::Opcode::LHU;
    case 0x6:
      return Instruction::Opcode::LWU;
    }
    break;

  case 0x23: // STORE
    switch (fields.funct3) {
    case 0x0:
      return Instruction::Opcode::SB;
    case 0x1:
      return Instruction::Opcode::SH;
    case 0x2:
      return Instruction::Opcode::SW;
    case 0x3:
      return Instruction::Opcode::SD;
    }
    break;

  case 0x63: // BRANCH
    switch (fields.funct3) {
    case 0x0:
      return Instruction::Opcode::BEQ;
    case 0x1:
      return Instruction::Opcode::BNE;
    case 0x4:
      return Instruction::Opcode::BLT;
    case 0x5:
      return Instruction::Opcode::BGE;
    case 0x6:
      return Instruction::Opcode::BLTU;
    case 0x7:
      return Instruction::Opcode::BGEU;
    }
    break;

  case 0x67: // JALR
    if (fields.funct3 == 0x0)
      return Instruction::Opcode::JALR;
    break;

  case 0x6F: // JAL
    return Instruction::Opcode::JAL;

  case 0x37: // LUI
    return Instruction::Opcode::LUI;

  case 0x17: // AUIPC
    return Instruction::Opcode::AUIPC;

  case 0x73: // SYSTEM
    if (fields.funct3 == 0x0 && fields.rd == 0x0 && fields.rs1 == 0x0) {
      // For ECALL and EBREAK, check the immediate field (bits 31-20)
      // Don't check rs2 since it's part of the immediate
      uint32_t imm = (raw >> 20) & 0xFFF;
      if (imm == 0x0)
        return Instruction::Opcode::ECALL;
      if (imm == 0x1)
        return Instruction::Opcode::EBREAK;
    }
    break;
  }

  return Instruction::Opcode::INVALID;
}

std::vector<Instruction::Operand>
Decoder::extractOperands(const DecodedFields &fields,
                         Instruction::Opcode opcode, uint32_t raw) const {
  std::vector<Instruction::Operand> operands;

  switch (opcode) {
  // R-type: rd, rs1, rs2
  case Instruction::Opcode::ADD:
  case Instruction::Opcode::SUB:
  case Instruction::Opcode::SLL:
  case Instruction::Opcode::SLT:
  case Instruction::Opcode::SLTU:
  case Instruction::Opcode::XOR:
  case Instruction::Opcode::SRL:
  case Instruction::Opcode::SRA:
  case Instruction::Opcode::OR:
  case Instruction::Opcode::AND:
  case Instruction::Opcode::ADDW:
  case Instruction::Opcode::SUBW:
  case Instruction::Opcode::SLLW:
  case Instruction::Opcode::SRLW:
  case Instruction::Opcode::SRAW:
    operands.emplace_back(fields.rd);
    operands.emplace_back(fields.rs1);
    operands.emplace_back(fields.rs2);
    break;

  // I-type: rd, rs1, imm
  case Instruction::Opcode::ADDI:
  case Instruction::Opcode::SLTI:
  case Instruction::Opcode::SLTIU:
  case Instruction::Opcode::XORI:
  case Instruction::Opcode::ORI:
  case Instruction::Opcode::ANDI:
  case Instruction::Opcode::SLLI:
  case Instruction::Opcode::SRLI:
  case Instruction::Opcode::SRAI:
  case Instruction::Opcode::ADDIW:
  case Instruction::Opcode::SLLIW:
  case Instruction::Opcode::SRLIW:
  case Instruction::Opcode::SRAIW:
    operands.emplace_back(fields.rd);
    operands.emplace_back(fields.rs1);
    operands.emplace_back(static_cast<int64_t>(extractITypeImmediate(raw)));
    break;

  case Instruction::Opcode::JALR:
    operands.emplace_back(fields.rd);
    operands.emplace_back(fields.rs1);
    operands.emplace_back(static_cast<int64_t>(extractITypeImmediate(raw)));
    break;

  // Load: rd, rs1, offset
  case Instruction::Opcode::LB:
  case Instruction::Opcode::LH:
  case Instruction::Opcode::LW:
  case Instruction::Opcode::LD:
  case Instruction::Opcode::LBU:
  case Instruction::Opcode::LHU:
  case Instruction::Opcode::LWU:
    operands.emplace_back(fields.rd);
    operands.emplace_back(fields.rs1);
    operands.emplace_back(static_cast<int64_t>(extractITypeImmediate(raw)));
    break;

  // Store: rs1, rs2, offset
  case Instruction::Opcode::SB:
  case Instruction::Opcode::SH:
  case Instruction::Opcode::SW:
  case Instruction::Opcode::SD:
    operands.emplace_back(fields.rs1);
    operands.emplace_back(fields.rs2);
    operands.emplace_back(static_cast<int64_t>(extractSTypeImmediate(raw)));
    break;

  // Branch: rs1, rs2, offset
  case Instruction::Opcode::BEQ:
  case Instruction::Opcode::BNE:
  case Instruction::Opcode::BLT:
  case Instruction::Opcode::BGE:
  case Instruction::Opcode::BLTU:
  case Instruction::Opcode::BGEU:
    operands.emplace_back(fields.rs1);
    operands.emplace_back(fields.rs2);
    operands.emplace_back(static_cast<int64_t>(extractBTypeImmediate(raw)));
    break;

  // U-type: rd, imm
  case Instruction::Opcode::LUI:
  case Instruction::Opcode::AUIPC:
    operands.emplace_back(fields.rd);
    operands.emplace_back(static_cast<int64_t>(extractUTypeImmediate(raw)));
    break;

  // J-type: rd, offset
  case Instruction::Opcode::JAL:
    operands.emplace_back(fields.rd);
    operands.emplace_back(static_cast<int64_t>(extractJTypeImmediate(raw)));
    break;

  // System: no operands
  case Instruction::Opcode::ECALL:
  case Instruction::Opcode::EBREAK:
    break;

  case Instruction::Opcode::INVALID:
    break;
  }

  return operands;
}

int32_t Decoder::extractITypeImmediate(uint32_t raw) const {
  uint32_t imm = (raw >> 20) & 0xFFF;
  return signExtend(imm, 12);
}

int32_t Decoder::extractSTypeImmediate(uint32_t raw) const {
  uint32_t imm = ((raw >> 25) & 0x7F) << 5;
  imm |= (raw >> 7) & 0x1F;
  return signExtend(imm, 12);
}

int32_t Decoder::extractBTypeImmediate(uint32_t raw) const {
  uint32_t imm = 0;
  imm |= ((raw >> 31) & 0x1) << 12; // imm[12]
  imm |= ((raw >> 7) & 0x1) << 11;  // imm[11]
  imm |= ((raw >> 25) & 0x3F) << 5; // imm[10:5]
  imm |= ((raw >> 8) & 0xF) << 1;   // imm[4:1]
  return signExtend(imm, 13);
}

int32_t Decoder::extractUTypeImmediate(uint32_t raw) const {
  return static_cast<int32_t>(raw & 0xFFFFF000);
}

int32_t Decoder::extractJTypeImmediate(uint32_t raw) const {
  uint32_t imm = 0;
  imm |= ((raw >> 31) & 0x1) << 20;  // imm[20]
  imm |= ((raw >> 12) & 0xFF) << 12; // imm[19:12]
  imm |= ((raw >> 20) & 0x1) << 11;  // imm[11]
  imm |= ((raw >> 21) & 0x3FF) << 1; // imm[10:1]
  return signExtend(imm, 21);
}

} // namespace riscv
} // namespace dinorisc