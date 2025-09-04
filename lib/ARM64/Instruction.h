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

  // Extension
  SXTB,
  SXTH,
  SXTW,
  UXTB,
  UXTH,

  // Other
  MOV,
  RET
};

enum class DataSize : uint8_t {
  B, // 8-bit
  H, // 16-bit
  W, // 32-bit
  X  // 64-bit
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

struct BranchInst {
  Opcode opcode;
  uint64_t target;
};

using InstructionKind =
    std::variant<ThreeOperandInst, TwoOperandInst, MemoryInst, BranchInst>;

struct Instruction {
  InstructionKind kind;

  std::string toString() const;
};

std::string registerToString(Register reg);
std::string opcodeToString(Opcode opcode);
std::string dataSizeToString(DataSize size);

} // namespace arm64
} // namespace dinorisc