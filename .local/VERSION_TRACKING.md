# 📍 VERSION TRACKING - LOCAL ONLY

**Purpose:** Central version management system for RADS project
**Usage:** For AI assistants and developers managing releases
**Status:** INTERNAL - DO NOT PUBLISH

---

## 📊 Current Version Status

| Version | Codename | Status | Release Date | Archive Location |
|---------|-----------|--------|--------------|-----------------|
| 0.0.7 | HYPERSONIC | 🔜 Planned | March 2026 (estimated) | N/A |
| 0.0.6 | FIGHTER JET | ✅ CURRENT | January 27, 2026 | Root directory |
| 0.0.5 | CHAMELEON | ✅ Previous | January 19, 2026 | `release/rads-0.0.5/` |
| 0.0.4 | Constellation | ✅ Archived | January 13, 2026 | `release/` (archived) |
| 0.0.3 | Butterfly | ✅ Archived | January 11, 2026 | `release/` (archived) |
| 0.0.2 | Core Language | ✅ Archived | January 11, 2026 | `release/` (archived) |
| 0.0.1 | Web Framework | ✅ Archived | January 10, 2026 | `release/` (archived) |

---

## 🎯 Next Version Information

### Planned Version: 0.0.7
**Target Codename:** HYPERSONIC
**Theme:** Ultimate Speed, Quantum Leap, Beyond Mach
**Target Release Date:** March 2026 (estimated)
**Planning Document:** `V0.0.7_HYPERSONIC_PLAN.md` (13 numbered features)
**Key Features Planned:**
- 1. LLVM JIT Compilation (10-50x speedup on hot paths)
- 2. Advanced Garbage Collection (generational, <5ms pauses)
- 3. Advanced LSP with Code Actions (refactoring, quick fixes)
- 4. Enhanced Debugger (conditional breakpoints, watch expressions)
- 5. ECMAScript Transpiler (JavaScript → RADS)
- 6. Type System Enhancements (optional types, generics, unions)
- 7. Comprehensive stdlib Expansion (50+ new functions)
- 8. Testing Framework Improvements (parallel, filtering, mocking)
- 9. Error Handling Improvements (try-catch, custom error types)
- 10. Complete API Documentation (auto-generated, inline help)
- 11. Build System Improvements (incremental, faster compilation)
- 12. Package Manager Integration (rstar CLI, registry)
- 13. WebAssembly Support (experimental, browser support)

---

## 📂 Files Requiring Version Updates

### MUST UPDATE for EVERY Version Change

| File | Current Version Location | Update Command | Notes |
|-------|------------------------|-----------------|-------|
| `README.md` | Line 9: `version-0.0.6` | Update version badge | Line ~85, ~92 in feature lists |
| `CHANGELOG.md` | Section header `[0.0.6]` | Add new section, update dates | Add at top, move others down |
| `V{VERSION}_PLAN.md` | Root directory files | Create new plan document | Template in V0.0.6_FIGHTER_JET_PLAN.md |
| `V{VERSION}_SUMMARY.md` | Root directory files | Create new summary | Use CHAMELEON summary as template |
| `src/core/main.c` | Version string | Search: `VERSION` or `"0.0.6"` | Update version string in binary |
| `tools/rads-lsp/Cargo.toml` | Version field | Line: `version = "0.0.6"` | Update Rust package version |
| `install.sh` | Version string | Search: `0.0.6` | Update download URLs |
| `examples/README.md` | Version references | Section: v0.0.6 | Update feature examples section |

### UPDATE for Release Cycle

| File | Update Frequency | Notes |
|-------|------------------|-------|
| `CHANGELOG.md` | Every release | Add new version section |
| `V{VERSION}_SUMMARY.md` | Every release | Create new summary document |
| `docs/releases.html` | Every release | Add release notes to website |
| `docs/V{VERSION}_QUICK_REFERENCE.md` | Every release | Create quick reference guide |

