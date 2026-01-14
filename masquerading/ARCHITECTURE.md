# RADS Masquerading - Technical Architecture

**Version:** 0.0.5  
**Status:** Design Document  
**Last Updated:** 2026-01-13

---

## 🏗️ SYSTEM OVERVIEW

The RADS Masquerading system is a multi-language transpiler that converts source code from popular programming languages into RADS. The system is designed to be:

- **Modular**: Each language is a plugin
- **Extensible**: Easy to add new languages
- **Fast**: Transpilation in milliseconds
- **Reliable**: Deterministic conversions with comprehensive testing

---

## 📐 ARCHITECTURE DIAGRAM

```
┌─────────────────────────────────────────────────────────────────┐
│                        rads-mask CLI                             │
│                                                                  │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐               │
│  │   Input    │  │  Language  │  │   Output   │               │
│  │  Handler   │→ │  Detector  │→ │  Handler   │               │
│  └────────────┘  └────────────┘  └────────────┘               │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Language Frontend                             │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Language-Specific Lexer                      │  │
│  │  (Tokenizes source code into language tokens)            │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                     │
│                           ▼                                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Language-Specific Parser                     │  │
│  │  (Builds language-specific AST)                           │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                     │
│                           ▼                                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              AST Normalizer                               │  │
│  │  (Converts to Universal Intermediate Representation)      │  │
│  └────────────────────────┬─────────────────────────────────┘  │
└───────────────────────────┼─────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│              Universal Intermediate Representation (UIR)         │
│                                                                  │
│  Language-agnostic AST that represents program semantics        │
│  - Control flow (if, for, while, etc.)                          │
│  - Functions, classes, modules                                  │
│  - Expressions, statements                                      │
│  - Type information (if available)                              │
└────────────────────────────┬────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Conversion Engine                             │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Dictionary Loader                            │  │
│  │  (Loads language-specific conversion rules)              │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                     │
│                           ▼                                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Pattern Matcher                              │  │
│  │  (Matches UIR patterns to dictionary rules)              │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                     │
│                           ▼                                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              RADS AST Builder                             │  │
│  │  (Constructs RADS-specific AST)                           │  │
│  └────────────────────────┬─────────────────────────────────┘  │
└───────────────────────────┼─────────────────────────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    RADS Code Generator                           │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Code Emitter                                 │  │
│  │  (Generates RADS source code)                             │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                     │
│                           ▼                                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Formatter                                    │  │
│  │  (Applies RADS code style)                                │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                     │
│                           ▼                                     │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │              Source Map Generator                         │  │
│  │  (Creates mapping for debugging)                          │  │
│  └────────────────────────┬─────────────────────────────────┘  │
└───────────────────────────┼─────────────────────────────────────┘
                           │
                           ▼
                    ┌──────────────┐
                    │ RADS Source  │
                    │   (.rads)    │
                    └──────────────┘
```

---

## 🧩 COMPONENT DETAILS

### 1. CLI Interface (`rads-mask`)

**Location:** `tools/rads-mask/main.c`

**Responsibilities:**
- Parse command-line arguments
- Detect source language (or use `--from` flag)
- Load appropriate language frontend
- Invoke conversion pipeline
- Write output files

**Usage:**
```bash
# Automatic language detection
rads-mask input.py -o output.rads

# Explicit language specification
rads-mask --from python input.py -o output.rads

# Batch conversion
rads-mask --from javascript src/**/*.js -o build/

# Show generated RADS without saving
rads-mask --from python input.py --print

# Generate with source maps for debugging
rads-mask --from python input.py -o output.rads --source-map
```

**Configuration:**
```c
typedef struct {
    char* input_file;
    char* output_file;
    char* source_language;      // "python", "javascript", etc.
    bool auto_detect;            // Auto-detect language
    bool generate_source_map;    // Generate .rads.map file
    bool print_only;             // Print to stdout
    bool verbose;                // Verbose output
    int optimization_level;      // 0-3
} MaskConfig;
```

---

### 2. Language Frontend

**Location:** `tools/rads-mask/src/frontend/`

