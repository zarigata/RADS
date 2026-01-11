# RADS REPL Features

The RADS Interactive REPL (Read-Eval-Print Loop) provides a powerful command-line interface for experimenting with RADS code.

## 🎯 New in v0.0.2

### ✅ Readline Support

The REPL now uses GNU Readline for enhanced line editing capabilities:

- **Arrow Keys** ⬆️⬇️⬅️➡️
  - ⬆️ **Up Arrow**: Navigate to previous commands in history
  - ⬇️ **Down Arrow**: Navigate to next commands in history
  - ⬅️ **Left Arrow**: Move cursor left in current line
  - ➡️ **Right Arrow**: Move cursor right in current line

- **Command History**
  - All executed commands are saved in history
  - Use Up/Down arrows to recall previous commands
  - Edit and re-execute previous commands
  - History persists within session

- **Line Editing**
  - `Ctrl+A`: Move cursor to beginning of line
  - `Ctrl+E`: Move cursor to end of line
  - `Ctrl+U`: Delete from cursor to beginning of line
  - `Ctrl+K`: Delete from cursor to end of line
  - `Ctrl+W`: Delete word before cursor
  - `Backspace`: Delete character before cursor
  - `Delete`: Delete character at cursor

- **Special Keys**
  - `Tab`: Auto-complete (future feature)
  - `Ctrl+C`: Cancel current line
  - `Ctrl+D`: Exit REPL (EOF)
  - `Ctrl+L`: Clear screen

- **Line Number Tracking**
  - Line numbers increment correctly: `rads[1]>`, `rads[2]>`, etc.
  - Helps track command history and errors

## 🎮 REPL Commands

Special commands start with a dot (`.`):

- `.help` - Show help message with available commands
- `.exit` or `.quit` - Exit the REPL
- `.clear` - Clear the screen and redisplay welcome message
- `.version` - Show RADS version information

## 💡 Usage Examples

### Basic Usage
```bash
$ rads

  🚀 RADS Interactive REPL v0.0.2
  ═══════════════════════════════════════
  Rapid Asynchronous Data Server Language

  Type .help for help, .exit to quit

rads[1]> turbo x = 42;
rads[2]> echo(x);
42
rads[3]> ⬆️  # Press Up Arrow to recall "echo(x);"
rads[3]> echo(x);
42
rads[4]>
```

### Arrow Key Navigation
```bash
rads[1]> turbo name = "RADS";
rads[2]> ⬆️  # Recalls previous command
rads[2]> turbo name = "RADS";  # Edit and modify
rads[2]> ⬅️⬅️⬅️⬅️⬅️⬅️  # Move cursor left
rads[2]> turbo name = "Turbo RADS";  # Edit in place
```

### Multi-line Editing
```bash
rads[1]> blast greet(name) {
...     echo("Hello, " + name + "!");
...  }
rads[2]> greet("World");
Hello, World!
```

### Command History
```bash
rads[1]> turbo a = 10;
rads[2]> turbo b = 20;
rads[3]> turbo c = a + b;
rads[4]> echo(c);
30
rads[5]> ⬆️⬆️⬆️⬆️  # Navigate up 4 commands
rads[5]> turbo a = 10;  # Recalled oldest command
```

## 🔧 Technical Details

### Readline Integration
- Library: GNU Readline
- Version: Compatible with readline 6.0+
- Features: Full line editing, history, key bindings

### Memory Management
- Command history stored in memory during session
- Proper cleanup on REPL exit
- No history file persistence (future feature)

### Keyboard Support
- **Standard Keys**: All alphanumeric and symbols
- **Arrow Keys**: Full navigation support
- **Numpad**: Number entry (0-9, operators)
- **Function Keys**: F1-F12 (available for future features)
- **Control Sequences**: Emacs-style keybindings

## 🚀 Future Enhancements

Planned features for future versions:

- **Tab Completion**: Auto-complete variable and function names
- **Syntax Highlighting**: Color-coded syntax in real-time
- **Persistent History**: Save command history to `~/.rads_history`
- **Multi-line Support**: Better handling of multi-line statements
- **Bracket Matching**: Highlight matching parentheses/braces
- **Smart Indentation**: Auto-indent based on context

## 🐛 Known Issues

- No tab completion yet
- History doesn't persist between sessions
- Multi-line editing could be improved
- No syntax highlighting in REPL

## 📚 Resources

- [GNU Readline Documentation](https://tiswww.case.edu/php/chet/readline/rltop.html)
- [Readline Cheatsheet](https://readline.kablamo.org/emacs.html)

---

*Keep it turbo, keep it radical!* 🚀
