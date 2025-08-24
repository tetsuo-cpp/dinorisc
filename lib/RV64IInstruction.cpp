#include "RV64IInstruction.h"
#include <iomanip>
#include <sstream>

namespace dinorisc {

std::string RV64IInstruction::toString() const {
  std::stringstream ss;
  ss << "RV64I[0x" << std::hex << std::setfill('0') << std::setw(8)
     << rawInstruction << " @ 0x" << address << "] ";

  if (!isValid()) {
    ss << "INVALID";
    return ss.str();
  }

  ss << opcodeToString(opcode);

  if (!operands.empty()) {
    ss << " ";
    for (size_t i = 0; i < operands.size(); ++i) {
      if (i > 0)
        ss << ", ";
      ss << operandToString(operands[i]);
    }
  }

  return ss.str();
}

std::string RV64IInstruction::opcodeToString(Opcode op) {
  switch (op) {
  case Opcode::ADD:
    return "ADD";
  case Opcode::ADDI:
    return "ADDI";
  case Opcode::ADDW:
    return "ADDW";
  case Opcode::ADDIW:
    return "ADDIW";
  case Opcode::SUB:
    return "SUB";
  case Opcode::SUBW:
    return "SUBW";
  case Opcode::AND:
    return "AND";
  case Opcode::ANDI:
    return "ANDI";
  case Opcode::OR:
    return "OR";
  case Opcode::ORI:
    return "ORI";
  case Opcode::XOR:
    return "XOR";
  case Opcode::XORI:
    return "XORI";
  case Opcode::SLL:
    return "SLL";
  case Opcode::SLLI:
    return "SLLI";
  case Opcode::SLLW:
    return "SLLW";
  case Opcode::SLLIW:
    return "SLLIW";
  case Opcode::SRL:
    return "SRL";
  case Opcode::SRLI:
    return "SRLI";
  case Opcode::SRLW:
    return "SRLW";
  case Opcode::SRLIW:
    return "SRLIW";
  case Opcode::SRA:
    return "SRA";
  case Opcode::SRAI:
    return "SRAI";
  case Opcode::SRAW:
    return "SRAW";
  case Opcode::SRAIW:
    return "SRAIW";
  case Opcode::SLT:
    return "SLT";
  case Opcode::SLTI:
    return "SLTI";
  case Opcode::SLTU:
    return "SLTU";
  case Opcode::SLTIU:
    return "SLTIU";
  case Opcode::LB:
    return "LB";
  case Opcode::LH:
    return "LH";
  case Opcode::LW:
    return "LW";
  case Opcode::LD:
    return "LD";
  case Opcode::LBU:
    return "LBU";
  case Opcode::LHU:
    return "LHU";
  case Opcode::LWU:
    return "LWU";
  case Opcode::SB:
    return "SB";
  case Opcode::SH:
    return "SH";
  case Opcode::SW:
    return "SW";
  case Opcode::SD:
    return "SD";
  case Opcode::BEQ:
    return "BEQ";
  case Opcode::BNE:
    return "BNE";
  case Opcode::BLT:
    return "BLT";
  case Opcode::BGE:
    return "BGE";
  case Opcode::BLTU:
    return "BLTU";
  case Opcode::BGEU:
    return "BGEU";
  case Opcode::JAL:
    return "JAL";
  case Opcode::JALR:
    return "JALR";
  case Opcode::LUI:
    return "LUI";
  case Opcode::AUIPC:
    return "AUIPC";
  case Opcode::ECALL:
    return "ECALL";
  case Opcode::EBREAK:
    return "EBREAK";
  case Opcode::INVALID:
    return "INVALID";
  }
  return "UNKNOWN";
}

std::string RV64IInstruction::operandToString(const Operand &op) {
  std::stringstream ss;
  switch (op.type) {
  case OperandType::REGISTER:
    ss << "x" << op.reg;
    break;
  case OperandType::IMMEDIATE:
    ss << op.imm;
    break;
  case OperandType::NONE:
    ss << "none";
    break;
  }
  return ss.str();
}

} // namespace dinorisc