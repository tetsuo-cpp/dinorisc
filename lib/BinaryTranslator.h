#pragma once

#include "ELFReader.h"
#include "RV64IDecoder.h"
#include "RV64IInstruction.h"
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
  std::unique_ptr<RV64IDecoder> decoder;

  void initializeTranslator();
  bool loadAndDecodeRISCV(const std::string &inputPath);
  bool executeWithDBT(const std::vector<RV64IInstruction> &instructions);

  // Current translation state
  std::vector<RV64IInstruction> instructions;
  uint64_t entryPoint;
};

} // namespace dinorisc
