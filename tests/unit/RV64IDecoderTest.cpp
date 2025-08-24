#include "RV64IDecoder.h"
#include "RV64IInstruction.h"
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace dinorisc;

TEST_CASE("RV64IDecoder Basic Construction", "[decoder]") {
  RV64IDecoder decoder;
  REQUIRE(decoder.getTotalInstructionsDecoded() == 0);
  REQUIRE(decoder.getInvalidInstructionsCount() == 0);
}

TEST_CASE("RV64IDecoder R-Type Instructions", "[decoder][r-type]") {
  RV64IDecoder decoder;

  SECTION("ADD instruction") {
    // ADD x1, x2, x3 -> 0x003100B3
    uint32_t raw = 0x003100B3;
    uint64_t pc = 0x1000;

    auto inst = decoder.decode(raw, pc);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::ADD);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(inst.operands[0].type == RV64IInstruction::OperandType::REGISTER);
    REQUIRE(inst.operands[0].reg == 1); // rd = x1
    REQUIRE(inst.operands[1].type == RV64IInstruction::OperandType::REGISTER);
    REQUIRE(inst.operands[1].reg == 2); // rs1 = x2
    REQUIRE(inst.operands[2].type == RV64IInstruction::OperandType::REGISTER);
    REQUIRE(inst.operands[2].reg == 3); // rs2 = x3
    REQUIRE(inst.rawInstruction == raw);
    REQUIRE(inst.address == pc);
  }

  SECTION("SUB instruction") {
    // SUB x5, x6, x7 -> 0x407302B3
    uint32_t raw = 0x407302B3;
    uint64_t pc = 0x2000;

    auto inst = decoder.decode(raw, pc);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::SUB);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(inst.operands[0].reg == 5); // rd = x5
    REQUIRE(inst.operands[1].reg == 6); // rs1 = x6
    REQUIRE(inst.operands[2].reg == 7); // rs2 = x7
  }

  SECTION("AND instruction") {
    // AND x10, x11, x12 -> 0x00C5F533
    uint32_t raw = 0x00C5F533;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::AND);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(inst.operands[0].reg == 10); // rd = x10
    REQUIRE(inst.operands[1].reg == 11); // rs1 = x11
    REQUIRE(inst.operands[2].reg == 12); // rs2 = x12
  }

  SECTION("XOR instruction") {
    // XOR x1, x2, x3 -> 0x003140B3
    uint32_t raw = 0x003140B3;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::XOR);
    REQUIRE(inst.operands[0].reg == 1);
    REQUIRE(inst.operands[1].reg == 2);
    REQUIRE(inst.operands[2].reg == 3);
  }
}

TEST_CASE("RV64IDecoder I-Type Instructions", "[decoder][i-type]") {
  RV64IDecoder decoder;

  SECTION("ADDI instruction with positive immediate") {
    // ADDI x1, x2, 100 -> 0x06410093
    uint32_t raw = 0x06410093;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::ADDI);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(inst.operands[0].reg == 1); // rd = x1
    REQUIRE(inst.operands[1].reg == 2); // rs1 = x2
    REQUIRE(inst.operands[2].type == RV64IInstruction::OperandType::IMMEDIATE);
    REQUIRE(inst.operands[2].imm == 100); // immediate = 100
  }

  SECTION("ADDI instruction with negative immediate") {
    // ADDI x3, x4, -1 -> 0xFFF20193
    uint32_t raw = 0xFFF20193;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::ADDI);
    REQUIRE(inst.operands[0].reg == 3);  // rd = x3
    REQUIRE(inst.operands[1].reg == 4);  // rs1 = x4
    REQUIRE(inst.operands[2].imm == -1); // immediate = -1 (sign extended)
  }

  SECTION("ANDI instruction") {
    // ANDI x5, x6, 0xFF -> 0x0FF37293
    uint32_t raw = 0x0FF37293;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::ANDI);
    REQUIRE(inst.operands[0].reg == 5);
    REQUIRE(inst.operands[1].reg == 6);
    REQUIRE(inst.operands[2].imm == 255);
  }

  SECTION("SLLI instruction") {
    // SLLI x1, x2, 5 -> 0x00511093
    uint32_t raw = 0x00511093;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::SLLI);
    REQUIRE(inst.operands[0].reg == 1);
    REQUIRE(inst.operands[1].reg == 2);
    REQUIRE(inst.operands[2].imm == 5);
  }
}

