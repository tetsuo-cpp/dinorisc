set(DINORISC_LIB_SOURCES
  BinaryTranslator.cpp
  ELFReader.cpp
  ExecutionEngine.cpp
  Lifter.cpp
  RISCV/Decoder.cpp
  RISCV/Instruction.cpp
  IR/IR.cpp
  ARM64/Instruction.cpp
  ARM64/Encoder.cpp
  Lowering/LivenessAnalysis.cpp
  Lowering/InstructionSelector.cpp
  Lowering/RegisterAllocator.cpp
)

set(DINORISC_LIB_HEADERS
  BinaryTranslator.h
  ELFReader.h
  ExecutionEngine.h
  GuestState.h
  Lifter.h
  RISCV/Decoder.h
  RISCV/Instruction.h
  IR/IR.h
  ARM64/Instruction.h
  ARM64/Encoder.h
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

# Build shared library with C API for Python bindings
add_library(DinoRISCShared SHARED
  ${DINORISC_LIB_SOURCES}
  CAPI.cpp
)

target_include_directories(DinoRISCShared PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(DinoRISCShared PROPERTIES
  OUTPUT_NAME dinorisc
  VERSION 1.0.0
  SOVERSION 1
)