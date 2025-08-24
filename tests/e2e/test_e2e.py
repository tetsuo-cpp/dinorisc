#!/usr/bin/env python3
"""
End-to-end tests for DinoRISC RISC-V binary translation tool.

This test suite compiles C programs to RISC-V ELF binaries using clang
and then feeds them to dinorisc to verify instruction decoding.
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


class TestDinoRISCDecoding:
    """Test that dinorisc can decode RISC-V ELF binaries."""

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
        "source_file",
        [
            "add.c",
            "loop.c",
            "branch.c",
            "function.c",
            "memory.c",
        ],
    )
    def test_dinorisc_decoding(self, source_file):
        """Test that dinorisc can successfully process compiled RISC-V binaries."""
        elf_path = None
        try:
            # Compile source to RISC-V ELF
            elf_path = self.compile_sample(source_file)

            # Run dinorisc on the ELF file
            cmd = [str(DINORISC_BIN), elf_path]
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)

            # dinorisc should run successfully (exit code 0)
            assert (
                result.returncode == 0
            ), f"dinorisc failed on {source_file}: {result.stderr}"

        finally:
            if elf_path and os.path.exists(elf_path):
                os.unlink(elf_path)


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
