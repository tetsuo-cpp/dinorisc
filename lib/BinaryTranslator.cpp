#include "BinaryTranslator.h"
#include <iomanip>
#include <iostream>

namespace dinorisc {

BinaryTranslator::BinaryTranslator() : entryPoint(0) { initializeTranslator(); }

BinaryTranslator::~BinaryTranslator() = default;

bool BinaryTranslator::executeProgram(const std::string &inputPath) {
  std::cout << "Loading and executing RISC-V binary: " << inputPath
            << std::endl;

  // Step 1: Load and decode RISC-V ELF binary
  if (!loadAndDecodeRISCV(inputPath)) {
    return false;
  }

  std::cout << "Loaded " << instructions.size() << " RISC-V instructions"
            << std::endl;
  std::cout << "Entry point: 0x" << std::hex << entryPoint << std::dec
            << std::endl;

  // Print first few instructions for verification
  std::cout << "\nFirst few instructions:" << std::endl;
  for (size_t i = 0; i < std::min(instructions.size(), size_t(10)); ++i) {
    std::cout << "[" << i << "] " << instructions[i].toString() << std::endl;
  }

  // Step 2: Start dynamic binary translation and execution
  if (!executeWithDBT(instructions)) {
    return false;
  }

  return true;
}

void BinaryTranslator::initializeTranslator() {
  elfReader = std::make_unique<ELFReader>();
  decoder = std::make_unique<RV64IDecoder>();
}

bool BinaryTranslator::loadAndDecodeRISCV(const std::string &inputPath) {
  // Load ELF file
  if (!elfReader->loadFile(inputPath)) {
    std::cerr << "Error loading ELF file: " << elfReader->getErrorMessage()
              << std::endl;
    return false;
  }

  if (!elfReader->isValidRISCV64()) {
    std::cerr << "Error: Not a valid RISC-V 64-bit executable" << std::endl;
    return false;
  }

  // Get entry point and text section
  entryPoint = elfReader->getEntryPoint();
  const auto &textSection = elfReader->getTextSection();

  std::cout << "Text section: VA=0x" << std::hex << textSection.virtualAddress
            << " Size=" << std::dec << textSection.size << " bytes"
            << std::endl;

  // Decode instructions
  instructions =
      decoder->decodeInstructions(textSection.data, textSection.virtualAddress);

  // Print decoding statistics
  std::cout << "Decoded " << decoder->getTotalInstructionsDecoded()
            << " instructions" << std::endl;
  if (decoder->getInvalidInstructionsCount() > 0) {
    std::cout << "Warning: " << decoder->getInvalidInstructionsCount()
              << " invalid instructions found" << std::endl;
  }

  return true;
}

bool BinaryTranslator::executeWithDBT(
    const std::vector<RV64IInstruction> &instructions) {
  std::cout << "\nStarting dynamic binary translation and execution..."
            << std::endl;
  std::cout << "DBT engine not yet implemented" << std::endl;

  // TODO: Implement the following:
  // 1. Create a virtual CPU state (registers, memory)
  // 2. Set up translation cache for ARM64 code blocks
  // 3. Implement basic block detection and translation
  // 4. Execute translated ARM64 code with runtime support
  // 5. Handle system calls, exceptions, and memory management

  return true;
}

} // namespace dinorisc