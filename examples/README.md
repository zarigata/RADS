# 📚 RADS Examples Library

This directory contains comprehensive example programs demonstrating various features of RADS programming language. From beginner basics to advanced real-world applications.

---

## 🗂️ Directory Structure

### 🎯 01-basics/
**Fundamental RADS programs for beginners**

| Example | Description | Difficulty |
|---------|-------------|------------|
| **hello_world.rads** | Classic "Hello World" program | ⭐ Beginner |
| **io_test.rads** | File I/O operations and streams | ⭐ Beginner |
| **new_features.rads** | Demonstration of language features | ⭐⭐ Easy |

**Learn:** Variables, functions, basic I/O, control flow

---

### 🌐 02-networking/
**Network programming and web development**

| Example | Description | Features |
|---------|-------------|----------|
| **http_server.rads** | HTTP web server with routing | Routes, middleware, JSON responses |
| **net_test.rads** | TCP networking basics | Sockets, connections, data transfer |
| **net_json_demo.rads** | JSON API examples | Request/response, JSON parsing |
| **net_full_demo.rads** | Complete networking features | Authentication, sessions |
| **advanced_net_test.rads** | Advanced socket programming | Non-blocking I/O, timeouts |
| **file_server.rads** | File server with authentication | File serving, security |

**Learn:** HTTP servers, TCP sockets, APIs, authentication

---

### 🎵 03-media/
**Media processing examples**

| Example | Description |
|---------|-------------|
| **image_processor.rads** | Image manipulation (resize, filters) |
| **audio_converter.rads** | Audio file conversion (formats, codecs) |
| **media_test.rads** | Media engine demonstration |

**Learn:** Media handling, file formats, data processing

---

### 🔧 04-advanced/
**Advanced programming topics**

| Example | Description |
|---------|-------------|
| **ffi_test.rads** | Foreign Function Interface (FFI) example - call C libraries from RADS |

**Learn:** Interop with C code, external libraries

---

### 🏗️ 05-showcase/
**Algorithm and data structure examples**

20 comprehensive examples covering:

- **01_fibonacci.rads** - Fibonacci sequence (recursion)
- **02_string_manipulation.rads** - String operations and parsing
- **03_calculator.rads** - Basic calculator with operators
- **04_sorting_algorithms.rads** - Bubble, quick, merge sort
- **05_prime_numbers.rads** - Prime number generation (Sieve algorithm)
- **06_struct_player.rads** - Struct usage for game objects
- **07_enum_game_state.rads** - Enum in game state machine
- **08_array_operations.rads** - Array functions (push, pop, length)
- **09_factorial.rads** - Factorial calculation (tail recursion)
- **10_pattern_matching.rads** - Pattern matching concepts
- **11_temperature_converter.rads** - Unit conversion (Celsius ↔ Fahrenheit)
- **12_matrix_operations.rads** - Matrix math (multiply, transpose)
- **13_binary_search.rads** - Binary search algorithm
- **14_palindrome_checker.rads** - Palindrome detection
- **15_gcd_lcm.rads** - GCD and LCM calculations
- **16_number_systems.rads** - Base conversion (binary, hex, octal)
- **17_linked_list.rads** - Linked list implementation
- **18_stack_implementation.rads** - Stack data structure
- **19_queue_implementation.rads** - Queue data structure
- **20_hash_table.rads** - Hash table implementation

**Learn:** Algorithms, data structures, computational thinking

---

### 🛩️ 06-v0.0.6/
**v0.0.6 "Fighter Jet" - New feature examples**

| Example | Description | Features Demonstrated |
|---------|-------------|---------------------|
| **websocket_echo.rads** | Real-time WebSocket echo server | WebSocket server, message handling, broadcast |
| **graphql_server.rads** | GraphQL API server | Schema, resolvers, queries, mutations |
| **profiler_demo.rads** | Performance profiling with flamegraph generation | Sampling profiler, HTML output, statistics |
| **debugger_demo.rads** | DAP debugger usage example | Breakpoints, stepping, variable inspection |

