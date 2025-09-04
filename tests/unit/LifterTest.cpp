#include "Lifter.h"
#include "IR/IR.h"
#include "RISCV/Instruction.h"
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>
#include <set>

using namespace dinorisc;
using namespace dinorisc::riscv;
using namespace dinorisc::ir;

namespace {

riscv::Instruction
createInstruction(riscv::Instruction::Opcode opcode,
                  std::vector<riscv::Instruction::Operand> operands,
                  uint64_t address = 0x1000) {
  return riscv::Instruction(opcode, std::move(operands), 0, address);
}

riscv::Instruction createRType(riscv::Instruction::Opcode opcode, uint32_t rd,
                               uint32_t rs1, uint32_t rs2,
                               uint64_t address = 0x1000) {
  return createInstruction(opcode,
                           {riscv::Instruction::Register(rd),
                            riscv::Instruction::Register(rs1),
                            riscv::Instruction::Register(rs2)},
                           address);
}

riscv::Instruction createIType(riscv::Instruction::Opcode opcode, uint32_t rd,
                               uint32_t rs1, int64_t imm,
                               uint64_t address = 0x1000) {
  return createInstruction(opcode,
                           {riscv::Instruction::Register(rd),
                            riscv::Instruction::Register(rs1),
                            riscv::Instruction::Immediate(imm)},
                           address);
}

riscv::Instruction createUType(riscv::Instruction::Opcode opcode, uint32_t rd,
                               int64_t imm, uint64_t address = 0x1000) {
  return createInstruction(
      opcode,
      {riscv::Instruction::Register(rd), riscv::Instruction::Immediate(imm)},
      address);
}

riscv::Instruction createBType(riscv::Instruction::Opcode opcode, uint32_t rs1,
                               uint32_t rs2, int64_t imm,
                               uint64_t address = 0x1000) {
  return createInstruction(opcode,
                           {riscv::Instruction::Register(rs1),
                            riscv::Instruction::Register(rs2),
                            riscv::Instruction::Immediate(imm)},
                           address);
}

riscv::Instruction createJType(riscv::Instruction::Opcode opcode, uint32_t rd,
                               int64_t imm, uint64_t address = 0x1000) {
  return createInstruction(
      opcode,
      {riscv::Instruction::Register(rd), riscv::Instruction::Immediate(imm)},
      address);
}

} // namespace

TEST_CASE("Lifter Basic Block Construction", "[lifter][basic-block]") {
  Lifter lifter;

  SECTION("Empty basic block") {
    std::vector<riscv::Instruction> instructions;
    auto block = lifter.liftBasicBlock(instructions);

    REQUIRE(block.instructions.empty());
    REQUIRE(std::holds_alternative<Branch>(block.terminator.kind));
    auto &branch = std::get<Branch>(block.terminator.kind);
    REQUIRE(branch.targetBlock == 0);
  }

  SECTION("Single instruction block without terminator") {
    auto addInst =
        createRType(riscv::Instruction::Opcode::ADD, 1, 2, 3, 0x1000);
    std::vector<riscv::Instruction> instructions = {addInst};

    auto block = lifter.liftBasicBlock(instructions);

    REQUIRE(block.instructions.size() == 1);
    REQUIRE(std::holds_alternative<Branch>(block.terminator.kind));
    auto &branch = std::get<Branch>(block.terminator.kind);
    REQUIRE(branch.targetBlock == 0x1004); // PC + 4
  }

  SECTION("Block with terminator instruction") {
    auto addInst =
        createRType(riscv::Instruction::Opcode::ADD, 1, 2, 3, 0x1000);
    auto beqInst =
        createBType(riscv::Instruction::Opcode::BEQ, 1, 2, 8, 0x1004);
    std::vector<riscv::Instruction> instructions = {addInst, beqInst};

    auto block = lifter.liftBasicBlock(instructions);

    REQUIRE(block.instructions.size() ==
            2); // ADD + comparison for BEQ condition
    REQUIRE(std::holds_alternative<CondBranch>(block.terminator.kind));
  }

  SECTION("Multiple instructions without terminator") {
    auto add1 = createRType(riscv::Instruction::Opcode::ADD, 1, 2, 3, 0x1000);
    auto add2 = createRType(riscv::Instruction::Opcode::ADD, 4, 1, 3, 0x1004);
    std::vector<riscv::Instruction> instructions = {add1, add2};

    auto block = lifter.liftBasicBlock(instructions);

    REQUIRE(block.instructions.size() == 2);
    REQUIRE(std::holds_alternative<Branch>(block.terminator.kind));
    auto &branch = std::get<Branch>(block.terminator.kind);
    REQUIRE(branch.targetBlock == 0x1008); // Last instruction + 4
  }
}

