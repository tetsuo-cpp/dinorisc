#include "BinaryTranslator.h"
#include <iostream>
#include <string>

static void printUsage(const char *programName) {
  std::cout << "Usage: " << programName << " <riscv_binary> <function_name>\n";
  std::cout
      << "Executes RISC-V 64-bit binaries using dynamic binary translation\n";
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printUsage(argv[0]);
    return 1;
  }

  std::string inputPath = argv[1];
  std::string functionName = argv[2];

  dinorisc::BinaryTranslator translator;

  // Execute specific function and return its result
  int exitCode = translator.executeFunction(inputPath, functionName);
  if (exitCode == -1) {
    std::cerr << "Error: Failed to load or execute function " << functionName
              << "\n";
    return 1;
  }

  std::cout << "Function " << functionName << " returned: " << exitCode << "\n";
  return exitCode;
}