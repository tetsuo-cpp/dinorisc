import ctypes
import os
import platform
from typing import Optional


class DinoRISCBindings:
    """Low-level ctypes bindings for the DinoRISC C API."""

    def __init__(self, lib_path: Optional[str] = None):
        if lib_path is None:
            lib_path = self._find_library()
        self._lib = ctypes.CDLL(lib_path)
        self._setup_functions()

    def _find_library(self) -> str:
        """Locate the DinoRISC shared library."""
        system = platform.system()
        if system == "Linux":
            lib_name = "libdinorisc.so"
        elif system == "Darwin":
            lib_name = "libdinorisc.dylib"
        else:
            raise RuntimeError(f"Unsupported platform: {system}")

        # Search in common locations
        search_paths = [
            os.path.join(os.path.dirname(__file__), "..", "..", "build", "lib"),
            os.path.join(os.path.dirname(__file__), "lib"),
            "/usr/local/lib",
            "/usr/lib",
        ]

        for path in search_paths:
            full_path = os.path.join(path, lib_name)
            if os.path.exists(full_path):
                return full_path

        raise RuntimeError(
            f"Could not find {lib_name}. "
            "Please build DinoRISC or set LD_LIBRARY_PATH"
        )

    def _setup_functions(self):
        """Configure ctypes function signatures."""
        # dinorisc_create
        self._lib.dinorisc_create.argtypes = []
        self._lib.dinorisc_create.restype = ctypes.c_void_p

        # dinorisc_destroy
        self._lib.dinorisc_destroy.argtypes = [ctypes.c_void_p]
        self._lib.dinorisc_destroy.restype = None

        # dinorisc_set_arguments
        self._lib.dinorisc_set_arguments.argtypes = [
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_uint64),
            ctypes.c_size_t,
        ]
        self._lib.dinorisc_set_arguments.restype = None

        # dinorisc_execute_function
        self._lib.dinorisc_execute_function.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]
        self._lib.dinorisc_execute_function.restype = ctypes.c_int

        # dinorisc_get_last_error
        self._lib.dinorisc_get_last_error.argtypes = [ctypes.c_void_p]
        self._lib.dinorisc_get_last_error.restype = ctypes.c_char_p

    def create(self) -> int:
        """Create a new translator instance."""
        return self._lib.dinorisc_create()

    def destroy(self, translator: int):
        """Destroy a translator instance."""
        self._lib.dinorisc_destroy(translator)

    def set_arguments(self, translator: int, args: list):
        """Set function arguments."""
        if not args:
            return
        args_array = (ctypes.c_uint64 * len(args))(*args)
        self._lib.dinorisc_set_arguments(translator, args_array, len(args))

    def execute_function(self, translator: int, binary_path: str, function_name: str) -> int:
        """Execute a function from a RISC-V binary."""
        return self._lib.dinorisc_execute_function(
            translator,
            binary_path.encode("utf-8"),
            function_name.encode("utf-8"),
        )

    def get_last_error(self, translator: int) -> Optional[str]:
        """Get the last error message."""
        err = self._lib.dinorisc_get_last_error(translator)
        if err:
            return err.decode("utf-8")
        return None
