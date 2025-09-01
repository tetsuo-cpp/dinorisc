set(DINORISC_LIB_SOURCES
  BinaryTranslator.cpp
  ELFReader.cpp
  Lifter.cpp
  RISCV/Decoder.cpp
  RISCV/Instruction.cpp
  IR/IR.cpp
  ARM64/Instruction.cpp
  Lowering/LivenessAnalysis.cpp
  Lowering/InstructionSelector.cpp
  Lowering/RegisterAllocator.cpp
)

set(DINORISC_LIB_HEADERS
  BinaryTranslator.h
  ELFReader.h
  Lifter.h
  RISCV/Decoder.h
  RISCV/Instruction.h
  IR/IR.h
  ARM64/Instruction.h
  Lowering/LivenessAnalysis.h
  Lowering/InstructionSelector.h
  Lowering/RegisterAllocator.h
)

add_library(DinoRISCLib STATIC
  ${DINORISC_LIB_SOURCES}
)

target_include_directories(DinoRISCLib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)