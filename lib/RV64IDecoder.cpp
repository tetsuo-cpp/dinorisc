#include "RV64IDecoder.h"
#include <iostream>

namespace dinorisc {

RV64IDecoder::RV64IDecoder() : totalDecoded(0), invalidCount(0) {}

RV64IDecoder::~RV64IDecoder() = default;

RV64IInstruction RV64IDecoder::decode(uint32_t rawInstruction,
                                      uint64_t pc) const {
  totalDecoded++;

  // Extract basic fields
  DecodedFields fields = extractFields(rawInstruction);

  // Determine the specific opcode
  RV64IInstruction::Opcode opcode = determineOpcode(fields, rawInstruction);

  if (opcode == RV64IInstruction::Opcode::INVALID) {
    invalidCount++;
  }

  // Extract operands based on the opcode
  std::vector<RV64IInstruction::Operand> operands =
      extractOperands(fields, opcode, rawInstruction);

  return RV64IInstruction(opcode, std::move(operands), rawInstruction, pc);
}

std::vector<RV64IInstruction>
RV64IDecoder::decodeInstructions(const uint8_t *data, size_t size,
                                 uint64_t baseAddress) const {
  std::vector<RV64IInstruction> instructions;

  if (!data || size == 0) {
    return instructions;
  }

  // Ensure size is multiple of 4 (32-bit instructions)
  if (size % 4 != 0) {
    std::cerr << "Warning: Data size " << size
              << " is not a multiple of 4 bytes. "
              << "Ignoring last " << (size % 4) << " bytes.\n";
  }

  size_t numInstructions = size / 4;
  instructions.reserve(numInstructions);

  for (size_t i = 0; i < numInstructions; ++i) {
    uint32_t rawInstruction = readLittleEndian32(data, i * 4);
    uint64_t pc = baseAddress + (i * 4);
    instructions.push_back(decode(rawInstruction, pc));
  }

  return instructions;
}

std::vector<RV64IInstruction>
RV64IDecoder::decodeInstructions(const std::vector<uint8_t> &data,
                                 uint64_t baseAddress) const {
  return decodeInstructions(data.data(), data.size(), baseAddress);
}

uint32_t RV64IDecoder::readLittleEndian32(const uint8_t *data,
                                          size_t offset) const {
  return static_cast<uint32_t>(data[offset]) |
         (static_cast<uint32_t>(data[offset + 1]) << 8) |
         (static_cast<uint32_t>(data[offset + 2]) << 16) |
         (static_cast<uint32_t>(data[offset + 3]) << 24);
}

RV64IDecoder::DecodedFields RV64IDecoder::extractFields(uint32_t raw) const {
  DecodedFields fields{};

  fields.opcode = raw & 0x7F;
  fields.rd = (raw >> 7) & 0x1F;
  fields.funct3 = (raw >> 12) & 0x7;
  fields.rs1 = (raw >> 15) & 0x1F;
  fields.rs2 = (raw >> 20) & 0x1F;
  fields.funct7 = (raw >> 25) & 0x7F;

  return fields;
}

int32_t RV64IDecoder::signExtend(uint32_t value, int bits) const {
  uint32_t signBit = 1U << (bits - 1);
  if (value & signBit) {
    return static_cast<int32_t>(value | (~0U << bits));
  }
  return static_cast<int32_t>(value);
}

RV64IInstruction::Opcode
RV64IDecoder::determineOpcode(const DecodedFields &fields, uint32_t raw) const {
  switch (fields.opcode) {
  case 0x33: // OP
    switch (fields.funct3) {
    case 0x0:
      return (fields.funct7 == 0x20) ? RV64IInstruction::Opcode::SUB
                                     : RV64IInstruction::Opcode::ADD;
    case 0x1:
      return RV64IInstruction::Opcode::SLL;
    case 0x2:
      return RV64IInstruction::Opcode::SLT;
    case 0x3:
      return RV64IInstruction::Opcode::SLTU;
    case 0x4:
      return RV64IInstruction::Opcode::XOR;
    case 0x5:
      return (fields.funct7 == 0x20) ? RV64IInstruction::Opcode::SRA
                                     : RV64IInstruction::Opcode::SRL;
    case 0x6:
      return RV64IInstruction::Opcode::OR;
    case 0x7:
      return RV64IInstruction::Opcode::AND;
    }
    break;

  case 0x3B: // OP_32
    switch (fields.funct3) {
    case 0x0:
      return (fields.funct7 == 0x20) ? RV64IInstruction::Opcode::SUBW
                                     : RV64IInstruction::Opcode::ADDW;
    case 0x1:
      return RV64IInstruction::Opcode::SLLW;
    case 0x5:
      return (fields.funct7 == 0x20) ? RV64IInstruction::Opcode::SRAW
                                     : RV64IInstruction::Opcode::SRLW;
    }
    break;

  case 0x13: // OP_IMM
    switch (fields.funct3) {
    case 0x0:
      return RV64IInstruction::Opcode::ADDI;
    case 0x1:
      return RV64IInstruction::Opcode::SLLI;
    case 0x2:
      return RV64IInstruction::Opcode::SLTI;
    case 0x3:
      return RV64IInstruction::Opcode::SLTIU;
    case 0x4:
      return RV64IInstruction::Opcode::XORI;
    case 0x5:
      return (fields.funct7 == 0x20) ? RV64IInstruction::Opcode::SRAI
                                     : RV64IInstruction::Opcode::SRLI;
    case 0x6:
      return RV64IInstruction::Opcode::ORI;
    case 0x7:
      return RV64IInstruction::Opcode::ANDI;
    }
    break;

  case 0x1B: // OP_IMM_32
    switch (fields.funct3) {
    case 0x0:
      return RV64IInstruction::Opcode::ADDIW;
    case 0x1:
      return RV64IInstruction::Opcode::SLLIW;
    case 0x5:
      return (fields.funct7 == 0x20) ? RV64IInstruction::Opcode::SRAIW
                                     : RV64IInstruction::Opcode::SRLIW;
    }
    break;

  case 0x03: // LOAD
    switch (fields.funct3) {
    case 0x0:
      return RV64IInstruction::Opcode::LB;
    case 0x1:
      return RV64IInstruction::Opcode::LH;
    case 0x2:
      return RV64IInstruction::Opcode::LW;
    case 0x3:
      return RV64IInstruction::Opcode::LD;
    case 0x4:
      return RV64IInstruction::Opcode::LBU;
    case 0x5:
      return RV64IInstruction::Opcode::LHU;
    case 0x6:
      return RV64IInstruction::Opcode::LWU;
    }
    break;

  case 0x23: // STORE
    switch (fields.funct3) {
    case 0x0:
      return RV64IInstruction::Opcode::SB;
    case 0x1:
      return RV64IInstruction::Opcode::SH;
    case 0x2:
      return RV64IInstruction::Opcode::SW;
    case 0x3:
      return RV64IInstruction::Opcode::SD;
    }
    break;

  case 0x63: // BRANCH
    switch (fields.funct3) {
    case 0x0:
      return RV64IInstruction::Opcode::BEQ;
    case 0x1:
      return RV64IInstruction::Opcode::BNE;
    case 0x4:
      return RV64IInstruction::Opcode::BLT;
    case 0x5:
      return RV64IInstruction::Opcode::BGE;
    case 0x6:
      return RV64IInstruction::Opcode::BLTU;
    case 0x7:
      return RV64IInstruction::Opcode::BGEU;
    }
    break;

  case 0x67: // JALR
    if (fields.funct3 == 0x0)
      return RV64IInstruction::Opcode::JALR;
    break;

  case 0x6F: // JAL
    return RV64IInstruction::Opcode::JAL;

  case 0x37: // LUI
    return RV64IInstruction::Opcode::LUI;

  case 0x17: // AUIPC
    return RV64IInstruction::Opcode::AUIPC;

  case 0x73: // SYSTEM
    if (fields.funct3 == 0x0 && fields.rd == 0x0 && fields.rs1 == 0x0) {
      // For ECALL and EBREAK, check the immediate field (bits 31-20)
      // Don't check rs2 since it's part of the immediate
      uint32_t imm = (raw >> 20) & 0xFFF;
      if (imm == 0x0)
        return RV64IInstruction::Opcode::ECALL;
      if (imm == 0x1)
        return RV64IInstruction::Opcode::EBREAK;
    }
    break;
  }

  return RV64IInstruction::Opcode::INVALID;
}

std::vector<RV64IInstruction::Operand>
RV64IDecoder::extractOperands(const DecodedFields &fields,
                              RV64IInstruction::Opcode opcode,
                              uint32_t raw) const {
  std::vector<RV64IInstruction::Operand> operands;

  switch (opcode) {
  // R-type: rd, rs1, rs2
  case RV64IInstruction::Opcode::ADD:
  case RV64IInstruction::Opcode::SUB:
  case RV64IInstruction::Opcode::SLL:
  case RV64IInstruction::Opcode::SLT:
  case RV64IInstruction::Opcode::SLTU:
  case RV64IInstruction::Opcode::XOR:
  case RV64IInstruction::Opcode::SRL:
  case RV64IInstruction::Opcode::SRA:
  case RV64IInstruction::Opcode::OR:
  case RV64IInstruction::Opcode::AND:
  case RV64IInstruction::Opcode::ADDW:
  case RV64IInstruction::Opcode::SUBW:
  case RV64IInstruction::Opcode::SLLW:
  case RV64IInstruction::Opcode::SRLW:
  case RV64IInstruction::Opcode::SRAW:
    operands.emplace_back(fields.rd);
    operands.emplace_back(fields.rs1);
    operands.emplace_back(fields.rs2);
    break;

  // I-type: rd, rs1, imm
  case RV64IInstruction::Opcode::ADDI:
  case RV64IInstruction::Opcode::SLTI:
  case RV64IInstruction::Opcode::SLTIU:
  case RV64IInstruction::Opcode::XORI:
  case RV64IInstruction::Opcode::ORI:
  case RV64IInstruction::Opcode::ANDI:
  case RV64IInstruction::Opcode::SLLI:
  case RV64IInstruction::Opcode::SRLI:
  case RV64IInstruction::Opcode::SRAI:
  case RV64IInstruction::Opcode::ADDIW:
  case RV64IInstruction::Opcode::SLLIW:
  case RV64IInstruction::Opcode::SRLIW:
  case RV64IInstruction::Opcode::SRAIW:
    operands.emplace_back(fields.rd);
    operands.emplace_back(fields.rs1);
    operands.emplace_back(static_cast<int64_t>(extractITypeImmediate(raw)));
    break;

  case RV64IInstruction::Opcode::JALR:
    operands.emplace_back(fields.rd);
    operands.emplace_back(fields.rs1);
    operands.emplace_back(static_cast<int64_t>(extractITypeImmediate(raw)));
    break;

  // Load: rd, rs1, offset
  case RV64IInstruction::Opcode::LB:
  case RV64IInstruction::Opcode::LH:
  case RV64IInstruction::Opcode::LW:
  case RV64IInstruction::Opcode::LD:
  case RV64IInstruction::Opcode::LBU:
  case RV64IInstruction::Opcode::LHU:
  case RV64IInstruction::Opcode::LWU:
    operands.emplace_back(fields.rd);
    operands.emplace_back(fields.rs1);
    operands.emplace_back(static_cast<int64_t>(extractITypeImmediate(raw)));
    break;

  // Store: rs1, rs2, offset
  case RV64IInstruction::Opcode::SB:
  case RV64IInstruction::Opcode::SH:
  case RV64IInstruction::Opcode::SW:
  case RV64IInstruction::Opcode::SD:
    operands.emplace_back(fields.rs1);
    operands.emplace_back(fields.rs2);
    operands.emplace_back(static_cast<int64_t>(extractSTypeImmediate(raw)));
    break;

  // Branch: rs1, rs2, offset
  case RV64IInstruction::Opcode::BEQ:
  case RV64IInstruction::Opcode::BNE:
  case RV64IInstruction::Opcode::BLT:
  case RV64IInstruction::Opcode::BGE:
  case RV64IInstruction::Opcode::BLTU:
  case RV64IInstruction::Opcode::BGEU:
    operands.emplace_back(fields.rs1);
    operands.emplace_back(fields.rs2);
    operands.emplace_back(static_cast<int64_t>(extractBTypeImmediate(raw)));
    break;

  // U-type: rd, imm
  case RV64IInstruction::Opcode::LUI:
  case RV64IInstruction::Opcode::AUIPC:
    operands.emplace_back(fields.rd);
    operands.emplace_back(static_cast<int64_t>(extractUTypeImmediate(raw)));
    break;

  // J-type: rd, offset
  case RV64IInstruction::Opcode::JAL:
    operands.emplace_back(fields.rd);
    operands.emplace_back(static_cast<int64_t>(extractJTypeImmediate(raw)));
    break;

  // System: no operands
  case RV64IInstruction::Opcode::ECALL:
  case RV64IInstruction::Opcode::EBREAK:
    break;

  case RV64IInstruction::Opcode::INVALID:
    break;
  }

  return operands;
}

int32_t RV64IDecoder::extractITypeImmediate(uint32_t raw) const {
  uint32_t imm = (raw >> 20) & 0xFFF;
  return signExtend(imm, 12);
}

int32_t RV64IDecoder::extractSTypeImmediate(uint32_t raw) const {
  uint32_t imm = ((raw >> 25) & 0x7F) << 5;
  imm |= (raw >> 7) & 0x1F;
  return signExtend(imm, 12);
}

int32_t RV64IDecoder::extractBTypeImmediate(uint32_t raw) const {
  uint32_t imm = 0;
  imm |= ((raw >> 31) & 0x1) << 12; // imm[12]
  imm |= ((raw >> 7) & 0x1) << 11;  // imm[11]
  imm |= ((raw >> 25) & 0x3F) << 5; // imm[10:5]
  imm |= ((raw >> 8) & 0xF) << 1;   // imm[4:1]
  return signExtend(imm, 13);
}

int32_t RV64IDecoder::extractUTypeImmediate(uint32_t raw) const {
  return static_cast<int32_t>(raw & 0xFFFFF000);
}

int32_t RV64IDecoder::extractJTypeImmediate(uint32_t raw) const {
  uint32_t imm = 0;
  imm |= ((raw >> 31) & 0x1) << 20;  // imm[20]
  imm |= ((raw >> 12) & 0xFF) << 12; // imm[19:12]
  imm |= ((raw >> 20) & 0x1) << 11;  // imm[11]
  imm |= ((raw >> 21) & 0x3FF) << 1; // imm[10:1]
  return signExtend(imm, 21);
}

} // namespace dinorisc