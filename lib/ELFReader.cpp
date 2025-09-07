#include "ELFReader.h"

namespace dinorisc {

ELFReader::ELFReader() : entryPoint(0), loaded(false) {}

bool ELFReader::loadFile(const std::string &filePath) {
  loaded = false;
  errorMessage.clear();

  if (!reader.load(filePath)) {
    errorMessage = "Failed to load ELF file: " + filePath;
    return false;
  }

  // Validate it's a RISC-V 64-bit executable
  if (reader.get_class() != ELFIO::ELFCLASS64) {
    errorMessage = "Not a 64-bit ELF file";
    return false;
  }

  if (reader.get_machine() != ELFIO::EM_RISCV) {
    errorMessage = "Not a RISC-V ELF file (machine type: " +
                   std::to_string(reader.get_machine()) + ")";
    return false;
  }

  if (reader.get_type() != ELFIO::ET_EXEC) {
    errorMessage = "Not an executable ELF file";
    return false;
  }

  // Get entry point
  entryPoint = reader.get_entry();

  // Find and extract .text section
  ELFIO::section *textSec = nullptr;
  for (const auto &sec : reader.sections) {
    if (sec->get_name() == ".text") {
      textSec = sec.get();
      break;
    }
  }

  if (!textSec) {
    errorMessage = "No .text section found in ELF file";
    return false;
  }

  // Check if .text section is executable
  if (!(textSec->get_flags() & ELFIO::SHF_EXECINSTR)) {
    errorMessage = ".text section is not executable";
    return false;
  }

  // Extract .text section data
  textSection.virtualAddress = textSec->get_address();
  textSection.size = textSec->get_size();

  const char *data = textSec->get_data();
  if (!data) {
    errorMessage = "Failed to read .text section data";
    return false;
  }

  textSection.data.assign(data, data + textSection.size);

  loaded = true;
  return true;
}

uint64_t ELFReader::getMainAddress() const {
  return getFunctionAddress("main");
}

uint64_t ELFReader::getFunctionAddress(const std::string &functionName) const {
  if (!loaded) {
    return 0;
  }

  // Look for function symbol in symbol table
  for (const auto &sec : reader.sections) {
    if (sec->get_type() == ELFIO::SHT_SYMTAB ||
        sec->get_type() == ELFIO::SHT_DYNSYM) {
      ELFIO::symbol_section_accessor symbols(reader, sec.get());

      for (ELFIO::Elf_Xword i = 0; i < symbols.get_symbols_num(); ++i) {
        std::string name;
        ELFIO::Elf64_Addr value;
        ELFIO::Elf_Xword size;
        unsigned char bind, type, other;
        ELFIO::Elf_Half section;

        if (symbols.get_symbol(i, name, value, size, bind, type, section,
                               other)) {
          if (name == functionName) {
            return value;
          }
        }
      }
    }
  }

  return 0;
}

} // namespace dinorisc
