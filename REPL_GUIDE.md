# RADS Interactive REPL Guide

## What is the REPL?

REPL stands for **Read-Eval-Print-Loop**. It's an interactive shell where you can type RADS code line by line and see results immediately. Perfect for:

- **Learning** RADS syntax
- **Testing** code snippets
- **Experimenting** with libraries
- **Quick calculations** and scripts

## Starting the REPL

Simply run `rads` without any arguments:

```bash
./rads
```

You'll see:
```
  🚀 RADS Interactive REPL v0.1.0-alpha
  ═══════════════════════════════════════
  Rapid Asynchronous Data Server Language

  Type .help for help, .exit to quit

rads[1]>
```

## Basic Usage

### Simple Statements

```rads
rads[1]> echo("Hello, RADS!")
Hello, RADS!

rads[2]> turbo x = 42
rads[3]> echo(x)
42
```

### Variables Persist

Variables you define are available in subsequent lines:

```rads
rads[1]> turbo name = "Alice"
rads[2]> turbo greeting = "Hello, " + name
rads[3]> echo(greeting)
Hello, Alice
```

### Multi-line Not Required

The REPL automatically adds semicolons if you forget them:

```rads
rads[1]> turbo x = 10    # Works without semicolon
rads[2]> echo(x * 2)
20
```

## REPL Commands

Commands start with a dot (`.`) and provide special functionality:

### .help
Show available commands and examples:
```
rads[1]> .help
```

### .exit or .quit
Exit the REPL:
```
rads[1]> .exit
👋 Goodbye!
```

### .clear
Clear the screen and show welcome message again:
```
rads[1]> .clear
```

### .version
Show RADS version information:
```
rads[1]> .version
RADS v0.1.0-alpha
Rapid Asynchronous Data Server Language
Built: Jan 10 2026 02:22:29
```

## Examples

### Math Operations
```rads
rads[1]> turbo a = 10
rads[2]> turbo b = 20
rads[3]> echo(a + b)
30
rads[4]> echo(a * b)
200
```

### String Manipulation
```rads
rads[1]> turbo first = "Hello"
rads[2]> turbo last = "World"
rads[3]> echo(first + " " + last)
Hello World
```

### Using Standard Library
```rads
rads[1]> turbo result = math.sqrt(16)
rads[2]> echo(result)
4.0

rads[3]> turbo rounded = math.floor(3.7)
rads[4]> echo(rounded)
3
```

### File I/O
```rads
rads[1]> io.write_file("test.txt", "Hello from REPL!")
rads[2]> turbo content = io.read_file("test.txt")
rads[3]> echo(content)
Hello from REPL!
```

## Tips & Tricks

### 1. No Semicolons Required
The REPL automatically adds them:
```rads
rads[1]> echo("Works!")  # With semicolon
rads[2]> echo("Also works")  # Without semicolon
```

### 2. Test Before Writing to File
Use the REPL to test code snippets before adding them to your `.rads` files:
```rads
rads[1]> turbo x = json.parse('{"name":"Alice"}')
rads[2]> echo(x)
# If it works, copy to your file!
```

### 3. Quick Calculations
Use it as a calculator:
```rads
rads[1]> echo(42 * 1.5 + 10)
73.0
```

### 4. Keyboard Shortcuts
- **Ctrl+C** - Exit REPL
- **Ctrl+D** - Exit REPL (EOF)
- **Arrow Up/Down** - Command history (coming soon!)

## Limitations

Current REPL limitations (will be improved in future versions):

- ❌ No arrow key history (yet)
- ❌ No tab completion (yet)
- ❌ No multi-line statements (yet)
- ❌ No syntax highlighting (yet)

## Troubleshooting

### "Error: Undefined variable"
Variables must be declared before use:
```rads
rads[1]> echo(x)  # Error if x doesn't exist
rads[2]> turbo x = 42
rads[3]> echo(x)  # Now works!
42
```

### Command Not Found
REPL commands start with `.`:
```rads
rads[1]> help      # Wrong
Unknown command: help

rads[1]> .help     # Correct
```

## What's Next?

Once you're comfortable with the REPL, try:

1. **Run example files**:
   ```bash
   ./rads examples/01-basics/hello_world.rads
   ```

2. **Build a web server**:
   ```bash
   ./rads examples/02-networking/http_server.rads
   ```

3. **Write your own programs**:
   ```bash
   echo 'echo("My first RADS program!")' > my_app.rads
   ./rads my_app.rads
   ```

## Future Enhancements

Coming in future versions:
- ✨ Arrow key history navigation
- ✨ Tab completion for functions
- ✨ Syntax highlighting
- ✨ Multi-line input with `\`
- ✨ `.save <file>` to save session
- ✨ `.load <file>` to load code

Happy coding! 🚀
