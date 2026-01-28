# RADS

Rapid Asynchronous Data Server - A high-performance, low-level programming language.

## Current Version: 0.0.7 "DARK MOON"

## The RADS Philosophy

**One binary. Infinite possibilities.**

RADS is distributed as a single, self-contained binary that can compile and run RADS programs anywhere. The installer uses bash and line-based command processing, making it work seamlessly across different Linux distributions and versions.

## Quick Start

### Automatic Installation (Recommended)

```bash
# Download the latest release (Linux x86_64)
wget https://github.com/zarigata/RADS/releases/download/v0.0.7/rads-v0.0.7-linux-x86_64.tar.gz

# Extract
tar -xzf rads-v0.0.7-linux-x86_64.tar.gz

# Run installer
cd rads-v0.0.7
./install.sh

# Verify installation
rads --version
rstar help
rpm help

# Visit Package Registry
# The Package Registry will be available at: https://zarigata.github.io/RADS
```

The `install.sh` script handles everything automatically - dependencies, compilation, and system-wide installation.

## Features

### Core Language (v0.0.7 Dark Moon)

- **Types**: int, float, string, bool, array, struct, enum, function, pointer
- **Control Flow**: if/else, for, while, switch, break, continue, return
- **Functions**: Named and anonymous functions, multiple return values, default parameters
- **Structs**: Custom data types with member access
- **Enums**: Type-safe enumerations
- **Arrays**: Dynamic arrays with push, pop, length operations

### Bytecode VM

- Stack-based virtual machine
- 3-5x performance improvement over interpreter
- Disassembler for debugging
- Bytecode validation and safety checks

### Profiling

- Sampling-based profiler
- Flamegraph generation
- Hotspot identification and call tree analysis
- Real-time statistics display

### Debugger

- DAP support with breakpoints
- Variable inspection
- Step controls
- Call stack inspection

### Networking & Web

- WebSocket server with 128+ concurrent connections
- GraphQL server with queries, mutations, subscriptions
- HTTP client and built-in server capabilities

### Database

- SQLite3 integration
- SQL query execution interface
- Parameterized queries for security
- Connection pooling and management

### Tools

- **rstar**: Package manager for installing and managing plugins
- **rpm**: Resource package manager for bundling assets
- **rads-mask**: Python transpiler

### Development Tools

- **LSP Server**: Rust-based language server
- Code completion
- Go to definition
- Refactoring

## Example Programs

### Hello World

