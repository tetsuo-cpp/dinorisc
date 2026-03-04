#include "ELFReader.h"

namespace dinorisc {

ELFReader::ELFReader() : entryPoint(0) {}

void ELFReader::loadFile(const std::string &filePath) {
  if (!reader.load(filePath)) {
    throw ELFError("Failed to load ELF file: " + filePath);
  }

  // Validate it's a RISC-V 64-bit executable
  if (reader.get_class() != ELFIO::ELFCLASS64) {
    throw ELFError("Not a 64-bit ELF file");
  }

  if (reader.get_machine() != ELFIO::EM_RISCV) {
    throw ELFError("Not a RISC-V ELF file (machine type: " +
                   std::to_string(reader.get_machine()) + ")");
  }

  if (reader.get_type() != ELFIO::ET_EXEC) {
    throw ELFError("Not an executable ELF file");
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
    throw ELFError("No .text section found in ELF file");
  }

  // Check if .text section is executable
  if (!(textSec->get_flags() & ELFIO::SHF_EXECINSTR)) {
    throw ELFError(".text section is not executable");
  }

  // Extract .text section data
  textSection.virtualAddress = textSec->get_address();

  const char *data = textSec->get_data();
  if (!data) {
    throw ELFError("Failed to read .text section data");
  }

  textSection.data.assign(data, data + textSec->get_size());
}

std::optional<uint64_t>
ELFReader::getFunctionAddress(const std::string &functionName) const {
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

  return std::nullopt;
}

} // namespace dinorisc