**Learn:** Real-time communication, performance optimization, debugging

**Run WebSocket Example:**
```bash
./rads examples/websocket_echo.rads
```

Then use websocat or browser WebSocket client to connect to `ws://localhost:8080`

**Run GraphQL Example:**
```bash
./rads examples/graphql_server.rads
```

Send queries to `http://localhost:8080/graphql`:
```graphql
query {
  user(id: "1") {
    id
    name
  }
}
```

**Run Profiler Demo:**
```bash
./rads examples/profiler_demo.rads
```

This will generate `flamegraph.html` - open in browser to see interactive visualization.

**Run Debugger Demo:**
```bash
./rads --debug examples/debugger_demo.rads
```

Use DAP client (VS Code, VS) to connect to localhost on default debug port.

---

### 📄 static/
**Static files for web server examples**

- **index.html** - Example HTML page
- **style.css** - Example CSS stylesheet

Used by HTTP server examples to serve static content.

---

## 🚀 Quick Start

### Running Examples

**To run any example:**
```bash
./rads examples/01-basics/hello_world.rads
```

**From the examples directory:**
```bash
cd examples
../rads 01-basics/hello_world.rads
```

**With bytecode mode (3-5x faster):**
```bash
./rads examples/01-basics/hello_world.rads --bytecode
```

**With debugging:**
```bash
./rads --debug examples/01-basics/hello_world.rads
```

### HTTP Server Example

The HTTP server demonstrates RADS' web framework capabilities:

```bash
./rads examples/02-networking/http_server.rads
```

Then visit:
- **Main page:** http://localhost:8080
- **API endpoint:** http://localhost:8080/api/users
- **Static files:** http://localhost:8080/static/

---

## 📖 Learning Path

### 1. Absolute Beginner (Start Here)
1. **hello_world.rads** - Your first RADS program
2. **io_test.rads** - Understand input/output
3. **new_features.rads** - See language features

### 2. Basic Programming
1. **variables.rads** - Data types and variables
2. **operations.rads** - Operators and expressions
3. **functions.rads** - Function definitions and calls

### 3. Control Flow
1. **fibonacci.rads** - Recursion
2. **factorial.rads** - Tail recursion
3. **temperature_converter.rads** - Conditional logic

### 4. Data Structures
1. **array_operations.rads** - Arrays
2. **06_struct_player.rads** - Structs
3. **linked_list.rads** - Custom data structures

### 5. Networking
1. **http_server.rads** - HTTP server basics
2. **net_test.rads** - TCP sockets
3. **net_json_demo.rads** - JSON APIs

### 6. Advanced v0.0.6 Features
1. **websocket_echo.rads** - Real-time communication
2. **graphql_server.rads** - Modern APIs
3. **profiler_demo.rads** - Performance optimization
4. **debugger_demo.rads** - Professional debugging

### 7. Algorithms
Explore the showcase directory for implementations of:
- Sorting algorithms
- Search algorithms
- Number theory
- Data structures

---

## 💡 Tips

1. **Start Simple:** Begin with hello_world.rads and work through basics
2. **Read the Code:** Comments explain key concepts
3. **Experiment:** Modify examples and see what happens
4. **Use Bytecode Mode:** Run with `--bytecode` for 3-5x speedup
5. **Profile Your Code:** Use profiler_demo.rads as a template
6. **Debug When Needed:** Use `--debug` flag for step-by-step execution

---

## 📚 Related Documentation

- **[CHANGELOG.md](../CHANGELOG.md)** - Version history and new features
- **[V0.0.6 FIGHTER JET Summary](../V0.0.6_FIGHTER_JET_SUMMARY.md)** - v0.0.6 release details
- **[REPL Guide](../docs/REPL_GUIDE.md)** - Interactive shell tutorial
- **[Package Manager](../docs/PACKAGE_MANAGER.md)** - Plugin system guide

---

## 🤝 Contributing

Have an idea for a new example? Contributions welcome! See [CONTRIBUTING.md](../CONTRIBUTING.md) for guidelines.

---

*Happy coding with RADS! 🚀✨*
