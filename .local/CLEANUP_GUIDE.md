# 🗂️ CLEANUP GUIDE - LOCAL ONLY

**Purpose:** Policies and procedures for maintaining clean project structure
**Usage:** Follow this guide before and after every release
**Status:** INTERNAL - DO NOT PUBLISH

---

## 🎯 Cleanup Philosophy

### Principles

1. **Never Delete Documentation**
   - Preserve all .md files
   - Keep CHANGELOG.md (complete history)
   - Retain all version summaries
   - Archive old release docs, don't delete

2. **Delete Old Binaries**
   - Remove release archives older than 3 versions
   - Delete temporary build artifacts
   - Clean up test binaries

3. **Archive, Don't Remove**
   - Move old version docs to archive folders
   - Keep structured archive system
   - Maintain historical context

4. **Clean Temporary Files**
   - Remove temporary editor files (*~, *.swp)
   - Delete backup files (*.bak, *.old)
   - Clean generated files

---

## 🗑️ What to Delete

### Release Binaries (Older than 3 versions)

**Location:** `release/` directory

**Pattern to Delete:**
```
rads-0.0.1*.tar.gz
rads-0.0.1*.txt
rads-0.0.2*.tar.gz
rads-0.0.2*.txt
```

**Commands:**
```bash
cd release/

# Delete old tarballs
rm -f rads-0.0.1* rads-0.0.2* rads-0.0.3*

# Keep only last 3 versions (0.0.4, 0.0.5, 0.0.6)
```

**Files to Keep:**
- All release folders (`rads-0.0.5/`, `rads-0.0.4/`, `rads-0.0.3/`)
- Latest 3 release binaries
- All .md documentation

### Build Artifacts

**Location:** Root directory and subdirectories

**Files to Delete:**
```bash
# Build directory (if exists)
rm -rf build/

# Binary symlinks (will be recreated)
rm -f rads rstar rpm rads-mask

# Compiled object files
find . -name "*.o" -type f -delete
find . -name "*.so" -type f -delete
find . -name "*.a" -type f -delete

# Temporary files
find . -name "*~" -type f -delete
find . -name "*.swp" -type f -delete
find . -name "*.bak" -type f -delete
find . -name "*.old" -type f -delete
find . -name "#*#" -type f -delete
```

### Documentation Archives (Very Old)

**Location:** Root directory

**When to Archive:**
- Release summaries older than 5 versions
- Phase documents that are obsolete
- Roadmap documents that are complete

**Archive Pattern:**
```bash
# Create archive folder
mkdir -p .archive

# Move old files
mv V0.0.1_COMPLETE.md .archive/
mv V0.0.2_PROGRESS.md .archive/
mv SESSION_2026_01_*.md .archive/

# Create archive manifest
echo "# Archived Files" > .archive/README.md
echo "Archived: $(date)" >> .archive/README.md
ls -1 >> .archive/README.md
```

---

## 🗂️ What to Archive

### Release Documentation

**Move to Archive When:** Version is 3+ releases old

**Files to Archive:**
```
V0.0.1_COMPLETE.md
V0.0.2_COMPLETE.md
V0.0.2_TEST_RESULTS.md
V0.0.2_PROGRESS.md
V0.0.3_BUTTERFLY_RELEASE.md
V0.0.3_FINAL_STATUS.md
V0.0.4_COMPLETE.md
```

**Archive Structure:**
```
.archive/
├── README.md                    # Archive manifest
├── 0.0.1/
│   ├── V0.0.1_COMPLETE.md
│   └── SESSION_COMPLETE_V0.0.3.md
├── 0.0.2/
│   ├── V0.0.2_COMPLETE.md
│   ├── V0.0.2_TEST_RESULTS.md
│   └── V0.0.2_PROGRESS.md
└── 0.0.3/
    ├── V0.0.3_BUTTERFLY_RELEASE.md
    └── V0.0.3_FINAL_STATUS.md
```

### Release Folders

**Archive When:** Version is 5+ releases old

**Pattern:**
```bash
# When moving to v0.0.8, archive v0.0.3

# Compress and archive
cd release/
tar -czf ../.archive/release/rads-0.0.3.tar.gz rads-0.0.3/

# Delete uncompressed folder
rm -rf rads-0.0.3/
```

**Keep in Root:**
- Latest 3 release folders (current + 2 previous)
- Current release binary and checksums
- All .md documentation in root

---

## 📂️ What to Keep (Never Delete)

