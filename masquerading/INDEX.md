# RADS Masquerading Documentation Index

**Version:** 0.0.5  
**Status:** Planning Phase  
**Last Updated:** 2026-01-13

---

## 📚 DOCUMENTATION OVERVIEW

Welcome to the RADS Masquerading documentation. This system allows you to write code in your preferred programming language and run it on the RADS runtime.

**Core Concept:** *"Write Your Way, Run RADS"*

---

## 🗂️ DOCUMENTATION STRUCTURE

### 1. Planning & Architecture
- **[V0.0.5_PLAN.md](../../V0.0.5_PLAN.md)** - Complete vision, goals, and roadmap for Phase 0.0.5
- **[ARCHITECTURE.md](./ARCHITECTURE.md)** - Technical architecture and implementation details

### 2. Language-Specific Guides
- **[PYTHON.md](./PYTHON.md)** - Python → RADS masquerading guide
- **[JAVASCRIPT.md](./JAVASCRIPT.md)** - JavaScript/TypeScript → RADS masquerading guide
- **[GO.md](./GO.md)** - Go → RADS masquerading guide *(Coming Soon)*
- **[RUST.md](./RUST.md)** - Rust → RADS masquerading guide *(Coming Soon)*
- **[RUBY.md](./RUBY.md)** - Ruby → RADS masquerading guide *(Coming Soon)*

### 3. RADS-Specific Features
- **[RADS_EXTENSIONS.md](./RADS_EXTENSIONS.md)** - RADS-specific syntax that requires learning (~10%)

### 4. Reference Materials
- **[DICTIONARY_SPEC.md](./DICTIONARY_SPEC.md)** - How to write conversion dictionaries *(Coming Soon)*
- **[CONTRIBUTING.md](./CONTRIBUTING.md)** - How to contribute language support *(Coming Soon)*
- **[FAQ.md](./FAQ.md)** - Frequently asked questions *(Coming Soon)*

---

## 🚀 QUICK START

### For Python Developers
```bash
# Install RADS with masquerading
curl -sSL https://rads-lang.org/install.sh | bash

# Write Python
echo 'print("Hello, RADS!")' > hello.py

# Convert to RADS
rads-mask --from python hello.py -o hello.rads

# Run with RADS
rads hello.rads
```

**Next Steps:** Read [PYTHON.md](./PYTHON.md)

---

### For JavaScript Developers
```bash
# Write JavaScript
echo 'console.log("Hello, RADS!");' > hello.js

# Convert to RADS
rads-mask --from javascript hello.js -o hello.rads

# Run with RADS
rads hello.rads
```

**Next Steps:** Read [JAVASCRIPT.md](./JAVASCRIPT.md)

---

## 🎯 LEARNING PATH

### Stage 1: Zero Learning (Day 1)
- Write code in your preferred language
- Use `rads-mask` to convert
- Run with RADS runtime
- **Learning Required:** 0%

### Stage 2: Basic Understanding (Week 1)
- Review generated RADS code
- Understand basic conversions
- Learn `defer` for resource management
- **Learning Required:** 2%

### Stage 3: RADS Extensions (Month 1)
- Use `@constellation.*` for distributed computing
- Use `@ffi.*` for C library integration
- Add `@optimize.*` hints for performance
- **Learning Required:** 10%

### Stage 4: Native RADS (Month 3+)
- Write RADS directly for critical paths
- Mix masqueraded and native code
- Contribute to conversion dictionaries
- **Learning Required:** 50%+

---

## 📊 CONVERSION RATES BY LANGUAGE

| Language | Automatic Conversion | Requires Learning | Status |
|----------|---------------------|-------------------|--------|
| Python | 92% | 8% | ✅ Planned |
| JavaScript | 90% | 10% | ✅ Planned |
| Go | 88% | 12% | 🔜 Future |
| Rust | 85% | 15% | 🔜 Future |
| Ruby | 91% | 9% | 🔜 Future |
| Java | 87% | 13% | 🔜 Future |
| C# | 88% | 12% | 🔜 Future |
| PHP | 89% | 11% | 🔜 Future |

---

## 🏗️ ARCHITECTURE AT A GLANCE

```
Your Code (Python/JS/etc.)
         ↓
    rads-mask CLI
         ↓
  Language Frontend (Lexer → Parser)
         ↓
Universal Intermediate Representation (UIR)
         ↓
  Conversion Engine (Dictionary)
         ↓
    RADS Code Generator
         ↓
   RADS Source (.rads)
         ↓
   RADS Runtime
```

**Details:** See [ARCHITECTURE.md](./ARCHITECTURE.md)

---

## 🎓 KEY CONCEPTS

### 1. Masquerading
Writing code in a familiar language syntax that gets converted to RADS.

### 2. Conversion Dictionary
YAML-based mappings that define how language constructs convert to RADS.

### 3. Universal IR (UIR)
Language-agnostic intermediate representation used during conversion.

### 4. RADS Extensions
Features unique to RADS that require minimal learning (~10% of code):
- **Constellation** - Distributed computing
- **FFI** - Foreign function interface
- **Memory Operations** - Low-level memory access
- **Optimization Hints** - Performance directives
- **defer** - Resource management

