# Security Policy

## Supported Versions

| Version | Supported          | Status |
| ------- | ------------------ | ------ |
| 0.0.11  | ✅ Active development | PULSAR |
| 0.0.10  | ✅ Supported | QUASAR |
| < 0.0.9 | ⚠️ Minimal support | Legacy |

## Reporting a Vulnerability

**DO NOT** open a public issue for security vulnerabilities.

Instead, please report security issues by:

1. **GitHub Security Advisory** (preferred)
   - Go to https://github.com/zarigata/RADS/security/advisories
   - Click "Report a vulnerability"
   - Fill in the details

2. **Email** (alternative)
   - Contact the maintainer via GitHub

### What to Include

- Description of the vulnerability
- Steps to reproduce
- Affected versions
- Potential impact
- Suggested fix (if available)

### Response Timeline

- **Initial response**: Within 48 hours
- **Status update**: Within 7 days
- **Fix timeline**: Depends on severity
  - Critical: 24-72 hours
  - High: 1 week
  - Medium: 2 weeks
  - Low: Next release

## Security Best Practices for RADS Users

### Installation

```bash
# Verify the install script before running
curl -fsSL https://raw.githubusercontent.com/zarigata/RADS/main/install.sh | less
# Then run
curl -fsSL https://raw.githubusercontent.com/zarigata/RADS/main/install.sh | bash
```

### Building from Source

- Always verify commit signatures when available
- Build in a clean environment
- Review build output for unexpected warnings

### Using RADS in Production

- Pin to specific versions in deployment
- Enable all compiler security flags
- Run with minimal privileges
- Validate all user input

## Known Security Considerations

### Language Runtime

- RADS uses libuv for async I/O (memory-safe patterns)
- SQLite queries should use parameterized statements (`db.query(sql, params)`)
- File operations should validate paths

### External Dependencies

| Dependency | Purpose | Security Notes |
|------------|---------|-----------------|
| libuv | Async I/O | Well-maintained, security-audited |
| sqlite3 | Database | Use parameterized queries |
| readline | REPL input | Standard library |

### Compiler Flags

RADS builds with security-hardening flags:
- `-fstack-protector-strong` - Stack protection
- `-Wformat -Wformat-security` - Format string protection
- `-D_FORTIFY_SOURCE=2` - Buffer overflow detection

## Security Changelog

### 2026-04-09
- Added SECURITY.md policy
- Added .gitattributes for consistent line endings
- Added compiler security flags to Makefile

---

*Last updated: 2026-04-09*