### Always Preserve

**Root Documentation:**
```
CHANGELOG.md              # Complete version history
README.md                # Main project README
V0.0.5_CHAMELEON_SUMMARY.md
V0.0.6_FIGHTER_JET_SUMMARY.md
V{VERSION}_{CODENAME}_PLAN.md   # For current + 2 previous
CONTRIBUTING.md           # Contribution guidelines
LICENSE                   # License file
```

**Documentation Hub:**
```
docs/README.md            # Documentation overview
docs/LANGUAGE_SPEC.md      # Language specification
docs/REPL_GUIDE.md        # REPL documentation
docs/PACKAGE_MANAGER.md    # Package manager docs
```

**Release Archives:**
```
release/rads-0.0.5/      # Previous release docs
release/rads-0.0.5/archive/  # Archive folder
release/rads-0.0.4/      # Keep at least 2 previous
release/rads-0.0.3/      # Archive if > 2 versions old
```

### Essential Documentation (Keep Forever)

| File | Reason | Archive Policy |
|------|---------|----------------|
| CHANGELOG.md | Complete history needed | Never archive |
| VERSION_TRACKING.md | Internal version management | Never archive |
| RELEASE_CHECKLIST.md | Release process guide | Never archive |
| WEBSITE_UPDATE_GUIDE.md | Website update procedures | Never archive |
| CLEANUP_GUIDE.md | This file | Never archive |
| README.md | Main entry point | Never archive |
| All V*_SUMMARY.md | Release summaries | Keep current + 5 previous |
| All V*_PLAN.md | Development plans | Keep current + 2 previous |

---

## 🔍 Cleanup Schedule

### Before Every Release

**Execute:**
```bash
# 1. Remove build artifacts
make clean
rm -rf build/ bin/

# 2. Delete temporary files
find . -name "*~" -type f -delete
find . -name "*.swp" -type f -delete
find . -name "*.bak" -type f -delete

# 3. Archive old release folders (if applicable)
cd release/
for version in 0.0.1 0.0.2; do
    if [ -d "rads-$version/archive" ]; then
        tar -czf ".archive/release/rads-$version.tar.gz" "rads-$version/"
        rm -rf "rads-$version/"
    fi
done
cd ..

# 4. Delete old binaries (keep last 3)
cd release/
ls -1 rads-0.0.*.tar.gz | head -n -3 | xargs -I {} basename {}
ALL_BINS=$(ls rads-0.0.*.tar.gz)
for bin in $ALL_BINS; do
    if [[ ! $KEEP_BINS =~ $bin ]]; then
        rm -f "$bin"
    fi
done
```

### After Every Release

**Execute:**
```bash
# 1. Archive old release docs (if needed)
# See VERSION_TRACKING.md for current state

# 2. Remove temporary build files
make clean

# 3. Delete old checksums (keep current)
cd release/
find . -name "*SHA256.txt" -not -name "*0.0.6*" -delete

# 4. Archive old phase/roadmap docs
# Move completed plans to archive
```

### Monthly Maintenance

**Execute:**
```bash
# 1. Clean up any temporary files
find . -type f -mtime +30 \( -name "*~" -o -name "*.tmp" -o -name "*.bak" \) -delete

# 2. Archive very old docs (if applicable)
# Check VERSION_TRACKING.md for guidance

# 3. Compress archives if very old
cd .archive/
find . -type f -mtime +365 -name "*.tar.gz" -exec gzip -k {} \;
```

---

## 🔍 Verification Checklist

### Pre-Cleanup Verification

- [ ] Identify all files to delete
- [ ] Verify no important documentation is targeted
- [ ] Confirm release versions to keep (last 3)
- [ ] Check for broken symlinks
- [ ] Ensure git ignore patterns don't delete important files

### Post-Cleanup Verification

- [ ] Deleted files no longer exist
- [ ] Important files still present
- [ ] Release downloads still work
- [ ] Documentation links still valid
- [ ] No broken references in CHANGELOG
- [ ] Git status shows expected changes

### Archive Integrity Check

- [ ] Archive compressed successfully
- [ ] Archive can be extracted
- [ ] Archive contents verified
- [ ] Archive manifest created
- [ ] Archive location documented in VERSION_TRACKING.md

---

## 🔗 File Size Management

### Target Sizes

