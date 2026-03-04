#include "BinaryTranslator.h"
#include "ARM64/Encoder.h"
#include "Lifter.h"
#include "Lowering/InstructionSelector.h"
#include "Lowering/LivenessAnalysis.h"
#include "Lowering/RegisterAllocator.h"
#include "RISCV/Decoder.h"
#include "RISCV/Instruction.h"
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>
#include <unistd.h>

namespace dinorisc {

static constexpr size_t SHADOW_MEMORY_SIZE = 8 * 1024 * 1024; // 8MB
static constexpr size_t STACK_RESERVE = 1024;                 // 1KB
static constexpr int MAX_EXECUTION_BLOCKS = 10000;

BinaryTranslator::BinaryTranslator() : textBaseAddress(0) {
  initializeTranslator();
}

BinaryTranslator::~BinaryTranslator() = default;

void BinaryTranslator::initializeTranslator() {
  elfReader = std::make_unique<ELFReader>();
  decoder = std::make_unique<riscv::Decoder>();
  encoder = std::make_unique<arm64::Encoder>();
  executionEngine = std::make_unique<ExecutionEngine>();

  void *shadowMem = mmap(nullptr, SHADOW_MEMORY_SIZE, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (shadowMem == MAP_FAILED) {
    throw std::runtime_error("Failed to allocate shadow memory");
  }

  guestState.shadowMemory = shadowMem;
  guestState.shadowMemorySize = SHADOW_MEMORY_SIZE;
  guestState.guestMemoryBase = 0x0;

  // Stack grows down from high address within shadow memory
  uint64_t stackTop = SHADOW_MEMORY_SIZE - STACK_RESERVE;
  guestState.x[2] = stackTop;

  std::cout << "Shadow memory allocated: " << SHADOW_MEMORY_SIZE
            << " bytes at host " << shadowMem << ", guest base 0x" << std::hex
            << guestState.guestMemoryBase << ", stack at 0x" << stackTop
            << std::dec << std::endl;
}

bool BinaryTranslator::loadRISCVBinary(const std::string &inputPath) {
  if (!elfReader->loadFile(inputPath)) {
    std::cerr << "Error loading ELF file: " << elfReader->getErrorMessage()
              << std::endl;
    return false;
  }

  const auto &textSection = elfReader->getTextSection();

  textSectionData = textSection.data;
  textBaseAddress = textSection.virtualAddress;

  return true;
}

std::vector<arm64::Instruction>
BinaryTranslator::translateToARM64(const ir::BasicBlock &irBlock) {
  std::cout << "  Starting ARM64 translation for IR block..." << std::endl;

  std::cout << "    Step 1: Instruction selection (IR -> ARM64)" << std::endl;
  lowering::InstructionSelector instructionSelector;
  auto arm64Instructions = instructionSelector.selectInstructions(irBlock);
  std::cout << "      Generated " << arm64Instructions.size()
            << " ARM64 instructions" << std::endl;

  std::cout << "    Step 2: Liveness analysis" << std::endl;
  lowering::LivenessAnalysis liveness(arm64Instructions);
  auto liveIntervals = liveness.computeLiveIntervals();
  std::cout << "      Computed " << liveIntervals.size() << " live intervals"
            << std::endl;

  std::cout << "    Step 3: Linear scan register allocation" << std::endl;
  lowering::RegisterAllocator registerAllocator;
  if (!registerAllocator.allocateRegisters(arm64Instructions, liveIntervals)) {
    std::cout << "      WARNING: Register allocation failed - using "
                 "placeholder registers"
              << std::endl;
  } else {
    std::cout << "      Register allocation successful" << std::endl;
  }

  std::cout << "    Final ARM64 instructions:" << std::endl;
  for (size_t i = 0; i < arm64Instructions.size(); ++i) {
    std::cout << "      [" << i << "] " << arm64Instructions[i].toString()
              << std::endl;
  }

  return arm64Instructions;
}

uint64_t BinaryTranslator::executeBlock(uint64_t pc) {
  if (!isValidPC(pc)) {
    std::cerr << "PC out of bounds: 0x" << std::hex << pc << std::dec
              << std::endl;
    return 0;
  }

  std::vector<riscv::Instruction> blockInstructions;
  size_t offset = pc - textBaseAddress;
  uint64_t currentPC = pc;
  Lifter lifter;

  while (offset < textSectionData.size()) {
    riscv::Instruction inst =
        decoder->decode(textSectionData.data(), offset, currentPC);
    if (!inst.isValid()) {
      std::cerr << "Invalid instruction at PC=0x" << std::hex << currentPC
                << std::dec << std::endl;
      break;
    }

    blockInstructions.push_back(inst);

    if (lifter.isTerminator(inst)) {
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

  std::cout << "  Lifting " << blockInstructions.size() << " instructions to IR"
            << std::endl;
  ir::BasicBlock irBlock = lifter.liftBasicBlock(blockInstructions);

  std::cout << "  Translating IR to ARM64" << std::endl;
  auto arm64Instructions = translateToARM64(irBlock);

  // Encode to machine code
  std::cout << "  Encoding to machine code" << std::endl;
  std::vector<uint8_t> machineCode;
  for (const auto &instruction : arm64Instructions) {
    auto encoded = encoder->encodeInstruction(instruction);
    machineCode.insert(machineCode.end(), encoded.begin(), encoded.end());
  }

  if (machineCode.empty()) {
    std::cerr << "Failed to encode machine code" << std::endl;
    return 0;
  }

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

  std::cout << "Text section: VA=0x" << std::hex << textBaseAddress
            << " Size=" << std::dec << textSectionData.size() << " bytes"
            << std::endl;

  auto functionAddr = elfReader->getFunctionAddress(functionName);
  if (!functionAddr.has_value()) {
    std::cerr << "Function '" << functionName << "' not found in binary"
              << std::endl;
    return -1;
  }

  guestState.pc = functionAddr.value();
  guestState.x[1] = 0;

  uint64_t currentPC = functionAddr.value();
  int maxBlocks = MAX_EXECUTION_BLOCKS;
  int blockCount = 0;

  while (blockCount < maxBlocks && currentPC != 0) {
    guestState.pc = currentPC;
    uint64_t nextPC = executeBlock(currentPC);

    if (nextPC == 0) {
      break;
    }

    currentPC = nextPC;
    blockCount++;
  }

  return static_cast<int>(getReturnValue());
}

void BinaryTranslator::setArgumentRegisters(const std::vector<uint64_t> &args) {
  for (size_t i = 0; i < args.size() && i < 8; ++i) {
    guestState.writeRegister(10 + i, args[i]);
  }
}

bool BinaryTranslator::isValidPC(uint64_t pc) const {
  return pc >= textBaseAddress && pc < textBaseAddress + textSectionData.size();
}

int BinaryTranslator::getReturnValue() const {
  return static_cast<int>(guestState.x[10]);
}

} // namespace dinorisc
