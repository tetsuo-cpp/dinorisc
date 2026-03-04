#include "../../lib/ExecutionEngine.h"
#include "../../lib/ARM64/Encoder.h"
#include "../../lib/Error.h"
#include "../../lib/GuestState.h"
#include <catch2/catch_test_macros.hpp>
#include <sys/mman.h>

using namespace dinorisc;
using namespace dinorisc::arm64;

static std::vector<uint8_t>
createMachineCode(const std::vector<Instruction> &instructions) {
  Encoder encoder;
  std::vector<uint8_t> machineCode;
  for (const auto &inst : instructions) {
    auto encoded = encoder.encodeInstruction(inst);
    machineCode.insert(machineCode.end(), encoded.begin(), encoded.end());
  }
  return machineCode;
}

static GuestState createInitialState() {
  GuestState state;
  state.pc = 0x10000;
  size_t memSize = 4096;
  state.shadowMemory = mmap(nullptr, memSize, PROT_READ | PROT_WRITE,
                            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  state.shadowMemorySize = memSize;
  state.guestMemoryBase = 0x20000;
  return state;
}

struct ExecResult {
  uint64_t nextPC;
  GuestState state;
};

static ExecResult
executeInstructions(const std::vector<Instruction> &instructions) {
  ExecutionEngine engine;
  auto machineCode = createMachineCode(instructions);
  auto state = createInitialState();
  uint64_t nextPC = engine.executeBlock(machineCode, &state);
  return {nextPC, std::move(state)};
}

TEST_CASE("ExecutionEngine - Basic arithmetic operations", "[execution]") {
  SECTION("ADD immediate - X0 = 42") {
    auto [nextPC, state] = executeInstructions({
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X1,
                                   Immediate{42}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X1,
                               Register::X0, 0}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x100C}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}},
    });

    REQUIRE(state.x[0] == 42);
    REQUIRE(nextPC == 0x100C);
  }

  SECTION("ADD with registers - X0 = X1 + X2") {
    auto [nextPC, state] = executeInstructions({
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X1,
                                   Immediate{10}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X1,
                               Register::X0, 8}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X2,
                                   Immediate{20}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X2,
                               Register::X0, 16}},
        Instruction{ThreeOperandInst{Opcode::ADD, DataSize::X, Register::X3,
                                     Register::X1, Register::X2}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X3,
                               Register::X0, 0}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x1010}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}},
    });

    REQUIRE(state.x[0] == 30);
    REQUIRE(state.x[1] == 10);
    REQUIRE(state.x[2] == 20);
    REQUIRE(nextPC == 0x1010);
  }

  SECTION("SUB with immediate") {
    auto [nextPC, state] = executeInstructions({
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X1,
                                   Immediate{100}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X1,
                               Register::X0, 8}},
        Instruction{ThreeOperandInst{Opcode::SUB, DataSize::X, Register::X2,
                                     Register::X1, Immediate{25}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X2,
                               Register::X0, 0}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x1014}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}},
    });

    REQUIRE(state.x[0] == 75);
    REQUIRE(state.x[1] == 100);
    REQUIRE(nextPC == 0x1014);
  }

  SECTION("MUL with registers") {
    auto [nextPC, state] = executeInstructions({
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X1,
                                   Immediate{7}}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X2,
                                   Immediate{6}}},
        Instruction{ThreeOperandInst{Opcode::MUL, DataSize::X, Register::X3,
                                     Register::X1, Register::X2}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X3,
                               Register::X0, 0}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x1018}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}},
    });

    REQUIRE(state.x[0] == 42);
    REQUIRE(nextPC == 0x1018);
  }
}

TEST_CASE("ExecutionEngine - Data movement", "[execution]") {
  SECTION("MOV immediate values") {
    auto [nextPC, state] = executeInstructions({
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X1,
                                   Immediate{0x1234}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X1,
                               Register::X0, 0}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X2,
                                   Immediate{0xABCD}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X2,
                               Register::X0, 8}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x101C}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}},
    });

    REQUIRE(state.x[0] == 0x1234);
    REQUIRE(state.x[1] == 0xABCD);
    REQUIRE(nextPC == 0x101C);
  }

  SECTION("MOV register to register") {
    auto [nextPC, state] = executeInstructions({
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X1,
                                   Immediate{0x5555}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X1,
                               Register::X0, 8}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X2,
                                   Register::X1}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X2,
                               Register::X0, 0}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x1020}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}},
    });

    REQUIRE(state.x[0] == 0x5555);
    REQUIRE(state.x[1] == 0x5555);
    REQUIRE(nextPC == 0x1020);
  }
}

TEST_CASE("ExecutionEngine - Complex sequences", "[execution]") {
  SECTION("Multiple arithmetic operations") {
    auto [nextPC, state] = executeInstructions({
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X1,
                                   Immediate{10}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X1,
                               Register::X0, 8}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X2,
                                   Immediate{5}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X2,
                               Register::X0, 16}},
        Instruction{ThreeOperandInst{Opcode::ADD, DataSize::X, Register::X3,
                                     Register::X1, Register::X2}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X3,
                               Register::X0, 24}},
        Instruction{ThreeOperandInst{Opcode::SUB, DataSize::X, Register::X4,
                                     Register::X3, Register::X2}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X4,
                               Register::X0, 32}},
        Instruction{ThreeOperandInst{Opcode::MUL, DataSize::X, Register::X5,
                                     Register::X3, Register::X4}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X5,
                               Register::X0, 0}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x1028}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}},
    });

    REQUIRE(state.x[0] == 150);
    REQUIRE(state.x[1] == 10);
    REQUIRE(state.x[2] == 5);
    REQUIRE(state.x[3] == 15);
    REQUIRE(state.x[4] == 10);
    REQUIRE(nextPC == 0x1028);
  }
}

TEST_CASE("ExecutionEngine - Edge cases", "[execution]") {
  SECTION("Empty machine code") {
    ExecutionEngine engine;
    std::vector<uint8_t> emptyCode;
    auto state = createInitialState();

    REQUIRE_THROWS_AS(engine.executeBlock(emptyCode, &state),
                      dinorisc::RuntimeError);
  }

  SECTION("Single RET instruction") {
    auto [nextPC, state] = executeInstructions({
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x102C}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}},
    });

    REQUIRE(nextPC == 0x102C);
  }
}
