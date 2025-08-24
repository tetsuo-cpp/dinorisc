# Test configuration for DinoRISC
cmake_minimum_required(VERSION 3.20)

# Add unit tests
add_subdirectory(unit)

# Add end-to-end tests
add_subdirectory(e2e)