Each supported language has its own frontend module:
```
frontend/
  python/
    lexer.c       # Python lexer
    parser.c      # Python parser
    normalizer.c  # Python → UIR converter
  javascript/
    lexer.c
    parser.c
    normalizer.c
  go/
    lexer.c
    parser.c
    normalizer.c
  ...
```

**Lexer Responsibilities:**
- Tokenize source code
- Handle language-specific syntax (indentation, semicolons, etc.)
- Report lexical errors

**Parser Responsibilities:**
- Build language-specific AST
- Handle language-specific grammar
- Report syntax errors

**Normalizer Responsibilities:**
- Convert language AST to Universal IR
- Resolve language-specific semantics
- Annotate with type information (if available)

---

### 3. Universal Intermediate Representation (UIR)

**Location:** `tools/rads-mask/src/uir/uir.h`

**Purpose:** Language-agnostic representation of program semantics

**Node Types:**
```c
typedef enum {
    UIR_PROGRAM,
    UIR_FUNCTION,
    UIR_CLASS,
    UIR_IF_STATEMENT,
    UIR_FOR_LOOP,
    UIR_WHILE_LOOP,
    UIR_RETURN,
    UIR_VARIABLE_DECL,
    UIR_ASSIGNMENT,
    UIR_BINARY_OP,
    UIR_UNARY_OP,
    UIR_CALL,
    UIR_MEMBER_ACCESS,
    UIR_ARRAY_LITERAL,
    UIR_OBJECT_LITERAL,
    UIR_STRING_LITERAL,
    UIR_NUMBER_LITERAL,
    UIR_BOOLEAN_LITERAL,
    UIR_NULL_LITERAL,
    // ... more node types
} UIRNodeType;

typedef struct UIRNode {
    UIRNodeType type;
    SourceLocation location;     // Original source location
    TypeInfo* type_info;         // Optional type information
    struct UIRNode** children;   // Child nodes
    int child_count;
    void* data;                  // Node-specific data
} UIRNode;
```

**Example UIR for Python code:**
```python
# Python
def add(a, b):
    return a + b
```

```
UIR_PROGRAM
└── UIR_FUNCTION (name="add")
    ├── params: ["a", "b"]
    └── body:
        └── UIR_RETURN
            └── UIR_BINARY_OP (op="+")
                ├── UIR_IDENTIFIER (name="a")
                └── UIR_IDENTIFIER (name="b")
```

---

### 4. Conversion Dictionary

**Location:** `tools/rads-mask/dictionaries/`

**Format:** YAML-based mapping files

**Structure:**
```yaml
# python.dict
version: "0.0.5"
language: "python"
target: "rads"

# Direct syntax mappings
syntax:
  function_def:
    pattern: "def {name}({params}):"
    converts_to: "fn {name}({params})"
    
  if_statement:
    pattern: "if {condition}:"
    converts_to: "if ({condition})"
    
  for_loop:
    pattern: "for {var} in {iterable}:"
    converts_to: "for (let {var} in {iterable})"

# Standard library mappings
stdlib:
  print:
    maps_to: "io.print"
    import_required: "io"
    
  len:
    maps_to: "array.length"
    import_required: "array"
    
  open:
    maps_to: "io.open"
    import_required: "io"
    args_transform: "same"  # Arguments map 1:1

# Complex idiom translations
idioms:
  list_comprehension:
    pattern: "[{expr} for {var} in {iterable}]"
    converts_to: "array.map({iterable}, fn({var}) { return {expr}; })"
    complexity: "medium"
    
  list_comprehension_with_filter:
    pattern: "[{expr} for {var} in {iterable} if {condition}]"
    converts_to: |
      array.map(
        array.filter({iterable}, fn({var}) { return {condition}; }),
        fn({var}) { return {expr}; }
      )
    complexity: "high"
    
  decorator:
    pattern: "@{decorator}\ndef {name}({params}):"
    converts_to: "let {name} = {decorator}(fn({params}) { ... });"
    complexity: "high"
    requires_manual_review: true

# Type mappings (for statically typed languages)
types:
  int: "int"
  float: "float"
  str: "string"
  bool: "bool"
  list: "array"
  dict: "map"
  None: "null"

# Import mappings
imports:
  "import os":
    converts_to: "import os;"
    note: "RADS os module provides similar functionality"
    
  "import sys":
    converts_to: "import sys;"
    
  "import json":
    converts_to: "import json;"
    
  "import http.server":
    converts_to: "import net;"
    note: "RADS net module provides HTTP server"

# Known incompatibilities
incompatibilities:
  - feature: "async/await"
    reason: "RADS uses different concurrency model (Constellation)"
    workaround: "Use @rads.constellation.async decorator"
    
  - feature: "with statement"
    reason: "RADS uses 'defer' for resource management"
    workaround: "Replace 'with' with try/finally or defer"
    
  - feature: "generators (yield)"
    reason: "Not yet implemented in RADS"
    workaround: "Use arrays or iterators"
```

