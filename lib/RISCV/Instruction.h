#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace dinorisc {
namespace riscv {

class Instruction {
public:
  enum class Opcode {
    // Arithmetic and Logic Instructions
    ADD,
    ADDI,
    ADDW,
    ADDIW,
    SUB,
    SUBW,
    AND,
    ANDI,
    OR,
    ORI,
    XOR,
    XORI,
    SLL,
    SLLI,
    SLLW,
    SLLIW,
    SRL,
    SRLI,
    SRLW,
    SRLIW,
    SRA,
    SRAI,
    SRAW,
    SRAIW,
    SLT,
    SLTI,
    SLTU,
    SLTIU,

    // Load Instructions
    LB,
    LH,
    LW,
    LD,
    LBU,
    LHU,
    LWU,

    // Store Instructions
    SB,
    SH,
    SW,
    SD,

    // Branch Instructions
    BEQ,
    BNE,
    BLT,
    BGE,
    BLTU,
    BGEU,

    // Jump Instructions
    JAL,
    JALR,

    // Upper Immediate Instructions
    LUI,
    AUIPC,

    // System Instructions
    ECALL,
    EBREAK,

    // Invalid instruction
    INVALID
  };

  struct Register {
    uint32_t value;
    explicit Register(uint32_t v) : value(v) {}
  };

  struct Immediate {
    int64_t value;
    explicit Immediate(int64_t v) : value(v) {}
  };

  struct None {};

  using Operand = std::variant<None, Register, Immediate>;

  // Public members - this is a simple data container
  Opcode opcode;
  std::vector<Operand> operands;
  uint32_t rawInstruction;
  uint64_t address;

  // Constructors
  Instruction() : opcode(Opcode::INVALID), rawInstruction(0), address(0) {}

  Instruction(Opcode op, std::vector<Operand> ops, uint32_t raw, uint64_t addr)
      : opcode(op), operands(std::move(ops)), rawInstruction(raw),
        address(addr) {}

  // Utility methods
  std::string toString() const;
  bool isValid() const { return opcode != Opcode::INVALID; }

  // Helper methods for common operand patterns
  bool hasRegisterOperand(size_t index) const {
    return index < operands.size() &&
           std::holds_alternative<Register>(operands[index]);
  }

  bool hasImmediateOperand(size_t index) const {
    return index < operands.size() &&
           std::holds_alternative<Immediate>(operands[index]);
  }

  uint32_t getRegister(size_t index) const {
    if (hasRegisterOperand(index)) {
      return std::get<Register>(operands[index]).value;
    }
    return 0;
  }

  int64_t getImmediate(size_t index) const {
    if (hasImmediateOperand(index)) {
      return std::get<Immediate>(operands[index]).value;
    }
    return 0;
  }

private:
  static std::string opcodeToString(Opcode op);
  static std::string operandToString(const Operand &op);
};

} // namespace riscv
} // namespace dinorisc