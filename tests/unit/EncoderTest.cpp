#include "../../lib/ARM64/Encoder.h"
#include "../../lib/Error.h"
#include <catch2/catch_test_macros.hpp>

using namespace dinorisc::arm64;

static uint32_t encode(const Instruction &inst) {
  Encoder encoder;
  auto bytes = encoder.encodeInstruction(inst);
  REQUIRE(bytes.size() == 4);
  return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}

TEST_CASE("Encoder - Three operand instructions", "[encoder]") {
  SECTION("ADD with registers") {
    REQUIRE(encode({ThreeOperandInst{Opcode::ADD, DataSize::X, Register::X0,
                                     Register::X1, Register::X2}}) ==
            0x8B020020);
  }

  SECTION("ADD with immediate") {
    REQUIRE(encode({ThreeOperandInst{Opcode::ADD, DataSize::X, Register::X0,
                                     Register::X1, Immediate{42}}}) ==
            0x9100A820);
  }

  SECTION("SUB with registers") {
    REQUIRE(encode({ThreeOperandInst{Opcode::SUB, DataSize::W, Register::X3,
                                     Register::X4, Register::X5}}) ==
            0x4B050083);
  }

  SECTION("AND with registers") {
    REQUIRE(encode({ThreeOperandInst{Opcode::AND, DataSize::X, Register::X0,
                                     Register::X1, Register::X2}}) ==
            0x8A020020);
  }

  SECTION("MUL with registers") {
    REQUIRE(encode({ThreeOperandInst{Opcode::MUL, DataSize::X, Register::X0,
                                     Register::X1, Register::X2}}) ==
            0x9B027C20);
  }
}

TEST_CASE("Encoder - Two operand instructions", "[encoder]") {
  SECTION("MOV with immediate") {
    REQUIRE(encode({TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x1234}}}) == 0xD2824680);
  }

  SECTION("MOV with register") {
    REQUIRE(encode({TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Register::X1}}) == 0xAA0103E0);
  }

  SECTION("SXTB") {
    REQUIRE(encode({TwoOperandInst{Opcode::SXTB, DataSize::X, Register::X0,
                                   Register::X1}}) == 0x93401C20);
  }

  SECTION("RET") {
    REQUIRE(encode({TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}}) == 0xD65F03C0);
  }
}

TEST_CASE("Encoder - Memory instructions", "[encoder]") {
  SECTION("LDR with positive offset") {
    REQUIRE(encode({MemoryInst{Opcode::LDR, DataSize::X, Register::X0,
                               Register::X1, 8}}) == 0xF9400420);
  }

  SECTION("STR with zero offset") {
    REQUIRE(encode({MemoryInst{Opcode::STR, DataSize::W, Register::X2,
                               Register::X3, 0}}) == 0xB9000062);
  }

  SECTION("LDR byte") {
    REQUIRE(encode({MemoryInst{Opcode::LDR, DataSize::B, Register::X0,
                               Register::X1, 4}}) == 0x39401020);
  }
}

TEST_CASE("Encoder - Branch instructions", "[encoder]") {
  SECTION("Unconditional branch") {
    REQUIRE(encode({BranchInst{Opcode::B, 0x1000}}) == 0x14000400);
  }

  SECTION("Conditional branch equal") {
    REQUIRE(encode({BranchInst{Opcode::B_EQ, 0x100}}) == 0x54000800);
  }

  SECTION("Conditional branch not equal") {
    REQUIRE(encode({BranchInst{Opcode::B_NE, 0x80}}) == 0x54000401);
  }
}

TEST_CASE("Encoder - Error cases", "[encoder]") {
  SECTION("Invalid immediate too large") {
    REQUIRE_THROWS_AS(
        encode({ThreeOperandInst{Opcode::ADD, DataSize::X, Register::X0,
                                 Register::X1, Immediate{0x1000}}}),
        dinorisc::EncodingError);
  }

  SECTION("Virtual register should fail") {
    REQUIRE_THROWS_AS(
        encode({ThreeOperandInst{Opcode::ADD, DataSize::X, VirtualRegister{42},
                                 Register::X1, Register::X2}}),
        dinorisc::EncodingError);
  }
}
