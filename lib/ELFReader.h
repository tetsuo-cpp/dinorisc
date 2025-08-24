#pragma once

#include <cstdint>
#include <memory>
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
  ~ELFReader();

  bool loadFile(const std::string &filePath);
  bool isValidRISCV64() const;

  uint64_t getEntryPoint() const { return entryPoint; }
  const TextSection &getTextSection() const { return textSection; }

  std::string getErrorMessage() const { return errorMessage; }

private:
  class ELFReaderImpl;
  std::unique_ptr<ELFReaderImpl> impl;

  uint64_t entryPoint;
  TextSection textSection;
  std::string errorMessage;
  bool loaded;
};

} // namespace dinorisc