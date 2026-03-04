#include "../../lib/ExecutionEngine.h"
#include "../../lib/ARM64/Encoder.h"
#include "../../lib/GuestState.h"
#include <catch2/catch_test_macros.hpp>
#include <sys/mman.h>

using namespace dinorisc;
using namespace dinorisc::arm64;

class ExecutionEngineTestHelper {
public:
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

    // Allocate shadow memory for tests
    size_t memSize = 4096; // 4KB should be enough for tests
    state.shadowMemory = mmap(nullptr, memSize, PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    state.shadowMemorySize = memSize;
    state.guestMemoryBase = 0x20000;

    return state;
  }

  static void cleanupState(GuestState &state) {
    if (state.shadowMemory) {
      munmap(state.shadowMemory, state.shadowMemorySize);
      state.shadowMemory = nullptr;
    }
  }
};

TEST_CASE("ExecutionEngine - Basic arithmetic operations", "[execution]") {
  ExecutionEngine engine;

  SECTION("ADD immediate - X0 = 42") {
    // Function that modifies GuestState.x[0] to 42
    // x0 contains pointer to GuestState
    // MOV x1, #42         ; Load immediate value 42 into x1
    // STR x1, [x0]        ; Store x1 into GuestState.x[0] (x0 points to
    // GuestState) MOV x0, #0x1000C    ; Set return PC RET
    std::vector<Instruction> instructions = {
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X1,
                                   Immediate{42}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X1,
                               Register::X0, 0}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x100C}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}}};

    auto machineCode =
        ExecutionEngineTestHelper::createMachineCode(instructions);
    auto guestState = ExecutionEngineTestHelper::createInitialState();

    uint64_t nextPC = engine.executeBlock(machineCode, &guestState);

    REQUIRE(guestState.x[0] == 42);
    REQUIRE(nextPC == 0x100C);
    ExecutionEngineTestHelper::cleanupState(guestState);
  }

  SECTION("ADD with registers - X0 = X1 + X2") {
    // Function that sets GuestState registers and performs ADD
    // x0 contains pointer to GuestState
    // MOV x1, #10         ; Load 10 into x1
    // STR x1, [x0, #8]    ; Store into GuestState.x[1] (offset 8)
    // MOV x2, #20         ; Load 20 into x2
    // STR x2, [x0, #16]   ; Store into GuestState.x[2] (offset 16)
    // ADD x3, x1, x2      ; Add x1 + x2 -> x3
    // STR x3, [x0]        ; Store result into GuestState.x[0]
    // MOV x0, #0x10010    ; Set return PC
    // RET
    std::vector<Instruction> instructions = {
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
                                   Register::X30}}};

    auto machineCode =
        ExecutionEngineTestHelper::createMachineCode(instructions);
    auto guestState = ExecutionEngineTestHelper::createInitialState();

    uint64_t nextPC = engine.executeBlock(machineCode, &guestState);

    REQUIRE(guestState.x[0] == 30);
    REQUIRE(guestState.x[1] == 10);
    REQUIRE(guestState.x[2] == 20);
    REQUIRE(nextPC == 0x1010);
    ExecutionEngineTestHelper::cleanupState(guestState);
  }

  SECTION("SUB with immediate") {
    // Function that performs SUB with immediate
    // MOV x1, #100        ; Load 100 into x1
    // STR x1, [x0, #8]    ; Store into GuestState.x[1]
    // SUB x2, x1, #25     ; Subtract 25 from x1 -> x2
    // STR x2, [x0]        ; Store result into GuestState.x[0]
    // MOV x0, #0x10014    ; Set return PC
    // RET
    std::vector<Instruction> instructions = {
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
                                   Register::X30}}};

    auto machineCode =
        ExecutionEngineTestHelper::createMachineCode(instructions);
    auto guestState = ExecutionEngineTestHelper::createInitialState();

    uint64_t nextPC = engine.executeBlock(machineCode, &guestState);

    REQUIRE(guestState.x[0] == 75);
    REQUIRE(guestState.x[1] == 100);
    REQUIRE(nextPC == 0x1014);
    ExecutionEngineTestHelper::cleanupState(guestState);
  }

  SECTION("MUL with registers") {
    // Function that performs MUL with registers
    // MOV x1, #7          ; Load 7 into x1
    // MOV x2, #6          ; Load 6 into x2
    // MUL x3, x1, x2      ; Multiply x1 * x2 -> x3
    // STR x3, [x0]        ; Store result into GuestState.x[0]
    // MOV x0, #0x10018    ; Set return PC
    // RET
    std::vector<Instruction> instructions = {
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
                                   Register::X30}}};

    auto machineCode =
        ExecutionEngineTestHelper::createMachineCode(instructions);
    auto guestState = ExecutionEngineTestHelper::createInitialState();

    uint64_t nextPC = engine.executeBlock(machineCode, &guestState);

    REQUIRE(guestState.x[0] == 42);
    REQUIRE(nextPC == 0x1018);
    ExecutionEngineTestHelper::cleanupState(guestState);
  }
}

