# DinoRISC Python API

Python bindings for DinoRISC, a RISC-V 64-bit to ARM64 dynamic binary translation tool.

## Installation

### Prerequisites

1. Build the DinoRISC shared library:
```bash
cd /path/to/dinorisc
mkdir build && cd build
cmake -G Ninja ..
ninja
```

2. Install the Python package:
```bash
cd python
pip install -e .
```

## Usage

### Basic Example

```python
import dinorisc

# Execute a function with arguments
result = dinorisc.execute_function("program.elf", "add", [5, 3])
print(f"Result: {result}")
```

### Using the Translator Class

```python
from dinorisc import Translator

# Create a translator instance
with Translator() as translator:
    result = translator.execute_function("program.elf", "fibonacci", [10])
    print(f"Fibonacci(10) = {result}")
```

### Specifying Library Path

If the shared library is not in a standard location:

```python
from dinorisc import Translator

translator = Translator(lib_path="/custom/path/libdinorisc.so")
result = translator.execute_function("program.elf", "main", [])
```

## API Reference

### `Translator`

Main class for interacting with DinoRISC.

#### Methods

- `__init__(lib_path: Optional[str] = None)`: Create a new translator instance
- `execute_function(binary_path: str, function_name: str, args: Optional[List[int]] = None) -> int`: Execute a RISC-V function
- `close()`: Release translator resources

### `execute_function`

Convenience function for one-off function execution.

```python
def execute_function(
    binary_path: str,
    function_name: str,
    args: Optional[List[int]] = None
) -> int
```

### `DinoRISCError`

Exception raised when DinoRISC operations fail.

## Requirements

- Python 3.7 or higher
- Linux or macOS (ARM64)
- DinoRISC shared library (libdinorisc.so or libdinorisc.dylib)

## License

MIT License
