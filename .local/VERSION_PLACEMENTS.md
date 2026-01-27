# 🔍 VERSION PLACEMENTS TRACKING
**Purpose:** Complete inventory of all files containing version strings
**Usage:** For automated version updates across the codebase
**Status:** INTERNAL - DO NOT PUBLISH

---

## 📊 Current Version: 0.0.6

---

## 🎯 CRITICAL FILES (MUST UPDATE EVERY VERSION)

### 1. User-Facing Documentation
| File | Line(s) | Pattern | Update Method |
|------|-----------|----------|---------------|
| `README.md` | 9 | `version-0.0.6` | Update badge |
| `README.md` | 25, 32 | `v0.0.6 "FIGHTER JET"` | Update feature section |
| `README.md` | 92, 95-96 | `rads-0.0.6` | Update download URLs |
| `CHANGELOG.md` | 7 | `[0.0.6]` | Add new section |
| `CHANGELOG.md` | Multiple | Date `2026-01-27` | Update release date |

### 2. Source Code Version Strings
| File | Line(s) | Pattern | Update Method |
|------|-----------|----------|---------------|
| `src/core/main.c` | TBD | `"0.0.6"` | Update version constant |
| `src/core/ast.h` | TBD | `"0.0.6"` | Update if exists |
| `tools/rads-lsp/Cargo.toml` | 3 | `version = "0.1.0"` | Update Rust version |

### 3. Version-Specific Documents (CREATE NEW FOR EACH VERSION)
| File | Purpose | Template |
|------|---------|----------|
| `V0.0.7_{CODENAME}_PLAN.md` | Development roadmap | Copy from V0.0.6 |
| `V0.0.7_{CODENAME}_SUMMARY.md` | Release summary | Copy from V0.0.6 |
| `docs/V0.0.7_QUICK_REFERENCE.md` | Quick reference guide | Copy from v0.0.6 |

### 4. Package Registry
| File | Line(s) | Pattern | Update Method |
|------|-----------|----------|---------------|
| `rads-registry/package.json` | 3 | `"0.1.0"` | Update version |
| `package.rads` | TBD | `version = "0.0.6"` | Update if exists |

### 5. Build & Installation
| File | Line(s) | Pattern | Update Method |
|------|-----------|----------|---------------|
| `install.sh` | Multiple | `0.0.6` | Update URLs |
| `examples/README.md` | Multiple | `v0.0.6` | Update examples list |

---

## 🔍 SEARCH PATTERNS FOR VERSION STRINGS

### Command to Find All Version References
```bash
# Find all 0.0.x patterns in root directory
grep -r "0\.0\.[0-9]" --include="*.md" .

# Find version in C source files
grep -r "VERSION\|\"0\.0\.[0-9]\"" src/

# Find version in configuration files
grep -r "version.*=" --include="*.toml" --include="*.json" .

# Find version in package.rads files
grep -r "^version" --include="*.rads"
```

---

## 📋 VERIFICATION CHECKLIST

After updating to new version (e.g., 0.0.7), verify:

- [ ] README.md badge updated
- [ ] README.md feature sections updated
- [ ] README.md download URLs updated
- [ ] CHANGELOG.md has new version section
- [ ] CHANGELOG.md moved previous version down
- [ ] V{VERSION}_PLAN.md created
- [ ] V{VERSION}_SUMMARY.md created
- [ ] V{VERSION}_QUICK_REFERENCE.md created in docs/
- [ ] Source code version strings updated
- [ ] Cargo.toml version updated
- [ ] package.json version updated (if applicable)
- [ ] install.sh URLs updated
- [ ] examples/README.md updated

---

## 🔧 AUTOMATED REPLACEMENT COMMANDS

### Example: Update from 0.0.6 to 0.0.7

```bash
# Replace in Markdown files
find . -name "*.md" -type f -exec sed -i 's/0\.0\.6/0.0.7/g' {} \;

# Replace in C source files
find src/ -name "*.c" -name "*.h" -type f -exec sed -i 's/"0\.0\.6"/"0.0.7"/g' {} \;

# Replace in configuration files
find . -name "*.toml" -name "*.json" -type f -exec sed -i 's/0\.0\.6/0.0.7/g' {} \;

# Replace in shell scripts
find . -name "*.sh" -type f -exec sed -i 's/0\.0\.6/0.0.7/g' {} \;
```

---

## ⚠️ EXCLUDE FROM REPLACEMENT

These files should NOT be auto-updated:
- Historical documents (V0.0.1-*.md, V0.0.2-*.md, etc.)
- CHANGELOG.md historical sections
- Archive folders
- Session history documents

---

## 📝 NOTES

1. **Semantic Versioning:** RADS follows MAJOR.MINOR.PATCH (0.0.x)
2. **Codenames:** Each version has a thematic codename (e.g., FIGHTER JET)
3. **Release Cycle:** ~2-3 weeks between versions
4. **Backward Compatibility:** Maintained unless breaking changes announced

---

**Last Updated:** January 27, 2026
**Current Version:** 0.0.6
**Next Version:** 0.0.7 (TBD)
