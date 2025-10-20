"""
DinoRISC Python API

A Python package for executing RISC-V binaries using dynamic binary translation.
"""

from typing import List, Optional

from .bindings import DinoRISCBindings

__version__ = "1.0.0"


class DinoRISCError(Exception):
    """Exception raised for DinoRISC errors."""

    pass


class Translator:
    """
    High-level interface to the DinoRISC binary translator.

    Example:
        >>> translator = Translator()
        >>> result = translator.execute_function("program.elf", "add", [5, 3])
        >>> print(result)
        8
    """

    def __init__(self, lib_path: Optional[str] = None):
        """
        Initialize a new translator instance.

        Args:
            lib_path: Optional path to the DinoRISC shared library.
                     If not provided, the library will be searched automatically.
        """
        self._bindings = DinoRISCBindings(lib_path)
        self._handle = self._bindings.create()
        if not self._handle:
            raise DinoRISCError("Failed to create translator instance")

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def close(self):
        """Release translator resources."""
        if self._handle:
            self._bindings.destroy(self._handle)
            self._handle = None

    def execute_function(
        self, binary_path: str, function_name: str, args: Optional[List[int]] = None
    ) -> int:
        """
        Execute a specific function from a RISC-V binary.

        Args:
            binary_path: Path to the RISC-V ELF binary.
            function_name: Name of the function to execute.
            args: Optional list of integer arguments (up to 8).

        Returns:
            The integer return value from the function.

        Raises:
            DinoRISCError: If execution fails or an error occurs.
        """
        if not self._handle:
            raise DinoRISCError("Translator has been closed")

        if args:
            if len(args) > 8:
                raise ValueError("Maximum of 8 arguments supported")
            self._bindings.set_arguments(self._handle, args)

        result = self._bindings.execute_function(
            self._handle, binary_path, function_name
        )

        if result == -1:
            error = self._bindings.get_last_error(self._handle)
            raise DinoRISCError(error or "Unknown error during execution")

        return result

    def __del__(self):
        self.close()


def execute_function(
    binary_path: str, function_name: str, args: Optional[List[int]] = None
) -> int:
    """
    Convenience function to execute a RISC-V function without managing the translator.

    Args:
        binary_path: Path to the RISC-V ELF binary.
        function_name: Name of the function to execute.
        args: Optional list of integer arguments (up to 8).

    Returns:
        The integer return value from the function.

    Raises:
        DinoRISCError: If execution fails or an error occurs.
    """
    with Translator() as translator:
        return translator.execute_function(binary_path, function_name, args)


__all__ = ["Translator", "DinoRISCError", "execute_function"]
