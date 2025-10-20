import os
import pytest
import sys

# Add the parent directory to path to import dinorisc
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))

import dinorisc


def get_test_binary_path(filename):
    """Get the path to a compiled test binary."""
    test_dir = os.path.join(
        os.path.dirname(__file__), "..", "..", "build", "tests", "e2e", "binaries"
    )
    return os.path.join(test_dir, filename)


class TestTranslator:
    """Test suite for the Translator class."""

    def test_create_translator(self):
        """Test creating and closing a translator."""
        translator = dinorisc.Translator()
        assert translator is not None
        translator.close()

    def test_context_manager(self):
        """Test using translator as context manager."""
        with dinorisc.Translator() as translator:
            assert translator is not None

    def test_execute_add_function(self):
        """Test executing the add function."""
        binary_path = get_test_binary_path("add")
        if not os.path.exists(binary_path):
            pytest.skip("Test binary not found. Run 'ninja' in build directory.")

        with dinorisc.Translator() as translator:
            result = translator.execute_function(binary_path, "add", [5, 3])
            assert result == 8

    def test_execute_fibonacci(self):
        """Test executing the fibonacci function."""
        binary_path = get_test_binary_path("fibonacci")
        if not os.path.exists(binary_path):
            pytest.skip("Test binary not found. Run 'ninja' in build directory.")

        with dinorisc.Translator() as translator:
            result = translator.execute_function(binary_path, "fibonacci", [10])
            assert result == 55

    def test_execute_no_args(self):
        """Test executing a function with no arguments."""
        binary_path = get_test_binary_path("add")
        if not os.path.exists(binary_path):
            pytest.skip("Test binary not found. Run 'ninja' in build directory.")

        with dinorisc.Translator() as translator:
            # Should work even if the function expects args
            result = translator.execute_function(binary_path, "add")
            assert isinstance(result, int)

    def test_invalid_binary_path(self):
        """Test error handling for invalid binary path."""
        with dinorisc.Translator() as translator:
            with pytest.raises(dinorisc.DinoRISCError):
                translator.execute_function("/nonexistent/binary", "main")

    def test_invalid_function_name(self):
        """Test error handling for invalid function name."""
        binary_path = get_test_binary_path("add")
        if not os.path.exists(binary_path):
            pytest.skip("Test binary not found. Run 'ninja' in build directory.")

        with dinorisc.Translator() as translator:
            with pytest.raises(dinorisc.DinoRISCError):
                translator.execute_function(binary_path, "nonexistent_function")

    def test_too_many_arguments(self):
        """Test error handling for too many arguments."""
        binary_path = get_test_binary_path("add")
        if not os.path.exists(binary_path):
            pytest.skip("Test binary not found. Run 'ninja' in build directory.")

        with dinorisc.Translator() as translator:
            with pytest.raises(ValueError):
                translator.execute_function(
                    binary_path, "add", [1, 2, 3, 4, 5, 6, 7, 8, 9]
                )

    def test_closed_translator(self):
        """Test error handling when using closed translator."""
        translator = dinorisc.Translator()
        translator.close()

        with pytest.raises(dinorisc.DinoRISCError):
            translator.execute_function("test", "main")


class TestConvenienceFunction:
    """Test suite for the convenience function."""

    def test_execute_function(self):
        """Test the execute_function convenience function."""
        binary_path = get_test_binary_path("add")
        if not os.path.exists(binary_path):
            pytest.skip("Test binary not found. Run 'ninja' in build directory.")

        result = dinorisc.execute_function(binary_path, "add", [10, 20])
        assert result == 30