TEST_CASE("Lifter Arithmetic Instructions", "[lifter][arithmetic]") {
  Lifter lifter;

  SECTION("ADD instruction") {
    auto inst = createRType(riscv::Instruction::Opcode::ADD, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 1);
    auto &irInst = block.instructions[0];
    REQUIRE(std::holds_alternative<BinaryOp>(irInst.kind));

    auto &binOp = std::get<BinaryOp>(irInst.kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Add);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("ADDI instruction") {
    auto inst = createIType(riscv::Instruction::Opcode::ADDI, 1, 2, 42);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 2); // constant + add

    // First instruction should be the constant
    auto &constInst = block.instructions[0];
    REQUIRE(std::holds_alternative<Const>(constInst.kind));
    auto &constOp = std::get<Const>(constInst.kind);
    REQUIRE(constOp.type == Type::i64);
    REQUIRE(constOp.value == 42);

    // Second instruction should be the add
    auto &addInst = block.instructions[1];
    REQUIRE(std::holds_alternative<BinaryOp>(addInst.kind));
    auto &binOp = std::get<BinaryOp>(addInst.kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Add);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("SUB instruction") {
    auto inst = createRType(riscv::Instruction::Opcode::SUB, 5, 6, 7);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 1);
    auto &irInst = block.instructions[0];
    REQUIRE(std::holds_alternative<BinaryOp>(irInst.kind));

    auto &binOp = std::get<BinaryOp>(irInst.kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Sub);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("ADDW instruction (32-bit with sign extension)") {
    auto inst = createRType(riscv::Instruction::Opcode::ADDW, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() ==
            4); // trunc rs1, trunc rs2, add32, sext

    // Check truncations
    REQUIRE(std::holds_alternative<Trunc>(block.instructions[0].kind));
    REQUIRE(std::holds_alternative<Trunc>(block.instructions[1].kind));

    // Check 32-bit add
    auto &addInst = block.instructions[2];
    REQUIRE(std::holds_alternative<BinaryOp>(addInst.kind));
    auto &binOp = std::get<BinaryOp>(addInst.kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Add);
    REQUIRE(binOp.type == Type::i32);

    // Check sign extension
    auto &sextInst = block.instructions[3];
    REQUIRE(std::holds_alternative<Sext>(sextInst.kind));
    auto &sext = std::get<Sext>(sextInst.kind);
    REQUIRE(sext.fromType == Type::i32);
    REQUIRE(sext.toType == Type::i64);
  }

  SECTION("ADDIW instruction (32-bit immediate with sign extension)") {
    auto inst = createIType(riscv::Instruction::Opcode::ADDIW, 1, 2, -10);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 4); // const, trunc, add32, sext

    // Check constant creation
    auto &constInst = block.instructions[0];
    REQUIRE(std::holds_alternative<Const>(constInst.kind));
    auto &constOp = std::get<Const>(constInst.kind);
    REQUIRE(constOp.type == Type::i32);

    // Check truncation
    auto &truncInst = block.instructions[1];
    REQUIRE(std::holds_alternative<Trunc>(truncInst.kind));

    // Check 32-bit add
    auto &addInst = block.instructions[2];
    REQUIRE(std::holds_alternative<BinaryOp>(addInst.kind));
    auto &binOp = std::get<BinaryOp>(addInst.kind);
    REQUIRE(binOp.type == Type::i32);

    // Check sign extension
    auto &sextInst = block.instructions[3];
    REQUIRE(std::holds_alternative<Sext>(sextInst.kind));
  }
}

