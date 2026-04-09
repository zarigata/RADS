# RADS Standard Libraries

Official standard library packages for RADS. Each library is maintained, versioned, and independently testable.

## Available Libraries

| Library | Version | Description | Dependencies |
|---------|---------|-------------|--------------|
| [crypto](./crypto/) | 0.1.0 | Cryptographic utilities (hash, encrypt, sign) | None |
| [http](./http/) | 0.1.0 | Enhanced HTTP client/server utilities | net |
| [math-extra](./math-extra/) | 0.1.0 | Advanced math (trig, stats, linear algebra) | math |
| [testing](./testing/) | 0.1.0 | Advanced testing utilities (mocking, fixtures) | test |
| [logging](./logging/) | 0.1.0 | Structured logging with levels and outputs | None |
| [config](./config/) | 0.1.0 | Configuration file parsing (JSON, YAML, TOML) | json |
| [parsing](./parsing/) | 0.1.0 | Parser combinators and text processing | regex |
| [concurrency](./concurrency/) | 0.1.0 | Advanced async patterns (channels, workers) | async_utils |

## Installation

```bash
rstar install crypto
rstar install http
rstar install math-extra
```

Or install all at once:

```bash
rstar install rads-stdlib-all
```

## Usage

```rads
import crypto;

blast main() {
    turbo hash = crypto.sha256("hello world");
    echo(hash);
}
```

## Library Structure

Each library follows this structure:

```
library-name/
├── package.rads       # Package manifest
├── src/
│   └── lib.rads       # Main library source
├── tests/
│   └── test_*.rads    # Test files
├── examples/
│   └── example_*.rads # Usage examples
└── README.md           # Documentation
```

## Contributing

To contribute a new library:

1. Create a directory in `libraries/` with your library name
2. Implement the library following the structure above
3. Add comprehensive tests
4. Document all public functions
5. Submit a pull request

## License

MIT License - See [LICENSE](../LICENSE) for details.