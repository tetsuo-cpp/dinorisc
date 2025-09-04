# Unit tests for DinoRISC components

# Create test executable for RISC-V Decoder
add_executable(DecoderTest
  DecoderTest.cpp
)

# Link with DinoRISCLib and Catch2
target_link_libraries(DecoderTest 
  PRIVATE 
  DinoRISCLib
  Catch2::Catch2WithMain
)

# Add the test to CTest
add_test(NAME DecoderUnitTest COMMAND DecoderTest)

# Create test executable for Lifter
add_executable(LifterTest
  LifterTest.cpp
)

# Link with DinoRISCLib and Catch2
target_link_libraries(LifterTest 
  PRIVATE 
  DinoRISCLib
  Catch2::Catch2WithMain
)

# Add the test to CTest
add_test(NAME LifterUnitTest COMMAND LifterTest)