TEST_CASE("Lifter Bitwise Operations", "[lifter][bitwise]") {
  Lifter lifter;

  SECTION("AND instruction") {
    auto inst = createRType(riscv::Instruction::Opcode::AND, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 1);
    auto &irInst = block.instructions[0];
    REQUIRE(std::holds_alternative<BinaryOp>(irInst.kind));

    auto &binOp = std::get<BinaryOp>(irInst.kind);
    REQUIRE(binOp.opcode == BinaryOpcode::And);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("ANDI instruction") {
    auto inst = createIType(riscv::Instruction::Opcode::ANDI, 1, 2, 0xFF);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 2); // constant + and

    auto &constInst = block.instructions[0];
    REQUIRE(std::holds_alternative<Const>(constInst.kind));
    auto &constOp = std::get<Const>(constInst.kind);
    REQUIRE(constOp.value == 0xFF);

    auto &andInst = block.instructions[1];
    REQUIRE(std::holds_alternative<BinaryOp>(andInst.kind));
    auto &binOp = std::get<BinaryOp>(andInst.kind);
    REQUIRE(binOp.opcode == BinaryOpcode::And);
  }

  SECTION("OR instruction") {
    auto inst = createRType(riscv::Instruction::Opcode::OR, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    auto &irInst = block.instructions[0];
    auto &binOp = std::get<BinaryOp>(irInst.kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Or);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("ORI instruction") {
    auto inst = createIType(riscv::Instruction::Opcode::ORI, 1, 2, 0x1000);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 2);
    auto &constOp = std::get<Const>(block.instructions[0].kind);
    REQUIRE(constOp.value == 0x1000);

    auto &binOp = std::get<BinaryOp>(block.instructions[1].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Or);
  }

  SECTION("XOR instruction") {
    auto inst = createRType(riscv::Instruction::Opcode::XOR, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    auto &binOp = std::get<BinaryOp>(block.instructions[0].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Xor);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("XORI instruction") {
    auto inst = createIType(riscv::Instruction::Opcode::XORI, 1, 2, -1);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 2);
    auto &binOp = std::get<BinaryOp>(block.instructions[1].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Xor);
  }
}

TEST_CASE("Lifter Shift Operations", "[lifter][shift]") {
  Lifter lifter;

  SECTION("SLL instruction") {
    auto inst = createRType(riscv::Instruction::Opcode::SLL, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 1);
    auto &binOp = std::get<BinaryOp>(block.instructions[0].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Shl);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("SLLI instruction") {
    auto inst = createIType(riscv::Instruction::Opcode::SLLI, 1, 2, 5);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 2); // constant + shift
    auto &constOp = std::get<Const>(block.instructions[0].kind);
    REQUIRE(constOp.value == 5);

    auto &binOp = std::get<BinaryOp>(block.instructions[1].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Shl);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("SRL instruction") {
    auto inst = createRType(riscv::Instruction::Opcode::SRL, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    auto &binOp = std::get<BinaryOp>(block.instructions[0].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Shr);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("SRLI instruction") {
    auto inst = createIType(riscv::Instruction::Opcode::SRLI, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 2);
    auto &binOp = std::get<BinaryOp>(block.instructions[1].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Shr);
  }

  SECTION("SRA instruction") {
    auto inst = createRType(riscv::Instruction::Opcode::SRA, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    auto &binOp = std::get<BinaryOp>(block.instructions[0].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Sar);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("SRAI instruction") {
    auto inst = createIType(riscv::Instruction::Opcode::SRAI, 1, 2, 7);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 2);
    auto &binOp = std::get<BinaryOp>(block.instructions[1].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Sar);
  }

  SECTION("SLLIW instruction (32-bit shift with sign extension)") {
    auto inst = createIType(riscv::Instruction::Opcode::SLLIW, 1, 2, 4);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 4); // const, trunc, shl32, sext

    // Check constant creation
    auto &constOp = std::get<Const>(block.instructions[0].kind);
    REQUIRE(constOp.type == Type::i32);
    REQUIRE(constOp.value == 4);

    // Check truncation
    REQUIRE(std::holds_alternative<Trunc>(block.instructions[1].kind));

    // Check 32-bit shift
    auto &shlInst = block.instructions[2];
    REQUIRE(std::holds_alternative<BinaryOp>(shlInst.kind));
    auto &binOp = std::get<BinaryOp>(shlInst.kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Shl);
    REQUIRE(binOp.type == Type::i32);

    // Check sign extension
    REQUIRE(std::holds_alternative<Sext>(block.instructions[3].kind));
  }
}

TEST_CASE("Lifter Comparison Instructions", "[lifter][comparison]") {
  Lifter lifter;

  SECTION("SLT instruction") {
    auto inst = createRType(riscv::Instruction::Opcode::SLT, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 1);
    auto &binOp = std::get<BinaryOp>(block.instructions[0].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::Lt);
    REQUIRE(binOp.type == Type::i64);
  }

  SECTION("SLTU instruction") {
    auto inst = createRType(riscv::Instruction::Opcode::SLTU, 1, 2, 3);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 1);
    auto &binOp = std::get<BinaryOp>(block.instructions[0].kind);
    REQUIRE(binOp.opcode == BinaryOpcode::LtU);
    REQUIRE(binOp.type == Type::i64);
  }
}

TEST_CASE("Lifter Memory Operations", "[lifter][memory]") {
  Lifter lifter;

  SECTION("LB instruction (load byte with sign extension)") {
    auto inst = createIType(riscv::Instruction::Opcode::LB, 1, 2, 8);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 4); // const, add, load, sext

    // Check immediate constant
    auto &constOp = std::get<Const>(block.instructions[0].kind);
    REQUIRE(constOp.value == 8);

    // Check address calculation
    auto &addOp = std::get<BinaryOp>(block.instructions[1].kind);
    REQUIRE(addOp.opcode == BinaryOpcode::Add);

    // Check load
    auto &loadOp = std::get<Load>(block.instructions[2].kind);
    REQUIRE(loadOp.type == Type::i8);
  }

  SECTION("LH instruction (load halfword with sign extension)") {
    auto inst = createIType(riscv::Instruction::Opcode::LH, 1, 2, 16);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 4); // const, add, load, sext
    auto &loadOp = std::get<Load>(block.instructions[2].kind);
    REQUIRE(loadOp.type == Type::i16);

    auto &sextOp = std::get<Sext>(block.instructions[3].kind);
    REQUIRE(sextOp.fromType == Type::i16);
    REQUIRE(sextOp.toType == Type::i64);
  }

  SECTION("LW instruction (load word with sign extension)") {
    auto inst = createIType(riscv::Instruction::Opcode::LW, 1, 2, 0);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 4); // const, add, load, sext
    auto &loadOp = std::get<Load>(block.instructions[2].kind);
    REQUIRE(loadOp.type == Type::i32);
  }

  SECTION("LD instruction (load doubleword)") {
    auto inst = createIType(riscv::Instruction::Opcode::LD, 1, 2, -8);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() ==
            3); // const, add, load (no extension needed)
    auto &loadOp = std::get<Load>(block.instructions[2].kind);
    REQUIRE(loadOp.type == Type::i64);
  }

  SECTION("LWU instruction (load word with zero extension)") {
    auto inst = createIType(riscv::Instruction::Opcode::LWU, 1, 2, 4);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 4); // const, add, load, zext
    auto &loadOp = std::get<Load>(block.instructions[2].kind);
    REQUIRE(loadOp.type == Type::i32);

    auto &zextOp = std::get<Zext>(block.instructions[3].kind);
    REQUIRE(zextOp.fromType == Type::i32);
    REQUIRE(zextOp.toType == Type::i64);
  }

  SECTION("SD instruction (store doubleword)") {
    // Create helper instruction to set up register value first
    auto setupInst =
        createIType(riscv::Instruction::Opcode::ADDI, 3, 0, 123); // x3 = 123
    auto storeInst = createIType(riscv::Instruction::Opcode::SD, 3, 2,
                                 16); // Store x3 to [x2+16]

    auto block = lifter.liftBasicBlock({setupInst, storeInst});

    // Should have: const(123), add(x0+123), const(16), add(x2+16), store
    REQUIRE(block.instructions.size() == 6);

    // Last instruction should be store
    REQUIRE(std::holds_alternative<Store>(block.instructions[5].kind));
  }

  SECTION("SW instruction (store word with truncation)") {
    auto setupInst = createIType(riscv::Instruction::Opcode::ADDI, 3, 0, 456);
    auto storeInst = createIType(riscv::Instruction::Opcode::SW, 3, 2, 8);

    auto block = lifter.liftBasicBlock({setupInst, storeInst});

    // Should include truncation before store
    bool foundTrunc = false;
    bool foundStore = false;
    for (const auto &inst : block.instructions) {
      if (std::holds_alternative<Trunc>(inst.kind)) {
        auto &trunc = std::get<Trunc>(inst.kind);
        REQUIRE(trunc.fromType == Type::i64);
        REQUIRE(trunc.toType == Type::i32);
        foundTrunc = true;
      }
      if (std::holds_alternative<Store>(inst.kind)) {
        foundStore = true;
      }
    }
    REQUIRE(foundTrunc);
    REQUIRE(foundStore);
  }
}

