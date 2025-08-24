#include "ELFReader.h"
#include <elfio/elfio.hpp>
#include <iostream>

namespace dinorisc {

class ELFReader::ELFReaderImpl {
public:
  ELFIO::elfio reader;
};

ELFReader::ELFReader()
    : impl(std::make_unique<ELFReaderImpl>()), entryPoint(0), loaded(false) {}

ELFReader::~ELFReader() = default;

bool ELFReader::loadFile(const std::string &filePath) {
  loaded = false;
  errorMessage.clear();

  if (!impl->reader.load(filePath)) {
    errorMessage = "Failed to load ELF file: " + filePath;
    return false;
  }

  // Validate it's a RISC-V 64-bit executable
  if (impl->reader.get_class() != ELFIO::ELFCLASS64) {
    errorMessage = "Not a 64-bit ELF file";
    return false;
  }

  if (impl->reader.get_machine() != ELFIO::EM_RISCV) {
    errorMessage = "Not a RISC-V ELF file (machine type: " +
                   std::to_string(impl->reader.get_machine()) + ")";
    return false;
  }

  if (impl->reader.get_type() != ELFIO::ET_EXEC) {
    errorMessage = "Not an executable ELF file";
    return false;
  }

  // Get entry point
  entryPoint = impl->reader.get_entry();

  // Find and extract .text section
  ELFIO::section *textSec = nullptr;
  ELFIO::Elf_Half secNum = impl->reader.sections.size();

  for (ELFIO::Elf_Half i = 0; i < secNum; ++i) {
    ELFIO::section *sec = impl->reader.sections[i];
    if (sec->get_name() == ".text") {
      textSec = sec;
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

  // Validate section size is multiple of 4 (RISC-V instructions are 32-bit)
  if (textSection.size % 4 != 0) {
    errorMessage = ".text section size is not a multiple of 4 bytes";
    return false;
  }

  loaded = true;
  return true;
}

bool ELFReader::isValidRISCV64() const {
  return loaded && impl->reader.get_class() == ELFIO::ELFCLASS64 &&
         impl->reader.get_machine() == ELFIO::EM_RISCV &&
         impl->reader.get_type() == ELFIO::ET_EXEC;
}

} // namespace dinorisc