---

### 5. Conversion Engine

**Location:** `tools/rads-mask/src/converter/`

**Components:**

#### Dictionary Loader
```c
typedef struct {
    char* language;
    char* version;
    HashMap* syntax_rules;
    HashMap* stdlib_mappings;
    HashMap* idiom_patterns;
    HashMap* type_mappings;
    HashMap* import_mappings;
    Array* incompatibilities;
} ConversionDictionary;

ConversionDictionary* load_dictionary(const char* language);
```

#### Pattern Matcher
```c
typedef struct {
    UIRNode* pattern;           // Pattern to match
    UIRNode* replacement;       // Replacement template
    int priority;               // Match priority
    bool requires_review;       // Needs manual review
} ConversionRule;

ConversionRule* match_pattern(UIRNode* node, ConversionDictionary* dict);
```

#### RADS AST Builder
```c
typedef struct RADSNode {
    RADSNodeType type;
    SourceLocation original_location;  // For source maps
    struct RADSNode** children;
    int child_count;
    void* data;
} RADSNode;

RADSNode* convert_uir_to_rads(UIRNode* uir, ConversionDictionary* dict);
```

---

### 6. Code Generator

**Location:** `tools/rads-mask/src/codegen/`

**Responsibilities:**
- Traverse RADS AST
- Emit RADS source code
- Apply formatting rules
- Generate source maps

**Code Emitter:**
```c
typedef struct {
    StringBuilder* output;
    int indentation_level;
    bool use_tabs;              // Tabs vs spaces
    int spaces_per_indent;
    SourceMapBuilder* source_map;
} CodeEmitter;

void emit_rads_code(RADSNode* ast, CodeEmitter* emitter);
```

**Source Map Format:**
```json
{
  "version": 3,
  "file": "output.rads",
  "sourceRoot": "",
  "sources": ["input.py"],
  "names": [],
  "mappings": "AAAA,SAAS,IAAI,CAAC,CAAC,EAAE,CAAC,EAAE;IAChB,OAAO,CAAC,GAAG,CAAC,CAAC;AACjB,CAAC"
}
```

---

## 🔄 CONVERSION PIPELINE

### Step-by-Step Example: Python → RADS

**Input (Python):**
```python
def fibonacci(n):
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

print(fibonacci(10))
```

**Step 1: Lexing**
```
Tokens:
[DEF, IDENTIFIER("fibonacci"), LPAREN, IDENTIFIER("n"), RPAREN, COLON, NEWLINE, INDENT,
 IF, IDENTIFIER("n"), LESS_EQUAL, NUMBER(1), COLON, NEWLINE, INDENT,
 RETURN, IDENTIFIER("n"), NEWLINE, DEDENT,
 RETURN, IDENTIFIER("fibonacci"), LPAREN, IDENTIFIER("n"), MINUS, NUMBER(1), RPAREN,
 PLUS, IDENTIFIER("fibonacci"), LPAREN, IDENTIFIER("n"), MINUS, NUMBER(2), RPAREN,
 NEWLINE, DEDENT, NEWLINE,
 IDENTIFIER("print"), LPAREN, IDENTIFIER("fibonacci"), LPAREN, NUMBER(10), RPAREN, RPAREN]
```