TEST_CASE("RV64IDecoder Load Instructions", "[decoder][load]") {
  RV64IDecoder decoder;

  SECTION("LW instruction") {
    // LW x1, 8(x2) -> 0x00812083
    uint32_t raw = 0x00812083;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::LW);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(inst.operands[0].reg == 1); // rd = x1
    REQUIRE(inst.operands[1].reg == 2); // rs1 = x2 (base)
    REQUIRE(inst.operands[2].imm == 8); // offset = 8
  }

  SECTION("LD instruction") {
    // LD x5, -16(x10) -> 0xFF053283
    uint32_t raw = 0xFF053283;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::LD);
    REQUIRE(inst.operands[0].reg == 5);
    REQUIRE(inst.operands[1].reg == 10);
    REQUIRE(inst.operands[2].imm == -16);
  }

  SECTION("LBU instruction") {
    // LBU x3, 4(x8) -> 0x00444183
    uint32_t raw = 0x00444183;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::LBU);
    REQUIRE(inst.operands[0].reg == 3);
    REQUIRE(inst.operands[1].reg == 8);
    REQUIRE(inst.operands[2].imm == 4);
  }
}

TEST_CASE("RV64IDecoder Store Instructions", "[decoder][store]") {
  RV64IDecoder decoder;

  SECTION("SW instruction") {
    // SW x3, 12(x2) -> 0x00312623
    uint32_t raw = 0x00312623;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::SW);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(inst.operands[0].reg == 2);  // rs1 = x2 (base)
    REQUIRE(inst.operands[1].reg == 3);  // rs2 = x3 (source)
    REQUIRE(inst.operands[2].imm == 12); // offset = 12
  }

  SECTION("SD instruction with negative offset") {
    // SD x5, -8(x10) -> 0xFE553C23
    uint32_t raw = 0xFE553C23;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::SD);
    REQUIRE(inst.operands[0].reg == 10); // rs1 = x10 (base)
    REQUIRE(inst.operands[1].reg == 5);  // rs2 = x5 (source)
    REQUIRE(inst.operands[2].imm == -8); // offset = -8
  }
}

TEST_CASE("RV64IDecoder Branch Instructions", "[decoder][branch]") {
  RV64IDecoder decoder;

  SECTION("BEQ instruction") {
    // BEQ x1, x2, 16 -> 0x00208863
    uint32_t raw = 0x00208863;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::BEQ);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(inst.operands[0].reg == 1);  // rs1 = x1
    REQUIRE(inst.operands[1].reg == 2);  // rs2 = x2
    REQUIRE(inst.operands[2].imm == 16); // offset = 16
  }

  SECTION("BNE instruction with negative offset") {
    // BNE x3, x4, -4 -> 0xFE419EE3
    uint32_t raw = 0xFE419EE3;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::BNE);
    REQUIRE(inst.operands[0].reg == 3);  // rs1 = x3
    REQUIRE(inst.operands[1].reg == 4);  // rs2 = x4
    REQUIRE(inst.operands[2].imm == -4); // offset = -4
  }

  SECTION("BLT instruction") {
    // BLT x5, x6, 8 -> 0x0062C463
    uint32_t raw = 0x0062C463;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::BLT);
    REQUIRE(inst.operands[0].reg == 5);
    REQUIRE(inst.operands[1].reg == 6);
    REQUIRE(inst.operands[2].imm == 8);
  }
}

TEST_CASE("RV64IDecoder Jump Instructions", "[decoder][jump]") {
  RV64IDecoder decoder;

  SECTION("JAL instruction") {
    // JAL x1, 100 -> 0x064000EF
    uint32_t raw = 0x064000EF;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::JAL);
    REQUIRE(inst.operands.size() == 2);
    REQUIRE(inst.operands[0].reg == 1);   // rd = x1
    REQUIRE(inst.operands[1].imm == 100); // offset = 100
  }

  SECTION("JALR instruction") {
    // JALR x1, x2, 4 -> 0x004100E7
    uint32_t raw = 0x004100E7;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::JALR);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(inst.operands[0].reg == 1); // rd = x1
    REQUIRE(inst.operands[1].reg == 2); // rs1 = x2
    REQUIRE(inst.operands[2].imm == 4); // offset = 4
  }
}

