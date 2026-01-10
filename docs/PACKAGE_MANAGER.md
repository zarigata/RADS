# RADStar Package Manager Guide

## What is RADStar?

**RADStar** (rstar) is RADS's built-in package manager for installing, managing, and running plugins and libraries. Think of it as `npm` for JavaScript or `pip` for Python - but for RADS.

## Quick Start

### Building RADStar

RADStar is automatically built when you build RADS:

```bash
make
```

This creates `bin/rstar` and a symlink `./rstar` for easy access.

### Basic Commands

```bash
# Show help
./rstar help

# List installed plugins
./rstar list

# Install a plugin (coming soon - full implementation)
./rstar install <plugin-name>

# Run a plugin
./rstar run <plugin-name>
```

## Package Format

RADS packages are described using a `package.rads` file:

```rads
name = "my-awesome-plugin"
version = "1.0.0"
dependencies = []
```

For plugins, you can also create a `plugin.radsmeta` file:

```
name = "my-plugin"
version = "1.0.0"
entry = "main.rads"
author = "Your Name"
description = "What this plugin does"
```

## Plugin Storage

Plugins are installed to `~/.rads/plugins/` by default. You can customize this with the `RADS_PLUGIN_HOME` environment variable:

```bash
export RADS_PLUGIN_HOME=/custom/path
./rstar list
```

## Current Status (v0.1.0-alpha)

RADStar is currently in skeleton/alpha stage with basic functionality:

### ✅ Implemented:
- Basic CLI structure
- `help` command
- `list` command (shows installed plugins)
- Plugin directory management
- Lock file support (plugins.lock)

### 🚧 Coming Soon:
- HTTP-based plugin registry
- `install` command with actual download
- `uninstall` command
- `update` command
- Dependency resolution
- Version management
- Checksum verification
- Package signing

## Development

### Plugin Lock File

Installed plugins are tracked in `~/.rads/plugins/plugins.lock`:

```
plugin-name|version|entry-file|install-path
media-kit|1.0.0|main.rads|/home/user/.rads/plugins/media-kit
```

### Directory Structure

```
~/.rads/
└── plugins/
    ├── plugins.lock          # Installed plugins registry
    ├── plugin-name-1/        # Plugin directory
    │   ├── plugin.radsmeta   # Plugin metadata
    │   └── main.rads         # Plugin entry point
    └── plugin-name-2/
        └── ...
```

## Creating a Plugin

1. Create your plugin code in a directory
2. Add a `plugin.radsmeta` file:
   ```
   name = "my-plugin"
   version = "1.0.0"
   entry = "main.rads"
   ```
3. Add your main RADS file (e.g., `main.rads`)
4. Share your plugin repository

## Future Enhancements

The v1.0.0 release will include:
- Public plugin registry (similar to npmjs.com)
- Search functionality
- Plugin ratings and downloads tracking
- Automated testing for published plugins
- Plugin templates and generators
- Integration with GitHub/GitLab
- Private plugin repositories

## Contributing

Want to help build RADStar? Check out:
- `tools/rstar/rstar.c` - Main implementation
- `tools/rstar/rstar_test.c` - Test suite

The code is designed to be simple and hackable. Each command is implemented in a small, focused function.

## Examples

### Example Plugin: Hello World

Create `hello-plugin/plugin.radsmeta`:
```
name = "hello-world"
version = "1.0.0"
entry = "hello.rads"
```

Create `hello-plugin/hello.rads`:
```rads
blast main() {
    echo("Hello from RADStar plugin!");
}
```

Install manually for testing:
```bash
mkdir -p ~/.rads/plugins/hello-world
cp hello-plugin/* ~/.rads/plugins/hello-world/
echo "hello-world|1.0.0|hello.rads|$HOME/.rads/plugins/hello-world" >> ~/.rads/plugins/plugins.lock
```

Run it:
```bash
./rstar run hello-world
```

## Troubleshooting

### "Error: could not resolve plugin root path"

Make sure your `HOME` environment variable is set, or set `RADS_PLUGIN_HOME`:
```bash
export RADS_PLUGIN_HOME=/path/to/plugins
```

### "No plugins.lock found"

This is normal if you haven't installed any plugins yet. Install your first plugin to create the lock file.

## Philosophy

RADStar follows these principles:
- **Fast**: Written in C, minimal overhead
- **Simple**: Easy to understand and modify
- **Reliable**: Lock files prevent version conflicts
- **Transparent**: All operations are visible and reversible
- **Hackable**: Clear code structure for contributors

---

**RADStar** - Making RADS plugins radically simple! ⭐