TEST_CASE("Lifter Upper Immediate Instructions", "[lifter][upper-immediate]") {
  Lifter lifter;

  SECTION("LUI instruction") {
    auto inst = createUType(riscv::Instruction::Opcode::LUI, 1, 0x12345);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 1);
    auto &constOp = std::get<Const>(block.instructions[0].kind);
    REQUIRE(constOp.type == Type::i64);
    REQUIRE(constOp.value == (0x12345ULL << 12)); // Shifted by 12 bits
  }

  SECTION("AUIPC instruction") {
    auto inst =
        createUType(riscv::Instruction::Opcode::AUIPC, 1, 0x1000, 0x2000);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() == 3); // pc_const, imm_const, add

    // First constant should be PC
    auto &pcConst = std::get<Const>(block.instructions[0].kind);
    REQUIRE(pcConst.value == 0x2000);

    // Second constant should be shifted immediate
    auto &immConst = std::get<Const>(block.instructions[1].kind);
    REQUIRE(immConst.value == (0x1000ULL << 12));

    // Addition of PC + immediate
    auto &addOp = std::get<BinaryOp>(block.instructions[2].kind);
    REQUIRE(addOp.opcode == BinaryOpcode::Add);
    REQUIRE(addOp.type == Type::i64);
  }
}

