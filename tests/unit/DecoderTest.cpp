#include "RISCV/Decoder.h"
#include "RISCV/Instruction.h"
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

using namespace dinorisc;
using namespace dinorisc::riscv;

// Helper function to convert uint32_t to little-endian byte array
std::vector<uint8_t> toBytes(uint32_t value) {
  return {
    static_cast<uint8_t>(value & 0xFF),
    static_cast<uint8_t>((value >> 8) & 0xFF),
    static_cast<uint8_t>((value >> 16) & 0xFF),
    static_cast<uint8_t>((value >> 24) & 0xFF)
  };
}

TEST_CASE("RV64IDecoder R-Type Instructions", "[decoder][r-type]") {
  Decoder decoder;

  SECTION("ADD instruction") {
    // ADD x1, x2, x3 -> 0x003100B3
    uint32_t raw = 0x003100B3;
    auto data = toBytes(raw);
    uint64_t pc = 0x1000;

    auto inst = decoder.decode(data.data(), 0, pc);

    REQUIRE(inst.opcode == Instruction::Opcode::ADD);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(std::holds_alternative<Instruction::Register>(inst.operands[0]));
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            1); // rd = x1
    REQUIRE(std::holds_alternative<Instruction::Register>(inst.operands[1]));
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            2); // rs1 = x2
    REQUIRE(std::holds_alternative<Instruction::Register>(inst.operands[2]));
    REQUIRE(std::get<Instruction::Register>(inst.operands[2]).value ==
            3); // rs2 = x3
    REQUIRE(inst.rawInstruction == raw);
    REQUIRE(inst.address == pc);
  }

  SECTION("SUB instruction") {
    // SUB x5, x6, x7 -> 0x407302B3
    uint32_t raw = 0x407302B3;
    auto data = toBytes(raw);
    uint64_t pc = 0x2000;

    auto inst = decoder.decode(data.data(), 0, pc);

    REQUIRE(inst.opcode == Instruction::Opcode::SUB);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            5); // rd = x5
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            6); // rs1 = x6
    REQUIRE(std::get<Instruction::Register>(inst.operands[2]).value ==
            7); // rs2 = x7
  }

  SECTION("AND instruction") {
    // AND x10, x11, x12 -> 0x00C5F533
    uint32_t raw = 0x00C5F533;
    auto data = toBytes(raw);

    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::AND);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            10); // rd = x10
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            11); // rs1 = x11
    REQUIRE(std::get<Instruction::Register>(inst.operands[2]).value ==
            12); // rs2 = x12
  }

  SECTION("XOR instruction") {
    // XOR x1, x2, x3 -> 0x003140B3
    uint32_t raw = 0x003140B3;
    auto data = toBytes(raw);

    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::XOR);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value == 1);
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value == 2);
    REQUIRE(std::get<Instruction::Register>(inst.operands[2]).value == 3);
  }
}

TEST_CASE("RV64IDecoder I-Type Instructions", "[decoder][i-type]") {
  Decoder decoder;

  SECTION("ADDI instruction with positive immediate") {
    // ADDI x1, x2, 100 -> 0x06410093
    uint32_t raw = 0x06410093;
    auto data = toBytes(raw);

    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::ADDI);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            1); // rd = x1
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            2); // rs1 = x2
    REQUIRE(std::holds_alternative<Instruction::Immediate>(inst.operands[2]));
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value ==
            100); // immediate = 100
  }

  SECTION("ADDI instruction with negative immediate") {
    // ADDI x3, x4, -1 -> 0xFFF20193
    uint32_t raw = 0xFFF20193;
    auto data = toBytes(raw);

    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::ADDI);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            3); // rd = x3
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            4); // rs1 = x4
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value ==
            -1); // immediate = -1 (sign extended)
  }

  SECTION("ANDI instruction") {
    // ANDI x5, x6, 0xFF -> 0x0FF37293
    uint32_t raw = 0x0FF37293;
    auto data = toBytes(raw);

    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::ANDI);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value == 5);
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value == 6);
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == 255);
  }

  SECTION("SLLI instruction") {
    // SLLI x1, x2, 5 -> 0x00511093
    uint32_t raw = 0x00511093;
    auto data = toBytes(raw);

    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::SLLI);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value == 1);
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value == 2);
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == 5);
  }
}

