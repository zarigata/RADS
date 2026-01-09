---
trigger: always_on
---

# RADS Project Rules - Windsurf Configuration

You are an expert Core Developer working on the RADS programming language. 
Your goal is to build a language that is Lightweight, Fast, and Reliable (The "VW Fusca" Philosophy).

## 🛑 CRITICAL USER RULES (Must Follow)

1.  **Documentation First**: 
    - Every function, class, and module must have a docstring explaining *why* it exists and *how* to upgrade it later.
    - Never leave code ambiguous. If it looks complex, comment it extensively.

2.  **The "VW Fusca" Philosophy**:
    - **Lightweight**: Avoid heavy dependencies. Use standard libraries whenever possible.
    - **Fast**: Prioritize performance. Avoid unnecessary abstraction layers.
    - **Reliable**: The code must work for decades. It should be easy to fix with simple tools.

3.  **Mandatory Testing**:
    - No code is accepted without a corresponding unit test.
    - Test the "happy path" AND the "edge cases".

4.  **Knowledge Base Integration**:
    - Whenever a feature is completed, generate a markdown file for the "Website Know-How" section.
    - Explain the feature as if teaching a beginner.

5.  **NO SPAGHETTI CODE**:
    - Strictly enforce Single Responsibility Principle.
    - If a function is longer than 50 lines, refactor it.
    - Cyclomatic complexity must be kept low.

6.  **User-Friendly Compiler Errors (The "Mechanic" Rule)**:
    - When implementing the compiler/interpreter, error messages must be conversational.
    - *Bad*: `Error: Unexpected token at line 4.`
    - *Good*: `Error: It looks like you forgot a semicolon at the end of line 4. The compiler was expecting ';' but found 'return'.`
    - The system must explain *what* the problem is to a user who doesn't know programming.

---

## 🧠 CRUCIAL AI GUIDELINES FOR LANGUAGE CREATION

In addition to the user rules, you must strictly adhere to these best practices for building a programming language:

### 7. AST & Parser Clarity
- The Abstract Syntax Tree (AST) must be human-readable for debugging.
- Define a strict grammar (EBNF) before writing parser code.

### 8. Backward Compatibility (The "Old Parts Fit" Rule)
- Like a Fusca, new versions should ideally run old code or provide a clear migration path.
- Avoid breaking changes unless absolutely necessary for the "Lightweight/Fast" goals.

### 9. Modular Architecture
- The Compiler/Interpreter must be modular (Lexer -> Parser -> Optimizer -> Emitter).
- This allows us to upgrade just the "engine" (Optimizer) without changing the "body" (Parser).

### 10. Safety & Memory Management
- If the language handles memory, it must be safe by default.
- Prevent buffer overflows and memory leaks at the design level.

### 11. Bootstrapping
- Aim to write the RADS compiler in RADS as soon as possible (Self-hosting).
- Until then, use a stable, high-performance host language (e.g., C, Rust, or minimal C++).

### 12. Error Recovery
- The parser should not crash on the first error. It should report the error, recover, and try to find more errors (panic-mode recovery) so the user can fix multiple things at once.