\`\`\`rads
blast main() {
    echo("Hello, RADS World!");
}
\`\`\`

### Variables and Types

\`\`\`rads
blast main() {
    // Integer
    turbo int age = 25;
    
    // Float
    turbo float price = 19.99;
    
    // String
    str name = "RADS";
    
    // Boolean
    turbo bool active = true;
    
    // Array
    array<int> numbers = [1, 2, 3, 4, 5];
    
    // Struct
    struct Point {
        int x;
        int y;
    }
    turbo Point origin = { x: 0, y: 0 };
    
    echo("Age: " + age);
    echo("Price: $" + price);
    echo("Name: " + name);
    echo("Active: " + active);
    echo("Count: " + length(numbers));
    echo("Origin: " + origin.x + ", " + origin.y);
}
\`\`\`

### Control Flow

\`\`\`rads
blast main() {
    turbo int x = 10;
    
    // If-else
    if (x > 5) {
        echo("x is greater than 5");
    } else {
        echo("x is 5 or less");
    }
    
    // For loop
    for (int i = 0; i < 5; i = i + 1) {
        echo("Count: " + i);
    }
    
    // While loop
    turbo int count = 0;
    while (count < 3) {
        echo("While count: " + count);
        count = count + 1;
    }
    
    // Switch statement
    enum Day {
        Monday,
        Tuesday,
        Wednesday,
        Thursday,
        Friday
    }
    turbo Day today = Day.Wednesday;
    
    switch (today) {
        case Monday:
            echo("It's Monday");
            break;
        case Wednesday:
            echo("It's Wednesday");
            break;
        default:
            echo("It's another day");
    }
}
\`\`\`

### Functions

\`\`\`rads
blast int add(int a, int b) {
    return a + b;
}

blast int multiply(int x, int y) {
    return x * y;
}

blast main() {
    turbo int result = add(5, 3);
    echo("5 + 3 = " + result);
    
    // Anonymous function with default parameter
    blast greet(str name = "World") {
        echo("Hello, " + name + "!");
    }
    
    greet("RADS");
    greet("Programmer");
}
\`\`\`

### HTTP Server

\`\`\`rads
import web;

blast main() {
    web_init();
    
    // Start HTTP server on port 8080
    web_serve("0.0.0", 8080);
    
    echo("Server started on http://localhost:8080");
    echo("Press Ctrl+C to stop");
}
\`\`\`

### WebSocket Server

\`\`\`rads
import websocket;

blast void handle_message(str msg) {
    echo("Received: " + msg);
}

blast main() {
    websocket_start("0.0.0", 8080, handle_message);
    echo("WebSocket server running on port 8080");
}
\`\`\`

### Database Operations

\`\`\`rads
import db;

blast main() {
    db_init(":memory:");
    
    // Create table
    db_exec("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT, email TEXT);");
    
    // Insert data
    db_exec("INSERT INTO users (name, email) VALUES ('Alice', 'alice@example.com');");
    
    // Query data
    db_result result = db_query("SELECT * FROM users;");
    
    while (db_next_row(result)) {
        echo("User: " + db_get_string(result, 1));
    }
    
    db_close();
}
\`\`\`

### Testing

\`\`\`rads
blast main() {
    test("Addition works", 2 + 2 == 4);
    test("String comparison", "hello" == "hello");
    test("Array length", length([1,2,3]) == 3);
    
    echo("All tests passed!");
}
\`\`\`

### Asynchronous Operations

\`\`\`rads
import async;

blast void callback(int result) {
    echo("Operation completed. Result: " + result);
}

blast main() {
    async_exec(callback);
    echo("Async operation started...");
}
\`\`\`

## REPL Usage

Start the interactive REPL:

\`\`\`bash
rads
\`\`\`

REPL Commands:
- `.help` - Show available commands
- `.exit` or `.quit` - Exit REPL
- `.bytecode` - Compile to bytecode mode
- `.disassemble` - Show bytecode disassembly
- `.profile` - Show execution statistics
- Type RADS code directly to execute

REPL Example:
\`\`\`rads
rads[1]> turbo int x = 10
rads[2]> echo("Value: " + x)
rads[3]> .exit
\`\`\`

## Package Management

### Install a Package

\`\`\`bash
# Using rstar
rstar install package-name

# List installed packages
rstar list

# Get package info
rstar info package-name

# Search packages
rstar search database
\`\`\`

### Create a Package

1. Create package directory:
\`\`\`bash
mkdir my-rads-package
cd my-rads-package
\`\`\`

2. Create package manifest (\`rads.json\`):
\`\`\`json
{
  "name": "my-package",
  "version": "1.0.0",
  "description": "A useful RADS package",
  "main": "src/main.rads",
  "keywords": ["rads", "library"],
  "author": "Your Name",
  "license": "MIT",
  "rads_version": "0.0.7"
}
\`\`\`

3. Add your RADS source files:
\`\`\`
my-rads-package/
├── rads.json
└── src/
    └── main.rads
\`\`\`

4. Publish your package:
\`\`\`bash
rstar publish
\`\`\`

## API Reference

### Standard Library Functions

#### String Operations
\`\`\`rads
str concat(str a, str b)           // Concatenate strings
str len(str s)                    // Get string length
str substring(str s, int start, int end)  // Extract substring
str to_upper(str s)              // Convert to uppercase
str to_lower(str s)              // Convert to lowercase
str trim(str s)                   // Remove whitespace
\`\`\`

#### Math Functions
\`\`\`rads
float math_abs(float x)           // Absolute value
float math_floor(float x)         // Round down
float math_ceil(float x)          // Round up
float math_sqrt(float x)          // Square root
float math_pow(float base, float exp)  // Power
float math_sin(float x)            // Sine
float math_cos(float x)            // Cosine
\`\`\`

#### Array Functions
\`\`\`rads
int length(array arr)           // Get array length
int push(array arr, value)     // Add element to end
int pop(array arr)              // Remove and return last element
array sort(array arr)           // Sort array
array reverse(array arr)          // Reverse array
\`\`\`

#### I/O Functions
\`\`\`rads
void echo(str message)           // Print to stdout
void error(str message)          // Print to stderr
void input(str prompt)           // Read from stdin
int read_file(str path)         // Read file contents
void write_file(str path, str content)  // Write file
\`\`\`

#### Async Functions
\`\`\`rads
void async_exec(void (*callback)(int))  // Execute function asynchronously
void async_wait(int timeout_ms)     // Wait for async operations
\`\`\`

#### Network Functions
\`\`\`rads
void http_request(str url)        // Make HTTP request
void ws_send(str message)         // Send WebSocket message
void ws_connect(str url)          // Connect to WebSocket server
\`\`\`

#### Database Functions
\`\`\`rads
void db_init(str path)            // Initialize database
void db_exec(str sql)             // Execute SQL
db_result db_query(str sql)        // Query database
bool db_next_row(db_result res)     // Move to next row
str db_get_string(db_result res, int col)  // Get string value
void db_close()                     // Close database
\`\`\`

### Compiler Options

\`\`\`bash
# Basic compilation
rads program.rads

# Compile to bytecode
rads --bytecode program.rads

# Enable optimization
rads -O2 program.rads

# Output assembly
rads --asm program.rads

# Run tests during compilation
rads --test program.rads
\`\`\`

### Environment Variables

\`\`\`bash
RADS_PATH=/usr/local/lib/rads     # Additional library path
RADS_LOG_LEVEL=info            # Logging level (debug, info, warn, error)
RADS_ASYNC_THREADS=4           # Number of async threads
\`\`\`

### Project Structure

\`\`\`
Rads/
├── bin/                    # Compiled tools
│   ├── rads             # Main compiler
│   ├── rstar           # Package manager
│   ├── rpm             # Resource package manager
│   └── rads-mask       # Python transpiler
├── src/
│   ├── core/            # Compiler components (lexer, parser, AST, bytecode, interpreter, profiler, debugger)
│   ├── stdlib/          # Standard library (strings, arrays, math, I/O, async, network, DB, etc.)
│   └── vm/             # Bytecode VM
├── tests/                # Test suites
├── examples/             # Example programs
├── docs/                # Documentation
└── tools/                # Additional tools (LSP server, etc.)
\`\`\`

### Build from Source

\`\`\`bash
# Prerequisites
- GCC or Clang compiler
- Make
- libuv development libraries

# On Debian/Ubuntu:
\`\`\`bash
sudo apt-get install build-essential libuv1-dev
\`\`\`

# On Fedora/RHEL:
\`\`\`bash
sudo dnf install gcc make libuv-devel
\`\`\`

# On macOS:
\`\`\`bash
brew install libuv
\`\`\`

## Project Structure

\`\`\`
Rads/
├── bin/                    # Compiled tools
├── src/
│   ├── core/            # Compiler components
│   ├── stdlib/          # Standard library
│   └── vm/             # Bytecode VM
├── tests/                # Test suites
├── examples/             # Example programs
└── docs/                # Documentation
\`\`\`

### Troubleshooting

### Build Issues

**Problem**: \`readline/readline.h\` No such file or directory

**Solution**: Install readline development library
\`\`\`bash
# Debian/Ubuntu
sudo apt-get install libreadline-dev

# Fedora/RHEL
sudo dnf install readline-devel

# macOS
brew install readline
\`\`\`

### Runtime Issues

**Problem**: "command not found" error

**Solution**: Ensure RADS is in your PATH: \`export PATH=$PATH:/usr/local/bin\`

### Import Errors

**Problem**: Cannot find module declarations

**Solution**: Check that source files exist and that imports match file structure. Run \`make clean && make\` after fixing import issues.

## Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

- Code should follow existing style
- Add tests for new features
- Update documentation
- Keep changes minimal and focused

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Acknowledgments

- libuv for excellent async I/O
- GNU readline for command-line editing
- SQLite for embedded database
- LLVM for compilation infrastructure
- The RADS community
