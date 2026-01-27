# RADS Examples

This directory contains example programs demonstrating various features of RADS programming language.

## Directory Structure

### 01-basics/
Fundamental RADS programs for beginners:
- **hello_world.rads** - Classic "Hello World" program
- **io_test.rads** - File I/O operations
- **new_features.rads** - Demonstration of language features

### 02-networking/
Network programming examples:
- **http_server.rads** - HTTP web server with routing
- **net_test.rads** - TCP networking basics
- **net_json_demo.rads** - JSON API examples
- **net_full_demo.rads** - Complete networking features
- **advanced_net_test.rads** - Advanced socket programming
- **file_server.rads** - File server with authentication

### 03-media/
Media processing examples:
- **image_processor.rads** - Image manipulation
- **audio_converter.rads** - Audio file conversion
- **media_test.rads** - Media engine demonstration

### 04-advanced/
Advanced programming topics:
- **ffi_test.rads** - Foreign Function Interface (FFI) example

### 05-showcase/
Algorithm and data structure examples:
- **01_fibonacci.rads** - Fibonacci sequence
- **02_string_manipulation.rads** - String operations
- **03_calculator.rads** - Basic calculator
- **04_sorting_algorithms.rads** - Bubble, quick, merge sort
- **05_prime_numbers.rads** - Prime number generation
- **06_struct_player.rads** - Struct usage
- **07_enum_game_state.rads** - Enum in game state
- **08_array_operations.rads** - Array functions
- **09_factorial.rads** - Factorial calculation
- **10_pattern_matching.rads** - Pattern matching
- **11_temperature_converter.rads** - Unit conversion
- **12_matrix_operations.rads** - Matrix math
- **13_binary_search.rads** - Binary search algorithm
- **14_palindrome_checker.rads** - Palindrome detection
- **15_gcd_lcm.rads** - GCD and LCM
- **16_number_systems.rads** - Base conversion
- **17_linked_list.rads** - Linked list
- **18_stack_implementation.rads** - Stack data structure
- **19_queue_implementation.rads** - Queue data structure
- **20_hash_table.rads** - Hash table

### 06-v0.0.6/
v0.0.6 "Fighter Jet" feature examples:
- **websocket_echo.rads** - Real-time WebSocket echo server
- **graphql_server.rads** - GraphQL API server with queries and mutations
- **profiler_demo.rads** - Performance profiling with flamegraph generation
- **debugger_demo.rads** - DAP debugger usage example

### static/
Static files for web server examples:
- **index.html** - Example HTML page
- **style.css** - Example CSS stylesheet

## Running Examples

To run any example:
```bash
./rads examples/01-basics/hello_world.rads
```

Or from the examples directory:
```bash
cd examples
../rads 01-basics/hello_world.rads
```

## HTTP Server Example

The HTTP server example demonstrates RADS' networking capabilities:
```bash
./rads examples/02-networking/http_server.rads
```

Then visit http://localhost:8080 in your browser.
