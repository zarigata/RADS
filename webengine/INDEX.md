# 🗂️ RADS Web Engine - Complete Index

**Master reference for all Web Engine files, documentation, and examples**

---

## 📁 Complete Folder Structure

```
Rads/
├── webengine/                           ← Web Engine Root
│   ├── INDEX.md                         ← This file
│   ├── README.md                        ← Main introduction
│   │
│   ├── docs/                            ← Documentation
│   │   ├── API.md                       ← Complete API reference
│   │   ├── QUICKSTART.md                ← Get started in 5 minutes
│   │   └── EXAMPLES.md                  ← Example cookbook
│   │
│   ├── examples/                        ← Working examples
│   │   ├── 01-basic/                    ← Basic usage
│   │   │   └── hello.rads               ← Hello world
│   │   ├── 02-javascript/               ← JavaScript integration
│   │   │   └── js_demo.rads             ← JS execution demo
│   │   ├── 03-html-css/                 ← HTML+CSS parsing
│   │   │   └── html_demo.rads           ← HTML/CSS demo
│   │   ├── 04-plugins/                  ← Plugin system
│   │   │   └── plugin_demo.rads         ← Plugin loading demo
│   │   └── 05-task-manager/             ← Full application
│   │       ├── README.md                ← Task manager docs
│   │       └── task_manager.rads        ← Complete app (250+ lines)
│   │
│   └── plugins/                         ← Plugin development
│       └── README.md                    ← Plugin guide
│
├── src/stdlib/                          ← Source code
│   ├── stdlib_webengine.c               ← Implementation (407 lines)
│   └── stdlib_webengine.h               ← Header file
│
└── docs/                                ← Main documentation
    ├── WEB_ENGINE_DESIGN.md             ← Architecture & design
    ├── WEB_ENGINE_API.md                ← Detailed API reference
    └── WEB_ENGINE_COMPLETE.md           ← Implementation summary
```

---

## 📚 Documentation Map

### Getting Started
1. **[webengine/README.md](README.md)**
   - Overview and introduction
   - Quick start guide
   - Feature list

2. **[webengine/docs/QUICKSTART.md](docs/QUICKSTART.md)**
   - 5-minute tutorial
   - First programs
   - REPL usage

3. **[webengine/docs/EXAMPLES.md](docs/EXAMPLES.md)**
   - Example cookbook
   - Common patterns
   - Best practices

### API Reference
4. **[webengine/docs/API.md](docs/API.md)**
   - Complete function reference
   - Parameters and returns
   - Usage examples

5. **[docs/WEB_ENGINE_API.md](../docs/WEB_ENGINE_API.md)**
   - Detailed API documentation
   - Advanced usage
   - Comparison tables

### Architecture
6. **[docs/WEB_ENGINE_DESIGN.md](../docs/WEB_ENGINE_DESIGN.md)**
   - System architecture
   - Design decisions
   - Implementation plan

7. **[docs/WEB_ENGINE_COMPLETE.md](../docs/WEB_ENGINE_COMPLETE.md)**
   - Implementation summary
   - Statistics and metrics
   - Achievement list

---

## 🎯 Examples Map

### Level 1: Basics
- **[examples/01-basic/hello.rads](examples/01-basic/hello.rads)**
  - Initialize web engine
  - Verify installation
  - 10 lines

### Level 2: JavaScript
- **[examples/02-javascript/js_demo.rads](examples/02-javascript/js_demo.rads)**
  - Execute JavaScript code
  - Use console.log()
  - Variables and expressions
  - 25 lines

### Level 3: HTML/CSS
- **[examples/03-html-css/html_demo.rads](examples/03-html-css/html_demo.rads)**
  - Parse HTML documents
  - Query DOM elements
  - Parse CSS stylesheets
  - 30 lines

### Level 4: Plugins
- **[examples/04-plugins/plugin_demo.rads](examples/04-plugins/plugin_demo.rads)**
  - Load plugins
  - Manage plugin registry
  - List installed plugins
  - 30 lines

### Level 5: Full Application
- **[examples/05-task-manager/task_manager.rads](examples/05-task-manager/task_manager.rads)**
  - Complete task manager web app
  - Pure RADS + HTML + CSS
  - SQLite database integration
  - HTTP server with routes
  - Beautiful UI with animations
  - CRUD operations
  - 250+ lines
  - **[README](examples/05-task-manager/README.md)** - Full documentation

---

## 🚀 Quick Access

### Run Examples

```bash
# Level 1: Basic
rads webengine/examples/01-basic/hello.rads

# Level 2: JavaScript
rads webengine/examples/02-javascript/js_demo.rads

# Level 3: HTML/CSS
rads webengine/examples/03-html-css/html_demo.rads

# Level 4: Plugins
rads webengine/examples/04-plugins/plugin_demo.rads

# Level 5: Task Manager
rads webengine/examples/05-task-manager/task_manager.rads
# Visit: http://localhost:5050
```

### Read Documentation

```bash
# Quick start
cat webengine/docs/QUICKSTART.md

# API reference
cat webengine/docs/API.md

# Examples
cat webengine/docs/EXAMPLES.md

# Design document
cat docs/WEB_ENGINE_DESIGN.md
```

---

## 🔧 API Quick Reference

