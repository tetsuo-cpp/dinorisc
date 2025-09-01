#include "IR.h"
#include <sstream>

namespace dinorisc {
namespace ir {

std::string Instruction::toString() const {
  std::stringstream ss;

  // Store instructions don't produce values
  if (!std::holds_alternative<Store>(kind)) {
    ss << "%" << valueId << " = ";
  }

  return std::visit(
      [&](const auto &inst) -> std::string {
        using T = std::decay_t<decltype(inst)>;
        if constexpr (std::is_same_v<T, Const>) {
          ss << typeToString(inst.type) << " " << inst.value;
        } else if constexpr (std::is_same_v<T, BinaryOp>) {
          ss << binaryOpcodeToString(inst.opcode) << " "
             << typeToString(inst.type) << " %" << inst.lhs << ", %"
             << inst.rhs;
        } else if constexpr (std::is_same_v<T, Sext>) {
          ss << "sext " << typeToString(inst.fromType) << " to "
             << typeToString(inst.toType) << " %" << inst.operand;
        } else if constexpr (std::is_same_v<T, Zext>) {
          ss << "zext " << typeToString(inst.fromType) << " to "
             << typeToString(inst.toType) << " %" << inst.operand;
        } else if constexpr (std::is_same_v<T, Trunc>) {
          ss << "trunc " << typeToString(inst.fromType) << " to "
             << typeToString(inst.toType) << " %" << inst.operand;
        } else if constexpr (std::is_same_v<T, Load>) {
          ss << "load " << typeToString(inst.type) << " %" << inst.address;
        } else if constexpr (std::is_same_v<T, Store>) {
          ss << "store %" << inst.value << ", %" << inst.address;
        }
        return ss.str();
      },
      kind);
}

std::string Terminator::toString() const {
  std::stringstream ss;

  return std::visit(
      [&](const auto &term) -> std::string {
        using T = std::decay_t<decltype(term)>;
        if constexpr (std::is_same_v<T, Branch>) {
          ss << "br bb" << term.targetBlock;
        } else if constexpr (std::is_same_v<T, CondBranch>) {
          ss << "condbr %" << term.condition << ", bb" << term.trueBlock
             << ", bb" << term.falseBlock;
        } else if constexpr (std::is_same_v<T, Return>) {
          if (term.hasValue) {
            ss << "ret %" << term.value;
          } else {
            ss << "ret";
          }
        }
        return ss.str();
      },
      kind);
}

std::string BasicBlock::toString() const {
  std::stringstream ss;
  ss << "BasicBlock:\n";
  for (const auto &inst : instructions) {
    ss << "  " << inst.toString() << "\n";
  }
  ss << "  " << terminator.toString() << "\n";
  return ss.str();
}

std::string binaryOpcodeToString(BinaryOpcode op) {
  switch (op) {
  case BinaryOpcode::Add:
    return "add";
  case BinaryOpcode::Sub:
    return "sub";
  case BinaryOpcode::Mul:
    return "mul";
  case BinaryOpcode::Div:
    return "div";
  case BinaryOpcode::DivU:
    return "divu";
  case BinaryOpcode::Rem:
    return "rem";
  case BinaryOpcode::RemU:
    return "remu";
  case BinaryOpcode::And:
    return "and";
  case BinaryOpcode::Or:
    return "or";
  case BinaryOpcode::Xor:
    return "xor";
  case BinaryOpcode::Shl:
    return "shl";
  case BinaryOpcode::Shr:
    return "shr";
  case BinaryOpcode::Sar:
    return "sar";
  case BinaryOpcode::Eq:
    return "eq";
  case BinaryOpcode::Ne:
    return "ne";
  case BinaryOpcode::Lt:
    return "lt";
  case BinaryOpcode::Le:
    return "le";
  case BinaryOpcode::Gt:
    return "gt";
  case BinaryOpcode::Ge:
    return "ge";
  case BinaryOpcode::LtU:
    return "ltu";
  case BinaryOpcode::LeU:
    return "leu";
  case BinaryOpcode::GtU:
    return "gtu";
  case BinaryOpcode::GeU:
    return "geu";
  }
  return "unknown";
}

std::string typeToString(Type type) {
  switch (type) {
  case Type::i1:
    return "i1";
  case Type::i8:
    return "i8";
  case Type::i16:
    return "i16";
  case Type::i32:
    return "i32";
  case Type::i64:
    return "i64";
  }
  return "unknown";
}

} // namespace ir
} // namespace dinorisc