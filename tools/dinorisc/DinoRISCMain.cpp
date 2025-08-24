#include "BinaryTranslator.h"
#include <iostream>
#include <string>

static void printUsage(const char *programName) {
  std::cout << "Usage: " << programName << " <input_binary> <output_binary>\n";
  std::cout << "Translates RISC-V 64-bit binaries to ARM64\n";
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printUsage(argv[0]);
    return 1;
  }

  std::string inputPath = argv[1];
  std::string outputPath = argv[2];

  dinorisc::BinaryTranslator translator;

  if (!translator.translateBinary(inputPath, outputPath)) {
    std::cerr << "Error: Failed to translate binary\n";
    return 1;
  }

  std::cout << "Translation completed successfully\n";
  return 0;
}