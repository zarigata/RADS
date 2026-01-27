# 🧹 ENVIRONMENT CLEANUP GUIDE
**Purpose:** Automated cleanup of old environments, temporary files, and archives
**Usage:** For maintaining clean repository structure
**Status:** INTERNAL - DO NOT PUBLISH

---

## 📋 CLEANUP POLICY

### What to Clean
1. **Old binary archives** (> 3 versions old)
2. **Temporary build files**
3. **Cache directories**
4. **Old checksum files**
5. **Alpha/beta artifacts** (after stable release)
6. **Environment test files**

### What to Keep
1. **All .md documentation** (NEVER DELETE)
2. **Current release artifacts**
3. **Last 3 versions of archives**
4. **Archive folders with historical notes**

---

## 🔍 CLEANUP TARGETS

### 1. Binary Archives
**Location:** `release/`
**Policy:** Keep only last 3 versions

```bash
# Find old release archives
ls -la release/ | grep -E "rads-0\.0\.[1-3].*\.tar\.gz"

# Old archives to delete:
# - rads-0.0.1-linux-x86_64.tar.gz
# - rads-0.0.1.0-alpha-linux-x86_64.tar.gz
# - rads-0.0.2-linux-x86_64.tar.gz (if > 3 versions old)
# - rads-0.0.3-linux-x86_64.tar.gz (if > 3 versions old)
```

**Cleanup Command:**
```bash
cd release/

# Delete old binary archives (keep last 3: 0.0.4, 0.0.5, 0.0.6)
rm -f rads-0.0.1*.tar.gz
rm -f rads-0.0.1*.tar.gz.sha256
rm -f rads-0.0.2*.tar.gz  # After v0.0.7
rm -f rads-0.0.2*.sha256     # After v0.0.8
rm -f rads-0.0.3*.tar.gz  # After v0.0.9
rm -f rads-0.0.3*.sha256     # After v0.1.0
```

### 2. Checksum Files
**Location:** `release/`
**Policy:** Delete checksums for old versions

```bash
# Old checksum files to delete:
# - rads-0.0.1-SHA256.txt
# - rads-0.0.1.0-alpha-linux-x86_64.tar.gz.sha256
# - rads-0.0.5-BIN-CHECKSUMS.txt (after v0.0.7)
```

### 3. Build Artifacts
**Locations:** Throughout codebase
**Policy:** Delete after successful build/release

```bash
# Common build artifacts
find . -name "*.o" -delete              # Object files
find . -name "*.a" -delete              # Static libraries
find . -type d -name "build" -exec rm -rf {} +  # Build directories
find . -type d -name "dist" -exec rm -rf {} +   # Distribution directories
find . -type d -name "target" -exec rm -rf {} +  # Rust target directories
```

### 4. Cache Directories
**Locations:** `.cache/`, `node_modules/`, etc.
**Policy:** Clear cache after major version changes

```bash
# Node.js cache
find . -type d -name "node_modules" -exec rm -rf {} + 2>/dev/null || true
rm -rf .cache

# Python cache (if applicable)
find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
find . -name "*.pyc" -delete

# C build cache
rm -rf .dSYM/
```

### 5. Temporary Files
**Locations:** Root, `src/`, `tests/`
**Policy:** Delete after testing

```bash
# Common temporary files
find . -name "*.tmp" -delete
find . -name "*.temp" -delete
find . -name "*.swp" -delete      # Vim swap files
find . -name "*~" -delete            # Backup files
find . -name ".DS_Store" -delete   # macOS metadata
```

### 6. Test/Environment Files
**Locations:** `tests/`, `.env*`
**Policy:** Keep recent tests, clean up old environments

```bash
# Old test environments
find . -name ".env.local" -delete
find . -name ".env.old" -delete
find . -name ".env.backup" -delete

# Keep .env and .env.example
```

---

## 🔄 AUTOMATED CLEANUP SCRIPT