TEST_CASE("ExecutionEngine - Data movement", "[execution]") {
  ExecutionEngine engine;

  SECTION("MOV immediate values") {
    // Function that loads immediate values into GuestState
    // MOV x1, #0x1234     ; Load 0x1234 into x1
    // STR x1, [x0]        ; Store into GuestState.x[0]
    // MOV x2, #0xABCD     ; Load 0xABCD into x2
    // STR x2, [x0, #8]    ; Store into GuestState.x[1]
    // MOV x0, #0x1001C    ; Set return PC
    // RET
    std::vector<Instruction> instructions = {
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
                                   Register::X30}}};

    auto machineCode =
        ExecutionEngineTestHelper::createMachineCode(instructions);
    auto guestState = ExecutionEngineTestHelper::createInitialState();

    uint64_t nextPC = engine.executeBlock(machineCode, &guestState);

    REQUIRE(guestState.x[0] == 0x1234);
    REQUIRE(guestState.x[1] == 0xABCD);
    REQUIRE(nextPC == 0x101C);
    ExecutionEngineTestHelper::cleanupState(guestState);
  }

  SECTION("MOV register to register") {
    // Function that demonstrates register-to-register moves
    // MOV x1, #0x5555     ; Load 0x5555 into x1
    // STR x1, [x0, #8]    ; Store into GuestState.x[1]
    // MOV x2, x1          ; Move x1 to x2
    // STR x2, [x0]        ; Store x2 into GuestState.x[0]
    // MOV x0, #0x10020    ; Set return PC
    // RET
    std::vector<Instruction> instructions = {
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
                                   Register::X30}}};

    auto machineCode =
        ExecutionEngineTestHelper::createMachineCode(instructions);
    auto guestState = ExecutionEngineTestHelper::createInitialState();

    uint64_t nextPC = engine.executeBlock(machineCode, &guestState);

    REQUIRE(guestState.x[0] == 0x5555);
    REQUIRE(guestState.x[1] == 0x5555);
    REQUIRE(nextPC == 0x1020);
    ExecutionEngineTestHelper::cleanupState(guestState);
  }
}

TEST_CASE("ExecutionEngine - Memory operations", "[execution]") {
  ExecutionEngine engine;

  SECTION("Basic memory test") {
    // Simple test that just verifies basic memory operations work
    // MOV x1, #0x1234     ; Load test value
    // STR x1, [x0]        ; Store into GuestState.x[0]
    // MOV x0, #0x10024    ; Set return PC
    // RET
    std::vector<Instruction> instructions = {
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X1,
                                   Immediate{0x1234}}},
        Instruction{MemoryInst{Opcode::STR, DataSize::X, Register::X1,
                               Register::X0, 0}},
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x1024}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}}};

    auto machineCode =
        ExecutionEngineTestHelper::createMachineCode(instructions);
    auto guestState = ExecutionEngineTestHelper::createInitialState();

    uint64_t nextPC = engine.executeBlock(machineCode, &guestState);

    REQUIRE(guestState.x[0] == 0x1234);
    REQUIRE(nextPC == 0x1024);
    ExecutionEngineTestHelper::cleanupState(guestState);
  }
}

TEST_CASE("ExecutionEngine - Complex sequences", "[execution]") {
  ExecutionEngine engine;

  SECTION("Multiple arithmetic operations") {
    // Complex function with multiple operations
    // MOV x1, #10         ; Load 10 into x1
    // STR x1, [x0, #8]    ; Store into GuestState.x[1]
    // MOV x2, #5          ; Load 5 into x2
    // STR x2, [x0, #16]   ; Store into GuestState.x[2]
    // ADD x3, x1, x2      ; x3 = 15
    // STR x3, [x0, #24]   ; Store into GuestState.x[3]
    // SUB x4, x3, x2      ; x4 = 10
    // STR x4, [x0, #32]   ; Store into GuestState.x[4]
    // MUL x5, x3, x4      ; x5 = 150
    // STR x5, [x0]        ; Store result into GuestState.x[0]
    // MOV x0, #0x10028    ; Set return PC
    // RET
    std::vector<Instruction> instructions = {
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
                                   Register::X30}}};

    auto machineCode =
        ExecutionEngineTestHelper::createMachineCode(instructions);
    auto guestState = ExecutionEngineTestHelper::createInitialState();

    uint64_t nextPC = engine.executeBlock(machineCode, &guestState);

    REQUIRE(guestState.x[0] == 150); // Final result
    REQUIRE(guestState.x[1] == 10);  // Original values preserved
    REQUIRE(guestState.x[2] == 5);
    REQUIRE(guestState.x[3] == 15); // Intermediate result
    REQUIRE(guestState.x[4] == 10); // Intermediate result
    REQUIRE(nextPC == 0x1028);
    ExecutionEngineTestHelper::cleanupState(guestState);
  }
}

TEST_CASE("ExecutionEngine - Edge cases", "[execution]") {
  ExecutionEngine engine;

  SECTION("Empty machine code") {
    std::vector<uint8_t> emptyCode;
    auto guestState = ExecutionEngineTestHelper::createInitialState();

    uint64_t nextPC = engine.executeBlock(emptyCode, &guestState);

    REQUIRE(nextPC == 0); // Should return 0 for empty code
    ExecutionEngineTestHelper::cleanupState(guestState);
  }

  SECTION("Single RET instruction") {
    // Minimal function that just returns a specific PC
    // MOV x0, #0x1002C    ; Set return PC
    // RET
    std::vector<Instruction> instructions = {
        Instruction{TwoOperandInst{Opcode::MOV, DataSize::X, Register::X0,
                                   Immediate{0x102C}}},
        Instruction{TwoOperandInst{Opcode::RET, DataSize::X, Register::X0,
                                   Register::X30}}};

    auto machineCode =
        ExecutionEngineTestHelper::createMachineCode(instructions);
    auto guestState = ExecutionEngineTestHelper::createInitialState();

    uint64_t nextPC = engine.executeBlock(machineCode, &guestState);

    REQUIRE(nextPC == 0x102C);
    ExecutionEngineTestHelper::cleanupState(guestState);
  }
}