# 🚀 RADS Programming Language

**R**apid **A**synchronous **D**ata **S**erver Language

*Blast through data at turbo speed! The ultimate retro-futuristic programming language.*

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Version](https://img.shields.io/badge/version-0.0.6--blue.svg)]()

---

## 🌐 Overview

RADS is a high-performance, low-level programming language designed for the modern retro enthusiast. It combines the raw power of C with the dynamic energy of the 2000s internet.

**Built for:**
- 🖥️ **Microservers & IoT**: Perfect for Raspberry Pi deployments running 24/7.
- ⚡ **High-Speed Processing**: Digest images, audio, and video at processor-level speeds.
- 🛠️ **Reliable Tooling**: Craft F2A (File-to-Any) servers with safety and precision.
- 🎨 **Aesthetic**: Frutiger Aero design philosophy meets nu-metal intensity.

## ✨ Features

### v0.0.6 "FIGHTER JET" - Mach Speed Upgrades
- 🛩️ **Bytecode VM**: Stack-based virtual machine for 3-5x performance
- ⚡ **Sampling Profiler**: Real-time profiling with flamegraph generation
- 🔥 **WebSocket Support**: Real-time bidirectional communication (128+ concurrent connections)
- 📊 **GraphQL Server**: Queries, mutations, subscriptions with JSON responses
- 🎯 **LSP Server**: Full Language Server Protocol (Rust-based)
- 🐛 **Debug Protocol**: DAP with breakpoints, step controls, variable inspection
- See [v0.0.6 plan](V0.0.6_FIGHTER_JET_PLAN.md) for details

### v0.0.5 "CHAMELEON" - Python Transpilation
- 🐍 **rads-mask**: Python to RADS transpiler (95%+ accuracy)
- 🧬 **Language-Agnostic AST**: Universal AST for multi-language support
- 📝 **Write Your Way, Run RADS**: Zero learning curve for Python developers

### 📦 RADS Package Registry (NEW!)
- 🌐 **Professional Registry**: npmjs.com-style package management for RADS
- 🔍 **Search & Browse**: Full-text search with filters (category, license, sort)
- 🚀 **Easy Publishing**: GitHub OAuth authentication, automated publishing workflow
- 📊 **Statistics**: Download counts, stars, and analytics
- 💰 **Zero Cost**: GitHub-native infrastructure (Pages + Actions)
- 🎨 **Modern UI**: Next.js + shadcn/ui, dark mode, responsive design
- See [registry architecture docs](docs/registry-architecture/) for details

### 📦 RADS Package Registry (NEW!)
- 🌐 **Professional Registry**: npmjs.com-style package management for RADS
- 🔍 **Search & Browse**: Full-text search with filters (category, license, sort)
- 🚀 **Easy Publishing**: GitHub OAuth authentication, automated publishing workflow
- 📊 **Statistics**: Download counts, stars, and analytics
- 💰 **Zero Cost**: GitHub-native infrastructure (Pages + Actions)
- 🎨 **Modern UI**: Next.js + shadcn/ui, dark mode, responsive design
- See [registry architecture docs](docs/registry-architecture/) for details

### v0.0.4 "Constellation" - Distributed Computing Platform
- 🌌 **Distributed Platform**: VM/Container-like instance management
- ⚡ **10-400x Faster**: Than Docker/Kubernetes
- 📊 **Resource Orchestration**: Automatic scaling, scheduling
- 🔗 **Service Mesh**: Load balancing, circuit breakers, monitoring
- 💾 **Distributed Filesystem**: DHT with replication
- 📈 **Auto-Scaling**: Policy-based scaling with cooldown
- 📊 **Monitoring**: Time-series database, log aggregation, alerting

### v0.0.3 "Butterfly" - Database & Testing
- 💾 **SQLite3 Integration**: Full database support with transactions
- 🧪 **Built-in Testing**: `test` keyword, assertions, test runner
- ⚡ **Enhanced Async**: Promise.all, Promise.race, Promise.timeout

### v0.0.2 "Core Language" - RGB REPL
- 🎨 **RGB Chroma Effects**: Gaming keyboard aesthetics with rainbow cursor
- 📖 **GNU Readline**: Command history, arrow key navigation
- 🏗️ **Core Language**: Structs, enums, arrays, string operations

### v0.0.1 "Web Framework"
- 🚀 **Interactive REPL**: Learn and experiment with instant feedback
- 🎨 **Beautiful Error Messages**: Color-coded errors with source context and helpful suggestions
- ⭐ **RADStar Package Manager**: Install and manage plugins with `rstar`
- ⚡ **Turbo Performance**: Compiles to native code with libuv async I/O
- 🌐 **Built-in HTTP Server**: Create web servers with minimal code
- 🔒 **Safety & Speed**: Choose your mode - Safe or Xtreme

## 🚀 Quick Start

### Installation

#### Option 1: Download Pre-built Binary (Recommended)

```bash
# Download the latest release
wget https://github.com/zarigata/rads/releases/download/v0.0.5/rads-0.0.5-linux-x86_64.tar.gz

# Extract
tar -xzf rads-0.0.5-linux-x86_64.tar.gz
cd rads-0.0.5

# Run installer
./install.sh

# Verify installation
rads --version
rstar help
rpm help

# Visit Package Registry
# Registry will be available at: https://zarigata.github.io/RADS
```

#### Option 2: Build from Source

**Prerequisites:**
- GCC or Clang compiler
- Make
- libuv development libraries

On Debian/Ubuntu:
```bash
sudo apt-get install build-essential libuv1-dev
```

On Fedora/RHEL:
```bash
sudo dnf install gcc make libuv-devel
```

On macOS:
```bash
brew install libuv
```

**Build:**
```bash
# Clone the repository
git clone https://github.com/zarigata/rads.git
cd rads

# Run the installer (builds and optionally installs)
./install.sh

# Or build manually
make
```

### Your First RADS Program

#### Using the REPL (Interactive Mode)

```bash
$ rads
  🚀 RADS Interactive REPL v0.1.0-alpha
  ═══════════════════════════════════════
  Rapid Asynchronous Data Server Language

  Type .help for help, .exit to quit

rads[1]> echo("Hello, RADS World! 🌐")
Hello, RADS World! 🌐

rads[2]> turbo x = 42
rads[3]> echo(x * 2)
84

rads[4]> .exit
👋 Goodbye!
```

#### Running a File

Create `hello.rads`:
```rads
blast main() {
    echo("Hello, RADS World! 🌐");

    turbo name = "Developer";
    echo("Welcome, " + name + "!");
}
```

Run it:
```bash
rads hello.rads
```

#### Try the Examples

```bash
# Basic examples
rads examples/01-basics/hello_world.rads

# HTTP server
rads examples/02-networking/http_server.rads

# See all examples
ls examples/
```

## 📦 Package Manager (RADStar)

RADS comes with `rstar`, a built-in package manager for plugins:

```bash
# List installed plugins
rstar list

# Install a plugin (coming soon - full implementation)
rstar install media-kit

# Run a plugin
rstar run my-plugin

# Get help
rstar help
```

Plugins are stored in `~/.rads/plugins/`. See [docs/PACKAGE_MANAGER.md](docs/PACKAGE_MANAGER.md) for details.

## 📚 Documentation

- **[REPL Guide](docs/REPL_GUIDE.md)** - Interactive shell tutorial
- **[Package Manager](docs/PACKAGE_MANAGER.md)** - Plugin system guide
- **[Examples](examples/)** - Code examples by category

## 🗺️ Roadmap

### v0.0.6 "FIGHTER JET" (Current) ✅
- ✅ Bytecode Compiler with VM and disassembler
- ✅ Sampling Profiler with flamegraph generation
- ✅ WebSocket server and client APIs
- ✅ GraphQL server with queries/mutations
- ✅ LSP server (Rust-based) with completion and navigation
- ✅ Debug protocol (DAP) with breakpoints and inspection

### v0.0.7 (Next) 🔜
- 🔜 JIT Compilation: LLVM-based JIT for hot paths
- 🔜 Advanced LSP: Full refactoring and code actions
- 🔜 Enhanced Debugger: Conditional breakpoints, watch expressions
- 🔜 Full GraphQL Spec: Subscriptions and live queries
- 🔜 ECMAScript Transpiler: JavaScript → RADS conversion

### v0.0.5 "CHAMELEON" (Completed) ✅
- ✅ Python to RADS transpiler (95%+ accuracy)
- ✅ Language-agnostic AST for multi-language support
- ✅ Write Your Way, Run RADS: Zero learning curve for Python developers
- ✅ Core compiler and interpreter
- ✅ Interactive REPL mode
- ✅ Beautiful error messages with suggestions
- ✅ RADStar package manager foundation
- ✅ HTTP/Web server support
- ✅ Async I/O with libuv
- ✅ Cross-platform build system

### v0.6.0 (Next) - Package Registry Launch
- 🚧 **Production Package Registry**: Full npmjs.com-style registry at registry.rads-lang.org
- 🔧 **rstar CLI Integration**: Install/search/publish packages from command line
- 📊 **Download Analytics**: Package statistics and download counts
- 📝 **Documentation**: Complete user and package author guides
- 🧪 **Automated Publishing**: GitHub Actions workflow for package publishing

### v1.0.0 (Future)
- 🔜 Language Server Protocol (LSP)
- 🔜 Debugger
- 🔜 Native compilation
- 🔜 Production-ready stdlib

## 🤝 Contributing

We welcome contributions from the community! See [CONTRIBUTING.md](CONTRIBUTING.md) for details.

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 💜 Authors

Created by **zarigata** and the RADS Community.

---
*"Keep it turbo, keep it radical!"* 🚀