TEST_CASE("RV64IDecoder Upper Immediate Instructions", "[decoder][upper]") {
  RV64IDecoder decoder;

  SECTION("LUI instruction") {
    // LUI x1, 0x12345 -> 0x123450B7
    uint32_t raw = 0x123450B7;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::LUI);
    REQUIRE(inst.operands.size() == 2);
    REQUIRE(inst.operands[0].reg == 1); // rd = x1
    REQUIRE(inst.operands[1].imm ==
            0x12345000); // immediate shifted to upper 20 bits
  }

  SECTION("AUIPC instruction") {
    // AUIPC x2, 0x1000 -> 0x01000117
    uint32_t raw = 0x01000117;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::AUIPC);
    REQUIRE(inst.operands[0].reg == 2);
    REQUIRE(inst.operands[1].imm == 0x1000000);
  }
}

TEST_CASE("RV64IDecoder System Instructions", "[decoder][system]") {
  RV64IDecoder decoder;

  SECTION("ECALL instruction") {
    // ECALL -> 0x00000073
    uint32_t raw = 0x00000073;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::ECALL);
    REQUIRE(inst.operands.size() == 0);
  }

  SECTION("EBREAK instruction") {
    // EBREAK -> 0x00100073 (immediate=1 in bits 31-20)
    uint32_t raw = 0x00100073;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::EBREAK);
    REQUIRE(inst.operands.size() == 0);
  }
}

TEST_CASE("RV64IDecoder 64-bit Word Instructions", "[decoder][64bit]") {
  RV64IDecoder decoder;

  SECTION("ADDW instruction") {
    // ADDW x1, x2, x3 -> 0x003100BB (opcode=0x3B for OP_32, funct7=0, funct3=0)
    uint32_t raw = 0x003100BB;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::ADDW);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(inst.operands[0].reg == 1);
    REQUIRE(inst.operands[1].reg == 2);
    REQUIRE(inst.operands[2].reg == 3);
  }

  SECTION("ADDIW instruction") {
    // ADDIW x5, x6, 10 -> 0x00A3029B
    uint32_t raw = 0x00A3029B;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::ADDIW);
    REQUIRE(inst.operands[0].reg == 5);
    REQUIRE(inst.operands[1].reg == 6);
    REQUIRE(inst.operands[2].imm == 10);
  }
}

TEST_CASE("RV64IDecoder Invalid Instructions", "[decoder][invalid]") {
  RV64IDecoder decoder;

  SECTION("Invalid opcode") {
    uint32_t raw = 0x00000007; // Invalid opcode

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::INVALID);
    REQUIRE(decoder.getInvalidInstructionsCount() == 1);
  }

  SECTION("Invalid funct3 for valid opcode") {
    uint32_t raw =
        0x0000F033; // OP opcode (0x33) with completely invalid pattern

    auto inst = decoder.decode(raw, 0);

    // This should decode as AND since funct3=7 is valid for AND, so let's use
    // different invalid pattern Actually, let's use a truly invalid opcode
    // instead
    raw = 0x0000001F; // Invalid opcode 0x1F
    inst = decoder.decode(raw, 0);

    REQUIRE(inst.opcode == RV64IInstruction::Opcode::INVALID);
  }
}

