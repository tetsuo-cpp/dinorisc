#include "CAPI.h"
#include "BinaryTranslator.h"
#include <cstring>
#include <exception>
#include <string>

struct DinoRISCTranslator {
  dinorisc::BinaryTranslator translator;
  std::string lastError;
};

DinoRISCTranslator *dinorisc_create() {
  try {
    return new DinoRISCTranslator();
  } catch (const std::exception &e) {
    return nullptr;
  }
}

void dinorisc_destroy(DinoRISCTranslator *translator) { delete translator; }

void dinorisc_set_arguments(DinoRISCTranslator *translator,
                            const uint64_t *args, size_t num_args) {
  if (!translator || !args)
    return;

  try {
    std::vector<uint64_t> argVec(args, args + num_args);
    translator->translator.setArgumentRegisters(argVec);
    translator->lastError.clear();
  } catch (const std::exception &e) {
    translator->lastError = e.what();
  }
}

int dinorisc_execute_function(DinoRISCTranslator *translator,
                              const char *binary_path,
                              const char *function_name) {
  if (!translator || !binary_path || !function_name) {
    if (translator)
      translator->lastError = "Invalid parameters";
    return -1;
  }

  try {
    translator->lastError.clear();
    int result = translator->translator.executeFunction(
        std::string(binary_path), std::string(function_name));
    if (result == -1) {
      translator->lastError = "Failed to load or execute function";
    }
    return result;
  } catch (const std::exception &e) {
    translator->lastError = e.what();
    return -1;
  }
}

const char *dinorisc_get_last_error(DinoRISCTranslator *translator) {
  if (!translator || translator->lastError.empty())
    return nullptr;
  return translator->lastError.c_str();
}
