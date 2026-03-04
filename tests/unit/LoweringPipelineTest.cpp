#include "Lowering/InstructionSelector.h"
#include "Lowering/LivenessAnalysis.h"
#include "Lowering/RegisterAllocator.h"
#include <catch2/catch_all.hpp>

using namespace dinorisc;
using namespace dinorisc::lowering;

class IRBuilder {
public:
  IRBuilder() : nextValueId(1) {}

  ir::ValueId addConst(ir::Type type, int64_t value) {
    ir::ValueId valueId = nextValueId++;
    ir::Instruction inst{valueId, ir::Const{type, value}};
    instructions.push_back(inst);
    return valueId;
  }

  ir::ValueId addBinaryOp(ir::BinaryOpcode opcode, ir::Type type,
                          ir::ValueId lhs, ir::ValueId rhs) {
    ir::ValueId valueId = nextValueId++;
    ir::Instruction inst{valueId, ir::BinaryOp{opcode, type, lhs, rhs}};
    instructions.push_back(inst);
    return valueId;
  }

  ir::ValueId addLoad(ir::Type type, ir::ValueId address) {
    ir::ValueId valueId = nextValueId++;
    ir::Instruction inst{valueId, ir::Load{type, address}};
    instructions.push_back(inst);
    return valueId;
  }

  void addStore(ir::ValueId value, ir::ValueId address) {
    ir::ValueId valueId = nextValueId++;
    ir::Instruction inst{valueId, ir::Store{value, address}};
    instructions.push_back(inst);
  }

  ir::ValueId addSext(ir::Type toType, ir::ValueId operand) {
    ir::ValueId valueId = nextValueId++;
    ir::Instruction inst{valueId, ir::Sext{toType, operand}};
    instructions.push_back(inst);
    return valueId;
  }

  ir::ValueId addZext(ir::Type toType, ir::ValueId operand) {
    ir::ValueId valueId = nextValueId++;
    ir::Instruction inst{valueId, ir::Zext{toType, operand}};
    instructions.push_back(inst);
    return valueId;
  }

  ir::ValueId addTrunc(ir::Type toType, ir::ValueId operand) {
    ir::ValueId valueId = nextValueId++;
    ir::Instruction inst{valueId, ir::Trunc{toType, operand}};
    instructions.push_back(inst);
    return valueId;
  }

  void setReturnTerminator(ir::ValueId value) {
    terminator = ir::Terminator{ir::Return{value}};
  }

  void setVoidReturnTerminator() {
    terminator = ir::Terminator{ir::Return{std::nullopt}};
  }

  void setBranchTerminator(uint64_t target) {
    terminator = ir::Terminator{ir::Branch{target}};
  }

  void setCondBranchTerminator(ir::ValueId condition, uint64_t trueBlock,
                               uint64_t falseBlock) {
    terminator =
        ir::Terminator{ir::CondBranch{condition, trueBlock, falseBlock}};
  }

  ir::BasicBlock build() { return ir::BasicBlock{instructions, terminator}; }

private:
  ir::ValueId nextValueId;
  std::vector<ir::Instruction> instructions;
  ir::Terminator terminator;
};

