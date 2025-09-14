#pragma once

#include <cstdint>
#include <string>
#include <variant>

namespace dinorisc {
namespace arm64 {

enum class Register : uint8_t {
  X0,
  X1,
  X2,
  X3,
  X4,
  X5,
  X6,
  X7,
  X8,
  X9,
  X10,
  X11,
  X12,
  X13,
  X14,
  X15,
  X16,
  X17,
  X18,
  X19,
  X20,
  X21,
  X22,
  X23,
  X24,
  X25,
  X26,
  X27,
  X28,
  X29, // Frame pointer
  X30, // Link register
  XSP  // Stack pointer
};

enum class Opcode {
  // Arithmetic
  ADD,
  SUB,
  MUL,
  UDIV,
  SDIV,

  // Bitwise
  AND,
  ORR,
  EOR,
  LSL,
  LSR,
  ASR,

  // Load/Store
  LDR,
  STR,

  // Compare and branch
  CMP,
  B,
  B_EQ,
  B_NE,
  B_LT,
  B_LE,
  B_GT,
  B_GE,

  // Conditional operations
  CSEL,
  CSET,

  // Extension
  SXTB,
  SXTH,
  SXTW,
  UXTB,
  UXTH,

  // Other
  MOV,
  MOVN,
  MOVZ,
  MOVK,
  RET
};

enum class DataSize : uint8_t {
  B, // 8-bit
  H, // 16-bit
  W, // 32-bit
  X  // 64-bit
};

enum class Condition : uint8_t {
  EQ = 0b0000, // Equal
  NE = 0b0001, // Not equal
  CS = 0b0010, // Carry set / unsigned higher or same
  CC = 0b0011, // Carry clear / unsigned lower
  MI = 0b0100, // Minus / negative
  PL = 0b0101, // Plus / positive or zero
  VS = 0b0110, // Overflow set
  VC = 0b0111, // Overflow clear
  HI = 0b1000, // Unsigned higher
  LS = 0b1001, // Unsigned lower or same
  GE = 0b1010, // Signed greater than or equal
  LT = 0b1011, // Signed less than
  GT = 0b1100, // Signed greater than
  LE = 0b1101, // Signed less than or equal
  AL = 0b1110, // Always
  NV = 0b1111  // Never
};

struct Immediate {
  uint64_t value;
};

using VirtualRegister = uint32_t;

using Operand = std::variant<Register, VirtualRegister, Immediate>;

struct ThreeOperandInst {
  Opcode opcode;
  DataSize size;
  Operand dest;
  Operand src1;
  Operand src2;
};

struct TwoOperandInst {
  Opcode opcode;
  DataSize size;
  Operand dest;
  Operand src;
};

struct MemoryInst {
  Opcode opcode;
  DataSize size;
  Operand reg;
  Operand baseReg;
  int32_t offset;
};

struct MoveWideInst {
  Opcode opcode; // MOVZ or MOVK
  DataSize size;
  Operand dest;
  uint16_t imm16; // 16-bit immediate
  uint8_t shift;  // LSL shift amount (0, 16, 32, or 48)
};

struct BranchInst {
  Opcode opcode;
  uint64_t target;
};

struct ConditionalInst {
  Opcode opcode;
  DataSize size;
  Operand dest;
  Condition condition;
};

struct ConditionalSelectInst {
  Opcode opcode;
  DataSize size;
  Operand dest;
  Operand src1;
  Operand src2;
  Condition condition;
};

using InstructionKind =
    std::variant<ThreeOperandInst, TwoOperandInst, MemoryInst, MoveWideInst,
                 BranchInst, ConditionalInst, ConditionalSelectInst>;

struct Instruction {
  InstructionKind kind;

  std::string toString() const;
};

std::string registerToString(Register reg);
std::string opcodeToString(Opcode opcode);
std::string dataSizeToString(DataSize size);
std::string conditionToString(Condition condition);

} // namespace arm64
} // namespace dinorisc