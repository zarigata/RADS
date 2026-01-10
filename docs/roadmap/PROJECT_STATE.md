# RADS: Rapid Asynchronous Data Server - Project State & Manifesto 🚀

## 🛸 The Vision
RADS is a high-performance, lightweight programming language born from the energy of 90s/2000s internet culture. It is designed to be a faster, "radically" more efficient alternative to Node.js, Python, and Rust for building micro-servers and high-speed data processors.

**Aesthetic Goals**: Frutiger Aero, Nu-Metal energy, Early Xbox/PS2 UI vibes.
**Performance Goals**: Near-C execution speed with modern async ergonomics.

---

## 🏗️ Technical Architecture
The RADS core is implemented in pure C (C11) for maximum portability and speed.

### 1. The Pipeline
- **Lexer (`src/lexer.c`)**: Tokenizes source using thematic keywords (`turbo`, `blast`, `xtreme`).
- **Parser (`src/parser.c`)**: A recursive descent parser that builds an Abstract Syntax Tree (AST).
- **Interpreter (`src/interpreter.c`)**: A tree-walking interpreter that executes the AST directly. Handles runtime values and environment-based variable/function lookup.

### 2. Standard Libraries
- **`io`**: Native file system operations.
- **`net`**: High-concurrency socket handling, HTTP server registration, and REST abstractions.
- **`media`**: Stubs for specialized file "digestion" (image/audio processing).
- **`ffi`**: Foreign Function Interface for calling external C binaries.

### 3. Tooling
- **RPM (`tools/rpm/rpm.c`)**: The RADS Package Manager for project initialization and dependency management.
- **`rads-convert`**: A high-performance batch processing CLI utility built on RADS.

---

## ✅ Progress & Achievements
- **Phase 1-2**: Established core infrastructure (Lexer, Parser, Interpreter).
- **Phase 3 (Expansion)**: Implemented advanced networking with first-class function support (callbacks), FFI linkage, and media stubs.
- **Phase 4.5**: Created a universal `install.sh` system for global deployment to `/usr/local/bin`.

---

## 🔮 Future Roadmap (The "Xtreme" Plan)
1. **Bytecode/Native Compilation**: Transition from tree-walking interpretation to a stack-based VM or LLVM backend for "blast" speed.
2. **Advanced Async Loop**: Implement a custom event loop (similar to libuv) for high-concurrency micro-servers.
3. **RPM Ecosystem**: Expand the registry to support versioning and secure plugin hosting.
4. **Deep Testing (Phase 4)**: Implement the full unit test suite and security audit.

---

## 🛠️ For Future Developers
- **Build**: Use `make` in the root directory.
- **Install**: Run `sudo ./install.sh`.
- **Contributing**: Standard C11, maintain the 90s aesthetic in all documentation and naming.
- **Native Extensions**: New functions can be added to standard libraries by implementing the `NativeFn` signature and calling `register_native`.

**Stay Radical.**  
*-- RADS Core Team & zarigata*