TEST_CASE("Lifter Control Flow Instructions", "[lifter][control-flow]") {
  Lifter lifter;

  SECTION("BEQ instruction") {
    auto inst = createBType(riscv::Instruction::Opcode::BEQ, 1, 2, 12, 0x1000);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(block.instructions.size() ==
            1); // Comparison instruction for branch condition
    REQUIRE(std::holds_alternative<CondBranch>(block.terminator.kind));

    auto &condBranch = std::get<CondBranch>(block.terminator.kind);
    REQUIRE(condBranch.trueBlock == 0x100C);  // PC + offset (0x1000 + 12)
    REQUIRE(condBranch.falseBlock == 0x1004); // Fall-through (PC + 4)
  }

  SECTION("BNE instruction") {
    auto inst = createBType(riscv::Instruction::Opcode::BNE, 3, 4, -8, 0x2000);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(std::holds_alternative<CondBranch>(block.terminator.kind));
    auto &condBranch = std::get<CondBranch>(block.terminator.kind);
    REQUIRE(condBranch.trueBlock == 0x1FF8);  // PC + offset (0x2000 - 8)
    REQUIRE(condBranch.falseBlock == 0x2004); // Fall-through
  }

  SECTION("BLT instruction") {
    auto inst = createBType(riscv::Instruction::Opcode::BLT, 5, 6, 16, 0x1000);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(std::holds_alternative<CondBranch>(block.terminator.kind));
    auto &condBranch = std::get<CondBranch>(block.terminator.kind);
    REQUIRE(condBranch.trueBlock == 0x1010); // PC + 16
  }

  SECTION("BLTU instruction") {
    auto inst = createBType(riscv::Instruction::Opcode::BLTU, 7, 8, 20, 0x1000);
    auto block = lifter.liftBasicBlock({inst});

    REQUIRE(std::holds_alternative<CondBranch>(block.terminator.kind));
    auto &condBranch = std::get<CondBranch>(block.terminator.kind);
    REQUIRE(condBranch.trueBlock == 0x1014); // PC + 20
  }

  SECTION("JAL instruction") {
    auto inst = createJType(riscv::Instruction::Opcode::JAL, 1, 24, 0x1000);
    auto block = lifter.liftBasicBlock({inst});

    // Should have return address constant stored to register
    REQUIRE(block.instructions.size() == 1);
    auto &constOp = std::get<Const>(block.instructions[0].kind);
    REQUIRE(constOp.value == 0x1004); // PC + 4 for return address

    // Should have unconditional branch terminator
    REQUIRE(std::holds_alternative<Branch>(block.terminator.kind));
    auto &branch = std::get<Branch>(block.terminator.kind);
    REQUIRE(branch.targetBlock == 0x1018); // PC + offset (0x1000 + 24)
  }

  SECTION("JALR instruction") {
    auto setupInst = createIType(riscv::Instruction::Opcode::ADDI, 2, 0,
                                 0x2000); // x2 = 0x2000
    auto inst = createIType(riscv::Instruction::Opcode::JALR, 1, 2, 4, 0x1000);
    auto block = lifter.liftBasicBlock({setupInst, inst});

    // Should store return address and create return terminator
    bool foundReturnAddr = false;
    for (const auto &irInst : block.instructions) {
      if (std::holds_alternative<Const>(irInst.kind)) {
        auto &constOp = std::get<Const>(irInst.kind);
        if (constOp.value == 0x1004) { // PC + 4 for return
          foundReturnAddr = true;
        }
      }
    }
    REQUIRE(foundReturnAddr);

    REQUIRE(std::holds_alternative<Return>(block.terminator.kind));
  }
}

