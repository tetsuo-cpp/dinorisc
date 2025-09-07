#!/usr/bin/env python3
"""
End-to-end tests for DinoRISC RISC-V binary translation tool.

This test suite compiles C programs to RISC-V ELF binaries using clang
and then feeds them to dinorisc to verify correct execution and results.
"""

import os
import subprocess
import tempfile
import pytest
from pathlib import Path

# Configuration
LLVM_CLANG = "clang"  # Use clang from PATH
PROJECT_ROOT = Path(__file__).parent.parent.parent
DINORISC_BIN = PROJECT_ROOT / "build" / "bin" / "dinorisc"
SAMPLES_DIR = Path(__file__).parent / "samples"

# RISC-V compilation flags for RV64I only
RISCV_CFLAGS = [
    "-target",
    "riscv64-unknown-elf",
    "-march=rv64i",
    "-mabi=lp64",
    "-nostdlib",
    "-ffreestanding",
    "-static",
    "-Wl,-Ttext=0x10000",  # Set text section base address
    "-Wl,--no-relax",  # Disable linker relaxations
]


class TestDinoRISCExecution:
    """Test that dinorisc can execute RISC-V ELF binaries and produce correct results."""

    # Test cases with expected return values
    TEST_CASES = {
        "add.c": 15,  # 5 + 10 = 15
        "function.c": 10,  # 3 + 7 = 10
        "loop.c": 45,  # sum of 0-9 = 45
        "branch.c": 14,  # 7 * 2 = 14 (since 7 > 5)
        "memory.c": 15,  # 1+2+3+4+5 = 15
        "stack_test.c": 30,  # 10 + 20 = 30
    }

    def setup_method(self):
        """Ensure dinorisc binary exists."""
        if not DINORISC_BIN.exists():
            pytest.skip(
                f"dinorisc binary not found at {DINORISC_BIN}. Run 'ninja' to build."
            )

    def compile_sample(self, source_file):
        """Compile a sample C file to RISC-V ELF and return the path."""
        source_path = SAMPLES_DIR / source_file
        tmp_fd, output_path = tempfile.mkstemp(suffix=".elf")
        os.close(tmp_fd)

        cmd = [LLVM_CLANG] + RISCV_CFLAGS + [str(source_path), "-o", output_path]
        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            os.unlink(output_path)
            pytest.fail(f"Failed to compile {source_file}: {result.stderr}")

        return output_path

    @pytest.mark.parametrize(
        "source_file,expected_return",
        list(TEST_CASES.items()),
    )
    def test_dinorisc_execution(self, source_file, expected_return):
        """Test that dinorisc correctly executes RISC-V binaries and produces expected results."""
        elf_path = None
        try:
            # Compile source to RISC-V ELF
            elf_path = self.compile_sample(source_file)

            # Run dinorisc with main function to get actual return value
            cmd = [str(DINORISC_BIN), elf_path, "main"]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

            # Check that execution succeeded
            assert (
                result.returncode != 1
            ), f"dinorisc execution failed on {source_file}: {result.stderr}"

            # The return code should be the program's return value
            actual_return = result.returncode
            assert (
                actual_return == expected_return
            ), f"Expected {source_file} to return {expected_return}, but got {actual_return}"

        finally:
            if elf_path and os.path.exists(elf_path):
                os.unlink(elf_path)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
