#include "BinaryTranslator.h"
#include "Error.h"
#include <iostream>
#include <string>

static void printUsage(const char *programName) {
  std::cout << "Usage: " << programName
            << " <riscv_binary> <function_name> [arg1] [arg2] ...\n";
  std::cout
      << "Executes RISC-V 64-bit binaries using dynamic binary translation\n";
  std::cout
      << "Arguments are passed to the function as integer parameters (max 8)\n";
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printUsage(argv[0]);
    return 1;
  }

  std::string inputPath = argv[1];
  std::string functionName = argv[2];

  // Parse function arguments (starting from argv[3])
  std::vector<uint64_t> functionArgs;
  for (int i = 3; i < argc && functionArgs.size() < 8; ++i) {
    try {
      uint64_t arg = std::stoull(argv[i]);
      functionArgs.push_back(arg);
    } catch (const std::exception &e) {
      std::cerr << "Error: Invalid argument '" << argv[i]
                << "'. Arguments must be integers.\n";
      return 1;
    }
  }

  try {
    dinorisc::BinaryTranslator translator;

    translator.setArgumentRegisters(functionArgs);

    int functionResult = translator.executeFunction(inputPath, functionName);

    std::cout << "Function " << functionName << " returned: " << functionResult
              << "\n";
    return 0;
  } catch (const dinorisc::Error &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
