#include "../../lib/ARM64/Encoder.h"
#include <catch2/catch_test_macros.hpp>

using namespace dinorisc::arm64;

TEST_CASE("Encoder - Three operand instructions", "[encoder]") {
  Encoder encoder;

  SECTION("ADD with registers") {
    ThreeOperandInst inst{Opcode::ADD, DataSize::X, Register::X0, Register::X1,
                          Register::X2};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0x8B020020);
  }

  SECTION("ADD with immediate") {
    ThreeOperandInst inst{Opcode::ADD, DataSize::X, Register::X0, Register::X1,
                          Immediate{42}};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0x9100A820);
  }

  SECTION("SUB with registers") {
    ThreeOperandInst inst{Opcode::SUB, DataSize::W, Register::X3, Register::X4,
                          Register::X5};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0x4B050083);
  }

  SECTION("AND with registers") {
    ThreeOperandInst inst{Opcode::AND, DataSize::X, Register::X0, Register::X1,
                          Register::X2};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0x8A020020);
  }

  SECTION("MUL with registers") {
    ThreeOperandInst inst{Opcode::MUL, DataSize::X, Register::X0, Register::X1,
                          Register::X2};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0x9B027C20);
  }
}

TEST_CASE("Encoder - Two operand instructions", "[encoder]") {
  Encoder encoder;

  SECTION("MOV with immediate") {
    TwoOperandInst inst{Opcode::MOV, DataSize::X, Register::X0,
                        Immediate{0x1234}};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0xD2A24680);
  }

  SECTION("MOV with register") {
    TwoOperandInst inst{Opcode::MOV, DataSize::X, Register::X0, Register::X1};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0xAA0103E0);
  }

  SECTION("SXTB") {
    TwoOperandInst inst{Opcode::SXTB, DataSize::X, Register::X0, Register::X1};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0x93401C20);
  }

  SECTION("RET") {
    TwoOperandInst inst{Opcode::RET, DataSize::X, Register::X0, Immediate{0}};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0xD65F03C0);
  }
}

TEST_CASE("Encoder - Memory instructions", "[encoder]") {
  Encoder encoder;

  SECTION("LDR with positive offset") {
    MemoryInst inst{Opcode::LDR, DataSize::X, Register::X0, Register::X1, 8};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0xF9400420);
  }

  SECTION("STR with zero offset") {
    MemoryInst inst{Opcode::STR, DataSize::W, Register::X2, Register::X3, 0};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0xB9000062);
  }

  SECTION("LDR byte") {
    MemoryInst inst{Opcode::LDR, DataSize::B, Register::X0, Register::X1, 4};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0x39401020);
  }
}

TEST_CASE("Encoder - Branch instructions", "[encoder]") {
  Encoder encoder;

  SECTION("Unconditional branch") {
    BranchInst inst{Opcode::B, 0x1000};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0x14000400);
  }

  SECTION("Conditional branch equal") {
    BranchInst inst{Opcode::B_EQ, 0x100};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0x54000800);
  }

  SECTION("Conditional branch not equal") {
    BranchInst inst{Opcode::B_NE, 0x80};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.size() == 4);

    uint32_t value = (encoded[3] << 24) | (encoded[2] << 16) |
                     (encoded[1] << 8) | encoded[0];
    REQUIRE(value == 0x54000401);
  }
}

TEST_CASE("Encoder - Error cases", "[encoder]") {
  Encoder encoder;

  SECTION("Invalid immediate too large") {
    ThreeOperandInst inst{Opcode::ADD, DataSize::X, Register::X0, Register::X1,
                          Immediate{0x1000}};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.empty());
  }

  SECTION("Virtual register should fail") {
    ThreeOperandInst inst{Opcode::ADD, DataSize::X, VirtualRegister{42},
                          Register::X1, Register::X2};
    Instruction instruction{inst};

    auto encoded = encoder.encodeInstruction(instruction);
    REQUIRE(encoded.empty());
  }
}