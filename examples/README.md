# RADS Examples

This directory contains example programs demonstrating various features of the RADS programming language.

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