**Step 2: Parsing (Python AST)**
```
Module
└── FunctionDef (name="fibonacci")
    ├── args: [arg(name="n")]
    └── body:
        ├── If
        │   ├── test: Compare (left=Name("n"), ops=[LessEq], comparators=[Num(1)])
        │   └── body: [Return(value=Name("n"))]
        └── Return
            └── value: BinOp
                ├── left: Call(func=Name("fibonacci"), args=[BinOp(Name("n"), Sub, Num(1))])
                ├── op: Add
                └── right: Call(func=Name("fibonacci"), args=[BinOp(Name("n"), Sub, Num(2))])
└── Expr
    └── Call(func=Name("print"), args=[Call(func=Name("fibonacci"), args=[Num(10)])])
```

**Step 3: Normalization (UIR)**
```
UIR_PROGRAM
├── UIR_FUNCTION (name="fibonacci")
│   ├── params: ["n"]
│   └── body:
│       ├── UIR_IF_STATEMENT
│       │   ├── condition: UIR_BINARY_OP (op="<=", left="n", right=1)
│       │   └── then_body: UIR_RETURN (value="n")
│       └── UIR_RETURN
│           └── UIR_BINARY_OP (op="+")
│               ├── UIR_CALL (func="fibonacci", args=[n-1])
│               └── UIR_CALL (func="fibonacci", args=[n-2])
└── UIR_CALL (func="print", args=[UIR_CALL(func="fibonacci", args=[10])])
```

**Step 4: Dictionary Lookup**
```yaml
# Matched rules:
- function_def: "def {name}({params}):" → "fn {name}({params})"
- if_statement: "if {condition}:" → "if ({condition})"
- return: "return {expr}" → "return {expr};"
- print: "print({args})" → "io.print({args})"
```

**Step 5: RADS AST Construction**
```
RADS_PROGRAM
├── RADS_IMPORT (module="io")
├── RADS_FUNCTION (name="fibonacci")
│   ├── params: ["n"]
│   └── body:
│       ├── RADS_IF
│       │   ├── condition: RADS_BINARY_OP (op="<=", left="n", right=1)
│       │   └── then_body: RADS_RETURN (value="n")
│       └── RADS_RETURN
│           └── RADS_BINARY_OP (op="+")
│               ├── RADS_CALL (func="fibonacci", args=[n-1])
│               └── RADS_CALL (func="fibonacci", args=[n-2])
└── RADS_CALL (func="io.print", args=[RADS_CALL(func="fibonacci", args=[10])])
```

**Step 6: Code Generation**
```rads
import io;

fn fibonacci(n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

io.print(fibonacci(10));
```

---

## 🎯 OPTIMIZATION STRATEGIES

### Level 0: Direct Translation (Default)
- 1:1 mapping where possible
- Preserves original structure
- Easiest to debug

### Level 1: Idiom Recognition
- Recognizes common patterns
- Converts to idiomatic RADS
- Example: List comprehensions → `array.map`

### Level 2: Semantic Optimization
- Analyzes program semantics
- Applies RADS-specific optimizations
- Example: Constant folding, dead code elimination

### Level 3: Aggressive Optimization
- Restructures code for performance
- May change execution order (preserving semantics)
- Example: Loop unrolling, function inlining

**Usage:**
```bash
rads-mask input.py -o output.rads --optimize 2
```

---

## 🐛 ERROR HANDLING

### Conversion Errors

**Unsupported Feature:**
```
Error: Unsupported Python feature at line 15
  │
15│ async def fetch_data():
  │ ^^^^^
  │
  = Python's 'async/await' is not directly supported in RADS
  = Suggestion: Use @rads.constellation.async decorator instead
  = See: docs/masquerading/PYTHON.md#async-await
```

**Ambiguous Conversion:**
```
Warning: Ambiguous conversion at line 23
  │
23│ result = data[0:5]
  │          ^^^^^^^^^
  │
  = Python slice notation has multiple RADS equivalents
  = Using: array.slice(data, 0, 5)
  = Alternative: data[0:5] (RADS also supports slice notation)
```

**Manual Review Required:**
```
Info: Manual review recommended at line 42
  │
42│ @complex_decorator
43│ def process(data):
  │ ^^^^^^^^^^^^^^^^^^
  │
  = Complex decorator pattern detected
  = Automatic conversion may not preserve all semantics
  = Please review generated RADS code
```

