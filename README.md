# 🚀 RADS Programming Language

**R**apid **A**synchronous **D**ata **S**erver Language

*Blast through data at turbo speed! The ultimate retro-futuristic programming language.*

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)]()
[![Version](https://img.shields.io/badge/version-0.1.0--alpha-purple.svg)]()

---

## 🌐 Overview

RADS is a high-performance, low-level programming language designed for the modern retro enthusiast. It combines the raw power of C with the dynamic energy of the 2000s internet.

**Built for:**
- 🖥️ **Microservers & IoT**: Perfect for Raspberry Pi deployments running 24/7.
- ⚡ **High-Speed Processing**: Digest images, audio, and video at processor-level speeds.
- 🛠️ **Reliable Tooling**: Craft F2A (File-to-Any) servers with safety and precision.
- 🎨 **Aesthetic**: Frutiger Aero design philosophy meets nu-metal intensity.

## ✨ Features

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
wget https://github.com/zarigata/rads/releases/download/v0.1.0-alpha/rads-0.1.0-alpha-linux-x86_64.tar.gz

# Extract
tar -xzf rads-0.1.0-alpha-linux-x86_64.tar.gz
cd rads-0.1.0-alpha-linux-x86_64

# Run installer
./install.sh

# Verify installation
rads --version
rstar help
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

### v0.1.0-alpha (Current) ✅
- ✅ Core compiler and interpreter
- ✅ Interactive REPL mode
- ✅ Beautiful error messages with suggestions
- ✅ RADStar package manager foundation
- ✅ HTTP/Web server support
- ✅ Async I/O with libuv
- ✅ Cross-platform build system

### v0.2.0 (Next)
- 🚧 Package registry and remote plugin installation
- 🚧 Web framework with middleware
- 🚧 Template engine
- 🚧 Database connectors
- 🚧 Enhanced standard library

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
