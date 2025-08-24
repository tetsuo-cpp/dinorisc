#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dinorisc {

class RV64IInstruction {
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

  enum class OperandType { REGISTER, IMMEDIATE, NONE };

  struct Operand {
    OperandType type;
    union {
      uint32_t reg;
      int64_t imm;
    };

    Operand() : type(OperandType::NONE), reg(0) {}
    explicit Operand(uint32_t r) : type(OperandType::REGISTER), reg(r) {}
    explicit Operand(int64_t i) : type(OperandType::IMMEDIATE), imm(i) {}
  };

  // Public members - this is a simple data container
  Opcode opcode;
  std::vector<Operand> operands;
  uint32_t rawInstruction;
  uint64_t address;

  // Constructors
  RV64IInstruction() : opcode(Opcode::INVALID), rawInstruction(0), address(0) {}

  RV64IInstruction(Opcode op, std::vector<Operand> ops, uint32_t raw,
                   uint64_t addr)
      : opcode(op), operands(std::move(ops)), rawInstruction(raw),
        address(addr) {}

  // Utility methods
  std::string toString() const;
  bool isValid() const { return opcode != Opcode::INVALID; }

  // Helper methods for common operand patterns
  bool hasRegisterOperand(size_t index) const {
    return index < operands.size() &&
           operands[index].type == OperandType::REGISTER;
  }

  bool hasImmediateOperand(size_t index) const {
    return index < operands.size() &&
           operands[index].type == OperandType::IMMEDIATE;
  }

  uint32_t getRegister(size_t index) const {
    return (hasRegisterOperand(index)) ? operands[index].reg : 0;
  }

  int64_t getImmediate(size_t index) const {
    return (hasImmediateOperand(index)) ? operands[index].imm : 0;
  }

private:
  static std::string opcodeToString(Opcode op);
  static std::string operandToString(const Operand &op);
};

} // namespace dinorisc