---

## 🧪 TESTING STRATEGY

### Unit Tests
- Test each dictionary rule individually
- Verify correct UIR generation
- Verify correct RADS generation

### Integration Tests
- Full pipeline tests (source → RADS)
- Real-world code examples
- Performance benchmarks

### Regression Tests
- Prevent breaking existing conversions
- Automated on every dictionary update

**Test Structure:**
```
tests/
  masquerading/
    python/
      test_syntax.c          # Basic syntax conversions
      test_stdlib.c          # Standard library mappings
      test_idioms.c          # Complex pattern conversions
      test_real_world.c      # Real Python projects
    javascript/
      test_syntax.c
      test_stdlib.c
      ...
```

---

## 📊 PERFORMANCE TARGETS

### Transpilation Speed
- **Target:** <100ms for 1,000 lines of code
- **Target:** <1s for 10,000 lines of code
- **Target:** Linear scaling O(n)

### Generated Code Performance
- **Target:** 95%+ speed of hand-written RADS
- **Target:** No unnecessary allocations
- **Target:** Optimal use of RADS stdlib

### Memory Usage
- **Target:** <100MB for transpiling 10,000 lines
- **Target:** Streaming mode for large files

---

## 🔧 EXTENSIBILITY

### Adding a New Language

**1. Create language directory:**
```bash
mkdir tools/rads-mask/src/frontend/ruby
```

**2. Implement lexer, parser, normalizer:**
```c
// ruby/lexer.c
Token* ruby_lex(const char* source);

// ruby/parser.c
RubyAST* ruby_parse(Token* tokens);

// ruby/normalizer.c
UIRNode* ruby_to_uir(RubyAST* ast);
```

**3. Create conversion dictionary:**
```yaml
# dictionaries/ruby.dict
version: "0.0.5"
language: "ruby"
target: "rads"

syntax:
  function_def:
    pattern: "def {name}({params})"
    converts_to: "fn {name}({params})"
  # ... more rules
```

**4. Register language:**
```c
// main.c
register_language("ruby", ruby_lex, ruby_parse, ruby_to_uir);
```

**5. Add tests:**
```c
// tests/masquerading/ruby/test_syntax.c
void test_ruby_function_def() {
    const char* ruby = "def add(a, b)\n  a + b\nend";
    const char* expected = "fn add(a, b) {\n    return a + b;\n}";
    assert_conversion(ruby, expected);
}
```

---

## 🚀 FUTURE ENHANCEMENTS

### Phase 2: Advanced Features
- **Type inference**: Infer types from usage patterns
- **Optimization hints**: Suggest RADS-specific optimizations
- **Interactive mode**: Step-by-step conversion with user feedback

### Phase 3: IDE Integration
- **VSCode extension**: Real-time conversion preview
- **Syntax highlighting**: Highlight RADS-specific extensions
- **Autocomplete**: Suggest RADS equivalents

### Phase 4: Bidirectional Conversion
- **RADS → Python**: Generate Python-like syntax from RADS
- **Round-trip testing**: Ensure Python → RADS → Python preserves semantics

---

## 📝 IMPLEMENTATION CHECKLIST

### Core Infrastructure
- [ ] CLI argument parsing
- [ ] Language detection
- [ ] UIR definition
- [ ] Dictionary loader
- [ ] Pattern matcher
- [ ] Code emitter
- [ ] Source map generator

### Python Support
- [ ] Python lexer
- [ ] Python parser
- [ ] Python → UIR normalizer
- [ ] Python dictionary (100+ rules)
- [ ] Python test suite (500+ tests)

### JavaScript Support
- [ ] JavaScript lexer
- [ ] JavaScript parser
- [ ] JavaScript → UIR normalizer
- [ ] JavaScript dictionary
- [ ] JavaScript test suite

### Documentation
- [ ] Architecture guide (this document)
- [ ] User guide for each language
- [ ] API reference
- [ ] Contributing guide

---

**Document Status:** Draft  
**Next Steps:** Team review and prototype implementation  
**Estimated Effort:** 3-4 months for Phase 1 (Python support)