```rads
// Initialization
web_init()                              // Initialize web engine

// JavaScript
js_eval(code)                           // Execute JavaScript
js_evalFile(path)                       // Run JS file

// HTML
html_parse(html)                        // Parse HTML
html_querySelector(doc, selector)       // Query DOM

// CSS
css_parse(css)                          // Parse CSS

// Plugins
pkg_load(name)                          // Load plugin
pkg_list()                              // List plugins
pkg_install(path)                       // Install plugin
```

---

## 📊 File Statistics

### Source Code
| File | Lines | Purpose |
|------|-------|---------|
| `src/stdlib/stdlib_webengine.c` | 407 | Main implementation |
| `src/stdlib/stdlib_webengine.h` | 9 | Header file |
| **Total** | **416** | **Production code** |

### Documentation
| File | Lines | Purpose |
|------|-------|---------|
| `webengine/README.md` | 150 | Main introduction |
| `webengine/docs/QUICKSTART.md` | 180 | Quick start guide |
| `webengine/docs/API.md` | 280 | API reference |
| `webengine/docs/EXAMPLES.md` | 320 | Example cookbook |
| `docs/WEB_ENGINE_DESIGN.md` | 600 | Design document |
| `docs/WEB_ENGINE_API.md` | 800 | Detailed API |
| `docs/WEB_ENGINE_COMPLETE.md` | 400 | Implementation summary |
| **Total** | **2,730** | **Comprehensive docs** |

### Examples
| File | Lines | Purpose |
|------|-------|---------|
| `examples/01-basic/hello.rads` | 15 | Basic usage |
| `examples/02-javascript/js_demo.rads` | 30 | JavaScript demo |
| `examples/03-html-css/html_demo.rads` | 35 | HTML/CSS demo |
| `examples/04-plugins/plugin_demo.rads` | 35 | Plugin demo |
| `examples/05-task-manager/task_manager.rads` | 250 | Full application |
| `examples/05-task-manager/README.md` | 300 | App documentation |
| **Total** | **665** | **Working examples** |

### Grand Total
- **Source Code:** 416 lines
- **Documentation:** 2,730 lines
- **Examples:** 665 lines
- **Total Lines:** 3,811 lines

---

## 🎓 Learning Path

### Beginner (Day 1)
1. Read `webengine/README.md`
2. Follow `webengine/docs/QUICKSTART.md`
3. Run `examples/01-basic/hello.rads`
4. Run `examples/02-javascript/js_demo.rads`

### Intermediate (Day 2-3)
1. Read `webengine/docs/API.md`
2. Run `examples/03-html-css/html_demo.rads`
3. Run `examples/04-plugins/plugin_demo.rads`
4. Study `webengine/docs/EXAMPLES.md`

### Advanced (Day 4-7)
1. Study `examples/05-task-manager/task_manager.rads`
2. Read `examples/05-task-manager/README.md`
3. Build your own web application
4. Create custom plugins

### Expert (Week 2+)
1. Read `docs/WEB_ENGINE_DESIGN.md`
2. Study `src/stdlib/stdlib_webengine.c`
3. Contribute to RADS
4. Build production apps

---

## 🔍 Find What You Need

### "How do I...?"

**...initialize the web engine?**
→ `webengine/docs/QUICKSTART.md` - Section "Step 1"

**...execute JavaScript?**
→ `webengine/docs/API.md` - Section "JavaScript API"
→ `examples/02-javascript/js_demo.rads`

**...parse HTML?**
→ `webengine/docs/API.md` - Section "HTML API"
→ `examples/03-html-css/html_demo.rads`

**...load plugins?**
→ `webengine/docs/API.md` - Section "Plugin API"
→ `examples/04-plugins/plugin_demo.rads`

**...build a web app?**
→ `examples/05-task-manager/task_manager.rads`
→ `examples/05-task-manager/README.md`

**...understand the architecture?**
→ `docs/WEB_ENGINE_DESIGN.md`

---

## 🏆 Highlights

### Best Starting Point
**[webengine/docs/QUICKSTART.md](docs/QUICKSTART.md)** - Get up and running in 5 minutes

### Best Example
**[examples/05-task-manager/task_manager.rads](examples/05-task-manager/task_manager.rads)** - Complete application

### Best API Reference
**[webengine/docs/API.md](docs/API.md)** - Complete function reference

### Best Architecture Doc
**[docs/WEB_ENGINE_DESIGN.md](../docs/WEB_ENGINE_DESIGN.md)** - System design

---

## 🎯 Common Tasks

### Run Task Manager
```bash
rads webengine/examples/05-task-manager/task_manager.rads
# Visit: http://localhost:5050
```

### Test JavaScript
```bash
rads webengine/examples/02-javascript/js_demo.rads
```

### Try REPL
```bash
rads
> web_init()
> js_eval("console.log('Hello!')")
> .exit
```

---

## 📧 Getting Help

1. **Quick Start Issues** → `webengine/docs/QUICKSTART.md`
2. **API Questions** → `webengine/docs/API.md`
3. **Example Problems** → `webengine/docs/EXAMPLES.md`
4. **Architecture Questions** → `docs/WEB_ENGINE_DESIGN.md`

---

## ✅ Checklist

Use this to verify you have everything:

- [ ] Read `webengine/README.md`
- [ ] Completed `webengine/docs/QUICKSTART.md`
- [ ] Ran all 5 example programs
- [ ] Built the task manager
- [ ] Reviewed API reference
- [ ] Explored source code

---

*RADS Web Engine - Complete Index*
*v0.0.3 "Butterfly" 🦋*

*Keep it TURBO, keep it RADICAL!* 🚀✨
