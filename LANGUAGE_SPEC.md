# RADS Language Specification v0.1

**R**apid **A**synchronous **D**ata **S**erver Language

*"Blast through data at turbo speed! 🚀"*

## Philosophy

RADS is designed for the modern retro enthusiast who wants:
- **Performance**: Near-C speed with low-level memory control
- **Simplicity**: Clean syntax that's easy to learn
- **Power**: Built-in async I/O, networking, and media processing
- **Extensibility**: Plugin system (RPM) for unlimited capabilities
- **Aesthetic**: 90s/2000s internet culture vibes (Frutiger Aero, nu-metal energy)

## Syntax Overview

### Hello World

```rads
blast main() {
    echo("Hello, RADS World! 🌐");
}
```

### Data Types

```rads
// Primitive types
turbo i32 speed = 9000;           // 32-bit integer (turbo = fast/optimized)
turbo i64 bignum = 123456789;     // 64-bit integer
turbo f32 pi = 3.14;              // 32-bit float
turbo f64 precise = 3.14159265;   // 64-bit float
turbo bool radical = true;        // boolean
turbo char letter = 'X';          // character

// Strings
str message = "Welcome to the zone!";

// Arrays
array<i32> numbers = [1, 2, 3, 4, 5];
array<str> names = ["Alice", "Bob", "Charlie"];

// Dynamic arrays (growable)
dynarray<i32> scores = dynarray();
scores.push(100);
scores.push(200);
```

### Keywords

**Control Flow**:
- `blast` - function declaration (like "def" or "fn")
- `loop` - while loop
- `cruise` - for loop (cruising through iterations)
- `if` / `else` / `elif` - conditionals
- `switch` / `case` / `default` - switch statements
- `break` / `continue` - loop control
- `return` - return from function

**Types & Memory**:
- `turbo` - optimized/fast variable (stack-allocated when possible)
- `str` - string type
- `array` - fixed-size array
- `dynarray` - dynamic array
- `struct` - structure definition
- `enum` - enumeration
- `null` - null value
- `alloc` - manual memory allocation
- `free` - manual memory deallocation

**Async & Concurrency**:
- `async` - async function
- `await` - await async operation
- `spawn` - spawn new thread/coroutine
- `stream` - data stream (for async I/O)

**Modules & Imports**:
- `import` - import module
- `export` - export symbol
- `plugin` - load plugin

**Special**:
- `echo` - print to stdout
- `xtreme` - compiler hint for maximum optimization (unsafe)

### Functions

```rads
// Basic function
blast greet(str name) -> str {
    return "Hello, " + name + "!";
}

// Async function
async blast fetchData(str url) -> str {
    turbo stream response = await http.get(url);
    return await response.text();
}

// Multiple return values
blast divmod(i32 a, i32 b) -> (i32, i32) {
    return (a / b, a % b);
}

// Generic function
blast<T> max(T a, T b) -> T {
    if (a > b) {
        return a;
    }
    return b;
}
```

### Control Flow

```rads
// If-else
if (speed > 9000) {
    echo("It's over 9000!");
} elif (speed > 5000) {
    echo("Pretty fast!");
} else {
    echo("Needs more turbo!");
}

// Loop (while)
turbo i32 count = 0;
loop (count < 10) {
    echo(count);
    count = count + 1;
}

// Cruise (for loop)
cruise (i in 0..10) {
    echo(i);
}

// Cruise with array
array<str> items = ["a", "b", "c"];
cruise (item in items) {
    echo(item);
}

// Switch
switch (value) {
    case 1:
        echo("One");
        break;
    case 2:
        echo("Two");
        break;
    default:
        echo("Other");
}
```

### Structs

```rads
struct Player {
    str name;
    i32 score;
    f32 health;
}

blast main() {
    turbo Player p1 = Player {
        name: "XxDarkSlayerxX",
        score: 9001,
        health: 100.0
    };
    
    echo(p1.name);
    p1.score = p1.score + 10;
}
```

### Enums

```rads
enum GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER
}

blast main() {
    turbo GameState state = GameState.MENU;
    
    if (state == GameState.MENU) {
        echo("In menu");
    }
}
```

### Error Handling

```rads
// Result type (like Rust)
blast divide(i32 a, i32 b) -> Result<i32, str> {
    if (b == 0) {
        return Err("Division by zero!");
    }
    return Ok(a / b);
}

blast main() {
    turbo result = divide(10, 2);
    
    if (result.is_ok()) {
        echo("Result: " + result.unwrap());
    } else {
        echo("Error: " + result.unwrap_err());
    }
}
```

### Async/Await

```rads
import net;

async blast handleRequest(stream req) -> str {
    turbo str body = await req.read_all();
    return "Received: " + body;
}

async blast main() {
    turbo server = net.Server("0.0.0.0", 8080);
    
    loop (true) {
        turbo stream client = await server.accept();
        spawn handleRequest(client);  // Handle in background
    }
}
```

### File I/O

