# RADS Standard Library Docs

Module-by-module reference for the RADS stdlib. Python-parity tracking lives in
[GAPLIST.md](GAPLIST.md) (created in Phase 1; until then these pages are the source of truth).

| Module | What it covers |
|---|---|
| [copy](copy.md) | shallow/deep copy helpers |
| [fnmatch](fnmatch.md) | Unix shell-style wildcard matching |
| [functools](functools.md) | function tools (partials, wrappers) |
| [http](http.md) | HTTP client primitives |
| [math](math.md) | math constants and functions |
| [operator](operator.md) | function forms of operators |
| [queue](queue.md) | FIFO/LIFO queues |
| [string](string.md) | string constants and helpers |
| [struct](struct.md) | byte packing/unpacking |
| [time](time.md) | time reading, formatting, sleep |

Other stdlib modules are implemented in C sources under `src/stdlib/` and
documented inside those files; migrate them here as they get touched.
