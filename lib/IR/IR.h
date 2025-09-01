#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace dinorisc {
namespace ir {

enum class Type { i1, i8, i16, i32, i64 };

using ValueId = uint64_t;

enum class BinaryOpcode {
  // Arithmetic
  Add,
  Sub,
  Mul,
  Div,
  DivU,
  Rem,
  RemU,

  // Bitwise
  And,
  Or,
  Xor,
  Shl,
  Shr,
  Sar,

  // Comparison
  Eq,
  Ne,
  Lt,
  Le,
  Gt,
  Ge,
  LtU,
  LeU,
  GtU,
  GeU
};

struct Const {
  Type type;
  uint64_t value;
};

struct BinaryOp {
  BinaryOpcode opcode;
  Type type;
  ValueId lhs;
  ValueId rhs;
};

struct Sext {
  Type fromType;
  Type toType;
  ValueId operand;
};

struct Zext {
  Type fromType;
  Type toType;
  ValueId operand;
};

struct Trunc {
  Type fromType;
  Type toType;
  ValueId operand;
};

struct Load {
  Type type;
  ValueId address;
};

struct Store {
  ValueId value;
  ValueId address;
};

using InstructionKind =
    std::variant<Const, BinaryOp, Sext, Zext, Trunc, Load, Store>;

struct Instruction {
  ValueId valueId;
  InstructionKind kind;

  std::string toString() const;
};

struct Branch {
  uint64_t targetBlock;
};

struct CondBranch {
  ValueId condition;
  uint64_t trueBlock;
  uint64_t falseBlock;
};

struct Return {
  ValueId value;
  bool hasValue;
};

using TerminatorKind = std::variant<Branch, CondBranch, Return>;

struct Terminator {
  TerminatorKind kind;

  std::string toString() const;
};

struct BasicBlock {
  std::vector<Instruction> instructions;
  Terminator terminator;

  std::string toString() const;
};

std::string binaryOpcodeToString(BinaryOpcode op);
std::string typeToString(Type type);

} // namespace ir
} // namespace dinorisc