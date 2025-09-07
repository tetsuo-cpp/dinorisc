#include "BinaryTranslator.h"
#include <iomanip>
#include <iostream>
#include <sys/mman.h>
#include <unistd.h>

namespace dinorisc {

BinaryTranslator::BinaryTranslator() : textBaseAddress(0), entryPoint(0) {
  initializeTranslator();
}

BinaryTranslator::~BinaryTranslator() {
  // Clean up shadow memory
  if (guestState.shadowMemory) {
    munmap(guestState.shadowMemory, guestState.shadowMemorySize);
  }
}

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
  encoder = std::make_unique<arm64::Encoder>();
  executionEngine = std::make_unique<ExecutionEngine>();

  // Initialize guest state with entry point
  guestState.pc = 0; // Will be set when loading binary

  // Allocate shadow memory for guest program (8MB)
  const size_t shadowSize = 8 * 1024 * 1024;
  void *shadowMem = mmap(nullptr, shadowSize, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (shadowMem == MAP_FAILED) {
    std::cerr << "Failed to allocate shadow memory" << std::endl;
    return;
  }

  guestState.shadowMemory = shadowMem;
  guestState.shadowMemorySize = shadowSize;
  guestState.guestMemoryBase = 0x80000000; // Common RISC-V memory base

  // Set up initial stack pointer (x2/sp) near the top of the memory region
  uint64_t stackTop = guestState.guestMemoryBase + shadowSize - 16;
  guestState.x[2] = stackTop; // x2 is the stack pointer

  std::cout << "Shadow memory allocated: " << shadowSize << " bytes at host "
            << shadowMem << ", guest base 0x" << std::hex
            << guestState.guestMemoryBase << ", stack at 0x" << stackTop
            << std::dec << std::endl;
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

  // If entry point is 0, try to use main function address
  if (entryPoint == 0) {
    uint64_t mainAddr = elfReader->getMainAddress();
    if (mainAddr != 0) {
      entryPoint = mainAddr;
      std::cout << "Entry point was 0, using main function at 0x" << std::hex
                << mainAddr << std::dec << std::endl;
    } else {
      std::cerr << "Warning: Entry point is 0 and no main symbol found"
                << std::endl;
    }
  }

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

  // Initialize guest state with entry point
  guestState.pc = entryPoint;

  // Simple execution loop - execute one basic block at a time
  uint64_t currentPC = entryPoint;
  int maxBlocks = 1000; // Increased limit for real execution
  int blockCount = 0;

  while (blockCount < maxBlocks && currentPC != 0 &&
         !shouldTerminate(currentPC)) {
    std::cout << "Executing block " << blockCount << " at PC=0x" << std::hex
              << currentPC << std::dec << std::endl;

    // Update guest state PC
    guestState.pc = currentPC;

    // Execute one basic block
    uint64_t nextPC = executeBlock(currentPC);

    if (nextPC == 0) {
      std::cout << "Execution completed or failed" << std::endl;
      break;
    }

    currentPC = nextPC;
    blockCount++;
  }

  std::cout << "DBT execution finished after " << blockCount << " blocks"
            << std::endl;
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

std::vector<uint8_t> BinaryTranslator::encodeToMachineCode(
    const std::vector<arm64::Instruction> &instructions) {
  std::vector<uint8_t> machineCode;

  for (const auto &instruction : instructions) {
    auto encoded = encoder->encodeInstruction(instruction);
    machineCode.insert(machineCode.end(), encoded.begin(), encoded.end());
  }

  return machineCode;
}

uint64_t BinaryTranslator::executeBlock(uint64_t pc) {
  // Check if PC is within text section bounds
  if (pc < textBaseAddress || pc >= textBaseAddress + textSectionData.size()) {
    std::cerr << "PC out of bounds: 0x" << std::hex << pc << std::dec
              << std::endl;
    return 0;
  }

  // Decode instructions starting from PC until we hit a terminator
  std::vector<riscv::Instruction> blockInstructions;
  size_t offset = pc - textBaseAddress;
  uint64_t currentPC = pc;

  while (offset < textSectionData.size()) {
    riscv::Instruction inst =
        decoder->decode(textSectionData.data(), offset, currentPC);
    if (!inst.isValid()) {
      std::cerr << "Invalid instruction at PC=0x" << std::hex << currentPC
                << std::dec << std::endl;
      break;
    }

    blockInstructions.push_back(inst);

    // Check if this is a terminator instruction
    if (lifter->isTerminator(inst)) {
      break;
    }

    offset += 4;
    currentPC += 4;
  }

  if (blockInstructions.empty()) {
    std::cerr << "No instructions decoded for block at PC=0x" << std::hex << pc
              << std::dec << std::endl;
    return 0;
  }

  // Lift to IR
  std::cout << "  Lifting " << blockInstructions.size() << " instructions to IR"
            << std::endl;
  ir::BasicBlock irBlock = lifter->liftBasicBlock(blockInstructions);

  // Translate to ARM64
  std::cout << "  Translating IR to ARM64" << std::endl;
  auto arm64Instructions = translateToARM64(irBlock);

  // Encode to machine code
  std::cout << "  Encoding to machine code" << std::endl;
  auto machineCode = encodeToMachineCode(arm64Instructions);

  if (machineCode.empty()) {
    std::cerr << "Failed to encode machine code" << std::endl;
    return 0;
  }

  // Execute the block
  std::cout << "  Executing " << machineCode.size() << " bytes of machine code"
            << std::endl;
  uint64_t nextPC = executionEngine->executeBlock(machineCode, &guestState);

  return nextPC;
}

int BinaryTranslator::executeFunction(const std::string &inputPath,
                                      const std::string &functionName) {
  if (!loadRISCVBinary(inputPath)) {
    return -1;
  }

  // Get function address
  uint64_t functionAddr = elfReader->getFunctionAddress(functionName);
  if (functionAddr == 0) {
    std::cerr << "Function '" << functionName << "' not found in binary"
              << std::endl;
    return -1;
  }

  std::cout << "Executing function '" << functionName << "' at address 0x"
            << std::hex << functionAddr << std::dec << std::endl;

  // Initialize guest state with function address
  guestState.pc = functionAddr;

  // Execute until completion or error
  uint64_t currentPC = functionAddr;
  int maxBlocks = 10000; // Large limit for real programs
  int blockCount = 0;

  while (blockCount < maxBlocks && currentPC != 0 &&
         !shouldTerminate(currentPC)) {
    guestState.pc = currentPC;
    uint64_t nextPC = executeBlock(currentPC);

    if (nextPC == 0) {
      break;
    }

    currentPC = nextPC;
    blockCount++;
  }

  // Return the value from register a0 (x10) - RISC-V return value register
  return static_cast<int>(getReturnValue());
}

void BinaryTranslator::setArgumentRegisters(const std::vector<uint64_t> &args) {
  // Set up RISC-V calling convention arguments in a0-a7 (x10-x17)
  for (size_t i = 0; i < args.size() && i < 8; ++i) {
    guestState.writeRegister(10 + i, args[i]);
  }
}

bool BinaryTranslator::shouldTerminate(uint64_t pc) {
  // Check if PC is out of bounds (program has exited)
  if (pc < textBaseAddress || pc >= textBaseAddress + textSectionData.size()) {
    return true;
  }

  // For now, simple bounds check. Could add other termination conditions:
  // - System calls
  // - Special exit instructions
  // - Infinite loop detection
  return false;
}

int BinaryTranslator::getReturnValue() const {
  // In RISC-V calling convention, return value is in register a0 (x10)
  return static_cast<int>(guestState.readRegister(10));
}

} // namespace dinorisc