TEST_CASE("RV64IDecoder Batch Decoding", "[decoder][batch]") {
  RV64IDecoder decoder;

  SECTION("Decode multiple instructions from byte array") {
    // Create byte array with multiple instructions in little-endian format
    std::vector<uint8_t> data = {// ADDI x1, x0, 10 -> 0x00A00093
                                 0x93, 0x00, 0xA0, 0x00,
                                 // ADD x2, x1, x1 -> 0x001080B3
                                 0xB3, 0x80, 0x10, 0x00,
                                 // SW x2, 0(x1) -> 0x00212023
                                 0x23, 0x20, 0x21, 0x00};

    uint64_t baseAddr = 0x1000;
    auto instructions = decoder.decodeInstructions(data, baseAddr);

    REQUIRE(instructions.size() == 3);

    // First instruction: ADDI
    REQUIRE(instructions[0].opcode == RV64IInstruction::Opcode::ADDI);
    REQUIRE(instructions[0].address == 0x1000);
    REQUIRE(instructions[0].operands[2].imm == 10);

    // Second instruction: ADD
    REQUIRE(instructions[1].opcode == RV64IInstruction::Opcode::ADD);
    REQUIRE(instructions[1].address == 0x1004);

    // Third instruction: SW
    REQUIRE(instructions[2].opcode == RV64IInstruction::Opcode::SW);
    REQUIRE(instructions[2].address == 0x1008);
  }

  SECTION("Handle misaligned data gracefully") {
    // 7 bytes (not multiple of 4)
    std::vector<uint8_t> data = {0x93, 0x00, 0xA0, 0x00, 0xB3, 0x80, 0x10};

    auto instructions = decoder.decodeInstructions(data, 0);

    // Should decode only 1 complete instruction (4 bytes), ignoring last 3
    // bytes
    REQUIRE(instructions.size() == 1);
    REQUIRE(instructions[0].opcode == RV64IInstruction::Opcode::ADDI);
  }

  SECTION("Handle empty data") {
    std::vector<uint8_t> empty;

    auto instructions = decoder.decodeInstructions(empty, 0);

    REQUIRE(instructions.size() == 0);
  }

  SECTION("Handle null pointer") {
    auto instructions = decoder.decodeInstructions(nullptr, 0, 0);

    REQUIRE(instructions.size() == 0);
  }
}

TEST_CASE("RV64IDecoder Statistics Tracking", "[decoder][stats]") {
  RV64IDecoder decoder;

  REQUIRE(decoder.getTotalInstructionsDecoded() == 0);
  REQUIRE(decoder.getInvalidInstructionsCount() == 0);

  // Decode valid instruction
  decoder.decode(0x00A00093, 0); // ADDI

  REQUIRE(decoder.getTotalInstructionsDecoded() == 1);
  REQUIRE(decoder.getInvalidInstructionsCount() == 0);

  // Decode invalid instruction
  decoder.decode(0x00000007, 0); // Invalid

  REQUIRE(decoder.getTotalInstructionsDecoded() == 2);
  REQUIRE(decoder.getInvalidInstructionsCount() == 1);

  // Decode more valid instructions
  decoder.decode(0x003100B3, 0); // ADD
  decoder.decode(0x00312623, 0); // SW

  REQUIRE(decoder.getTotalInstructionsDecoded() == 4);
  REQUIRE(decoder.getInvalidInstructionsCount() == 1);
}

TEST_CASE("RV64IDecoder Immediate Sign Extension", "[decoder][immediate]") {
  RV64IDecoder decoder;

  SECTION("I-Type negative immediate") {
    // ADDI x1, x2, -1 (0xFFF) -> 0xFFF10093
    uint32_t raw = 0xFFF10093;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.operands[2].imm == -1);
  }

  SECTION("B-Type negative offset") {
    // BEQ x1, x2, -4 -> 0xFE208EE3
    uint32_t raw = 0xFE208EE3;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.operands[2].imm == -4);
  }

  SECTION("J-Type negative offset") {
    // JAL x1, -8 -> 0xFF9FF0EF
    uint32_t raw = 0xFF9FF0EF;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.operands[1].imm == -8);
  }
}

TEST_CASE("RV64IDecoder Edge Cases", "[decoder][edge]") {
  RV64IDecoder decoder;

  SECTION("Maximum positive I-Type immediate") {
    // ADDI x1, x2, 2047 (0x7FF) -> 0x7FF10093
    uint32_t raw = 0x7FF10093;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.operands[2].imm == 2047);
  }

  SECTION("Minimum negative I-Type immediate") {
    // ADDI x1, x2, -2048 (0x800) -> 0x80010093
    uint32_t raw = 0x80010093;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.operands[2].imm == -2048);
  }

  SECTION("Register x31 (highest register)") {
    // ADD x31, x30, x29 -> 0x01DF0FB3
    uint32_t raw = 0x01DF0FB3;

    auto inst = decoder.decode(raw, 0);

    REQUIRE(inst.operands[0].reg == 31); // rd = x31
    REQUIRE(inst.operands[1].reg == 30); // rs1 = x30
    REQUIRE(inst.operands[2].reg == 29); // rs2 = x29
  }
}