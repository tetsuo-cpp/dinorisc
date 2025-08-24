#pragma once

#include <cstdint>
#include <string>

namespace dinorisc {

class BinaryTranslator {
public:
  BinaryTranslator();
  ~BinaryTranslator();

  bool translateBinary(const std::string &inputPath,
                       const std::string &outputPath);

private:
  void initializeTranslator();
  bool translateRISCVToARM64(const uint8_t *data, size_t size);
};

} // namespace dinorisc
