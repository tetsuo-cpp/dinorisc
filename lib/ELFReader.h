#pragma once

#include "Error.h"
#include <cstdint>
#include <elfio/elfio.hpp>
#include <optional>
#include <string>
#include <vector>

namespace dinorisc {

struct TextSection {
  uint64_t virtualAddress;
  std::vector<uint8_t> data;
};

class ELFReader {
public:
  ELFReader();

  void loadFile(const std::string &filePath);

  uint64_t getEntryPoint() const { return entryPoint; }
  const TextSection &getTextSection() const { return textSection; }

  // Get address of any function symbol by name
  std::optional<uint64_t>
  getFunctionAddress(const std::string &functionName) const;

private:
  ELFIO::elfio reader;
  uint64_t entryPoint;
  TextSection textSection;
};

} // namespace dinorisc