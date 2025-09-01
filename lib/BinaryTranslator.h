#pragma once

#include "ELFReader.h"
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

private:
  std::unique_ptr<ELFReader> elfReader;
  std::unique_ptr<riscv::Decoder> decoder;
  std::unique_ptr<Lifter> lifter;
  std::unique_ptr<lowering::InstructionSelector> instructionSelector;
  std::unique_ptr<lowering::RegisterAllocator> registerAllocator;

  void initializeTranslator();
  bool loadRISCVBinary(const std::string &inputPath);
  bool executeWithDBT();

  // Translate an IR block to ARM64 instructions
  std::vector<arm64::Instruction>
  translateToARM64(const ir::BasicBlock &irBlock);

  // Current translation state
  std::vector<uint8_t> textSectionData;
  uint64_t textBaseAddress;
  uint64_t entryPoint;
};

} // namespace dinorisc
