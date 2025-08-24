# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DinoRISC is a RISC-V 64-bit to ARM64 dynamic binary translation tool written in C++. The project follows LLVM naming and formatting conventions.

## Architecture

- **lib/**: Core translation library (`DinoRISCLib`) containing the main translation logic
  - `BinaryTranslator`: Main class orchestrating the dynamic binary translation process
  - `ELFReader`: Parses RISC-V ELF binaries using ELFIO library
  - `RV64IDecoder`: Decodes RV64I instructions from binary data
  - `RV64IInstruction`: Represents decoded RISC-V instructions with all formats
- **tools/dinorisc/**: Command-line executable that executes RISC-V binaries
- **third_party/**: External dependencies (ELFIO library, Catch2 testing framework)
- **tests/**: Unit and integration tests
- **build/**: Build artifacts (created by CMake/Ninja)

## Development Commands

### Building
```bash
mkdir build && cd build
cmake -G Ninja ..
ninja
```

### Code Formatting
```bash
ninja format  # Runs clang-format on all source files
```

### Testing
```bash
ninja && ctest --output-on-failure  # Run all tests
./bin/RV64IDecoderTest              # Run specific unit test
```

### Running
```bash
./bin/dinorisc <riscv_binary>
```

## Code Conventions

- Follow LLVM naming conventions:
  - Classes and functions: `CamelCase` 
  - Variables and parameters: `camelCase`
  - Use `#pragma once` instead of include guards
- C++17 standard
- Namespace: `dinorisc`
- The project uses a `.clang-format` file for consistent formatting

## Build System

- CMake with Ninja generator preferred
- Static library approach: core logic in `DinoRISCLib`, executable links against it
- Compiler flags: `-Wall -Wextra -Wno-unused-parameter`
- Build outputs go to `build/bin/` and `build/lib/`
- Testing enabled by default with `DINORISC_ENABLE_TESTING=ON`

## Git Commit Guidelines

- Do not mention Claude or add Claude co-author attribution in commits
- Write concise, descriptive commit messages focused on the changes made