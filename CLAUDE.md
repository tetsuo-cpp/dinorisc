# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

DinoRISC is a RISC-V 64-bit to ARM64 dynamic binary translation tool written in C++. The project follows LLVM naming and formatting conventions.

## Architecture

- **lib/**: Core translation library (`DinoRISCLib`) containing the main translation logic
  - `BinaryTranslator`: Main class orchestrating the dynamic binary translation process
  - `ELFReader`: Parses RISC-V ELF binaries using ELFIO library
  - `Lifter`: Converts RISC-V instructions to SSA intermediate representation (IR)
  - `RISCV/Decoder`: Decodes RISC-V instructions from binary data
  - `RISCV/Instruction`: Represents decoded RISC-V instructions with all formats
  - `IR/`: Block-local SSA intermediate representation for translation pipeline
  - `ARM64/Instruction`: ARM64 instruction representations with opcodes and operands
  - `ARM64/Encoder`: Generates ARM64 machine code from instruction objects
  - `ExecutionEngine`: Runtime execution infrastructure with dispatcher-based control flow
  - `Lowering/`: Instruction selection and register allocation for ARM64 code generation
    - `InstructionSelector`: Translates SSA IR to ARM64 instructions with virtual registers
    - `LivenessAnalysis`: Computes live intervals for virtual registers within basic blocks
    - `RegisterAllocator`: Linear scan register allocation algorithm
  - `CAPI.h/CAPI.cpp`: C API for language bindings (used by Python package)
- **tools/dinorisc/**: Command-line executable that executes RISC-V binaries
- **python/**: Python package for invoking DinoRISC
  - `dinorisc/`: Python module with high-level API and ctypes bindings
  - `tests/`: Python API test suite using pytest
  - `setup.py`: Package installation script
- **third_party/**: External dependencies (ELFIO library, Catch2 testing framework)
- **tests/**: Unit and end-to-end tests
  - `unit/`: Unit tests using Catch2 framework
  - `e2e/`: End-to-end tests using pytest, includes RISC-V C sample programs
- **build/**: Build artifacts (created by CMake/Ninja)

## Development Commands

### Building
```bash
mkdir build && cd build
cmake -G Ninja ..
ninja                    # Build static library and executables
ninja DinoRISCShared     # Build shared library for Python bindings
```

### Code Formatting
```bash
ninja format  # Runs clang-format on C++ files and black on Python files
```

### Testing
```bash
ninja && ctest --output-on-failure  # Run all tests
./bin/DecoderTest                   # Run specific unit test
./bin/EncoderTest                   # Run ARM64 encoder unit tests
./bin/ExecutionEngineTest           # Run execution engine unit tests
./bin/LoweringPipelineTest          # Run lowering pipeline unit tests
ninja test-e2e                      # Run end-to-end tests only
```

**Note**: E2E tests require:
- `clang` with RISC-V target support in PATH
- `pytest` installed in the active Python environment

### Running
```bash
./bin/dinorisc <riscv_binary> <function_name> [args...]
```

### Python API

Install the Python package:
```bash
cd python
pip install -e .
```

Use the Python API:
```python
import dinorisc

# Execute a function with arguments
result = dinorisc.execute_function("program.elf", "add", [5, 3])

# Or use the Translator class
with dinorisc.Translator() as translator:
    result = translator.execute_function("program.elf", "fibonacci", [10])
```

Run Python API tests:
```bash
cd python
pytest tests/
```

**Note**: Python API requires the shared library (`libdinorisc.so`) built via `ninja DinoRISCShared`

## Code Conventions

- Follow LLVM naming conventions:
  - Classes and functions: `CamelCase` 
  - Variables and parameters: `camelCase`
  - Use `#pragma once` instead of include guards
- C++17 standard
- Namespace: `dinorisc`
- The project uses a `.clang-format` file for consistent formatting

## Coding Style Principles

- **Write concise, idiomatic code**: Favor clear, direct solutions over verbose implementations
- **Avoid over-engineering**: Implement only what is needed; don't build for hypothetical future requirements
- **Avoid overly defensive code**: Don't add unnecessary checks for conditions that shouldn't occur in normal operation
- **Use comments judiciously**: Add comments sparingly and only when they provide value
- **Focus comments on intent**: Explain *why* the code exists and *what* it's trying to achieve, not *how* it works (the code should be self-explanatory)
- **Prefer clear data structures**: Use either classes with meaningful methods or clean POD structs with aggregate initialization; avoid mixing tiny helper methods into data-only structs

## Build System

- CMake with Ninja generator preferred
- Two library targets:
  - `DinoRISCLib`: Static library for C++ executables
  - `DinoRISCShared`: Shared library (`libdinorisc.so`) for Python bindings with C API
- Compiler flags: `-Wall -Wextra -Wno-unused-parameter`
- Build outputs go to `build/bin/` and `build/lib/`
- Testing enabled by default with `DINORISC_ENABLE_TESTING=ON`
- Python dependencies:
  - For e2e tests: `pytest`, `black` (see `tests/e2e/requirements.txt`)
  - For Python API: `pytest` (see `python/requirements-dev.txt`)

## Git Commit Guidelines

- Do not mention Claude or add Claude co-author attribution in commits
- Write concise, descriptive commit messages focused on the changes made