bool hasOnlyPhysicalRegisters(
    const std::vector<arm64::Instruction> &instructions) {
  for (const auto &inst : instructions) {
    const auto checkOperand = [](const arm64::Operand &operand) {
      return !std::holds_alternative<arm64::VirtualRegister>(operand);
    };

    bool hasVirtualReg = std::visit(
        [&](const auto &instKind) -> bool {
          using T = std::decay_t<decltype(instKind)>;

          if constexpr (std::is_same_v<T, arm64::ThreeOperandInst>) {
            return !checkOperand(instKind.dest) ||
                   !checkOperand(instKind.src1) || !checkOperand(instKind.src2);
          } else if constexpr (std::is_same_v<T, arm64::TwoOperandInst>) {
            return !checkOperand(instKind.dest) || !checkOperand(instKind.src);
          } else if constexpr (std::is_same_v<T, arm64::MemoryInst>) {
            return !checkOperand(instKind.reg) ||
                   !checkOperand(instKind.baseReg);
          } else if constexpr (std::is_same_v<T, arm64::MoveWideInst>) {
            return !checkOperand(instKind.dest);
          } else if constexpr (std::is_same_v<T, arm64::ConditionalInst>) {
            return !checkOperand(instKind.dest);
          } else if constexpr (std::is_same_v<T,
                                              arm64::ConditionalSelectInst>) {
            return !checkOperand(instKind.dest) ||
                   !checkOperand(instKind.src1) || !checkOperand(instKind.src2);
          } else {
            return false;
          }
        },
        inst.kind);

    if (hasVirtualReg) {
      return false;
    }
  }
  return true;
}

bool containsOpcode(const std::vector<arm64::Instruction> &instructions,
                    arm64::Opcode expectedOpcode) {
  for (const auto &inst : instructions) {
    bool found = std::visit(
        [expectedOpcode](const auto &instKind) {
          return instKind.opcode == expectedOpcode;
        },
        inst.kind);
    if (found)
      return true;
  }
  return false;
}

std::vector<arm64::Instruction> lowerAndVerify(IRBuilder &builder) {
  InstructionSelector selector;
  auto instructions = selector.selectInstructions(builder.build());

  LivenessAnalysis liveness(instructions);
  auto liveIntervals = liveness.computeLiveIntervals();

  RegisterAllocator allocator;
  bool success = allocator.allocateRegisters(instructions, liveIntervals);
  if (!success) {
    throw std::runtime_error("Register allocation failed");
  }

  REQUIRE(!instructions.empty());
  REQUIRE(hasOnlyPhysicalRegisters(instructions));
  return instructions;
}

TEST_CASE("Lowering pipeline basic arithmetic", "[lowering]") {
  SECTION("Simple addition") {
    IRBuilder builder;
    auto v1 = builder.addConst(ir::Type::i64, 10);
    auto v2 = builder.addConst(ir::Type::i64, 20);
    builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, v1, v2);
    builder.setVoidReturnTerminator();

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::MOV));
    REQUIRE(containsOpcode(result, arm64::Opcode::ADD));
  }

  SECTION("Arithmetic chain") {
    IRBuilder builder;
    auto v1 = builder.addConst(ir::Type::i64, 5);
    auto v2 = builder.addConst(ir::Type::i64, 10);
    auto v3 = builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, v1, v2);
    auto v4 = builder.addConst(ir::Type::i64, 3);
    auto v5 = builder.addBinaryOp(ir::BinaryOpcode::Mul, ir::Type::i64, v3, v4);
    builder.setReturnTerminator(v5);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::ADD));
    REQUIRE(containsOpcode(result, arm64::Opcode::MUL));
  }

  SECTION("All arithmetic operations") {
    IRBuilder builder;
    auto v1 = builder.addConst(ir::Type::i64, 100);
    auto v2 = builder.addConst(ir::Type::i64, 20);

    auto addResult =
        builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, v1, v2);
    auto subResult =
        builder.addBinaryOp(ir::BinaryOpcode::Sub, ir::Type::i64, v1, v2);
    auto mulResult = builder.addBinaryOp(ir::BinaryOpcode::Mul, ir::Type::i64,
                                         addResult, subResult);

    builder.setReturnTerminator(mulResult);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::ADD));
    REQUIRE(containsOpcode(result, arm64::Opcode::SUB));
    REQUIRE(containsOpcode(result, arm64::Opcode::MUL));
  }
}

