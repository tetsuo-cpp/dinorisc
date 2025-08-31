set(DINORISC_LIB_SOURCES
  BinaryTranslator.cpp
  ELFReader.cpp
  RISCV/Decoder.cpp
  RISCV/Instruction.cpp
)

set(DINORISC_LIB_HEADERS
  BinaryTranslator.h
  ELFReader.h
  RISCV/Decoder.h
  RISCV/Instruction.h
)

add_library(DinoRISCLib STATIC
  ${DINORISC_LIB_SOURCES}
)

target_include_directories(DinoRISCLib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)