TEST_CASE("RV64IDecoder Load Instructions", "[decoder][load]") {
  Decoder decoder;

  SECTION("LW instruction") {
    // LW x1, 8(x2) -> 0x00812083
    uint32_t raw = 0x00812083;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::LW);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            1); // rd = x1
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            2); // rs1 = x2 (base)
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value ==
            8); // offset = 8
  }

  SECTION("LD instruction") {
    // LD x5, -16(x10) -> 0xFF053283
    uint32_t raw = 0xFF053283;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::LD);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value == 5);
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value == 10);
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == -16);
  }

  SECTION("LBU instruction") {
    // LBU x3, 4(x8) -> 0x00444183
    uint32_t raw = 0x00444183;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::LBU);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value == 3);
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value == 8);
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == 4);
  }
}

TEST_CASE("RV64IDecoder Store Instructions", "[decoder][store]") {
  Decoder decoder;

  SECTION("SW instruction") {
    // SW x3, 12(x2) -> 0x00312623
    uint32_t raw = 0x00312623;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::SW);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            2); // rs1 = x2 (base)
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            3); // rs2 = x3 (source)
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value ==
            12); // offset = 12
  }

  SECTION("SD instruction with negative offset") {
    // SD x5, -8(x10) -> 0xFE553C23
    uint32_t raw = 0xFE553C23;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::SD);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            10); // rs1 = x10 (base)
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            5); // rs2 = x5 (source)
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value ==
            -8); // offset = -8
  }
}

TEST_CASE("RV64IDecoder Branch Instructions", "[decoder][branch]") {
  Decoder decoder;

  SECTION("BEQ instruction") {
    // BEQ x1, x2, 16 -> 0x00208863
    uint32_t raw = 0x00208863;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::BEQ);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            1); // rs1 = x1
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            2); // rs2 = x2
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value ==
            16); // offset = 16
  }

  SECTION("BNE instruction with negative offset") {
    // BNE x3, x4, -4 -> 0xFE419EE3
    uint32_t raw = 0xFE419EE3;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::BNE);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            3); // rs1 = x3
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            4); // rs2 = x4
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value ==
            -4); // offset = -4
  }

  SECTION("BLT instruction") {
    // BLT x5, x6, 8 -> 0x0062C463
    uint32_t raw = 0x0062C463;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::BLT);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value == 5);
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value == 6);
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == 8);
  }
}

TEST_CASE("RV64IDecoder Jump Instructions", "[decoder][jump]") {
  Decoder decoder;

  SECTION("JAL instruction") {
    // JAL x1, 100 -> 0x064000EF
    uint32_t raw = 0x064000EF;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::JAL);
    REQUIRE(inst.operands.size() == 2);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            1); // rd = x1
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[1]).value ==
            100); // offset = 100
  }

  SECTION("JALR instruction") {
    // JALR x1, x2, 4 -> 0x004100E7
    uint32_t raw = 0x004100E7;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::JALR);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            1); // rd = x1
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            2); // rs1 = x2
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value ==
            4); // offset = 4
  }
}

TEST_CASE("RV64IDecoder Upper Immediate Instructions", "[decoder][upper]") {
  Decoder decoder;

  SECTION("LUI instruction") {
    // LUI x1, 0x12345 -> 0x123450B7
    uint32_t raw = 0x123450B7;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::LUI);
    REQUIRE(inst.operands.size() == 2);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            1); // rd = x1
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[1]).value ==
            0x12345000); // immediate shifted to upper 20 bits
  }

  SECTION("AUIPC instruction") {
    // AUIPC x2, 0x1000 -> 0x01000117
    uint32_t raw = 0x01000117;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::AUIPC);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value == 2);
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[1]).value ==
            0x1000000);
  }
}

TEST_CASE("RV64IDecoder System Instructions", "[decoder][system]") {
  Decoder decoder;

  SECTION("ECALL instruction") {
    // ECALL -> 0x00000073
    uint32_t raw = 0x00000073;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::ECALL);
    REQUIRE(inst.operands.size() == 0);
  }

  SECTION("EBREAK instruction") {
    // EBREAK -> 0x00100073 (immediate=1 in bits 31-20)
    uint32_t raw = 0x00100073;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::EBREAK);
    REQUIRE(inst.operands.size() == 0);
  }
}

TEST_CASE("RV64IDecoder 64-bit Word Instructions", "[decoder][64bit]") {
  Decoder decoder;

  SECTION("ADDW instruction") {
    // ADDW x1, x2, x3 -> 0x003100BB (opcode=0x3B for OP_32, funct7=0, funct3=0)
    uint32_t raw = 0x003100BB;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::ADDW);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value == 1);
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value == 2);
    REQUIRE(std::get<Instruction::Register>(inst.operands[2]).value == 3);
  }

  SECTION("ADDIW instruction") {
    // ADDIW x5, x6, 10 -> 0x00A3029B
    uint32_t raw = 0x00A3029B;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(inst.opcode == Instruction::Opcode::ADDIW);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value == 5);
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value == 6);
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == 10);
  }
}

