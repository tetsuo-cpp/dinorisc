#include "Encoder.h"
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <variant>

namespace dinorisc {
namespace arm64 {

std::vector<uint8_t> Encoder::encodeInstruction(const Instruction &inst) {
  uint32_t encoded = 0;

  std::visit(
      [&](const auto &instruction) {
        using T = std::decay_t<decltype(instruction)>;
        if constexpr (std::is_same_v<T, ThreeOperandInst>) {
          encoded = encodeThreeOperandInst(instruction);
        } else if constexpr (std::is_same_v<T, TwoOperandInst>) {
          encoded = encodeTwoOperandInst(instruction);
        } else if constexpr (std::is_same_v<T, MemoryInst>) {
          encoded = encodeMemoryInst(instruction);
        } else if constexpr (std::is_same_v<T, MoveWideInst>) {
          encoded = encodeMoveWideInst(instruction);
        } else if constexpr (std::is_same_v<T, BranchInst>) {
          encoded = encodeBranchInst(instruction);
        } else if constexpr (std::is_same_v<T, ConditionalInst>) {
          encoded = encodeConditionalInst(instruction);
        } else if constexpr (std::is_same_v<T, ConditionalSelectInst>) {
          encoded = encodeConditionalSelectInst(instruction);
        }
      },
      inst.kind);

  return {static_cast<uint8_t>(encoded & 0xFF),
          static_cast<uint8_t>((encoded >> 8) & 0xFF),
          static_cast<uint8_t>((encoded >> 16) & 0xFF),
          static_cast<uint8_t>((encoded >> 24) & 0xFF)};
}

uint32_t Encoder::encodeThreeOperandInst(const ThreeOperandInst &inst) {
  uint32_t encoded = 0;
  uint32_t sf = getSfBit(inst.size);
  uint32_t rd = encodeRegister(inst.dest);
  uint32_t rn = encodeRegister(inst.src1);

  switch (inst.opcode) {
  case Opcode::ADD: {
    if (isImmediate(inst.src2)) {
      // ADD (immediate): sf 0 0 1 0 0 0 1 0 sh imm12 Rn Rd
      // sf=bit31, bits30-29=00, bits28-23=100010, sh=bit22, imm12=bits21-10,
      // Rn=bits9-5, Rd=bits4-0
      uint32_t imm = std::get<Immediate>(inst.src2).value;
      if (imm > 0xFFF)
        throw std::runtime_error("ADD immediate value too large (>12 bits)");
      uint32_t sh = 0; // No shift for simple immediate
      encoded = (sf << 31) | (0b00100010 << 23) | (sh << 22) | (imm << 10) |
                (rn << 5) | rd;
    } else {
      // ADD (shifted register): sf 0 0 0 1 0 1 1 shift 0 Rm imm6 Rn Rd
      // sf=bit31, bits30-24=0001011, shift=bits23-22, bit21=0, Rm=bits20-16,
      // imm6=bits15-10, Rn=bits9-5, Rd=bits4-0
      uint32_t rm = encodeRegister(inst.src2);
      uint32_t shift = 0; // LSL
      uint32_t imm6 = 0;  // No shift amount
      encoded = (sf << 31) | (0b0001011 << 24) | (shift << 22) | (rm << 16) |
                (imm6 << 10) | (rn << 5) | rd;
    }
    break;
  }
  case Opcode::SUB: {
    if (isImmediate(inst.src2)) {
      // SUB (immediate): sf 1 0 1 0 0 0 1 0 sh imm12 Rn Rd
      // sf=bit31, bits30-29=10, bits28-23=100010, sh=bit22, imm12=bits21-10,
      // Rn=bits9-5, Rd=bits4-0
      uint32_t imm = std::get<Immediate>(inst.src2).value;
      if (imm > 0xFFF)
        throw std::runtime_error("SUB immediate value too large (>12 bits)");
      uint32_t sh = 0; // No shift for simple immediate
      encoded = (sf << 31) | (0b10100010 << 23) | (sh << 22) | (imm << 10) |
                (rn << 5) | rd;
    } else {
      // SUB (shifted register): sf 1 0 0 1 0 1 1 shift 0 Rm imm6 Rn Rd
      // sf=bit31, bits30-24=1001011, shift=bits23-22, bit21=0, Rm=bits20-16,
      // imm6=bits15-10, Rn=bits9-5, Rd=bits4-0
      uint32_t rm = encodeRegister(inst.src2);
      uint32_t shift = 0; // LSL
      uint32_t imm6 = 0;  // No shift amount
      encoded = (sf << 31) | (0b1001011 << 24) | (shift << 22) | (rm << 16) |
                (imm6 << 10) | (rn << 5) | rd;
    }
    break;
  }
  case Opcode::AND: {
    if (isImmediate(inst.src2)) {
      throw std::runtime_error("AND with immediate not supported");
    }
    // AND (shifted register): sf 0 0 0 1 0 1 0 shift 0 Rm imm6 Rn Rd
    // sf=bit31, bits30-25=000101, shift=bits24-23, bit22=0, Rm=bits21-16,
    // imm6=bits15-10, Rn=bits9-5, Rd=bits4-0
    uint32_t rm = encodeRegister(inst.src2);
    uint32_t shift = 0; // LSL
    uint32_t imm6 = 0;  // No shift amount
    encoded = (sf << 31) | (0b000101 << 25) | (shift << 23) | (rm << 16) |
              (imm6 << 10) | (rn << 5) | rd;
    break;
  }
  case Opcode::ORR: {
    if (isImmediate(inst.src2)) {
      throw std::runtime_error("ORR with immediate not supported");
    }
    // ORR (shifted register): sf 0 1 0 1 0 1 0 shift 0 Rm imm6 Rn Rd
    // sf=bit31, bits30-25=010101, shift=bits24-23, bit22=0, Rm=bits21-16,
    // imm6=bits15-10, Rn=bits9-5, Rd=bits4-0
    uint32_t rm = encodeRegister(inst.src2);
    uint32_t shift = 0; // LSL
    uint32_t imm6 = 0;  // No shift amount
    encoded = (sf << 31) | (0b010101 << 25) | (shift << 23) | (rm << 16) |
              (imm6 << 10) | (rn << 5) | rd;
    break;
  }
  case Opcode::EOR: {
    if (isImmediate(inst.src2)) {
      throw std::runtime_error("EOR with immediate not supported");
    }
    // EOR (shifted register): sf 1 1 0 1 0 1 0 shift 0 Rm imm6 Rn Rd
    // sf=bit31, bits30-25=110101, shift=bits24-23, bit22=0, Rm=bits21-16,
    // imm6=bits15-10, Rn=bits9-5, Rd=bits4-0
    uint32_t rm = encodeRegister(inst.src2);
    uint32_t shift = 0; // LSL
    uint32_t imm6 = 0;  // No shift amount
    encoded = (sf << 31) | (0b110101 << 25) | (shift << 23) | (rm << 16) |
              (imm6 << 10) | (rn << 5) | rd;
    break;
  }
  case Opcode::MUL: {
    if (isImmediate(inst.src2)) {
      throw std::runtime_error("MUL with immediate not supported");
    }
    // MUL is alias of MADD: sf 0 0 1 1 0 1 1 0 0 0 Rm 0 Ra Rn Rd
    // sf=bit31, bits30-21=0011011000, Rm=bits20-16, bits15-10=000000 (Ra=XZR),
    // Rn=bits9-5, Rd=bits4-0
    uint32_t rm = encodeRegister(inst.src2);
    uint32_t ra = 31; // XZR for MUL (makes it MADD with 0 addend)
    encoded = (sf << 31) | (0b0011011000 << 21) | (rm << 16) | (ra << 10) |
              (rn << 5) | rd;
    break;
  }
  case Opcode::CMP: {
    if (isImmediate(inst.src2)) {
      // CMP (immediate): sf 1 1 1 0 0 0 1 0 sh imm12 Rn Rt
      // This is SUBS with Rt=XZR (compare is subtract with result discarded)
      // sf=bit31, bits30-29=11, bits28-23=100010, sh=bit22, imm12=bits21-10,
      // Rn=bits9-5, Rt=bits4-0 (Rt=31 for XZR)
      uint32_t imm = std::get<Immediate>(inst.src2).value;
      if (imm > 0xFFF)
        throw std::runtime_error("CMP immediate value too large (>12 bits)");
      uint32_t sh = 0;   // No shift for simple immediate
      uint32_t xzr = 31; // XZR register
      encoded = (sf << 31) | (0b11100010 << 23) | (sh << 22) | (imm << 10) |
                (rn << 5) | xzr;
    } else {
      // CMP (shifted register): sf 1 1 0 1 0 1 1 shift 0 Rm imm6 Rn Rd
      // This is SUBS with Rd=XZR
      // sf=bit31, bits30-24=1101011, shift=bits23-22, bit21=0, Rm=bits20-16,
      // imm6=bits15-10, Rn=bits9-5, Rd=bits4-0 (Rd=31 for XZR)
      uint32_t rm = encodeRegister(inst.src2);
      uint32_t shift = 0; // LSL
      uint32_t imm6 = 0;  // No shift amount
      uint32_t xzr = 31;  // XZR register
      encoded = (sf << 31) | (0b1101011 << 24) | (shift << 22) | (rm << 16) |
                (imm6 << 10) | (rn << 5) | xzr;
    }
    break;
  }
  case Opcode::LSL: {
    if (isImmediate(inst.src2)) {
      // LSL (immediate) is alias of UBFM: sf 1 0 0 1 0 0 1 1 N immr imms Rn Rd
      // For LSL #n: immr = (64-n) % 64 for 64-bit, (32-n) % 32 for 32-bit
      // imms = 63-n for 64-bit, 31-n for 32-bit
      uint64_t shiftAmount = std::get<Immediate>(inst.src2).value;
      uint32_t datasize = (sf == 1) ? 64 : 32;
      if (shiftAmount >= datasize)
        throw std::runtime_error("LSL shift amount out of range");
      uint32_t N = sf;
      uint32_t immr = (datasize - shiftAmount) % datasize;
      uint32_t imms = datasize - 1 - shiftAmount;
      encoded = (sf << 31) | (0b10100110 << 23) | (N << 22) | (immr << 16) |
                (imms << 10) | (rn << 5) | rd;
    } else {
      // LSL (register) is alias of LSLV: sf 0 0 1 1 0 1 0 1 1 0 Rm 0 0 1 0 0 0
      // Rn Rd Pattern: sf 00 11010 110 Rm 001000 Rn Rd
      uint32_t rm = encodeRegister(inst.src2);
      encoded = (sf << 31) | (0b0011010110 << 21) | (rm << 16) |
                (0b001000 << 10) | (rn << 5) | rd;
    }
    break;
  }
  case Opcode::LSR: {
    if (isImmediate(inst.src2)) {
      // LSR (immediate) is alias of UBFM: sf 1 0 0 1 0 0 1 1 N immr imms Rn Rd
      // For LSR #n: immr = n, imms = 63 for 64-bit, 31 for 32-bit
      uint64_t shiftAmount = std::get<Immediate>(inst.src2).value;
      uint32_t datasize = (sf == 1) ? 64 : 32;
      if (shiftAmount >= datasize)
        throw std::runtime_error("LSR shift amount out of range");
      uint32_t N = sf;
      uint32_t immr = shiftAmount;
      uint32_t imms = datasize - 1;
      encoded = (sf << 31) | (0b10100110 << 23) | (N << 22) | (immr << 16) |
                (imms << 10) | (rn << 5) | rd;
    } else {
      // LSR (register) is alias of LSRV: sf 0 0 1 1 0 1 0 1 1 0 Rm 0 0 1 0 Rn
      // Rd
      uint32_t rm = encodeRegister(inst.src2);
      encoded = (sf << 31) | (0b00110101100 << 20) | (rm << 16) |
                (0b0110 << 10) | (rn << 5) | rd;
    }
    break;
  }
  case Opcode::ASR: {
    if (isImmediate(inst.src2)) {
      // ASR (immediate) is alias of SBFM: sf 0 0 1 0 0 1 1 N immr imms Rn Rd
      // For ASR #n: immr = n, imms = 63 for 64-bit, 31 for 32-bit
      uint64_t shiftAmount = std::get<Immediate>(inst.src2).value;
      uint32_t datasize = (sf == 1) ? 64 : 32;
      if (shiftAmount >= datasize)
        throw std::runtime_error("ASR shift amount out of range");
      uint32_t N = sf;
      uint32_t immr = shiftAmount;
      uint32_t imms = datasize - 1;
      encoded = (sf << 31) | (0b00100110 << 23) | (N << 22) | (immr << 16) |
                (imms << 10) | (rn << 5) | rd;
    } else {
      // ASR (register) is alias of ASRV: sf 0 0 1 1 0 1 0 1 1 0 Rm 0 0 1 0 Rn
      // Rd
      uint32_t rm = encodeRegister(inst.src2);
      encoded = (sf << 31) | (0b00110101100 << 20) | (rm << 16) |
                (0b1010 << 10) | (rn << 5) | rd;
    }
    break;
  }
  default:
    throw std::runtime_error("Unsupported three-operand instruction opcode");
  }

  return encoded;
}

uint32_t Encoder::encodeTwoOperandInst(const TwoOperandInst &inst) {
  uint32_t encoded = 0;
  uint32_t sf = getSfBit(inst.size);
  uint32_t rd = encodeRegister(inst.dest);

  switch (inst.opcode) {
  case Opcode::MOV: {
    if (isImmediate(inst.src)) {
      uint64_t imm = std::get<Immediate>(inst.src).value;
      if (imm > 0xFFFF) {
        throw std::runtime_error("MOV immediate value too large (>16 bits)");
      }
      // MOVZ encoding: sf 1 0 0 1 0 1 0 hw imm16 Rd
      uint32_t hw = 0; // No shift for simple immediate
      encoded = (sf << 31) | (0b10100101 << 23) | (hw << 21) |
                ((imm & 0xFFFF) << 5) | rd;
    } else {
      // MOV (register) is alias of ORR: sf 0 1 0 1 0 1 0 shift 0 Rm imm6 Rn Rd
      // (with Rn=XZR) sf=bit31, bits30-25=010101, shift=bits24-23, bit22=0,
      // Rm=bits21-16, imm6=bits15-10, Rn=bits9-5=11111, Rd=bits4-0
      uint32_t rm = encodeRegister(inst.src);
      uint32_t shift = 0;   // LSL
      uint32_t imm6 = 0;    // No shift amount
      uint32_t rn_xzr = 31; // XZR for MOV (ORR with zero)
      encoded = (sf << 31) | (0b010101 << 25) | (shift << 23) | (rm << 16) |
                (imm6 << 10) | (rn_xzr << 5) | rd;
    }
    break;
  }
  case Opcode::SXTB: {
    // SXTB is alias of SBFM: sf 0 0 1 0 0 1 1 N immr imms Rn Rd
    // sf=bit31, bits30-29=00, bits28-23=100110, N=bit22, immr=bits21-16=000000,
    // imms=bits15-10=000111, Rn=bits9-5, Rd=bits4-0
    uint32_t rn = encodeRegister(inst.src);
    uint32_t N = sf;   // N field matches sf for SXTB
    uint32_t immr = 0; // Extract from bit 0
    uint32_t imms = 7; // Extract 8 bits (bits 7:0)
    encoded = (sf << 31) | (0b00100110 << 23) | (N << 22) | (immr << 16) |
              (imms << 10) | (rn << 5) | rd;
    break;
  }
  case Opcode::SXTH: {
    // SXTH is alias of SBFM: sf 0 0 1 0 0 1 1 N immr imms Rn Rd
    // sf=bit31, bits30-23=00100110, N=bit22, immr=bits21-16=000000,
    // imms=bits15-10=001111, Rn=bits9-5, Rd=bits4-0
    uint32_t rn = encodeRegister(inst.src);
    uint32_t N = sf;    // N field matches sf
    uint32_t immr = 0;  // Extract from bit 0
    uint32_t imms = 15; // Extract 16 bits (bits 15:0)
    encoded = (sf << 31) | (0b00100110 << 23) | (N << 22) | (immr << 16) |
              (imms << 10) | (rn << 5) | rd;
    break;
  }
  case Opcode::SXTW: {
    // SXTW is alias of SBFM: 1 0 0 1 0 0 1 1 1 immr imms Rn Rd (64-bit only)
    // sf=bit31=1, bits30-23=00100110, N=bit22=1, immr=bits21-16=000000,
    // imms=bits15-10=011111, Rn=bits9-5, Rd=bits4-0
    uint32_t rn = encodeRegister(inst.src);
    uint32_t N = 1;     // Always 64-bit for SXTW
    uint32_t immr = 0;  // Extract from bit 0
    uint32_t imms = 31; // Extract 32 bits (bits 31:0)
    encoded = (1 << 31) | (0b00100110 << 23) | (N << 22) | (immr << 16) |
              (imms << 10) | (rn << 5) | rd;
    break;
  }
  case Opcode::RET: {
    uint32_t rn = encodeRegister(inst.src);
    encoded = 0xD65F0000 | (rn << 5);
    break;
  }
  case Opcode::MOVN: {
    if (!isImmediate(inst.src)) {
      throw std::runtime_error("MOVN only supports immediate operands");
    }
    // MOVN encoding: sf 0 0 1 0 0 1 0 1 hw imm16 Rd
    // Result = ~(imm16 << (hw*16))
    uint64_t imm = std::get<Immediate>(inst.src).value;
    if (imm > 0xFFFF) {
      throw std::runtime_error("MOVN immediate value too large (>16 bits)");
    }
    uint32_t hw = 0; // No shift for now
    encoded = (sf << 31) | (0b00100101 << 23) | (hw << 21) |
              ((imm & 0xFFFF) << 5) | rd;
    break;
  }
  case Opcode::CMP: {
    // CMP for two-operand version - dest register is ignored, src is first
    // operand
    uint32_t rn = encodeRegister(inst.dest); // First operand
    if (isImmediate(inst.src)) {
      // CMP (immediate): sf 1 1 1 0 0 0 1 0 sh imm12 Rn Rt
      // This is SUBS with Rt=XZR (compare is subtract with result discarded)
      uint32_t imm = std::get<Immediate>(inst.src).value;
      if (imm > 0xFFF)
        throw std::runtime_error("CMP immediate value too large (>12 bits)");
      uint32_t sh = 0;   // No shift for simple immediate
      uint32_t xzr = 31; // XZR register
      encoded = (sf << 31) | (0b11100010 << 23) | (sh << 22) | (imm << 10) |
                (rn << 5) | xzr;
    } else {
      // CMP (shifted register): sf 1 1 0 1 0 1 1 shift 0 Rm imm6 Rn Rd
      // This is SUBS with Rd=XZR
      uint32_t rm = encodeRegister(inst.src);
      uint32_t shift = 0; // LSL
      uint32_t imm6 = 0;  // No shift amount
      uint32_t xzr = 31;  // XZR register
      encoded = (sf << 31) | (0b1101011 << 24) | (shift << 22) | (rm << 16) |
                (imm6 << 10) | (rn << 5) | xzr;
    }
    break;
  }
  default:
    throw std::runtime_error("Unsupported two-operand instruction opcode");
  }

  return encoded;
}

uint32_t Encoder::encodeMemoryInst(const MemoryInst &inst) {
  uint32_t encoded = 0;
  uint32_t rt = encodeRegister(inst.reg);
  uint32_t rn = encodeRegister(inst.baseReg);

  int32_t offset = inst.offset;
  uint32_t size = 0;

  switch (inst.size) {
  case DataSize::B:
    size = 0b00;
    break;
  case DataSize::H:
    size = 0b01;
    break;
  case DataSize::W:
    size = 0b10;
    break;
  case DataSize::X:
    size = 0b11;
    break;
  }

  switch (inst.opcode) {
  case Opcode::LDR: {
    if (offset >= 0 && (offset % (1 << size)) == 0) {
      // LDR (immediate, unsigned offset): size 1 1 1 0 0 1 0 1 imm12 Rn Rt
      // size=bits31-30, bits29-22=11100101, imm12=bits21-10, Rn=bits9-5,
      // Rt=bits4-0
      uint32_t scaledOffset = offset >> size;
      if (scaledOffset <= 0xFFF) {
        encoded = (size << 30) | (0b11100101 << 22) | (scaledOffset << 10) |
                  (rn << 5) | rt;
      } else {
        throw std::runtime_error("LDR scaled offset too large (>4095)");
      }
    } else if (offset >= -256 && offset <= 255) {
      // LDR (immediate, pre/post-index): size 1 1 1 0 0 0 0 0 imm9 1 1 Rn Rt
      // (post-index) Using unscaled immediate variant for simplicity
      encoded = (size << 30) | (0b11100000 << 22) | ((offset & 0x1FF) << 12) |
                (0b11 << 10) | (rn << 5) | rt;
    } else {
      throw std::runtime_error("LDR offset out of range");
    }
    break;
  }
  case Opcode::STR: {
    if (offset >= 0 && (offset % (1 << size)) == 0) {
      // STR (immediate, unsigned offset): size 1 1 1 0 0 1 0 0 imm12 Rn Rt
      // size=bits31-30, bits29-22=11100100, imm12=bits21-10, Rn=bits9-5,
      // Rt=bits4-0
      uint32_t scaledOffset = offset >> size;
      if (scaledOffset <= 0xFFF) {
        encoded = (size << 30) | (0b11100100 << 22) | (scaledOffset << 10) |
                  (rn << 5) | rt;
      } else {
        throw std::runtime_error("STR scaled offset too large (>4095)");
      }
    } else if (offset >= -256 && offset <= 255) {
      // STR (immediate, pre/post-index): size 1 1 1 0 0 0 0 0 imm9 0 1 Rn Rt
      // (post-index) Using unscaled immediate variant for simplicity
      encoded = (size << 30) | (0b11100000 << 22) | ((offset & 0x1FF) << 12) |
                (0b01 << 10) | (rn << 5) | rt;
    } else {
      throw std::runtime_error("STR offset out of range");
    }
    break;
  }
  default:
    throw std::runtime_error("Unsupported memory instruction opcode");
  }

  return encoded;
}

uint32_t Encoder::encodeBranchInst(const BranchInst &inst) {
  uint32_t encoded = 0;
  int64_t offset = static_cast<int64_t>(inst.target);

  if (offset < -0x2000000 || offset > 0x1FFFFFF) {
    throw std::runtime_error("Branch target out of range");
  }

  uint32_t imm26 = (offset >> 2) & 0x3FFFFFF;

  switch (inst.opcode) {
  case Opcode::B:
    // B (branch): 0 0 0 1 0 1 imm26
    // bits31-26=000101, imm26=bits25-0 (signed immediate, word-aligned)
    encoded = (0b000101 << 26) | imm26;
    break;
  case Opcode::B_EQ:
  case Opcode::B_NE:
  case Opcode::B_LT:
  case Opcode::B_LE:
  case Opcode::B_GT:
  case Opcode::B_GE: {
    // B.cond (conditional branch): 0 1 0 1 0 1 0 0 imm19 0 cond
    // bits31-25=0101010, imm19=bits23-5, bit4=0, cond=bits3-0
    if (offset < -0x40000 || offset > 0x3FFFF) {
      throw std::runtime_error("Conditional branch target out of range");
    }
    uint32_t imm19 = (offset >> 2) & 0x7FFFF;
    uint32_t cond = getConditionCode(inst.opcode);
    encoded = (0b0101010 << 25) | (imm19 << 5) | cond;
    break;
  }
  default:
    throw std::runtime_error("Unsupported branch instruction opcode");
  }

  return encoded;
}

uint32_t Encoder::encodeMoveWideInst(const MoveWideInst &inst) {
  uint32_t encoded = 0;
  uint32_t sf = getSfBit(inst.size);
  uint32_t rd = encodeRegister(inst.dest);

  // hw field encodes the shift amount: 0=LSL #0, 1=LSL #16, 2=LSL #32, 3=LSL
  // #48
  uint32_t hw = inst.shift / 16;
  if (hw > 3) {
    throw std::runtime_error("Invalid shift amount for move wide instruction");
  }

  switch (inst.opcode) {
  case Opcode::MOVZ:
    // MOVZ: sf 1 0 1 0 0 1 0 1 hw imm16 Rd
    // sf=bit31, bits30-23=10100101, hw=bits22-21, imm16=bits20-5, Rd=bits4-0
    encoded = (sf << 31) | (0b10100101 << 23) | (hw << 21) |
              (static_cast<uint32_t>(inst.imm16) << 5) | rd;
    break;
  case Opcode::MOVK:
    // MOVK: sf 1 1 1 0 0 1 0 1 hw imm16 Rd
    // sf=bit31, bits30-23=11100101, hw=bits22-21, imm16=bits20-5, Rd=bits4-0
    encoded = (sf << 31) | (0b11100101 << 23) | (hw << 21) |
              (static_cast<uint32_t>(inst.imm16) << 5) | rd;
    break;
  default:
    throw std::runtime_error("Unsupported move wide instruction opcode");
  }

  return encoded;
}

uint32_t Encoder::encodeRegister(const Operand &operand) {
  if (std::holds_alternative<Register>(operand)) {
    Register reg = std::get<Register>(operand);
    if (reg == Register::XSP) {
      return 31;
    }
    return static_cast<uint32_t>(reg);
  }
  if (std::holds_alternative<VirtualRegister>(operand)) {
    throw std::runtime_error(
        "Cannot encode virtual register - register allocation required");
  }
  throw std::runtime_error("Invalid operand type for register encoding");
}

bool Encoder::isImmediate(const Operand &operand) {
  return std::holds_alternative<Immediate>(operand);
}

uint32_t Encoder::getSfBit(DataSize size) {
  return (size == DataSize::X) ? 1 : 0;
}

uint32_t Encoder::getConditionCode(Opcode opcode) {
  switch (opcode) {
  case Opcode::B_EQ:
    return 0b0000;
  case Opcode::B_NE:
    return 0b0001;
  case Opcode::B_LT:
    return 0b1011;
  case Opcode::B_LE:
    return 0b1101;
  case Opcode::B_GT:
    return 0b1100;
  case Opcode::B_GE:
    return 0b1010;
  default:
    throw std::runtime_error("Unsupported condition code opcode");
  }
}

uint32_t Encoder::encodeConditionalInst(const ConditionalInst &inst) {
  uint32_t encoded = 0;
  uint32_t sf = getSfBit(inst.size);
  uint32_t rd = encodeRegister(inst.dest);

  switch (inst.opcode) {
  case Opcode::CSET: {
    // CSET: sf 0 0 1 1 0 1 0 1 0 0 11111 cond 0 1 11111 Rd
    // This is actually CSINC Rd, XZR, XZR, !cond
    // sf=bit31, bits30-21=0011010100, Rm=11111, cond=!condition,
    // bits11-10=01, Rn=11111, Rd=bits4-0
    uint32_t cond =
        static_cast<uint32_t>(inst.condition) ^ 1; // Invert condition
    encoded = (sf << 31) | (0b0011010100 << 21) | (0b11111 << 16) |
              (cond << 12) | (0b01 << 10) | (0b11111 << 5) | rd;
    break;
  }
  default:
    throw std::runtime_error("Unsupported conditional instruction");
  }

  return encoded;
}

uint32_t
Encoder::encodeConditionalSelectInst(const ConditionalSelectInst &inst) {
  uint32_t encoded = 0;
  uint32_t sf = getSfBit(inst.size);
  uint32_t rd = encodeRegister(inst.dest);
  uint32_t rn = encodeRegister(inst.src1);

  switch (inst.opcode) {
  case Opcode::CSEL: {
    if (isImmediate(inst.src1) || isImmediate(inst.src2)) {
      throw std::runtime_error("CSEL with immediate operands not supported");
    }
    // CSEL: sf 0 0 1 1 0 1 0 1 0 0 Rm cond 0 0 Rn Rd
    // sf=bit31, bits30-21=0011010100, Rm=bits20-16, cond=bits15-12,
    // bits11-10=00, Rn=bits9-5, Rd=bits4-0
    uint32_t rm = encodeRegister(inst.src2);
    uint32_t cond = static_cast<uint32_t>(inst.condition);
    encoded = (sf << 31) | (0b0011010100 << 21) | (rm << 16) | (cond << 12) |
              (rn << 5) | rd;
    break;
  }
  default:
    throw std::runtime_error("Unsupported conditional select instruction");
  }

  return encoded;
}

} // namespace arm64
} // namespace dinorisc