### VERIFY Past Version Data (5 versions back)

| Version | Documentation Present? | Location | Action |
|---------|---------------------|-----------|--------|
| 0.0.6 | ✅ Yes | `V0.0.6_FIGHTER_JET_SUMMARY.md` | N/A |
| 0.0.5 | ✅ Yes | `V0.0.5_CHAMELEON_SUMMARY.md` | N/A |
| 0.0.4 | ✅ Yes | `V0.0.4_STATUS.md` | N/A |
| 0.0.3 | ✅ Yes | `docs/V0.0.3_BUTTERFLY_RELEASE.md` | N/A |
| 0.0.2 | ✅ Yes | `docs/V0.0.2_COMPLETE.md` | N/A |
| 0.0.1 | ✅ Yes | `docs/V0.0.1_COMPLETE.md` | N/A |

**Action:** All past 5 versions documented. Complete archive.

---

## 🔍 Finding Version Information

### Search Patterns

```bash
# In root directory files
grep -r "0\.0\.[0-9]" *.md

# In CHANGELOG
grep "^\## \[0\.0\." CHANGELOG.md

# In C source files
grep -r "VERSION\|\"0\.0\.[0-9]\"" src/

# In Makefile
grep "VERSION\|0\.0\.[0-9]" Makefile

# In HTML docs
grep "0\.0\.[0-9]" docs/*.html
```

### Key Locations

1. **Root Level:**
   - `V0.0.6_FIGHTER_JET_PLAN.md`
   - `V0.0.6_FIGHTER_JET_SUMMARY.md`
   - `README.md`
   - `CHANGELOG.md`

2. **Source Code:**
   - `src/core/main.c` - Version displayed by `--version`
   - `src/core/ast.h` - If version constant exists

3. **Build Files:**
   - `Makefile` - If version variable exists
   - `tools/rads-lsp/Cargo.toml` - LSP version
   - `install.sh` - Download URLs

4. **Documentation:**
   - `docs/index.html` - Homepage version
   - `docs/releases.html` - Release notes
   - `docs/README.md` - Documentation hub

5. **Archives:**
   - `release/rads-0.0.5/` - Previous release
   - `release/rads-0.0.5/archive/` - Archive folder

---

## 🔄 Version Update Process

### Step-by-Step Checklist

1. **Prepare New Version**
   - [ ] Determine new version number (e.g., 0.0.7)
   - [ ] Choose codename (theme-based)
   - [ ] Create `V0.0.7_PLAN.md` with roadmap
   - [ ] Update `VERSION_TRACKING.md` with planned version

2. **Update Source Code**
   - [ ] Update `src/core/main.c` version string
   - [ ] Update `tools/rads-lsp/Cargo.toml` version
   - [ ] Update any other build files with version

3. **Update Documentation**
   - [ ] Update `README.md` version badge and links
   - [ ] Update `examples/README.md` version references
   - [ ] Create `V0.0.7_PLAN.md` with full roadmap

4. **During Development**
   - [ ] Update `CHANGELOG.md` incrementally with changes
   - [ ] Update plan document as features are implemented
   - [ ] Keep track of breaking changes

5. **Prepare Release**
   - [ ] Create `V0.0.7_{CODENAME}_SUMMARY.md`
   - [ ] Finalize `CHANGELOG.md` entry for 0.0.7
   - [ ] Update version history in documentation
   - [ ] Create release notes for GitHub

6. **Release**
   - [ ] Create GitHub release with tag
   - [ ] Upload release assets
   - [ ] Trigger GitHub Actions if needed
   - [ ] Update download links in `README.md`

7. **Post-Release**
   - [ ] Archive previous release data to `release/` folder
   - [ ] Update website documentation
   - [ ] Create quick reference guide
   - [ ] Update this `VERSION_TRACKING.md` with new current version

---

## 🔗 GitHub Actions Integration

### Release Workflow