```rads
import io;

blast main() {
    // Read file
    turbo str content = io.read_file("input.txt");
    echo(content);
    
    // Write file
    io.write_file("output.txt", "Hello, file!");
    
    // Stream processing (for large files)
    turbo stream file = io.open("large.dat", "r");
    loop (true) {
        turbo str line = file.read_line();
        if (line == null) break;
        echo(line);
    }
    file.close();
}
```

### Media Processing

```rads
import media;

blast main() {
    // Audio conversion
    turbo audio = media.load_audio("input.mp3");
    audio.convert("output.wav", format: "wav", bitrate: 320);
    
    // Image processing
    turbo img = media.load_image("photo.jpg");
    img.resize(800, 600);
    img.apply_filter("vintage");
    img.save("output.png");
}
```

### HTTP Server Example

```rads
import net;

async blast handle_request(stream req) -> str {
    return """
    <html>
        <head><title>RADS Server</title></head>
        <body>
            <h1>Welcome to RADS! 🚀</h1>
            <p>Blasting data at turbo speed!</p>
        </body>
    </html>
    """;
}

async blast main() {
    turbo server = net.http_server("0.0.0.0", 8080);
    echo("Server running on http://localhost:8080");
    
    server.route("/", handle_request);
    await server.serve();
}
```

## Memory Model

### Stack vs Heap

- `turbo` variables are stack-allocated when possible (fast)
- Dynamic types (`str`, `dynarray`, etc.) are heap-allocated
- Manual memory management available with `alloc` and `free`

```rads
blast main() {
    // Stack allocated (fast)
    turbo i32 x = 42;
    
    // Heap allocated (flexible)
    str message = "Hello";
    
    // Manual allocation
    turbo ptr<i32> data = alloc<i32>(1024);  // Allocate array of 1024 ints
    data[0] = 100;
    free(data);  // Must manually free
}
```

### Ownership (Optional Safety)

RADS supports optional ownership tracking (inspired by Rust):

```rads
blast main() {
    turbo str s1 = "Hello";
    turbo str s2 = s1;  // Copy (strings are value types in safe mode)
    
    // With explicit move
    turbo dynarray<i32> arr1 = dynarray();
    turbo dynarray<i32> arr2 = move(arr1);  // arr1 is now invalid
}
```

## Plugin System (RPM)

### Installing Plugins

```bash
# Install plugin from registry
rads rpm install json-parser

# Install from local file
rads rpm install ./my-plugin.rpm

# List installed plugins
rads rpm list

# Remove plugin
rads rpm remove json-parser
```

### Using Plugins

```rads
plugin "json-parser";

blast main() {
    turbo str json_str = '{"name": "RADS", "version": "0.1"}';
    turbo obj = json.parse(json_str);
    echo(obj.name);  // "RADS"
}
```

### Creating Plugins

Plugins are written in C and compiled to shared libraries:

```c
// my_plugin.c
#include "rads_plugin.h"

RADS_EXPORT void my_function(RadsVM* vm) {
    rads_push_string(vm, "Hello from plugin!");
}

RADS_PLUGIN_INIT {
    rads_register_function("my_function", my_function);
}
```

Compile:
```bash
gcc -shared -fPIC -o my_plugin.rpm my_plugin.c
```

## Standard Library

### Core (`core`)
- String manipulation
- Math functions
- Type conversions
- Memory utilities

### I/O (`io`)
- File reading/writing
- Stream processing
- Console I/O

### Networking (`net`)
- HTTP client/server
- TCP/UDP sockets
- WebSocket support
- FTP server

### Media (`media`)
- Audio: MP3, WAV, OGG, FLAC
- Image: JPEG, PNG, GIF, BMP
- Video: Basic operations

### Async (`async`)
- Coroutines
- Thread pool
- Async I/O
- Channels

### System (`sys`)
- Process management
- Environment variables
- Command execution
- File system operations

## Compilation & Execution

### Compile to Bytecode

```bash
rads compile program.rads -o program.rbc
```

### Run Bytecode

```bash
rads run program.rbc
```

### Direct Execution

```bash
rads program.rads
```

### Compile to Native (Future)

```bash
rads compile program.rads --native -o program
./program
```

## Performance Characteristics

- **Startup**: < 10ms for bytecode, < 1ms for native
- **Memory**: Minimal overhead, close to C
- **Speed**: 2-5x slower than C, 10-50x faster than Python
- **Binary Size**: Small (< 1MB for simple programs)

## Safety Levels

RADS supports different safety modes:

```rads
// Safe mode (default) - bounds checking, null checks
blast safe_function() {
    turbo array<i32> arr = [1, 2, 3];
    echo(arr[10]);  // Runtime error: index out of bounds
}

// Xtreme mode - no safety checks, maximum performance
xtreme blast fast_function() {
    turbo array<i32> arr = [1, 2, 3];
    echo(arr[10]);  // Undefined behavior (like C)
}
```

## Future Features (Roadmap)

- [ ] JIT compilation for hot code paths
- [ ] Native code generation via LLVM
- [ ] GPU compute support
- [ ] WebAssembly target
- [ ] Interactive REPL
- [ ] Package repository hosting
- [ ] IDE language server (LSP)
- [ ] Debugger integration

---

**Version**: 0.1.0-alpha  
**Last Updated**: 2026-01-04  
**License**: MIT
