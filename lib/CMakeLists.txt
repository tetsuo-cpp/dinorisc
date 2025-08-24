set(DINORISC_LIB_SOURCES
  BinaryTranslator.cpp
)

set(DINORISC_LIB_HEADERS
  BinaryTranslator.h
)

add_library(DinoRISCLib STATIC
  ${DINORISC_LIB_SOURCES}
)

target_include_directories(DinoRISCLib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)