**File:** `.github/workflows/publish-to-registry.yml`

**Trigger:** On GitHub release (published)

**Tasks:**
1. Validate package metadata
2. Create release assets
3. Upload to registry repository

**To Use:**
```bash
# Create release tag
git tag v0.0.7
git push origin v0.0.7

# Create release on GitHub
gh release create v0.0.7 --title "v0.0.7 {Codename}" --notes-file RELEASE_NOTES.md

# This triggers the workflow automatically
```

### Website Deployment

**File:** `.github/workflows/gh-pages.yml`

**Trigger:** Push to main branch

**Tasks:**
1. Deploy docs/ to GitHub Pages
2. Update version information

**To Use:**
Website updates automatically on push to main branch. Ensure docs/ files are updated before pushing.

---

## 🗂️ Release Archive Management

### Archive Structure

```
release/
├── rads-0.0.5/              # Current previous release
│   ├── archive/                  # Old files from 0.0.5
│   ├── API_REFERENCE.md
│   └── README-MASK.md
├── rads-0.0.5-BIN-CHECKSUMS.txt
├── rads-0.0.5-SHA256.txt
├── rads-0.0.5-linux-x86_64.tar.gz
└── rads-0.0.1.0-alpha-linux-x86_64.tar.gz
```

### Cleanup Policy

**Delete These After 3 Versions:**
- Binary archives older than 3 versions
- Checksum files for old versions
- Alpha/beta archives after stable release

**Keep These:**
- All release summary documents (V*.md)
- CHANGELOG.md (contains all history)
- README.md for each release folder
- Archive subfolders with historical notes

### Cleanup Commands

```bash
# Identify old release archives
ls -la release/ | grep -E "0\.0\.[1-3]"

# Delete old binary archives (keep last 3)
rm -f release/rads-0.0.1.0-alpha-linux-x86_64.tar.gz
rm -f release/rads-0.0.1.0-alpha-linux-x86_64.tar.gz.sha256

# Keep docs, delete old binaries
cd release/
for version in 0.0.1 0.0.2 0.0.3; do
    if [ -d "rads-$version" ]; then
        # Keep docs, delete binaries
        find rads-$version/ -maxdepth 1 -type f -name "*.tar.gz" -delete
        find rads-$version/ -maxdepth 1 -type f -name "*.txt" -delete
    fi
done
```

---

## 📊 Version Information Matrix

### By Version

| Version | Codename | Release Date | Key Features | Files to Keep | Files to Archive |
|---------|-----------|--------------|--------------|----------------|-----------------|
| 0.0.6 | FIGHTER JET | 2026-01-27 | Bytecode, LSP, Debugger, Profiler, WebSocket, GraphQL | All (current) |
| 0.0.5 | CHAMELEON | 2026-01-19 | Python transpilation, AST | Release folder |
| 0.0.4 | Constellation | 2026-01-13 | Distributed computing | Release folder |
| 0.0.3 | Butterfly | 2026-01-11 | Database, Testing | Release folder |
| 0.0.2 | Core Language | 2026-01-11 | RGB REPL, Structs, Enums | Release folder |
| 0.0.1 | Web Framework | 2026-01-10 | HTTP, Routing, Templates | Release folder |

### By Feature

