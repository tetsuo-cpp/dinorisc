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

# Create test executable for Lowering Pipeline
add_executable(LoweringPipelineTest
  LoweringPipelineTest.cpp
)

# Link with DinoRISCLib and Catch2
target_link_libraries(LoweringPipelineTest
  PRIVATE
  DinoRISCLib
  Catch2::Catch2WithMain
)

# Add the test to CTest
add_test(NAME LoweringPipelineUnitTest COMMAND LoweringPipelineTest)

# Create test executable for ARM64 Encoder
add_executable(EncoderTest
  EncoderTest.cpp
)

# Link with DinoRISCLib and Catch2
target_link_libraries(EncoderTest
  PRIVATE
  DinoRISCLib
  Catch2::Catch2WithMain
)

# Add the test to CTest
add_test(NAME EncoderUnitTest COMMAND EncoderTest)