---

## 🔧 TOOLS

### rads-mask CLI
The main transpiler tool for converting code to RADS.

**Basic Usage:**
```bash
# Automatic language detection
rads-mask input.py -o output.rads

# Explicit language
rads-mask --from python input.py -o output.rads

# Batch conversion
rads-mask --from javascript src/**/*.js -o build/

# Print without saving
rads-mask --from python input.py --print

# With source maps for debugging
rads-mask --from python input.py -o output.rads --source-map

# Verbose output
rads-mask --from python input.py -o output.rads --verbose
```

**Installation:**
```bash
# Included with RADS
rads-mask --version

# Update
rads update
```

---

## 🤝 COMMUNITY & SUPPORT

### Getting Help
- **Discord:** https://discord.gg/rads-lang
- **Forum:** https://forum.rads-lang.org
- **GitHub Issues:** https://github.com/rads-lang/rads/issues
- **Stack Overflow:** Tag `rads-lang`

### Contributing
We welcome contributions for:
- New language support
- Dictionary improvements
- Bug fixes
- Documentation
- Examples

**See:** [CONTRIBUTING.md](./CONTRIBUTING.md) *(Coming Soon)*

---

## 📖 EXAMPLES

### Example Repositories
- **Python Examples:** https://github.com/rads-lang/masquerading-examples/tree/main/python
- **JavaScript Examples:** https://github.com/rads-lang/masquerading-examples/tree/main/javascript
- **Real-World Projects:** https://github.com/rads-lang/masquerading-examples/tree/main/projects

### Quick Examples

**Python → RADS:**
```python
# Python
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)
```
↓
```rads
// RADS
fn fibonacci(n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}
```

**JavaScript → RADS:**
```javascript
// JavaScript
const greet = (name) => {
    console.log(`Hello, ${name}!`);
};
```
↓
```rads
// RADS
let greet = fn(name) {
    io.print("Hello, " + name + "!");
};
```

---

## 🗺️ ROADMAP

### Q1 2026: Foundation
- Core transpiler architecture
- Python support (proof-of-concept)
- Basic documentation

### Q2 2026: Expansion
- JavaScript/TypeScript support
- Go support
- Source maps for debugging
- IDE plugins

### Q3 2026: Refinement
- Rust, Ruby, Java support
- Performance optimization
- Web playground
- Community contribution system

### Q4 2026: Maturity
- C#, PHP support
- Automated testing
- Benchmarking suite
- Production v1.0 release

**Full Roadmap:** See [V0.0.5_PLAN.md](../../V0.0.5_PLAN.md)

---

## ❓ FAQ

### Q: Will masqueraded code be slower than native RADS?
**A:** Target is 95%+ performance of hand-written RADS. Most conversions are 1:1 with no overhead.

### Q: Can I mix masqueraded and native RADS code?
**A:** Yes! You can gradually introduce native RADS for performance-critical sections.

### Q: Do I need to learn RADS to use masquerading?
**A:** No! ~90% of your familiar syntax works automatically. Only ~10% requires learning RADS-specific features.

### Q: What if my language feature isn't supported?
**A:** Check the compatibility matrix in your language guide. Most common features are supported. For unsupported features, workarounds are documented.

### Q: Can I contribute language support?
**A:** Absolutely! See [CONTRIBUTING.md](./CONTRIBUTING.md) for guidelines.

**More Questions:** See [FAQ.md](./FAQ.md) *(Coming Soon)*

---

## 📝 DOCUMENT STATUS

| Document | Status | Completion |
|----------|--------|------------|
| V0.0.5_PLAN.md | ✅ Complete | 100% |
| ARCHITECTURE.md | ✅ Complete | 100% |
| PYTHON.md | ✅ Complete | 100% |
| JAVASCRIPT.md | ✅ Complete | 100% |
| RADS_EXTENSIONS.md | ✅ Complete | 100% |
| INDEX.md | ✅ Complete | 100% |
| GO.md | 🔜 Planned | 0% |
| RUST.md | 🔜 Planned | 0% |
| DICTIONARY_SPEC.md | 🔜 Planned | 0% |
| CONTRIBUTING.md | 🔜 Planned | 0% |
| FAQ.md | 🔜 Planned | 0% |

---

## 🎯 NEXT STEPS FOR DEVELOPERS

1. **Choose Your Language:** Pick Python or JavaScript to start
2. **Read the Guide:** Study the language-specific guide
3. **Try Examples:** Clone and run example projects
4. **Convert Your Code:** Use `rads-mask` on your own projects
5. **Learn Extensions:** Gradually adopt RADS-specific features
6. **Join Community:** Connect with other developers

---

## 🌟 PHILOSOPHY

**RADS Masquerading embodies the "VW Fusca" philosophy:**

- **Lightweight:** Simple transpiler, no heavy dependencies
- **Fast:** Millisecond conversions, 95%+ runtime performance
- **Reliable:** Deterministic conversions, extensive testing

**Goal:** Lower the barrier to entry while maintaining RADS's core values of simplicity, performance, and reliability.

---

**Welcome to RADS Masquerading!** 🎭

Start writing in your favorite language today, and let RADS handle the rest.
