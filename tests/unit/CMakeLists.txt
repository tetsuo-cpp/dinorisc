# Unit tests for DinoRISC components

# Create test executable for RV64IDecoder
add_executable(RV64IDecoderTest
  RV64IDecoderTest.cpp
)

# Link with DinoRISCLib and Catch2
target_link_libraries(RV64IDecoderTest 
  PRIVATE 
  DinoRISCLib
  Catch2::Catch2WithMain
)

# Add the test to CTest
add_test(NAME RV64IDecoderUnitTest COMMAND RV64IDecoderTest)