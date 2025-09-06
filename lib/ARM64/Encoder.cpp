#include "Encoder.h"
#include <algorithm>
#include <cassert>
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
        } else if constexpr (std::is_same_v<T, BranchInst>) {
          encoded = encodeBranchInst(instruction);
        }
      },
      inst.kind);

  if (encoded == 0) {
    return {};
  }

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
      uint32_t imm = encodeImmediate(inst.src2, 12);
      if (!isValidImmediate(imm, 12))
        return 0;
      encoded = (sf << 31) | (0b00100010 << 23) | (imm << 10) | (rn << 5) | rd;
    } else {
      uint32_t rm = encodeRegister(inst.src2);
      encoded = (sf << 31) | (0b00001011 << 21) | (rm << 16) | (rn << 5) | rd;
    }
    break;
  }
  case Opcode::SUB: {
    if (isImmediate(inst.src2)) {
      uint32_t imm = encodeImmediate(inst.src2, 12);
      if (!isValidImmediate(imm, 12))
        return 0;
      encoded = (sf << 31) | (0b01100010 << 23) | (imm << 10) | (rn << 5) | rd;
    } else {
      uint32_t rm = encodeRegister(inst.src2);
      encoded = (sf << 31) | (0b01001011 << 21) | (rm << 16) | (rn << 5) | rd;
    }
    break;
  }
  case Opcode::AND: {
    if (isImmediate(inst.src2)) {
      return 0;
    }
    uint32_t rm = encodeRegister(inst.src2);
    encoded = (sf << 31) | (0b00001010 << 21) | (rm << 16) | (rn << 5) | rd;
    break;
  }
  case Opcode::ORR: {
    if (isImmediate(inst.src2)) {
      return 0;
    }
    uint32_t rm = encodeRegister(inst.src2);
    encoded = (sf << 31) | (0b00101010 << 21) | (rm << 16) | (rn << 5) | rd;
    break;
  }
  case Opcode::EOR: {
    if (isImmediate(inst.src2)) {
      return 0;
    }
    uint32_t rm = encodeRegister(inst.src2);
    encoded = (sf << 31) | (0b01001010 << 21) | (rm << 16) | (rn << 5) | rd;
    break;
  }
  case Opcode::MUL: {
    if (isImmediate(inst.src2)) {
      return 0;
    }
    uint32_t rm = encodeRegister(inst.src2);
    encoded = (sf << 31) | (0b00011011 << 21) | (rm << 16) | (0b011111 << 10) |
              (rn << 5) | rd;
    break;
  }
  default:
    return 0;
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
      if (imm <= 0xFFFF) {
        encoded = (sf << 31) | (0b10100101 << 23) | ((imm & 0xFFFF) << 5) | rd;
      } else {
        return 0;
      }
    } else {
      uint32_t rm = encodeRegister(inst.src);
      encoded =
          (sf << 31) | (0b00101010 << 21) | (rm << 16) | (0b011111 << 5) | rd;
    }
    break;
  }
  case Opcode::SXTB: {
    if (isImmediate(inst.src))
      return 0;
    uint32_t rn = encodeRegister(inst.src);
    encoded =
        (sf << 31) | (0b00010011 << 23) | (0b000000 << 10) | (rn << 5) | rd;
    break;
  }
  case Opcode::SXTH: {
    if (isImmediate(inst.src))
      return 0;
    uint32_t rn = encodeRegister(inst.src);
    encoded =
        (sf << 31) | (0b00010011 << 23) | (0b000001 << 10) | (rn << 5) | rd;
    break;
  }
  case Opcode::SXTW: {
    if (isImmediate(inst.src))
      return 0;
    uint32_t rn = encodeRegister(inst.src);
    encoded =
        (1 << 31) | (0b00010011 << 23) | (0b000010 << 10) | (rn << 5) | rd;
    break;
  }
  case Opcode::RET: {
    if (isImmediate(inst.src)) {
      encoded = 0xD65F03C0;
    } else {
      uint32_t rn = encodeRegister(inst.src);
      encoded = 0xD6400000 | (rn << 5);
    }
    break;
  }
  default:
    return 0;
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
      uint32_t scaledOffset = offset >> size;
      if (scaledOffset <= 0xFFF) {
        encoded = (size << 30) | (0b111001001 << 21) | (scaledOffset << 10) |
                  (rn << 5) | rt;
      } else {
        return 0;
      }
    } else if (offset >= -256 && offset <= 255) {
      encoded = (size << 30) | (0b111000010 << 21) | ((offset & 0x1FF) << 12) |
                (rn << 5) | rt;
    } else {
      return 0;
    }
    break;
  }
  case Opcode::STR: {
    if (offset >= 0 && (offset % (1 << size)) == 0) {
      uint32_t scaledOffset = offset >> size;
      if (scaledOffset <= 0xFFF) {
        encoded = (size << 30) | (0b111001000 << 21) | (scaledOffset << 10) |
                  (rn << 5) | rt;
      } else {
        return 0;
      }
    } else if (offset >= -256 && offset <= 255) {
      encoded = (size << 30) | (0b111000000 << 21) | ((offset & 0x1FF) << 12) |
                (rn << 5) | rt;
    } else {
      return 0;
    }
    break;
  }
  default:
    return 0;
  }

  return encoded;
}

uint32_t Encoder::encodeBranchInst(const BranchInst &inst) {
  uint32_t encoded = 0;
  int64_t offset = static_cast<int64_t>(inst.target);

  if (offset < -0x2000000 || offset > 0x1FFFFFF) {
    return 0;
  }

  uint32_t imm26 = (offset >> 2) & 0x3FFFFFF;

  switch (inst.opcode) {
  case Opcode::B:
    encoded = (0b000101 << 26) | imm26;
    break;
  case Opcode::B_EQ:
  case Opcode::B_NE:
  case Opcode::B_LT:
  case Opcode::B_LE:
  case Opcode::B_GT:
  case Opcode::B_GE: {
    if (offset < -0x40000 || offset > 0x3FFFF) {
      return 0;
    }
    uint32_t imm19 = (offset >> 2) & 0x7FFFF;
    uint32_t cond = getConditionCode(inst.opcode);
    encoded = (0b0101010 << 25) | (imm19 << 5) | cond;
    break;
  }
  default:
    return 0;
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
  return 0;
}

uint32_t Encoder::encodeImmediate(const Operand &operand, uint8_t bitWidth) {
  if (std::holds_alternative<Immediate>(operand)) {
    uint64_t value = std::get<Immediate>(operand).value;
    uint64_t mask = (1ULL << bitWidth) - 1;
    return static_cast<uint32_t>(value & mask);
  }
  return 0;
}

bool Encoder::isImmediate(const Operand &operand) {
  return std::holds_alternative<Immediate>(operand);
}

bool Encoder::isValidImmediate(uint64_t value, uint8_t bitWidth) {
  uint64_t maxValue = (1ULL << bitWidth) - 1;
  return value <= maxValue;
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
    return 0b0000;
  }
}

} // namespace arm64
} // namespace dinorisc