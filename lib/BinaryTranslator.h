#pragma once

#include "ARM64/Encoder.h"
#include "ELFReader.h"
#include "ExecutionEngine.h"
#include "GuestState.h"
#include "Lifter.h"
#include "Lowering/InstructionSelector.h"
#include "Lowering/LivenessAnalysis.h"
#include "Lowering/RegisterAllocator.h"
#include "RISCV/Decoder.h"
#include "RISCV/Instruction.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace dinorisc {

class BinaryTranslator {
public:
  BinaryTranslator();
  ~BinaryTranslator();

  bool executeProgram(const std::string &inputPath);

  // Execute specific function and return the exit code from register a0 (x10)
  int executeFunction(const std::string &inputPath,
                      const std::string &functionName);

  // Set up initial register state for function arguments
  void setArgumentRegisters(const std::vector<uint64_t> &args);

private:
  std::unique_ptr<ELFReader> elfReader;
  std::unique_ptr<riscv::Decoder> decoder;
  std::unique_ptr<Lifter> lifter;
  std::unique_ptr<lowering::InstructionSelector> instructionSelector;
  std::unique_ptr<lowering::RegisterAllocator> registerAllocator;
  std::unique_ptr<arm64::Encoder> encoder;
  std::unique_ptr<ExecutionEngine> executionEngine;

  // Guest CPU state
  GuestState guestState;

  void initializeTranslator();
  bool loadRISCVBinary(const std::string &inputPath);
  bool executeWithDBT();

  // Translate an IR block to ARM64 instructions
  std::vector<arm64::Instruction>
  translateToARM64(const ir::BasicBlock &irBlock);

  // Encode ARM64 instructions to machine code
  std::vector<uint8_t>
  encodeToMachineCode(const std::vector<arm64::Instruction> &instructions);

  // Execute a single basic block
  uint64_t executeBlock(uint64_t pc);

  // Check if execution should terminate (e.g., invalid PC, system call)
  bool shouldTerminate(uint64_t pc);

  // Extract return value from guest state
  int getReturnValue() const;

  // Current translation state
  std::vector<uint8_t> textSectionData;
  uint64_t textBaseAddress;
  uint64_t entryPoint;
};

} // namespace dinorisc
