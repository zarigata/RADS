# RADS LSP Server

Language Server Protocol implementation for RADS.

## Features

- **Code Completion**: Autocomplete for RADS keywords, standard library functions, and variables
- **Go to Definition**: Navigate to function and variable definitions
- **Hover Documentation**: Show function signatures and descriptions on hover
- **Document Symbols**: Outline view of functions and variables
- **Diagnostics**: Real-time error detection and suggestions

## Building

```bash
cd tools/rads-lsp
cargo build --release
```

## Usage

```bash
# Start the LSP server
cargo run --bin rads-lsp

# Install to PATH
cargo install --path .
```

## Editor Integration

### VS Code

Add to `settings.json`:
```json
{
  "rads-lsp.enable": true,
  "rads-lsp.path": "/path/to/rads-lsp"
}
```

### Vim/Neovim

Install `coc-rads` or configure `nvim-lspconfig`.

### JetBrains

Install the RADS plugin from the marketplace.

## LSP Capabilities

- Text Document Synchronization (Full)
- Completion (Trigger on `.`)
- Hover Provider
- Go to Definition
- Document Symbols
- Diagnostics

## Protocol

The server communicates using JSON-RPC over stdin/stdout.

## Implementation Notes

- Built with Tokio for async I/O
- Uses Tower for LSP protocol handling
- Implements RADS-specific completions and diagnostics
- Performance optimized with incremental updates
