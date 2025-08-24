#include "BinaryTranslator.h"
#include <iostream>
#include <string>

static void printUsage(const char *programName) {
  std::cout << "Usage: " << programName << " <riscv_binary>\n";
  std::cout
      << "Executes RISC-V 64-bit binaries using dynamic binary translation\n";
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printUsage(argv[0]);
    return 1;
  }

  std::string inputPath = argv[1];

  dinorisc::BinaryTranslator translator;

  if (!translator.executeProgram(inputPath)) {
    std::cerr << "Error: Failed to execute binary\n";
    return 1;
  }

  std::cout << "Execution completed successfully\n";
  return 0;
}