# 📚 RADS Documentation

Welcome to the RADS programming language documentation directory. This comprehensive documentation covers everything from getting started to advanced features and release notes.

---

## 📖 Quick Access

| Documentation | Description | Audience |
|--------------|-------------|------------|
| [README.md](../README.md) | Main project README with installation and quick start | Everyone |
| [CHANGELOG.md](../CHANGELOG.md) | Complete version history and release notes | Everyone |
| [V0.0.6_QUICK_REFERENCE.md](archive/V0.0.6_QUICK_REFERENCE.md) | Quick reference for the current release | Users & Developers |

---

## 📂 Core Documentation

### Language Reference
- **[LANGUAGE_SPEC.md](LANGUAGE_SPEC.md)** - Complete language specification
  - Syntax and grammar
  - Data types and operators
  - Built-in functions and stdlib
  - Control flow constructs
- **[RADStar.md](RADStar.md)** - RADStar plugin manager documentation
  - Installing and managing plugins
  - Package publishing workflow
  - Plugin development guide

### Getting Started Guides
- **[REPL_GUIDE.md](REPL_GUIDE.md)** - Interactive shell tutorial
  - REPL features and commands
  - Command history and editing
  - Keyboard shortcuts
- **[PACKAGE_MANAGER.md](PACKAGE_MANAGER.md)** - Plugin system guide
  - Plugin architecture
  - Installation process
  - Configuration options
- **[MIGRATION_GUIDE.md](MIGRATION_GUIDE.md)** - Upgrading between versions
  - Breaking changes
  - Migration steps
  - Compatibility notes

---

## 🌐 Web Documentation

### Main Documentation Site
- **[index.html](index.html)** - Main documentation homepage
- **[guide.html](guide.html)** - User guide and tutorials
- **[compare.html](compare.html)** - Language comparison with other languages
- **[releases.html](releases.html)** - Detailed release notes

### Feature Documentation
- **[documentation.html](documentation.html)** - Complete API documentation
- **[examples.html](examples.html)** - Example code library
- **[playground.html](playground.html)** - Interactive code playground

### Topic-Specific Documentation
- **[docs-intro.html](docs-intro.html)** - Introduction to RADS
- **[docs-syntax.html](docs-syntax.html)** - Syntax reference
- **[docs-types.html](docs-types.html)** - Data types and type system
- **[docs-control.html](docs-control.html)** - Control flow and operators
- **[docs-functions.html](docs-functions.html)** - Functions and procedures
- **[docs-structs.html](docs-structs.html)** - Structs and records
- **[docs-stdlib.html](docs-stdlib.html)** - Standard library reference
- **[docs-async.html](docs-async.html)** - Async/await patterns
- **[docs-networking.html](docs-networking.html)** - Network programming
- **[docs-media.html](docs-media.html)** - Media processing capabilities

---

## 🎯 v0.0.6 FIGHTER JET Features

