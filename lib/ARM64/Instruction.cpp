#include "Instruction.h"
#include <sstream>

namespace dinorisc {
namespace arm64 {

std::string registerToString(Register reg) {
  switch (reg) {
  case Register::X0:
    return "x0";
  case Register::X1:
    return "x1";
  case Register::X2:
    return "x2";
  case Register::X3:
    return "x3";
  case Register::X4:
    return "x4";
  case Register::X5:
    return "x5";
  case Register::X6:
    return "x6";
  case Register::X7:
    return "x7";
  case Register::X8:
    return "x8";
  case Register::X9:
    return "x9";
  case Register::X10:
    return "x10";
  case Register::X11:
    return "x11";
  case Register::X12:
    return "x12";
  case Register::X13:
    return "x13";
  case Register::X14:
    return "x14";
  case Register::X15:
    return "x15";
  case Register::X16:
    return "x16";
  case Register::X17:
    return "x17";
  case Register::X18:
    return "x18";
  case Register::X19:
    return "x19";
  case Register::X20:
    return "x20";
  case Register::X21:
    return "x21";
  case Register::X22:
    return "x22";
  case Register::X23:
    return "x23";
  case Register::X24:
    return "x24";
  case Register::X25:
    return "x25";
  case Register::X26:
    return "x26";
  case Register::X27:
    return "x27";
  case Register::X28:
    return "x28";
  case Register::X29:
    return "x29";
  case Register::X30:
    return "x30";
  case Register::XSP:
    return "sp";
  }
  return "unknown";
}

std::string opcodeToString(Opcode opcode) {
  switch (opcode) {
  case Opcode::ADD:
    return "add";
  case Opcode::SUB:
    return "sub";
  case Opcode::MUL:
    return "mul";
  case Opcode::UDIV:
    return "udiv";
  case Opcode::SDIV:
    return "sdiv";
  case Opcode::AND:
    return "and";
  case Opcode::ORR:
    return "orr";
  case Opcode::EOR:
    return "eor";
  case Opcode::LSL:
    return "lsl";
  case Opcode::LSR:
    return "lsr";
  case Opcode::ASR:
    return "asr";
  case Opcode::LDR:
    return "ldr";
  case Opcode::STR:
    return "str";
  case Opcode::CMP:
    return "cmp";
  case Opcode::B:
    return "b";
  case Opcode::B_EQ:
    return "b.eq";
  case Opcode::B_NE:
    return "b.ne";
  case Opcode::B_LT:
    return "b.lt";
  case Opcode::B_LE:
    return "b.le";
  case Opcode::B_GT:
    return "b.gt";
  case Opcode::B_GE:
    return "b.ge";
  case Opcode::SXTB:
    return "sxtb";
  case Opcode::SXTH:
    return "sxth";
  case Opcode::SXTW:
    return "sxtw";
  case Opcode::UXTB:
    return "uxtb";
  case Opcode::UXTH:
    return "uxth";
  case Opcode::MOV:
    return "mov";
  case Opcode::RET:
    return "ret";
  }
  return "unknown";
}

std::string dataSizeToString(DataSize size) {
  switch (size) {
  case DataSize::B:
    return "b";
  case DataSize::H:
    return "h";
  case DataSize::W:
    return "w";
  case DataSize::X:
    return "x";
  }
  return "unknown";
}

static std::string operandToString(const Operand &operand) {
  if (std::holds_alternative<Register>(operand)) {
    return registerToString(std::get<Register>(operand));
  } else if (std::holds_alternative<VirtualReg>(operand)) {
    return "v" + std::to_string(std::get<VirtualReg>(operand).id);
  } else {
    return "#" + std::to_string(std::get<Immediate>(operand).value);
  }
}

std::string Instruction::toString() const {
  std::ostringstream oss;

  std::visit(
      [&](const auto &inst) {
        using T = std::decay_t<decltype(inst)>;

        if constexpr (std::is_same_v<T, ThreeOperandInst>) {
          oss << opcodeToString(inst.opcode) << " "
              << operandToString(inst.dest) << ", "
              << operandToString(inst.src1) << ", "
              << operandToString(inst.src2);
        } else if constexpr (std::is_same_v<T, TwoOperandInst>) {
          oss << opcodeToString(inst.opcode) << " "
              << operandToString(inst.dest) << ", "
              << operandToString(inst.src);
        } else if constexpr (std::is_same_v<T, MemoryInst>) {
          oss << opcodeToString(inst.opcode) << " " << operandToString(inst.reg)
              << ", [" << operandToString(inst.baseReg);
          if (inst.offset != 0) {
            oss << ", #" << inst.offset;
          }
          oss << "]";
        } else if constexpr (std::is_same_v<T, BranchInst>) {
          oss << opcodeToString(inst.opcode) << " 0x" << std::hex
              << inst.target;
        }
      },
      kind);

  return oss.str();
}

} // namespace arm64
} // namespace dinorisc