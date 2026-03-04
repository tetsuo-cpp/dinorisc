# DinoRISC

RISC-V 64-bit to ARM64 dynamic binary translator.

## Overview

DinoRISC translates and executes RV64I ELF binaries on ARM64 hardware. It reads a RISC-V ELF executable, decodes instructions into a block-local SSA intermediate representation, lowers through instruction selection and register allocation to ARM64 machine code, and executes it natively via a block-based dispatch loop.

## Dependencies

- **C++17** compiler (Clang or GCC)
- **CMake** >= 3.20
- [ELFIO](https://github.com/serge1/ELFIO) and [Catch2](https://github.com/catchorg/Catch2) are submodules under `third_party/`

For end-to-end tests:

- `clang` with RISC-V target support
- Python 3 with `pytest`

## Building

```bash
cmake -G Ninja -B build
ninja -C build
```

## Usage

```
dinorisc <riscv_binary> <function_name> [arg1] [arg2] ...
```

Executes a named function from a RISC-V ELF binary. Up to 8 integer arguments can be passed and are mapped to registers `a0`–`a7`. The function's return value (from `a0`) is printed to stdout.

```bash
./build/bin/dinorisc program.elf main
./build/bin/dinorisc math.elf add 3 5
```

## Architecture

The translation pipeline processes one basic block at a time:

| Stage | Description |
|---|---|
| **ELF Reader** | Parses RV64 ELF binaries (ELFIO), extracts `.text` section and symbol table |
| **Decoder** | Decodes 32-bit RISC-V instructions, extracting opcodes, registers, and immediates |
| **Lifter** | Converts decoded instructions into a block-local SSA intermediate representation |
| **Instruction Selector** | Translates IR operations to ARM64 instructions with virtual registers |
| **Liveness Analysis** | Computes live intervals for virtual registers within each block |
| **Register Allocator** | Linear scan allocation over ARM64 general-purpose registers (X1–X28) |
| **Encoder** | Emits raw ARM64 machine code bytes from instruction objects |
| **Execution Engine** | Maps code into executable memory (`mmap`/`mprotect`), dispatches blocks in a loop |

Guest state is maintained in a `GuestState` struct (32 integer registers + PC) with an 8 MB shadow memory region for loads and stores.

## Supported Instructions

The following RV64I instruction categories are supported:

- **Arithmetic** — `ADD`, `ADDI`, `ADDW`, `ADDIW`, `SUB`
- **Bitwise** — `AND`, `ANDI`, `OR`, `ORI`, `XOR`, `XORI`
- **Shifts** — `SLL`, `SLLI`, `SLLIW`, `SRL`, `SRLI`, `SRA`, `SRAI`
- **Comparisons** — `SLT`, `SLTU`, `SLTI`, `SLTIU`
- **Loads** — `LB`, `LH`, `LW`, `LWU`, `LD`
- **Stores** — `SW`, `SD`
- **Branches** — `BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU`
- **Jumps** — `JAL`, `JALR` (including `RET` recognition)
- **Upper immediate** — `LUI`, `AUIPC`

## Compiling RISC-V Binaries

DinoRISC only supports the base RV64I integer instruction set — no M (multiply/divide), A (atomics), F/D (floating point), or C (compressed) extensions. Binaries must be statically linked, freestanding ELF executables. Linker relaxations must be disabled since they can rewrite instructions into forms we don't handle.

```bash
clang \
  -target riscv64-unknown-elf \
  -march=rv64i \
  -mabi=lp64 \
  -nostdlib \
  -ffreestanding \
  -static \
  -Wl,-Ttext=0x10000 \
  -Wl,--no-relax \
  -o program.elf program.c
```

| Flag | Why |
|---|---|
| `-target riscv64-unknown-elf` | Bare-metal RV64 ELF target (no OS runtime) |
| `-march=rv64i` | Base integer ISA only — no extensions |
| `-mabi=lp64` | LP64 soft-float ABI (no floating-point registers) |
| `-nostdlib -ffreestanding` | No standard library or C runtime; we execute individual functions directly |
| `-static` | Statically linked — no dynamic loader |
| `-Wl,-Ttext=0x10000` | Place `.text` at a known base address |
| `-Wl,--no-relax` | Disable linker relaxations (e.g. converting `call` sequences to `jal`) |

See `tests/e2e/samples/` for example C programs that work with DinoRISC.

## Testing

```bash
# all tests
ninja -C build && ctest --test-dir build --output-on-failure

# run a specific test
ctest --test-dir build -R DecoderUnitTest --output-on-failure

# end-to-end tests only
ctest --test-dir build -R E2ETests --output-on-failure
```

E2E tests require a `clang` with RISC-V target support on your `PATH`. On macOS with Homebrew, the default Apple Clang doesn't include it — use the LLVM formula instead:

```bash
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```
