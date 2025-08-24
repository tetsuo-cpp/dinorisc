set(DINORISC_SOURCES
  DinoRISCMain.cpp
)

add_executable(dinorisc
  ${DINORISC_SOURCES}
)

target_link_libraries(dinorisc
  DinoRISCLib
)
