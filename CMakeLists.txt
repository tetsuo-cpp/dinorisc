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

# Find clang-format
find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)

# Add format target if clang-format is found
if(CLANG_FORMAT_EXECUTABLE)
  file(GLOB_RECURSE ALL_SOURCE_FILES
    ${CMAKE_SOURCE_DIR}/lib/*.cpp
    ${CMAKE_SOURCE_DIR}/lib/*.h
    ${CMAKE_SOURCE_DIR}/tools/*.cpp
    ${CMAKE_SOURCE_DIR}/tools/*.h
  )

  add_custom_target(format
    COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${ALL_SOURCE_FILES}
    COMMENT "Running clang-format on all source files"
  )
endif()

# Add subdirectories
add_subdirectory(lib)
add_subdirectory(tools)
