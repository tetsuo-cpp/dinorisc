#pragma once

#include <cstdint>
#include <elfio/elfio.hpp>
#include <string>
#include <vector>

namespace dinorisc {

struct TextSection {
  uint64_t virtualAddress;
  uint64_t size;
  std::vector<uint8_t> data;
};

class ELFReader {
public:
  ELFReader();

  bool loadFile(const std::string &filePath);

  uint64_t getEntryPoint() const { return entryPoint; }
  const TextSection &getTextSection() const { return textSection; }

  std::string getErrorMessage() const { return errorMessage; }

private:
  ELFIO::elfio reader;
  uint64_t entryPoint;
  TextSection textSection;
  std::string errorMessage;
  bool loaded;
};

} // namespace dinorisc