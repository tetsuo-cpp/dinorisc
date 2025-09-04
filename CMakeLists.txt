cmake_minimum_required(VERSION 3.20)
project(DinoRISC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add third-party dependencies
include_directories(${CMAKE_SOURCE_DIR}/third_party/elfio)

# Testing support
option(DINORISC_ENABLE_TESTING "Enable testing" ON)
if(DINORISC_ENABLE_TESTING)
  enable_testing()
  add_subdirectory(third_party/Catch2)
endif()

# Add format target for C++ and Python files
file(GLOB_RECURSE ALL_SOURCE_FILES
  ${CMAKE_SOURCE_DIR}/lib/*.cpp
  ${CMAKE_SOURCE_DIR}/lib/*.h
  ${CMAKE_SOURCE_DIR}/tools/*.cpp
  ${CMAKE_SOURCE_DIR}/tools/*.h
  ${CMAKE_SOURCE_DIR}/tests/*.cpp
  ${CMAKE_SOURCE_DIR}/tests/*.h
  ${CMAKE_SOURCE_DIR}/tests/e2e/samples/*.c
  ${CMAKE_SOURCE_DIR}/tests/e2e/samples/*.h
)

file(GLOB_RECURSE ALL_PYTHON_FILES
  ${CMAKE_SOURCE_DIR}/tests/*.py
  ${CMAKE_SOURCE_DIR}/tools/*.py
)

add_custom_target(format
  COMMAND clang-format -i ${ALL_SOURCE_FILES}
  COMMAND black ${ALL_PYTHON_FILES}
  COMMENT "Running clang-format and black on source files"
)

# Add subdirectories
add_subdirectory(lib)
add_subdirectory(tools)

# Add tests if enabled
if(DINORISC_ENABLE_TESTING)
  add_subdirectory(tests)
endif()
