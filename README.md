# 🚀 RADS Programming Language

**R**apid **A**synchronous **D**ata **S**erver Language

*Blast through data at turbo speed! Inspired by the golden age of the internet (90s/2000s).*

```rads
blast main() {
    echo("Welcome to RADS! 🌐");
    echo("Where performance meets retro aesthetics!");
}
```

## What is RADS?

RADS is a high-performance, low-level programming language designed for building:
- 🖥️ **Lightweight microservers** (perfect for Raspberry Pi)
- 🎵 **Media processing tools** (audio/video/image conversion)
- 🌐 **Network services** (HTTP, FTP, WebSocket servers)
- ⚡ **High-speed data processing** (near-C performance)

### Key Features

- **🔥 Blazing Fast**: Compiles to native code with performance close to C
- **🎨 90s/2000s Aesthetic**: Keywords like `blast`, `turbo`, `cruise`, `xtreme`
- **🔌 Plugin System**: RPM (RADS Package Manager) like Python's pip
- **⚡ Async/Await**: Built-in async I/O for high-performance servers
- **🛡️ Safety Options**: Choose between safe mode or xtreme mode
- **📦 Lightweight**: Minimal runtime, perfect for embedded systems

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/yourusername/rads.git
cd rads

# Build from source
make

# Install system-wide
sudo make install
```

### Your First RADS Program

Create `hello.rads`:

```rads
blast main() {
    echo("Hello, RADS World! 🚀");
}
```

Run it:

```bash
rads hello.rads
```

## Example Programs

### HTTP Server (Raspberry Pi Ready!)

```rads
import net;

async blast handle_request(stream req) -> str {
    return "<h1>RADS Server Running! 🚀</h1>";
}

async blast main() {
    turbo server = net.http_server("0.0.0.0", 8080);
    echo("Server running on http://localhost:8080");
    
    server.route("/", handle_request);
    await server.serve();
}
```

### Audio Converter

```rads
import media;

blast main() {
    turbo audio = media.load_audio("input.mp3");
    audio.convert("output.wav", format: "wav", bitrate: 320);
    echo("Conversion complete! 🎵");
}
```

### Image Processor

```rads
import media;

blast main() {
    turbo img = media.load_image("photo.jpg");
    img.resize(800, 600);
    img.apply_filter("vintage");  // That 90s look!
    img.save("output.png");
    echo("Image processed! 📸");
}
```

## Language Highlights

### Turbo Variables (Stack-Optimized)

```rads
turbo i32 speed = 9000;  // Fast stack allocation
turbo f64 pi = 3.14159;
turbo bool radical = true;
```

### Async/Await for Servers

```rads
async blast fetch_data(str url) -> str {
    turbo response = await http.get(url);
    return await response.text();
}
```

### Structs & Enums

```rads
struct Player {
    str name;
    i32 score;
    f32 health;
}

enum GameState {
    MENU,
    PLAYING,
    PAUSED
}
```

## Plugin System (RPM)

Install plugins just like pip:

```bash
# Install a plugin
rads rpm install json-parser

# List installed plugins
rads rpm list

# Remove a plugin
rads rpm remove json-parser
```

Use in your code:

```rads
plugin "json-parser";

blast main() {
    turbo obj = json.parse('{"name": "RADS"}');
    echo(obj.name);
}
```

## Performance

RADS is designed for speed:

- ⚡ **2-5x slower than C** (still incredibly fast!)
- 🚀 **10-50x faster than Python**
- 💾 **Minimal memory overhead**
- 🔥 **< 10ms startup time**

Perfect for:
- Raspberry Pi servers running 24/7
- Real-time media processing
- High-throughput network services
- Embedded systems

## Documentation

- 📖 [Language Specification](docs/LANGUAGE_SPEC.md)
- 🎓 [Tutorial](docs/TUTORIAL.md) *(coming soon)*
- 📚 [Standard Library Reference](docs/STDLIB.md) *(coming soon)*
- 🔌 [Plugin Development Guide](docs/PLUGINS.md) *(coming soon)*

## Project Status

**Current Version**: 0.1.0-alpha (In Development)

### Roadmap

- [x] Language specification
- [/] Core compiler (lexer, parser, VM)
- [ ] Standard library
- [ ] Plugin system (RPM)
- [ ] Example programs
- [ ] Performance benchmarks
- [ ] Documentation
- [ ] Native code generation (LLVM)

## Contributing

RADS is in active development! Contributions welcome.

## License

MIT License - See [LICENSE](LICENSE) for details.

## Credits

Created with 💜 by the RADS community

Inspired by the aesthetic and energy of:
- 🎨 Frutiger Aero design
- 🎸 Nu-metal music
- 🎮 Early Xbox/PS2 era
- 🌐 90s/2000s internet culture

---

**"Keep it turbo, keep it radical!"** 🚀✨
