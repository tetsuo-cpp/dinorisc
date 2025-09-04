#include "BinaryTranslator.h"
#include <iomanip>
#include <iostream>

namespace dinorisc {

BinaryTranslator::BinaryTranslator() : textBaseAddress(0), entryPoint(0) {
  initializeTranslator();
}

BinaryTranslator::~BinaryTranslator() = default;

bool BinaryTranslator::executeProgram(const std::string &inputPath) {
  std::cout << "Loading and executing RISC-V binary: " << inputPath
            << std::endl;

  // Step 1: Load RISC-V ELF binary
  if (!loadRISCVBinary(inputPath)) {
    return false;
  }

  std::cout << "Loaded text section: " << textSectionData.size() << " bytes"
            << std::endl;
  std::cout << "Entry point: 0x" << std::hex << entryPoint << std::dec
            << std::endl;

  // Print all instructions and their IR lifting
  std::cout << "\nLifting instructions to IR basic blocks:" << std::endl;
  size_t numInstructions = textSectionData.size() / 4;

  // Decode all instructions first
  std::vector<riscv::Instruction> instructions;
  for (size_t i = 0; i < numInstructions; ++i) {
    uint64_t pc = textBaseAddress + (i * 4);
    riscv::Instruction inst =
        decoder->decode(textSectionData.data(), i * 4, pc);
    instructions.push_back(inst);
  }

  // Process instructions in basic blocks
  size_t blockNum = 0;
  for (size_t i = 0; i < instructions.size();) {
    std::vector<riscv::Instruction> blockInstructions;
    size_t blockStart = i;

    // Collect instructions until we hit a terminator
    while (i < instructions.size()) {
      blockInstructions.push_back(instructions[i]);
      if (lifter->isTerminator(instructions[i])) {
        i++;
        break;
      }
      i++;
    }

    // Print the basic block
    std::cout << "Basic Block " << blockNum++ << ":" << std::endl;
    for (size_t j = 0; j < blockInstructions.size(); ++j) {
      std::cout << "  [" << (blockStart + j)
                << "] RISC-V: " << blockInstructions[j].toString() << std::endl;
    }

    // Lift to IR basic block and print
    ir::BasicBlock irBlock = lifter->liftBasicBlock(blockInstructions);
    std::cout << "  IR Block: " << irBlock.toString() << std::endl;

    // Translate IR block to ARM64
    auto arm64Instructions = translateToARM64(irBlock);
    std::cout << std::endl;
  }

  // Step 2: Start dynamic binary translation and execution
  if (!executeWithDBT()) {
    return false;
  }

  return true;
}

void BinaryTranslator::initializeTranslator() {
  elfReader = std::make_unique<ELFReader>();
  decoder = std::make_unique<riscv::Decoder>();
  lifter = std::make_unique<Lifter>();
  instructionSelector = std::make_unique<lowering::InstructionSelector>();
  registerAllocator = std::make_unique<lowering::RegisterAllocator>();
}

bool BinaryTranslator::loadRISCVBinary(const std::string &inputPath) {
  // Load ELF file
  if (!elfReader->loadFile(inputPath)) {
    std::cerr << "Error loading ELF file: " << elfReader->getErrorMessage()
              << std::endl;
    return false;
  }

  // Get entry point and text section
  entryPoint = elfReader->getEntryPoint();
  const auto &textSection = elfReader->getTextSection();

  std::cout << "Text section: VA=0x" << std::hex << textSection.virtualAddress
            << " Size=" << std::dec << textSection.size << " bytes"
            << std::endl;

  // Store raw text section data for on-demand decoding
  textSectionData = textSection.data;
  textBaseAddress = textSection.virtualAddress;

  return true;
}

bool BinaryTranslator::executeWithDBT() {
  std::cout << "\nStarting dynamic binary translation and execution..."
            << std::endl;
  std::cout << "DBT engine not yet implemented" << std::endl;

  // TODO: Implement the following:
  // 1. Create a virtual CPU state (registers, memory)
  // 2. Set up translation cache for ARM64 code blocks
  // 3. Implement basic block detection and translation using on-demand decoding
  // 4. Execute translated ARM64 code with runtime support
  // 5. Handle system calls, exceptions, and memory management
  //
  // Example on-demand decoding during execution:
  // uint64_t currentPC = entryPoint;
  // while (executing) {
  //   size_t offset = (currentPC - textBaseAddress);
  //   riscv::Instruction inst = decoder->decode(textSectionData.data(), offset,
  //   currentPC);
  //   // Translate and execute instruction...
  // }

  return true;
}

std::vector<arm64::Instruction>
BinaryTranslator::translateToARM64(const ir::BasicBlock &irBlock) {
  std::cout << "  Starting ARM64 translation for IR block..." << std::endl;

  // Step 1: Instruction Selection (IR -> ARM64)
  std::cout << "    Step 1: Instruction selection (IR -> ARM64)" << std::endl;
  auto arm64Instructions = instructionSelector->selectInstructions(irBlock);
  std::cout << "      Generated " << arm64Instructions.size()
            << " ARM64 instructions" << std::endl;

  // Step 2: Liveness Analysis
  std::cout << "    Step 2: Liveness analysis" << std::endl;
  lowering::LivenessAnalysis liveness(arm64Instructions);
  auto liveIntervals = liveness.computeLiveIntervals();
  std::cout << "      Computed " << liveIntervals.size() << " live intervals"
            << std::endl;

  // Step 3: Register Allocation
  std::cout << "    Step 3: Linear scan register allocation" << std::endl;
  if (!registerAllocator->allocateRegisters(arm64Instructions, liveIntervals)) {
    std::cout << "      WARNING: Register allocation failed - using "
                 "placeholder registers"
              << std::endl;
  } else {
    std::cout << "      Register allocation successful" << std::endl;
  }

  // Log the final ARM64 instructions
  std::cout << "    Final ARM64 instructions:" << std::endl;
  for (size_t i = 0; i < arm64Instructions.size(); ++i) {
    std::cout << "      [" << i << "] " << arm64Instructions[i].toString()
              << std::endl;
  }

  return arm64Instructions;
}

} // namespace dinorisc
