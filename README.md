# 🚀 RADS v0.0.7 "DARK MOON"

---

## ⚡ ONE COMMAND INSTALLATION

\`\`\`bash
curl -fsSL https://install.rads-lang.org | bash
\`\`\`

That is it! RADS downloads, builds itself, and installs automatically.

**Why This Works Everywhere:**
- ✅ Detects your Linux distribution automatically
- ✅ Installs all dependencies (libuv, readline, sqlite3, gcc, make)
- ✅ Builds from source with optimal settings
- ✅ Installs to /usr/local/bin for system-wide access
- ✅ Works on Debian, Ubuntu, Fedora, RHEL, CentOS, Arch, Alpine, and more

**After Installation:**
\`\`\`bash
# Verify it works
rads --version

# Start coding
rads
\`\`\`

---

## 🎯 WHAT IS RADS?

RADS (Rapid Asynchronous Data Server) is a blazing fast C-based programming language that combines raw C power with modern async capabilities.

**Perfect for:**
- 🖥️ Microservers & IoT (Raspberry Pi, embedded systems)
- ⚡ High-speed data processing (video, audio, image at CPU speeds)
- 🛠️ Reliable tooling (F2A file servers with precision)
- 🎨 Retro-futuristic aesthetics (Cyberpunk, Frutiger Aero)

**Language Power:**
\`\`\`rads
// Lightning fast with bytecode VM (3-5x speedup)
blast main() {
    turbo int x = 42;
    echo("x * 2 = " + (x * 2));  // JIT compiled!
}
\`\`\`

---

## ✨ v0.0.7 "DARK MOON" FEATURES

### 🛩️ Core Engine
- **Bytecode VM** - Stack-based virtual machine with 3-5x performance boost
- **Disassembler** - View compiled bytecode for debugging
- **Sampling Profiler** - Real-time profiling with flamegraph output
- **Hotspot Analysis** - Automatic performance bottleneck detection

### 🐛 Advanced Debugger
- **DAP Support** - Debug Adapter Protocol for VS Code, Neovim, etc.
- **Conditional Breakpoints** - Break when expressions are true
- **Variable Watch** - Monitor variables in real-time
- **Call Stack Inspection** - Navigate through function calls
- **Step Controls** - Step-over, step-into, step-out commands

### 🌐 Networking & Web
- **WebSocket Server** - Real-time bidirectional communication (128+ concurrent connections)
- **GraphQL Server** - Queries, mutations, subscriptions with JSON responses
- **HTTP Client** - Built-in HTTP request handling
- **HTTP Server** - Create web servers with minimal code

### 📊 Database Integration
- **SQLite3 Support** - Full database with transactions
- **Parameterized Queries** - Secure query building
- **Connection Pooling** - Efficient connection management

### 🎮 Development Tools
- **LSP Server** - Rust-based language server for code completion, go-to-definition, refactoring
- **RGB REPL** - Gaming keyboard aesthetic with rainbow cursor effects
- **GNU Readline** - Command history with arrow key navigation
- **Beautiful Errors** - Color-coded error messages with helpful suggestions

### 🛠️ Package Ecosystem
- **rstar** - Package manager for installing and managing RADS packages
- **rpm** - Resource package manager for bundling assets
- **rads-mask** - Python to RADS transpiler (95%+ accuracy)
- **Registry** - Full npmjs.com-style package registry at https://rads.rads-lang.org

### 🧪 Type System
- **Types**: int, float, string, bool, array, struct, enum, function, pointer
- **Control Flow**: if/else, for, while, switch, break, continue, return
- **Functions**: Named and anonymous, multiple return values, default parameters
- **Structs**: Custom data types with member access
- **Enums**: Type-safe enumerations
- **Arrays**: Dynamic arrays with push, pop, length operations

---

## 📚 START CODING NOW

### Your First Program

\`\`\`rads
blast main() {
    echo("Hello, RADS World! 🚀");
}
\`\`\`

Save as hello.rads and run:
\`\`\`bash
rads hello.rads
\`\`\`

### Interactive REPL

\`\`\`bash
rads
\`\`\`

REPL Commands:
- Type RADS code directly and press Enter to execute
- .help - Show all commands
- .exit or .quit - Leave REPL
- Use arrow keys for command history
- Press Tab for auto-completion

### Example: Variables & Types

\`\`\`rads
blast main() {
    // Numbers
    turbo int age = 25;
    
    // Float
    turbo float price = 19.99;
    
    // String
    str name = "RADS";
    
    // Boolean
    turbo bool active = true;
    
    // Array
    array<int> numbers = [1, 2, 3, 4, 5];
    
    // Enum
    enum Status {
        Pending,
        InProgress,
        Completed
    }
    
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

### Example: Control Flow

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

### Example: Functions

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
    greet("Developer");
}
\`\`\`

### Example: HTTP Server

\`\`\`rads
import web;

blast main() {
    web_init();
    
    // Start HTTP server on port 8080
    web_serve("0.0.0.0", 8080);
    
    echo("Server started on http://localhost:8080");
    echo("Press Ctrl+C to stop");
}
\`\`\`

### Example: WebSocket Server

\`\`\`rads
import websocket;

blast void handle_message(str msg) {
    echo("Received: " + msg);
}

blast main() {
    websocket_start("0.0.0.0", 8080, handle_message);
    echo("WebSocket server running on port 8080");
}
\`\`\`

### Example: Database Operations

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

### Example: Async Operations

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

---

## 🛠️ API REFERENCE

### String Operations

\`\`\`rads
str concat(str a, str b)           // Concatenate strings
str len(str s)                    // Get string length
str substring(str s, int start, int end)  // Extract substring
str to_upper(str s)              // Convert to uppercase
str to_lower(str s)              // Convert to lowercase
str trim(str s)                   // Remove whitespace
\`\`\`

### Math Functions

\`\`\`rads
float math_abs(float x)           // Absolute value
float math_floor(float x)         // Round down
float math_ceil(float x)          // Round up
float math_sqrt(float x)          // Square root
float math_pow(float base, float exp)  // Power
float math_sin(float x)            // Sine
float math_cos(float x)            // Cosine
\`\`\`

### Array Functions

\`\`\`rads
int length(array arr)           // Get array length
int push(array arr, value)     // Add element to end
int pop(array arr)              // Remove and return last element
array sort(array arr)           // Sort array
array reverse(array arr)          // Reverse array
\`\`\`

### I/O Functions

\`\`\`rads
void echo(str message)           // Print to stdout
void error(str message)          // Print to stderr
void input(str prompt)           // Read from stdin
int read_file(str path)         // Read file contents
void write_file(str path, str content)  // Write file
\`\`\`

### Async Functions

\`\`\`rads
void async_exec(void (*callback)(int))  // Execute function asynchronously
void async_wait(int timeout_ms)     // Wait for async operations
\`\`\`

### Network Functions

\`\`\`rads
void http_request(str url)        // Make HTTP request
void ws_send(str message)         // Send WebSocket message
void ws_connect(str url)          // Connect to WebSocket server
\`\`\`

### Database Functions

\`\`\`rads
void db_init(str path)            // Initialize database
void db_exec(str sql)             // Execute SQL
db_result db_query(str sql)        // Query database
bool db_next_row(db_result res)     // Move to next row
str db_get_string(db_result res, int col)  // Get string value
void db_close()                     // Close database
\`\`\`

---

## 📦 PACKAGE MANAGEMENT

### Install a Package

\`\`\`bash
rstar install package-name
\`\`\`

### List Packages

\`\`\`bash
rstar list
\`\`\`

### Search Packages

\`\`\`bash
rstar search database
rstar search graphql
\`\`\`

### Package Info

\`\`\`bash
rstar info package-name
\`\`\`

### Create Your Own Package

\`\`\`bash
# 1. Create package directory
mkdir my-rads-package
cd my-rads-package

# 2. Create package manifest
cat > rads.json << 'EOF'
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
EOF

# 3. Add your RADS files
mkdir src
cat > src/main.rads << 'EOF'
blast main() {
    echo("Hello from my package!");
}
EOF

# 4. Publish
rstar publish
\`\`\`

---

## 🔧 COMPILER OPTIONS

### Basic Compilation

\`\`\`bash
# Compile and run
rads program.rads

# Compile to bytecode
rads --bytecode program.rads

# Enable optimization
rads -O2 program.rads

# Output assembly
rads --asm program.rads
\`\`\`

### Environment Variables

\`\`\`bash
RADS_PATH=/usr/local/lib/rads      # Additional library path
RADS_LOG_LEVEL=debug              # Logging level
RADS_ASYNC_THREADS=4              # Number of async threads
\`\`\`

---

## 📂 PROJECT STRUCTURE

\`\`\`
Rads/
├── bin/                    # Compiled tools
│   ├── rads             # Main compiler
│   ├── rstar           # Package manager
│   ├── rpm             # Resource manager
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

---

## 🔍 TROUBLESHOOTING

### Build Issues

**Problem**: readline/readline.h: No such file or directory

**Solution**:
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

**Solution**:
\`\`\`bash
# Ensure RADS is in your PATH
export PATH=$PATH:/usr/local/bin

# Verify installation
which rads
rads --version
\`\`\`

### Performance Issues

**Problem**: Program running slowly

**Solution**:
\`\`\`bash
# Enable optimization
rads -O2 program.rads

# Use bytecode mode
rads --bytecode program.rads

# Profile with built-in profiler
rads --profile program.rads
\`\`\`

---

## 📚 LEARN MORE

- **Package Registry**: https://rads.rads-lang.org - Browse and install RADS packages
- **Documentation**: https://docs.rads-lang.org - Complete API reference and tutorials
- **GitHub Repository**: https://github.com/zarigata/RADS - Source code, issues, discussions

---

## 🤝 CONTRIBUTING

We welcome contributions! See CONTRIBUTING.md for guidelines.

- Code should follow existing style
- Add tests for new features
- Update documentation
- Keep changes minimal and focused

---

## 📜 LICENSE

MIT License - see LICENSE file for details.

---

## 🙏 ACKNOWLEDGMENTS

- libuv for excellent async I/O
- GNU readline for command-line editing
- SQLite for embedded database
- LLVM for compilation infrastructure
- The RADS community

---

**One command to rule them all. Start coding today!** 🚀