TEST_CASE("Lowering pipeline memory operations", "[lowering]") {
  SECTION("Load and store") {
    IRBuilder builder;
    auto address = builder.addConst(ir::Type::i64, 0x1000);
    auto loadedValue = builder.addLoad(ir::Type::i64, address);
    auto newValue = builder.addConst(ir::Type::i64, 42);
    builder.addStore(newValue, address);
    builder.setReturnTerminator(loadedValue);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::LDR));
    REQUIRE(containsOpcode(result, arm64::Opcode::STR));
  }

  SECTION("Address calculation with load") {
    IRBuilder builder;
    auto baseAddr = builder.addConst(ir::Type::i64, 0x1000);
    auto offset = builder.addConst(ir::Type::i64, 8);
    auto address = builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64,
                                       baseAddr, offset);
    auto value = builder.addLoad(ir::Type::i32, address);
    builder.setReturnTerminator(value);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::ADD));
    REQUIRE(containsOpcode(result, arm64::Opcode::LDR));
  }
}

TEST_CASE("Lowering pipeline type conversions", "[lowering]") {
  SECTION("Sign extension") {
    IRBuilder builder;
    auto value32 =
        builder.addConst(ir::Type::i32, 0xFFFFFF80); // Negative value
    auto extended = builder.addSext(ir::Type::i64, value32);
    builder.setReturnTerminator(extended);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::SXTW));
  }

  SECTION("Zero extension") {
    IRBuilder builder;
    auto value16 = builder.addConst(ir::Type::i16, 0x8000);
    auto extended = builder.addZext(ir::Type::i64, value16);
    builder.setReturnTerminator(extended);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::UXTH));
  }

  SECTION("Truncation") {
    IRBuilder builder;
    auto value64 = builder.addConst(ir::Type::i64, 0x123456789ABCDEF0);
    auto truncated = builder.addTrunc(ir::Type::i32, value64);
    builder.setReturnTerminator(truncated);

    lowerAndVerify(builder);
  }
}

TEST_CASE("Lowering pipeline control flow", "[lowering]") {
  SECTION("Conditional branch") {
    IRBuilder builder;
    auto v1 = builder.addConst(ir::Type::i64, 10);
    auto v2 = builder.addConst(ir::Type::i64, 20);
    auto cmp = builder.addBinaryOp(ir::BinaryOpcode::Lt, ir::Type::i1, v1, v2);
    builder.setCondBranchTerminator(cmp, 100, 200);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::CMP));
  }

  SECTION("Unconditional branch") {
    IRBuilder builder;
    builder.setBranchTerminator(100);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::MOV));
    REQUIRE(containsOpcode(result, arm64::Opcode::RET));
  }
}

TEST_CASE("Lowering pipeline register allocation scenarios", "[lowering]") {
  SECTION("Low register pressure") {
    IRBuilder builder;
    auto v1 = builder.addConst(ir::Type::i64, 1);
    auto v2 = builder.addConst(ir::Type::i64, 2);
    auto v3 = builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, v1, v2);
    builder.setReturnTerminator(v3);

    lowerAndVerify(builder);
  }

  SECTION("Medium register pressure") {
    IRBuilder builder;
    std::vector<ir::ValueId> values;

    for (int i = 0; i < 8; ++i) {
      values.push_back(builder.addConst(ir::Type::i64, i * 10));
    }

    ir::ValueId accumulator = values[0];
    for (size_t i = 1; i < values.size(); ++i) {
      accumulator = builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64,
                                        accumulator, values[i]);
    }

    builder.setReturnTerminator(accumulator);

    lowerAndVerify(builder);
  }

  SECTION("Parallel computations") {
    IRBuilder builder;

    auto a1 = builder.addConst(ir::Type::i64, 10);
    auto a2 = builder.addConst(ir::Type::i64, 20);
    auto a3 = builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, a1, a2);

    auto b1 = builder.addConst(ir::Type::i64, 30);
    auto b2 = builder.addConst(ir::Type::i64, 40);
    auto b3 = builder.addBinaryOp(ir::BinaryOpcode::Mul, ir::Type::i64, b1, b2);

    auto final =
        builder.addBinaryOp(ir::BinaryOpcode::Sub, ir::Type::i64, a3, b3);
    builder.setReturnTerminator(final);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::ADD));
    REQUIRE(containsOpcode(result, arm64::Opcode::MUL));
    REQUIRE(containsOpcode(result, arm64::Opcode::SUB));
  }
}

