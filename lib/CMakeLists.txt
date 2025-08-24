set(DINORISC_LIB_SOURCES
  BinaryTranslator.cpp
  ELFReader.cpp
  RV64IDecoder.cpp
  RV64IInstruction.cpp
)

set(DINORISC_LIB_HEADERS
  BinaryTranslator.h
  ELFReader.h
  RV64IDecoder.h
  RV64IInstruction.h
)

add_library(DinoRISCLib STATIC
  ${DINORISC_LIB_SOURCES}
)

target_include_directories(DinoRISCLib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)