| Feature | First Version | Files Containing | Notes |
|---------|--------------|-------------------|-------|
| HTTP Server | 0.0.1 | V0.0.1_COMPLETE.md, CHANGELOG | |
| REPL | 0.0.1 | V0.0.1_COMPLETE.md, docs/REPL_* | |
| Database | 0.0.3 | V0.0.3_BUTTERFLY_RELEASE.md | |
| Testing | 0.0.3 | V0.0.3_BUTTERFLY_RELEASE.md | |
| Distributed | 0.0.4 | docs/constellation/* | |
| Python Transpiler | 0.0.5 | V0.0.5_CHAMELEON_SUMMARY.md | |
| Bytecode VM | 0.0.6 | V0.0.6_FIGHTER_JET_SUMMARY.md | |
| LSP Server | 0.0.6 | V0.0.6_FIGHTER_JET_SUMMARY.md | |
| Debugger | 0.0.6 | V0.0.6_FIGHTER_JET_SUMMARY.md | |
| Profiler | 0.0.6 | V0.0.6_FIGHTER_JET_SUMMARY.md | |
| WebSocket | 0.0.6 | V0.0.6_FIGHTER_JET_SUMMARY.md | |
| GraphQL | 0.0.6 | V0.0.6_FIGHTER_JET_SUMMARY.md | |

---

## 🎯 AI Assistant Guidelines

### When You're Asked to Update Version

1. **READ THIS FILE FIRST** - Understand current state
2. **CHECK VERSION TRACKING** - See what's current
3. **UPDATE MATRIX** - Mark version as previous, add new one
4. **FOLLOW CHECKLIST** - Use step-by-step process above
5. **VERIFY PAST VERSIONS** - Ensure 5 versions of data present
6. **ARCHIVE OLD** - Move to appropriate folders
7. **CLEANUP** - Remove old environment files

### When You're Asked to Create Release

1. **CREATE SUMMARY** - New V{VERSION}_{CODENAME}_SUMMARY.md
2. **UPDATE CHANGELOG** - Add complete version entry
3. **UPDATE README** - New version badges and links
4. **UPDATE WEBSITE** - docs/releases.html, docs/index.html
5. **ARCHIVE PREVIOUS** - Move to release/ folder
6. **CREATE TAG** - Git tag v{VERSION}
7. **GITHUB RELEASE** - Create release with notes
8. **VERIFY** - Check all files updated

### When You're Asked to Cleanup

1. **CHECK POLICY** - What's old (3+ versions)
2. **BACKUP DOCS** - Never delete .md documentation
3. **DELETE BINARIES** - Old .tar.gz, .zip files
4. **KEEP ARCHIVES** - Archive folders with historical notes
5. **VERIFY** - Ensure all 5 previous versions documented

---

## 📝 Quick Reference Commands

### Version Updates
```bash
# Find all version strings
grep -r "0\.0\.[0-9]" --include="*.md" --include="*.c" --include="*.html"

# Replace version (example: 0.0.6 → 0.0.7)
sed -i 's/0\.0\.6/0.0.7/g' README.md CHANGELOG.md

# In C files
sed -i 's/"0\.0\.6"/"0.0.7"/g' src/core/main.c
```

### Release Creation
```bash
# Create release tag
git tag -a v0.0.7 -m "Release v0.0.7"
git push origin v0.0.7

# Create release via GitHub CLI
gh release create v0.0.7 \
  --title "v0.0.7 {CODENAME}" \
  --notes-file RELEASE_NOTES.md
```

### Website Updates
```bash
# Update version in HTML
sed -i 's/0\.0\.6/0.0.7/g' docs/*.html

# Commit and push
git add docs/
git commit -m "docs: update to v0.0.7"
git push
```

### Cleanup
```bash
# Remove old release binaries
find release/ -name "*linux-x86_64.tar.gz" -mtime +90 -delete

# Compress old releases
cd release/
tar -czf archive-old.tar.gz rads-0.0.1 rads-0.0.2
rm -rf rads-0.0.1 rads-0.0.2
```

---

## 🔗 Related Files

- `RELEASE_CHECKLIST.md` - Detailed release checklist
- `WEBSITE_UPDATE_GUIDE.md` - Website update procedures
- `CLEANUP_GUIDE.md` - Cleanup policies and procedures
- `RELEASE_PROCESS.md` - Complete release workflow

---

**Last Updated:** January 27, 2026
**Maintained By:** AI Assistant Automation
**Next Review:** Before v0.0.7 release

---

*This file is for internal use only. Do not publish to public documentation.*