TEST_CASE("Lowering pipeline complex patterns", "[lowering]") {
  SECTION("Load-modify-store pattern") {
    IRBuilder builder;
    auto address = builder.addConst(ir::Type::i64, 0x2000);
    auto loadedValue = builder.addLoad(ir::Type::i64, address);
    auto increment = builder.addConst(ir::Type::i64, 1);
    auto newValue = builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64,
                                        loadedValue, increment);
    builder.addStore(newValue, address);
    builder.setReturnTerminator(newValue);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::LDR));
    REQUIRE(containsOpcode(result, arm64::Opcode::ADD));
    REQUIRE(containsOpcode(result, arm64::Opcode::STR));
  }

  SECTION("Mixed data types") {
    IRBuilder builder;
    auto val8 = builder.addConst(ir::Type::i8, 255);
    auto val16 = builder.addConst(ir::Type::i16, 1000);
    auto val32 = builder.addConst(ir::Type::i32, 100000);

    auto ext8to64 = builder.addZext(ir::Type::i64, val8);
    auto ext16to64 = builder.addSext(ir::Type::i64, val16);
    auto trunc32to16 = builder.addTrunc(ir::Type::i16, val32);

    auto sum = builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64,
                                   ext8to64, ext16to64);
    builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64, sum,
                        builder.addZext(ir::Type::i64, trunc32to16));
    builder.setVoidReturnTerminator();

    lowerAndVerify(builder);
  }

  SECTION("Comparison with conditional branch") {
    IRBuilder builder;
    auto x = builder.addConst(ir::Type::i64, 42);
    auto y = builder.addConst(ir::Type::i64, 100);
    auto cmp = builder.addBinaryOp(ir::BinaryOpcode::Lt, ir::Type::i1, x, y);
    builder.setCondBranchTerminator(cmp, 1000, 2000);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::CMP));
  }
}

TEST_CASE("Lowering pipeline edge cases", "[lowering]") {
  SECTION("Empty basic block with void return") {
    IRBuilder builder;
    builder.setVoidReturnTerminator();

    lowerAndVerify(builder);
  }

  SECTION("Single constant") {
    IRBuilder builder;
    auto value = builder.addConst(ir::Type::i64, 123);
    builder.setReturnTerminator(value);

    auto result = lowerAndVerify(builder);
    REQUIRE(containsOpcode(result, arm64::Opcode::MOV));
  }

  SECTION("Long dependency chain") {
    IRBuilder builder;
    ir::ValueId current = builder.addConst(ir::Type::i64, 1);

    for (int i = 0; i < 10; ++i) {
      auto next = builder.addConst(ir::Type::i64, i + 2);
      current = builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64,
                                    current, next);
    }

    builder.setReturnTerminator(current);

    lowerAndVerify(builder);
  }

  SECTION("Different types in same block") {
    IRBuilder builder;
    auto val8 = builder.addConst(ir::Type::i8, 10);
    auto val16 = builder.addConst(ir::Type::i16, 1000);
    auto val32 = builder.addConst(ir::Type::i32, 100000);
    auto val64 = builder.addConst(ir::Type::i64, 1000000000);

    builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i16,
                        builder.addZext(ir::Type::i16, val8), val16);
    builder.addBinaryOp(ir::BinaryOpcode::Add, ir::Type::i64,
                        builder.addZext(ir::Type::i64, val32), val64);

    builder.setVoidReturnTerminator();

    lowerAndVerify(builder);
  }
}