| File Type | Target Size | Action if Exceeded |
|-----------|-------------|-------------------|
| Release Binary | < 5 MB | Compress, split into parts |
| Release Tarball | < 10 MB | Exclude unnecessary files |
| Documentation | N/A | Keep all, it's important |
| Archive | < 100 MB | Compress individually |

### Size Check Commands

```bash
# Check release binary size
du -h release/rads-0.0.6-linux-x86_64.tar.gz

# Check total release folder size
du -sh release/

# Check documentation size
du -sh docs/

# Check git repository size
du -sh .git/
```

---

## 🔄 Automated Cleanup Script

### Script: `.local/cleanup.sh`

```bash
#!/bin/bash
# RADS Cleanup Script
# Run before/after releases

set -e

echo "🧹 RADS Cleanup Script"

# 1. Clean build artifacts
echo "Cleaning build artifacts..."
make clean > /dev/null 2>&1 || true
rm -rf build/ bin/

# 2. Remove temporary files
echo "Removing temporary files..."
find . -name "*~" -type f -delete
find . -name "*.swp" -type f -delete
find . -name "*.bak" -type f -delete
find . -name "*.old" -type f -delete
find . -name "#*#" -type f -delete

# 3. Archive old releases
echo "Archiving old releases..."
cd release/
for version in 0.0.1 0.0.2; do
    if [ -d "rads-$version/archive" ]; then
        mkdir -p ../.archive/release/
        tar -czf "../.archive/release/rads-$version.tar.gz" "rads-$version/"
        rm -rf "rads-$version/"
        echo "Archived: rads-$version"
    fi
done
cd ..

# 4. Delete old binaries
echo "Cleaning old release binaries..."
cd release/
KEEP_VERSIONS=("0.0.4" "0.0.5" "0.0.6")
for file in rads-0.0.*.tar.gz rads-0.0.*.txt; do
    VERSION=$(echo "$file" | grep -oP '0\.0\.[0-9]')
    KEEP=false
    for keep in "${KEEP_VERSIONS[@]}"; do
        if [[ "$VERSION" == "$keep" ]]; then
            KEEP=true
            break
        fi
    done
    if [ "$KEEP" = false ]; then
        rm -f "$file"
        echo "Deleted: $file"
    fi
done
cd ..

# 5. Compress old archives
echo "Compressing old archives..."
cd .archive/
find . -type f -mtime +365 -name "*.tar.gz" | while read file; do
    gzip -k "$file"
    echo "Compressed: $file"
done
cd ..

# 6. Summary
echo "✅ Cleanup complete!"
echo "📊 Summary:"
echo "  - Build artifacts removed"
echo "  - Temporary files deleted"
echo "  - Old releases archived"
echo "  - Old binaries cleaned"
echo "  - Old archives compressed"
```

**To Use:**
```bash
chmod +x .local/cleanup.sh
.local/cleanup.sh
```

---

## 🔍 Cleanup Troubleshooting

### Common Issues

**Issue:** Accidentally deleted important file

**Solution:**
```bash
# Restore from git
git checkout HEAD -- <file>

# Or from archive
cd .archive/
tar -xzf <archive-file>
```

**Issue:** Cleanup script deleted wrong files

**Solution:**
1. Check git status: `git status`
2. Restore from git: `git checkout HEAD -- <files>`
3. Review cleanup script
4. Run again carefully

**Issue:** Archive extraction fails

**Solution:**
```bash
# Verify archive integrity
gunzip -t <archive-file>

# If corrupted, restore from git history
git log --all --full-history -- <file>
```

---

## 📞 Emergency Restore

### From Git History

```bash
# List deleted files in recent commits
git log --diff-filter=D --summary

# Restore specific file
git checkout HEAD~1 -- <file>

# Restore all from specific commit
git checkout <commit-hash> -- .
```

### From Backup Archive

```bash
# List archives
ls -la .archive/release/

# Extract specific archive
cd .archive/release/
tar -xzf rads-0.0.3.tar.gz

# Move back to original location
mv rads-0.0.3/* ../../release/
```

---

## 🔗 Related Files

- `VERSION_TRACKING.md` - Current version status
- `RELEASE_CHECKLIST.md` - Release preparation guide
- `WEBSITE_UPDATE_GUIDE.md` - Website update procedures
- `RELEASE_PROCESS.md` - Complete release workflow

---

**Last Updated:** January 27, 2026
**Maintained By:** Cleanup Manager
**Next Review:** Before v0.0.7 cleanup

---

*This file is for internal use only. Do not publish to public documentation.*