### Shell Script: `cleanup.sh`
```bash
#!/bin/bash
# cleanup.sh - Automated environment cleanup

set -e

CURRENT_VERSION=$1
if [ -z "$CURRENT_VERSION" ]; then
    echo "Usage: $0 <CURRENT_VERSION>"
    echo "Example: $0 0.0.6"
    exit 1
fi

echo "🧹 Cleaning up environment for v${CURRENT_VERSION}..."

# 1. Calculate versions to keep (last 3)
KEEP_VERSIONS=$(seq $((CURRENT_VERSION - 3)) $CURRENT_VERSION)

# 2. Delete old binary archives
echo "📦 Cleaning old binary archives..."
cd release/
for version_dir in rads-*; do
    version=$(echo $version_dir | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
    if [ -z "$version" ]; then
        continue
    fi

    # Check if version should be kept
    keep=false
    for keep_ver in $KEEP_VERSIONS; do
        if [ "$version" = "$keep_ver" ]; then
            keep=true
            break
        fi
    done

    if [ "$keep" = false ] && [ -d "$version_dir" ]; then
        echo "  ❌ Deleting old version: $version_dir"
        # Keep documentation, delete binaries
        find "$version_dir" -maxdepth 1 -type f \( -name "*.tar.gz" -o -name "*.txt" -o -name "*CHECKSUM*" \) -delete
    else
        echo "  ✅ Keeping: $version_dir"
    fi
done
cd ..

# 3. Delete old checksum files
echo "📋 Cleaning old checksum files..."
cd release/
for checksum in *-SHA256.txt *-CHECKSUMS.txt; do
    if [ -f "$checksum" ]; then
        version=$(echo $checksum | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')
        if [ -z "$version" ]; then
            continue
        fi

        keep=false
        for keep_ver in $KEEP_VERSIONS; do
            if [ "$version" = "$keep_ver" ]; then
                keep=true
                break
            fi
        done

        if [ "$keep" = false ]; then
            echo "  ❌ Deleting: $checksum"
            rm -f "$checksum"
        fi
    fi
done
cd ..

# 4. Clean build artifacts
echo "🔨 Cleaning build artifacts..."
find . -type d \( -name "build" -o -name "dist" -o -name "target" \) -exec rm -rf {} + 2>/dev/null || true
find . -name "*.o" -delete
find . -name "*.a" -delete

# 5. Clean caches
echo "💾 Cleaning caches..."
find . -type d -name "node_modules" -exec rm -rf {} + 2>/dev/null || true
find . -type d -name "__pycache__" -exec rm -rf {} + 2>/dev/null || true
find . -name "*.pyc" -delete

# 6. Clean temporary files
echo "🗑  Cleaning temporary files..."
find . -name "*.tmp" -delete
find . -name "*.temp" -delete
find . -name "*.swp" -delete
find . -name "*~" -delete
find . -name ".DS_Store" -delete

# 7. Clean old environment files
echo "🌍 Cleaning environment files..."
find . -name ".env.local" -delete
find . -name ".env.old" -delete
find . -name ".env.backup" -delete

echo "✨ Cleanup complete!"
echo ""
echo "📊 Summary:"
echo "  Kept versions: $KEEP_VERSIONS"
echo "  Binary archives deleted: (last 3 versions kept)"
echo "  Build artifacts cleaned"
echo "  Caches cleared"
echo "  Temporary files removed"
```

### Usage
```bash
chmod +x cleanup.sh
./cleanup.sh 0.0.6
```

---

## 📊 CLEANUP SCHEDULE

### When to Run
- **After Every Release:** Run cleanup script with new version number
- **Monthly:** Clear caches and temporary files
- **Pre-Release:** Ensure clean build environment

### Automated via GitHub Actions
Create `.github/workflows/cleanup.yml`:
```yaml
name: Cleanup Environment

on:
  release:
    types: [published]
  schedule:
    - cron: '0 0 1 * *'  # Monthly on 1st

jobs:
  cleanup:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Extract version
        id: version
        run: |
          VERSION=${{ github.event.release.tag_name }}
          echo "version=${VERSION#v}" >> $GITHUB_OUTPUT

      - name: Run cleanup
        run: |
          chmod +x .local/cleanup.sh
          .local/cleanup.sh ${{ steps.version.outputs.version }}
```

---

## ⚠️ DANGEROUS OPERATIONS

### DO NOT DELETE
- **NEVER** delete any `.md` files
- **NEVER** delete `CHANGELOG.md`
- **NEVER** delete `README.md`
- **NEVER** delete historical version documents
- **NEVER** delete `.local/` directory

### ALWAYS KEEP
- All version summary documents (`V*.md`)
- All archive folders with documentation
- `.local/` tracking files
- Test files demonstrating features

---

## 📋 VERIFICATION AFTER CLEANUP

### Checklist
- [ ] Current version binaries present
- [ ] Previous 2 versions present
- [ ] All .md documentation intact
- [ ] No build artifacts in `src/`
- [ ] No temporary files in root
- [ ] `.local/` directory intact
- [ ] `.env` files preserved (non-local)
- [ ] Cache directories empty

### Manual Verification Commands
```bash
# Check for old binaries
ls -la release/ | grep -E "rads-0\.0\.[1-3]"

# Check for build artifacts
find src/ -name "*.o"
find src/ -name "*.a"
find . -type d -name "build"

# Check for temporary files
find . -name "*.tmp"
find . -name "*.swp"

# Verify docs preserved
ls -la V*.md
ls -la docs/*.md
```

---

## 🔧 SPACE SAVING

### Estimated Space Savings
```
Binary archives: ~50-100 MB per version
Build artifacts: ~10-50 MB
Caches: ~100-500 MB
Temporary files: ~1-10 MB

Total potential savings: ~200-800 MB per cleanup
```

---

**Last Updated:** January 27, 2026
**Maintained By:** AI Assistant Automation
**Run After:** Each version release