TEST_CASE("RV64IDecoder Invalid Instructions", "[decoder][invalid]") {
  Decoder decoder;

  SECTION("Invalid opcode") {
    uint32_t raw = 0x00000007; // Invalid opcode
    auto data = toBytes(raw);

    REQUIRE_THROWS_AS(decoder.decode(data.data(), 0, 0), std::runtime_error);
  }

  SECTION("Invalid funct3 for valid opcode") {
    uint32_t raw = 0x0000001F; // Invalid opcode 0x1F
    auto data = toBytes(raw);

    REQUIRE_THROWS_AS(decoder.decode(data.data(), 0, 0), std::runtime_error);
  }
}

TEST_CASE("RV64IDecoder Memory Decode", "[decoder][memory]") {
  Decoder decoder;

  SECTION("Decode instruction from byte array") {
    // ADDI x1, x0, 10 -> 0x00A00093 in little-endian bytes
    std::vector<uint8_t> data = {0x93, 0x00, 0xA0, 0x00};
    uint64_t pc = 0x1000;

    auto inst = decoder.decode(data.data(), 0, pc);

    REQUIRE(inst.opcode == Instruction::Opcode::ADDI);
    REQUIRE(inst.operands.size() == 3);
    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value == 1);
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value == 0);
    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == 10);
    REQUIRE(inst.rawInstruction == 0x00A00093);
    REQUIRE(inst.address == pc);
  }

  SECTION("Decode from different offset") {
    // Two instructions back-to-back
    std::vector<uint8_t> data = {// First: ADDI x1, x0, 10 -> 0x00A00093
                                 0x93, 0x00, 0xA0, 0x00,
                                 // Second: ADD x2, x1, x1 -> 0x001080B3
                                 0xB3, 0x80, 0x10, 0x00};
    uint64_t pc1 = 0x1000;
    uint64_t pc2 = 0x1004;

    auto first = decoder.decode(data.data(), 0, pc1);
    auto second = decoder.decode(data.data(), 4, pc2);

    REQUIRE(first.opcode == Instruction::Opcode::ADDI);
    REQUIRE(first.rawInstruction == 0x00A00093);
    REQUIRE(first.address == pc1);

    REQUIRE(second.opcode == Instruction::Opcode::ADD);
    REQUIRE(second.rawInstruction == 0x001080B3);
    REQUIRE(second.address == pc2);
  }
}

TEST_CASE("RV64IDecoder Immediate Sign Extension", "[decoder][immediate]") {
  Decoder decoder;

  SECTION("I-Type negative immediate") {
    // ADDI x1, x2, -1 (0xFFF) -> 0xFFF10093
    uint32_t raw = 0xFFF10093;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == -1);
  }

  SECTION("B-Type negative offset") {
    // BEQ x1, x2, -4 -> 0xFE208EE3
    uint32_t raw = 0xFE208EE3;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == -4);
  }

  SECTION("J-Type negative offset") {
    // JAL x1, -8 -> 0xFF9FF0EF
    uint32_t raw = 0xFF9FF0EF;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(std::get<Instruction::Immediate>(inst.operands[1]).value == -8);
  }
}

TEST_CASE("RV64IDecoder Edge Cases", "[decoder][edge]") {
  Decoder decoder;

  SECTION("Maximum positive I-Type immediate") {
    // ADDI x1, x2, 2047 (0x7FF) -> 0x7FF10093
    uint32_t raw = 0x7FF10093;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == 2047);
  }

  SECTION("Minimum negative I-Type immediate") {
    // ADDI x1, x2, -2048 (0x800) -> 0x80010093
    uint32_t raw = 0x80010093;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(std::get<Instruction::Immediate>(inst.operands[2]).value == -2048);
  }

  SECTION("Register x31 (highest register)") {
    // ADD x31, x30, x29 -> 0x01DF0FB3
    uint32_t raw = 0x01DF0FB3;

    auto data = toBytes(raw);
    auto inst = decoder.decode(data.data(), 0, 0);

    REQUIRE(std::get<Instruction::Register>(inst.operands[0]).value ==
            31); // rd = x31
    REQUIRE(std::get<Instruction::Register>(inst.operands[1]).value ==
            30); // rs1 = x30
    REQUIRE(std::get<Instruction::Register>(inst.operands[2]).value ==
            29); // rs2 = x29
  }
}