TEST_CASE("Lifter Register Handling", "[lifter][registers]") {
  Lifter lifter;

  SECTION("x0 register always returns zero") {
    // x0 should always be zero, even if we try to set it
    auto inst1 = createIType(riscv::Instruction::Opcode::ADDI, 0, 0,
                             100); // Try to set x0 = 100
    auto inst2 =
        createRType(riscv::Instruction::Opcode::ADD, 1, 0, 0); // x1 = x0 + x0

    auto block = lifter.liftBasicBlock({inst1, inst2});

    // First instruction should generate constant 100 but not affect x0
    // Second instruction should use zero for both operands
    bool foundZeroConstant = false;
    for (const auto &inst : block.instructions) {
      if (std::holds_alternative<Const>(inst.kind)) {
        auto &constOp = std::get<Const>(inst.kind);
        if (constOp.value == 0) {
          foundZeroConstant = true;
        }
      }
    }
    REQUIRE(foundZeroConstant);
  }

  SECTION("Register values persist across instructions") {
    auto inst1 =
        createIType(riscv::Instruction::Opcode::ADDI, 1, 0, 42); // x1 = 42
    auto inst2 =
        createIType(riscv::Instruction::Opcode::ADDI, 2, 1, 8); // x2 = x1 + 8

    auto block = lifter.liftBasicBlock({inst1, inst2});

    // Should have: const(0), add(x0+42), const(8), add(x1+8), const(0) for x0
    // access
    REQUIRE(block.instructions.size() == 5);

    // Verify we have the right number and types of instructions
    int constCount = 0;
    int addCount = 0;
    for (const auto &inst : block.instructions) {
      if (std::holds_alternative<Const>(inst.kind))
        constCount++;
      if (std::holds_alternative<BinaryOp>(inst.kind))
        addCount++;
    }
    REQUIRE(constCount == 3); // 0, 42, 8
    REQUIRE(addCount == 2);   // Two additions
  }

  SECTION("SSA value IDs are unique") {
    auto inst1 = createRType(riscv::Instruction::Opcode::ADD, 1, 2, 3);
    auto inst2 = createRType(riscv::Instruction::Opcode::ADD, 4, 5, 6);

    auto block = lifter.liftBasicBlock({inst1, inst2});

    // All instructions should have unique value IDs
    std::set<ValueId> seenIds;
    for (const auto &inst : block.instructions) {
      REQUIRE(seenIds.find(inst.valueId) == seenIds.end());
      seenIds.insert(inst.valueId);
    }
  }
}

