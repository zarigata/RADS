# RADS Package Registry - Package Author Guide

**Version:** 1.0
**Last Updated:** January 26, 2026

---

## Welcome, Package Authors!

This guide will help you create, publish, and maintain RADS packages. Whether you're building a library, framework, or tool, we'll walk you through the process.

---

## Table of Contents

1. [Getting Started](#getting-started)
2. [Package Structure](#package-structure)
3. [Creating package.rads](#creating-package-rads)
4. [Versioning](#versioning)
5. [Testing](#testing)
6. [Publishing](#publishing)
7. [Maintenance](#maintenance)
8. [Best Practices](#best-practices)

---

## Getting Started

### Prerequisites

Before creating a RADS package, ensure you have:

1. **RADS Installed**
   ```bash
   rads --version  # Should show v0.0.5 or later
   ```

2. **GitHub Account**
   - Free account at https://github.com
   - Used for hosting and publishing

3. **Package Registry Access**
   - Login at: https://registry.rads-lang.org/login
   - GitHub OAuth authentication

4. **Development Environment**
   - Code editor (VS Code, Vim, etc.)
   - Git for version control
   - Testing setup

### Choosing a Package Type

Decide what kind of package you're creating:

**Library/Framework**
- Reusable code for other developers
- Examples: database connectors, HTTP clients, utility functions
- Target: Other RADS developers

**Application/Tool**
- Standalone programs or utilities
- Examples: CLI tools, build tools, converters
- Target: End users

**Plugin**
- Extends RADS functionality
- Examples: database drivers, media codecs, web server middleware
- Target: RADS runtime

---

## Package Structure

### Recommended Directory Structure

```
my-rads-package/
├── package.rads              # Package manifest (REQUIRED)
├── README.md                 # Package documentation (REQUIRED)
├── LICENSE                    # License file (RECOMMENDED)
├── src/                      # Source code
│   ├── main.rads            # Package entry point
│   └── ...                   # Other source files
├── tests/                     # Test files
│   └── test_basic.rads       # Test examples
├── examples/                  # Usage examples
│   └── example.rads          # Example usage
└── .github/
    └── workflows/
        └── publish-to-registry.yml  # Auto-publish workflow
```

### Required Files

#### package.rads

```rads
name = "my-package"
version = "1.0.0"
description = "A brief description of your package"
author = "your-github-username"
```

#### README.md

Minimum README contents:

```markdown
# My RADS Package

A brief description.

## Installation

\`\`\`bash
rstar install my-package
\`\`\`

## Usage

\`\`\`rads
import my_package

blast main() {
    my_package.do_something();
}
\`\`\`

## License

MIT License - see LICENSE file
```

---

## Creating package.rads

### Required Fields

**name** - Package identifier
- Lowercase letters, numbers, hyphens only
- Maximum length: 100 characters
- Examples: `database-sqlite`, `web-framework`, `util-logger`

**version** - Semantic version
- Format: `MAJOR.MINOR.PATCH`
- Examples: `1.0.0`, `2.3.1`, `10.20.30`
- Must be valid semver

**description** - Package summary
- One or two sentences
- Maximum length: 200 characters
- Describe what the package does

**author** - Package maintainer
- Must match GitHub username
- Used for publishing authorization

### Optional Fields

**displayName** - Human-readable name
- Can include spaces, capitalization
- Defaults to `name` if omitted
- Example: "My Awesome Lib" vs "my-awesome-lib"

**homepage** - Documentation URL
- Links to package documentation website
- Helps users learn more
- Example: `https://my-package-docs.com`

**license** - License identifier
- Must be valid SPDX identifier
- Default: `MIT` if omitted
- Examples: `Apache-2.0`, `GPL-3.0`, `BSD-3`, `ISC`

**keywords** - Search terms
- Comma-separated or array
- Maximum 10 keywords
- Help users discover your package
- Example: `database, sql, sqlite, crud, orm`

**category** - Package category
- One of: `database`, `web`, `utils`, `media`, `networking`, `testing`, `cli`, `other`
- Used for filtering in registry
- Default: `other` if omitted

### Complete Example

```rads
name = "database-sqlite"
displayName = "RADS SQLite Database Library"
version = "1.2.0"
description = "Full-featured SQLite3 database connector for RADS with async I/O support"
author = "rads-dev"
homepage = "https://rads-sqlite-docs.dev"
license = "MIT"
keywords = ["database", "sqlite", "sql", "async", "crud"]
category = "database"
```

---

## Versioning

### Semantic Versioning (Semver)

Follow semver for version numbers:

**Format:** `MAJOR.MINOR.PATCH`

- **MAJOR** (x.0.0): Incompatible API changes
- **MINOR** (x.y.0): Backwards-compatible features
- **PATCH** (x.y.z): Backwards-compatible bug fixes

**Examples:**
- `1.0.0` → Initial release
- `1.1.0` → New feature (backward compatible)
- `1.1.1` → Bug fix (backward compatible)
- `2.0.0` → Breaking changes

### When to Increment

**MAJOR:**
- Removing or changing public APIs
- Making incompatible changes
- Major rewrites

**MINOR:**
- Adding new public APIs
- Deprecating old APIs (with migration path)
- New features without breaking changes

**PATCH:**
- Bug fixes
- Non-functional changes
- Documentation updates
- Performance improvements

### Pre-releases

Mark pre-release versions:

```rads
version = "1.0.0-alpha.1"
version = "1.0.0-beta.2"
version = "1.0.0-rc.1"
```

**Suffixes:**
- `alpha` - Early development, may break
- `beta` - Feature complete, testing needed
- `rc` (release candidate) - Production-ready, final testing

### Release Tags

Use semver-compliant tags for GitHub Releases:

```bash
# Correct
v1.0.0
v2.3.1
v1.0.0-alpha.1

# Incorrect
1.0.0
v1.0
version-1.0.0
```

---

## Testing

### Writing Tests

Create test files in `tests/` directory:

```rads
// tests/test_basic.rads
import "src/main.rads"

blast test_database_connection() {
    turbo result = main.rads.connect(":memory:");
    
    if (result.is_ok()) {
        echo("✅ Database connection test passed");
    } else {
        echo("❌ Database connection test failed");
    }
}

blast test_query_execution() {
    // Test query functionality
}

blast test_transaction_handling() {
    // Test transactions
}
```

### Running Tests

Use the RADS `test` keyword (if implemented):

```bash
# Run all tests
rads tests/*.rads

# Run specific test
rads tests/test_basic.rads

# Test with coverage (future feature)
rads tests/*.rads --coverage
```

### Test Coverage Goals

- **Unit Tests:** Test individual functions
- **Integration Tests:** Test component interactions
- **Edge Cases:** Boundary conditions, error handling
- **Documentation Tests:** Verify examples work

---

## Publishing

### Automated Publishing (Recommended)

**Step 1: Add Publish Workflow**

Create `.github/workflows/publish-to-registry.yml`:

```yaml
name: Publish to RADS Registry

on:
  release:
    types: [published]

jobs:
  publish-to-registry:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Validate and Publish
        run: |
          # Workflow handles validation and publishing
          # See full docs at:
          # https://github.com/zarigata/rads/tree/main/.github/workflows
```

**Step 2: Configure GitHub Secret**

Add `GH_REGISTRY_TOKEN` to your repository:
1. Generate PAT at: https://github.com/settings/tokens
2. Permissions: `repo:read`, `issues:write`
3. Add as secret in repository settings

**Step 3: Update and Push**

```bash
# Update version
echo 'version = "1.0.0"' > package.rads
git add package.rads
git commit -m "Bump version to 1.0.0"
git push origin main
```

**Step 4: Create GitHub Release**

```bash
# Using GitHub CLI
gh release create v1.0.0 \
  --title "v1.0.0" \
  --notes "Initial release" \
  --target main
```

**Step 5: Automatic Publishing**

- Workflow triggers automatically
- Validates package.rads
- Creates package archive
- Updates registry database
- Package appears on registry.rads-lang.org

### Manual Publishing (Alternative)

If automated publishing fails or you prefer manual:

**Step 1: Login to Registry**

1. Visit: https://registry.rads-lang.org/login
2. Click "Sign in with GitHub"
3. Authorize RADS Registry app

**Step 2: Navigate to Publish Page**

1. Click "Publish Package" in navbar
2. Fill package form

**Step 3: Fill Package Information**

Required fields:
- **Package Name:** Lowercase, hyphens, numbers
- **Version:** Semver format (x.y.z)
- **Description:** One or two sentences
- **Repository URL:** GitHub repository link
- **README:** Full package documentation (Markdown)

Optional fields:
- **Display Name:** Human-readable package title
- **Homepage:** Documentation website URL
- **Category:** Dropdown selection
- **License:** SPDX identifier (default: MIT)
- **Keywords:** Comma-separated terms

**Step 4: Submit and Verify**

1. Click "Publish Package" button
2. Wait for success message
3. Visit package detail page
4. Verify all information displays correctly

---

## Maintenance

### Updating Packages

**For Bug Fixes (Patch):**
```rads
# Increment PATCH version
version = "1.0.1"

# Update package.rads
git add package.rads
git commit -m "Fix: fix null pointer crash"
git push origin main

# Create release
gh release create v1.0.1 --notes "Fix crash when database is null"
```

**For New Features (Minor):**
```rads
# Increment MINOR version
version = "1.1.0"

# Update package.rads
git add package.rads
git commit -m "Feat: add async transaction support"
git push origin main

# Create release
gh release create v1.1.0 --notes "Add async transaction support"
```

**For Breaking Changes (Major):**
```rads
# Increment MAJOR version
version = "2.0.0"

# Update package.rads
git add package.rads
git commit -m "BREAKING CHANGE: Redesign API"
git push origin main

# Create release
gh release create v2.0.0 --notes "Breaking: API redesign"
```

### Responding to Issues

**Best Practices:**
1. **Acknowledge Quickly:** Respond within 24-48 hours
2. **Be Respectful:** Assume good intentions
3. **Ask for Details:** Request reproduction steps, environment
4. **Use Issue Templates:** If available, fill them out
5. **Label Issues:** Use labels for tracking (bug, enhancement, question)
6. **Link to Issues:** Reference related issues
7. **Close with Commit:** Close issues with fix commit reference

### Security Updates

**Vulnerability Disclosure:**
1. **Private Disclosure:** Report privately first
2. **Fix Timeline:** Set reasonable deadline (7-14 days)
3. **Coordinated Release:** Coordinate disclosure
4. **Patch Updates:** Release patch versions promptly
5. **Credit:** Credit vulnerability reporter
6. **Public Disclosure:** Announce after fix is released

**Example Timeline:**
- Day 0: Vulnerability reported privately
- Day 2: Team confirms vulnerability
- Day 7: Fix prepared, release planned
- Day 14: Patch released, public disclosure
- Day 15: Users encouraged to update

---

## Best Practices

### 1. Documentation

**Write Great README:**
```markdown
# Package Name

[Logo or Image]

Brief description (1-2 sentences).

## Features

- Feature 1
- Feature 2
- Feature 3

## Installation

\`\`\`bash
rstar install your-package
\`\`\`

## Quick Start

\`\`\`rads
import your_package

blast main() {
    // 5-minute example
}
\`\`\`

## API Reference

[Link to full API documentation]

## Examples

More detailed examples...

## License

This project is licensed under the MIT License.
\`\`\`

### Documentation Principles

- **Start Simple:** Installation and basic usage first
- **Be Complete:** Cover all major features
- **Provide Examples:** Real-world usage scenarios
- **Keep Updated:** Update docs with code changes
- **Be Clear:** Avoid jargon, explain concepts

### 2. API Design

**Stable Public API:**
- Document all public functions
- Maintain backward compatibility
- Use semantic versioning for breaking changes
- Provide migration guides for major versions

**Consistent Naming:**
- Use descriptive function names
- Follow RADS naming conventions
- Comment complex logic
- Provide type annotations

**Error Handling:**
- Return error objects or use Result types
- Document all error conditions
- Provide helpful error messages
- Handle edge cases gracefully

### 3. Package Dependencies

**Best Practices:**
- **Minimize Dependencies:** Fewer deps = easier installation
- **Pin Versions:** Use specific versions for stability
- **Document Dependencies:** List in README
- **Handle Missing Deps:** Provide helpful error messages
- **Test with Common Dep Versions:** Ensure compatibility

**When to Avoid Dependencies:**
- Very small functionality
- Can be reimplemented easily
- Standard library alternatives exist

### 4. Performance

**Optimization Tips:**
- **Async I/O:** Use async/await for I/O operations
- **Minimize Allocations:** Reuse buffers, avoid unnecessary copies
- **Lazy Loading:** Load data only when needed
- **Cache Results:** Cache expensive operations
- **Profile:** Use profiling tools to find bottlenecks

### 5. Security

**Security Checklist:**

- [ ] Input validation on all public APIs
- [ ] Output encoding/sanitization
- [ ] Safe string handling (buffer sizes)
- [ ] SQL injection prevention (use parameterized queries)
- [ ] Path traversal prevention
- [ ] Memory safety checks
- [ ] No hardcoded credentials
- [ ] Secure random number generation
- [ ] Rate limiting where appropriate
- [ ] Security audit of dependencies

### 6. Metadata Quality

**Improve Discoverability:**

- **Descriptive Name:** Make it clear what package does
- **Accurate Category:** Choose correct category
- **Relevant Keywords:** Use search terms users would use
- **Complete Description:** Describe purpose, features, use cases
- **Multiple Keywords:** Provide 5-10 relevant terms
- **Homepage Link:** Point to comprehensive documentation

---

## Publishing Checklist

Before publishing your package, verify:

### package.rads
- [ ] name follows naming conventions
- [ ] version is valid semver
- [ ] description is clear and concise
- [ ] author matches your GitHub username
- [ ] Optional fields are accurate

### Documentation
- [ ] README.md exists and is complete
- [ ] Installation instructions are clear
- [ ] Usage examples are provided
- [ ] API documentation (if applicable)
- [ ] Examples actually work

### Testing
- [ ] All tests pass
- [ ] Edge cases are covered
- [ ] Examples tested
- [ ] Installation tested on clean system

### Publishing
- [ ] GitHub release workflow added
- [ ] GitHub secret configured
- [ ] Version incremented correctly
- [ ] Release notes are helpful

### After Publishing
- [ ] Package appears on registry
- [ ] Package metadata is correct
- [ ] Installation works end-to-end
- [ ] Users can find package via search

---

## Examples and Templates

### Minimal Package Template

```
minimal-package/
├── package.rads
├── README.md
├── src/
│   └── main.rads
└── tests/
    └── test_basic.rads
```

**package.rads:**
```rads
name = "minimal-package"
version = "1.0.0"
description = "A minimal RADS package template"
author = "your-github-username"
```

### Library Package Template

```
my-library/
├── package.rads
├── README.md
├── LICENSE
├── src/
│   ├── api.rads
│   └── utils.rads
├── examples/
│   └── example.rads
└── tests/
    └── test_api.rads
```

### CLI Tool Template

```
my-cli/
├── package.rads
├── README.md
├── src/
│   ├── main.rads
│   ├── commands.rads
│   └── config.rads
├── examples/
│   └── usage.rads
└── tests/
    └── test_commands.rads
```

---

## Community and Support

### Getting Help

**Documentation:**
- User Guide: [USER_GUIDE.md](USER_GUIDE.md)
- API Reference: [API_REFERENCE.md](API_REFERENCE.md)
- CLI Guide: [CLI_GUIDE.md](CLI_GUIDE.md)

**Community:**
- GitHub Issues: https://github.com/zarigata/rads/issues
- GitHub Discussions: https://github.com/zarigata/rads/discussions
- Registry Support: https://registry.rads-lang.org/docs

### Contributing

**Ways to Contribute:**
1. Report bugs and issues
2. Suggest new features
3. Improve documentation
4. Submit pull requests
5. Share examples
6. Help other users

**Code of Conduct:**
- Be respectful and inclusive
- Focus on what is best for the project
- Assume good intentions
- Constructive feedback welcome

---

## Summary

This guide covers everything you need to create, publish, and maintain RADS packages successfully.

**Key Takeaways:**
- ✅ Follow semver for versioning
- ✅ Write comprehensive documentation
- ✅ Test thoroughly before publishing
- ✅ Use automated publishing workflow
- ✅ Maintain packages responsibly
- ✅ Engage with the community

**Next Steps:**
1. Create your package repository
2. Set up package structure
3. Write package.rads and README.md
4. Implement core functionality
5. Write and run tests
6. Publish to RADS Registry

---

**Last Updated:** January 26, 2026
**Version:** 1.0

*"Happy packaging! Build amazing things for the RADS community!"* 📦🚀
