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

    # Test cases with (source_file, function_name, args, expected_return)
    TEST_CASES = [
        # Basic addition tests
        ("add.c", "add", [5, 10], 15),
        ("add.c", "add", [100, -50], 50),
        ("add.c", "add", [0, 0], 0),
        ("add.c", "add", [-10, -20], -30),
        # Function call tests
        ("function.c", "add_numbers", [3, 7], 10),
        ("function.c", "add_numbers", [15, 25], 40),
        ("function.c", "add_numbers", [0, 1], 1),
        # Loop tests (sum from 0 to n-1)
        ("loop.c", "sum_to_n", [10], 45),  # 0+1+2+...+9 = 45
        ("loop.c", "sum_to_n", [1], 0),  # just 0
        ("loop.c", "sum_to_n", [5], 10),  # 0+1+2+3+4 = 10
        ("loop.c", "sum_to_n", [0], 0),  # no iterations
        # Branch tests
        ("branch.c", "conditional_calc", [7, 5], 14),  # 7 > 5, so 7 * 2 = 14
        ("branch.c", "conditional_calc", [3, 5], 4),  # 3 <= 5, so 3 + 1 = 4
        ("branch.c", "conditional_calc", [5, 5], 6),  # 5 <= 5, so 5 + 1 = 6
        ("branch.c", "conditional_calc", [10, 8], 20),  # 10 > 8, so 10 * 2 = 20
        # Memory/array tests
        ("memory.c", "array_sum", [5], 15),  # 1+2+3+4+5 = 15
        ("memory.c", "array_sum", [3], 6),  # 1+2+3 = 6
        ("memory.c", "array_sum", [1], 1),  # just 1
        ("memory.c", "array_sum", [10], 55),  # 1+2+...+10 = 55
        # Stack tests
        ("stack_test.c", "stack_add", [10, 20], 30),
        ("stack_test.c", "stack_add", [0, 5], 5),
        ("stack_test.c", "stack_add", [-5, 15], 10),
        # Fibonacci tests
        ("fibonacci.c", "fibonacci", [0], 0),  # base case
        ("fibonacci.c", "fibonacci", [1], 1),  # base case
        ("fibonacci.c", "fibonacci", [2], 1),  # F(2) = 1
        ("fibonacci.c", "fibonacci", [5], 5),  # F(5) = 5
        ("fibonacci.c", "fibonacci", [10], 55),  # F(10) = 55
        ("fibonacci.c", "fibonacci", [15], 610),  # F(15) = 610
    ]

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
        "source_file,function_name,args,expected_return",
        TEST_CASES,
    )
    def test_dinorisc_execution(
        self, source_file, function_name, args, expected_return
    ):
        """Test that dinorisc correctly executes RISC-V binaries and produces expected results."""
        elf_path = None
        try:
            # Compile source to RISC-V ELF
            elf_path = self.compile_sample(source_file)

            # Run dinorisc with function name and arguments
            cmd = [str(DINORISC_BIN), elf_path, function_name] + [
                str(arg) for arg in args
            ]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

            # Check that execution succeeded
            assert (
                result.returncode == 0
            ), f"dinorisc execution failed on {source_file}:{function_name}({args}): {result.stderr}"

            # Parse the function return value from stdout
            import re

            match = re.search(r"Function \w+ returned: (-?\d+)", result.stdout)
            assert match, f"Could not parse return value from stdout: {result.stdout}"

            actual_return = int(match.group(1))
            assert (
                actual_return == expected_return
            ), f"Expected {source_file}:{function_name}({args}) to return {expected_return}, but got {actual_return}"

        finally:
            if elf_path and os.path.exists(elf_path):
                os.unlink(elf_path)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
