#pragma once

#include "ELFReader.h"
#include "ExecutionEngine.h"
#include "GuestState.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace dinorisc {

namespace riscv {
class Decoder;
}
namespace arm64 {
class Encoder;
struct Instruction;
} // namespace arm64
namespace ir {
struct BasicBlock;
}

class BinaryTranslator {
public:
  BinaryTranslator();
  ~BinaryTranslator();

  int executeFunction(const std::string &inputPath,
                      const std::string &functionName);

  void setArgumentRegisters(const std::vector<uint64_t> &args);

private:
  std::unique_ptr<ELFReader> elfReader;
  std::unique_ptr<riscv::Decoder> decoder;
  std::unique_ptr<arm64::Encoder> encoder;
  std::unique_ptr<ExecutionEngine> executionEngine;
  GuestState guestState;

  void initializeTranslator();
  void loadRISCVBinary(const std::string &inputPath);
  std::vector<arm64::Instruction>
  translateToARM64(const ir::BasicBlock &irBlock);
  uint64_t executeBlock(uint64_t pc);
  bool isValidPC(uint64_t pc) const;
  int getReturnValue() const;

  std::vector<uint8_t> textSectionData;
  uint64_t textBaseAddress;
};

} // namespace dinorisc