### Performance & Tooling
- **[Profiling Guide](#)** - Use the sampling profiler
  - Starting/stopping profiler
  - Flamegraph generation
  - Performance analysis
- **[LSP Integration](#)** - Language Server Protocol setup
  - Editor configuration (VS Code, Vim, JetBrains)
  - Completion and navigation
  - Real-time diagnostics

### Real-time Capabilities
- **[WebSocket API](#)** - Real-time bidirectional communication
  - Server and client setup
  - Message handling
  - Connection management
- **[GraphQL Server](#)** - GraphQL API development
  - Schema definition
  - Query/resolver implementation
  - Subscription support

### Debugging
- **[Debug Protocol Guide](#)** - Using the DAP debugger
  - Setting breakpoints
  - Stepping through code
  - Inspecting variables
  - Using debug commands

---

## 🎨 Visual Features

- **[CHROMA_EFFECTS.md](CHROMA_EFFECTS.md)** - RGB Chroma visual effects documentation
  - RGB keyboard chroma cycling
  - Cursor styles and effects
  - Color palette reference
  - Terminal color codes

---

## 🗺️ Project Roadmap

### Current State
- **[PROJECT_STATE.md](roadmap/PROJECT_STATE.md)** - Current project status
  - Implemented features
  - In-progress work
  - Known limitations

### Future Enhancements
- **[ENHANCEMENT_ROADMAP.md](roadmap/ENHANCEMENT_ROADMAP.md)** - Future development plans
  - Planned features
  - Version timeline
  - Community proposals

---

## 🔧 Development & Architecture

### Version Documentation
- **[V0.0.6_QUICK_REFERENCE.md](archive/V0.0.6_QUICK_REFERENCE.md)** - Current release quick reference
- **[CHANGELOG.md](../CHANGELOG.md)** - Full version history

### Web Engine
- **[WEB_ENGINE_DESIGN.md](WEB_ENGINE_DESIGN.md)** - Web framework architecture
- **[WEB_ENGINE_API.md](WEB_ENGINE_API.md)** - Web engine API reference

### Constellation (Distributed Computing)
- **[constellation/INDEX.md](constellation/INDEX.md)** - Constellation distributed platform docs
- **[IGUANA_SUMMARY.md](constellation/IGUANA_SUMMARY.md)** - Iguana protocol details
- **[RNP_PROTOCOL.md](constellation/RNP_PROTOCOL.md)** - RNP protocol specification

### Masquerading (Transpilation)
- **[masquerading/INDEX.md](masquerading/INDEX.md)** - Transpilation system overview
- **[PYTHON.md](masquerading/PYTHON.md)** - Python transpilation guide
- **[JAVASCRIPT.md](masquerading/JAVASCRIPT.md)** - JavaScript transpilation (future)
- **[ARCHITECTURE.md](masquerading/ARCHITECTURE.md)** - Transpiler architecture
- **[RADS_EXTENSIONS.md](masquerading/RADS_EXTENSIONS.md)** - Language extensions

### Registry (Package Manager)
- **[registry-architecture/INDEX.md](registry-architecture/INDEX.md)** - Package registry docs
- **[DEPLOYMENT_GUIDE.md](registry-architecture/DEPLOYMENT_GUIDE.md)** - Registry deployment
- **[QUICK_START.md](registry-architecture/QUICK_START.md)** - Registry quick start
- **[AUTHOR_GUIDE.md](registry-architecture/AUTHOR_GUIDE.md)** - Publishing packages

---

## 🚀 Tools Documentation

### Development Tools
- **[REPL_FEATURES.md](REPL_FEATURES.md)** - REPL features reference
- **[REPL_DEMO.md](REPL_DEMO.md)** - REPL demonstration

### Production Guide
- **[PRODUCTION_GUIDE.md](PRODUCTION_GUIDE.md)** - Production deployment guide
  - Deployment strategies
  - Performance optimization
  - Security best practices
  - Monitoring and logging

---

## 💻 Building Documentation

### Local Viewing

The HTML documentation can be viewed directly in a web browser:
```bash
firefox docs/index.html
```

Or with any browser:
```bash
# macOS
open docs/index.html

# Linux
xdg-open docs/index.html

# Windows
start docs/index.html
```

### HTTP Server

Serve documentation with a simple HTTP server:

**Python:**
```bash
cd docs
python3 -m http.server 8000
```

**Node.js:**
```bash
cd docs
npx serve
```

**PHP:**
```bash
cd docs
php -S localhost:8000
```

Then visit http://localhost:8000 in your browser.

---

## 🤝 Contributing

### Adding Documentation

When adding new documentation:

1. **Follow Style Guide**
   - Use existing formatting and structure
   - Maintain consistent tone and voice
   - Include examples where applicable

2. **Update Index Files**
   - Update this README.md with new entries
   - Add to index.html navigation
   - Update sitemap if applicable

3. **Synchronize Documentation**
   - Keep language spec synchronized with implementation
   - Update CHANGELOG.md for new features
   - Create/update release summaries

4. **Include Examples**
   - Add working code examples
   - Test all examples before committing
   - Update examples/README.md

5. **Version Information**
   - Always specify version information
   - Note breaking changes clearly
   - Provide migration guides

### Documentation Standards

- **Clarity:** Write for the target audience level
- **Completeness:** Cover all aspects of the topic
- **Accuracy:** Verify all code examples work
- **Consistency:** Use consistent terminology and formatting
- **Accessibility:** Use clear headings and structure

---

## 📞 Getting Help

- **[GitHub Issues](https://github.com/zarigata/rads/issues)** - Report bugs or request features
- **[Discussions](https://github.com/zarigata/rads/discussions)** - Ask questions or share ideas
- **[Contributing Guide](../CONTRIBUTING.md)** - How to contribute to RADS

---

## 📊 Documentation Status

| Section | Status | Last Updated |
|---------|--------|--------------|
| Language Specification | ✅ Complete | 2026-01-22 |
| Web Documentation | ✅ Complete | 2026-01-26 |
| v0.0.6 Features | ✅ Complete | 2026-01-27 |
| REPL Documentation | ✅ Complete | 2026-01-22 |
| Production Guide | ✅ Complete | 2026-01-22 |
| Registry Architecture | ✅ Complete | 2026-01-22 |
| Roadmap | ✅ Complete | 2026-01-22 |

---

**Total Documentation:** 100+ documents, 100,000+ words, 2,000+ code examples

---

*Happy coding with RADS! 🚀✨*