TEST_CASE("Lifter Edge Cases and Error Handling", "[lifter][edge-cases]") {
  Lifter lifter;

  SECTION("Unsupported instruction throws exception") {
    auto inst = createInstruction(riscv::Instruction::Opcode::INVALID, {});

    REQUIRE_THROWS_AS(lifter.liftBasicBlock({inst}), std::runtime_error);
  }

  SECTION("Multiple consecutive terminators") {
    auto beq1 = createBType(riscv::Instruction::Opcode::BEQ, 1, 2, 8, 0x1000);
    auto beq2 = createBType(riscv::Instruction::Opcode::BEQ, 3, 4, 12, 0x1004);

    auto block = lifter.liftBasicBlock({beq1, beq2});

    // Should only process first terminator but still generate comparison
    REQUIRE(block.instructions.size() == 1); // Comparison for first branch
    REQUIRE(std::holds_alternative<CondBranch>(block.terminator.kind));
  }

  SECTION("Block with only terminator") {
    auto jal = createJType(riscv::Instruction::Opcode::JAL, 1, 100, 0x1000);
    auto block = lifter.liftBasicBlock({jal});

    REQUIRE(block.instructions.size() == 1); // Return address constant
    REQUIRE(std::holds_alternative<Branch>(block.terminator.kind));
  }

  SECTION("Zero register in different instruction types") {
    // Test that x0 behaves correctly across different instruction patterns
    auto addToZero = createRType(riscv::Instruction::Opcode::ADD, 0, 1,
                                 2); // x0 = x1 + x2 (should be ignored)
    auto addFromZero = createRType(riscv::Instruction::Opcode::ADD, 1, 0,
                                   2); // x1 = x0 + x2 (should use zero)

    auto block = lifter.liftBasicBlock({addToZero, addFromZero});

    // Verify x0 usage results in zero constant
    bool foundZeroForX0 = false;
    for (const auto &inst : block.instructions) {
      if (std::holds_alternative<BinaryOp>(inst.kind)) {
        auto &binOp = std::get<BinaryOp>(inst.kind);
        // Check if any operand is a constant zero (representing x0)
        for (const auto &checkInst : block.instructions) {
          if (std::holds_alternative<Const>(checkInst.kind) &&
              std::get<Const>(checkInst.kind).value == 0 &&
              (checkInst.valueId == binOp.lhs ||
               checkInst.valueId == binOp.rhs)) {
            foundZeroForX0 = true;
          }
        }
      }
    }
    REQUIRE(foundZeroForX0);
  }
}
