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

  using Operand = std::variant<Register, Immediate>;

  Opcode opcode;
  std::vector<Operand> operands;
  uint32_t rawInstruction;
  uint64_t address;

  Instruction() : opcode(Opcode::INVALID), rawInstruction(0), address(0) {}

  Instruction(Opcode op, std::vector<Operand> ops, uint32_t raw, uint64_t addr)
      : opcode(op), operands(std::move(ops)), rawInstruction(raw),
        address(addr) {}

  std::string toString() const;
  bool isValid() const { return opcode != Opcode::INVALID; }

  uint32_t getRegister(size_t index) const {
    return std::get<Register>(operands[index]).value;
  }

  int64_t getImmediate(size_t index) const {
    return std::get<Immediate>(operands[index]).value;
  }

private:
  static std::string opcodeToString(Opcode op);
  static std::string operandToString(const Operand &op);
};

} // namespace riscv
} // namespace dinorisc
