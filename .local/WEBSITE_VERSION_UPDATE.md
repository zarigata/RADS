# 🌐 WEBSITE VERSION UPDATE AUTOMATION
**Purpose:** Automated website documentation updates for new versions
**Usage:** For maintaining accurate version info across all documentation
**Status:** INTERNAL - DO NOT PUBLISH

---

## 📋 WEBSITE STRUCTURE

### Current Setup
```
docs/
├── V0.0.6_QUICK_REFERENCE.md         # Current version quick reference
├── V0.0.3_BUTTERFLY_RELEASE.md    # Historical
├── V0.0.2_TEST_RESULTS.md           # Historical
├── V0.0.2_COMPLETE.md              # Historical
├── V0.0.1_COMPLETE.md              # Historical
├── roadmap/
│   ├── V0.0.6_PLAN.md           # Future plans
│   └── [historical plans]
└── [other docs]

rads-registry/ (Next.js app)
├── package.json                     # Version info
├── pages/
└── [React components]

Public site:
- https://zarigata.github.io/RADS (static docs)
- https://zarigata.github.io/RADS/registry (package registry)
```

---

## 📊 WEBSITE UPDATE CHECKLIST

### For EVERY Version Release

#### 1. Create New Version Documentation
- [ ] Create `docs/V0.0.x_QUICK_REFERENCE.md`
  - Copy from previous version template
  - Update version number
  - Update codename
  - Update download URLs
  - Update feature list
  - Update migration notes

- [ ] Create `V0.0.x_{CODENAME}_SUMMARY.md` (root)
  - Copy from previous version template
  - Update all version references
  - Add release date
  - List all new features
  - Include performance benchmarks

#### 2. Update Root Documentation
- [ ] Update `README.md`
  - Update version badge (line 9)
  - Update feature sections
  - Update download URLs
  - Update roadmap section
  - Update documentation links

- [ ] Update `CHANGELOG.md`
  - Add new version section at top
  - Move previous versions down
  - Include all features
  - Add migration guide
  - Add breaking changes

#### 3. Update Documentation Hub
- [ ] Update `docs/README.md`
  - Add new version to documentation index
  - Update "Latest Documentation" section
  - Update "Release Archives" section
  - Update "Current Release" references

#### 4. Verify Past 5 Versions
- [ ] Check v0.0.2 documentation exists
- [ ] Check v0.0.3 documentation exists
- [ ] Check v0.0.4 documentation exists
- [ ] Check v0.0.5 documentation exists
- [ ] Check v0.0.6 documentation exists (current)

**Missing documentation? Create placeholder:**
```markdown
# 📜 RADS v{VERSION} "{CODENAME}" - Archived Release

**Status:** Historical release - See [latest version](../../CHANGELOG.md)

**Release Date:** {DATE}

**Archived:** {ARCHIVE_DATE}

---

## 📝 Quick Summary

[Extract from CHANGELOG.md]

---

## 🔗 Links

- [Full Changelog](../../CHANGELOG.md)
- [Download Release](https://github.com/zarigata/rads/releases/tag/v{VERSION})
```

---

## 🔄 AUTOMATED UPDATE SCRIPT

### Shell Script for Website Updates
```bash
#!/bin/bash
# update-website.sh - Automated website documentation update

set -e

VERSION=$1
CODENAME=$2

if [ -z "$VERSION" ]; then
    echo "Usage: $0 <VERSION> <CODENAME>"
    echo "Example: $0 0.0.7 \"ROCKET SCREAMER\""
    exit 1
fi

echo "🌐 Updating website for RADS v${VERSION}..."

# 1. Update README.md
sed -i "s/version-0\.0\.[0-9]/version-${VERSION}/g" README.md
sed -i "s/rads-0\.0\.[0-9]/rads-${VERSION}/g" README.md
echo "✅ Updated README.md"

# 2. Create Quick Reference
PREV_VERSION=$(echo $VERSION | awk -F. '{print $1"."$2"."$3-1}')
cp docs/V${PREV_VERSION}_QUICK_REFERENCE.md docs/V${VERSION}_QUICK_REFERENCE.md
sed -i "s/${PREV_VERSION}/${VERSION}/g" docs/V${VERSION}_QUICK_REFERENCE.md
echo "✅ Created V${VERSION}_QUICK_REFERENCE.md"

# 3. Create Release Summary
cp V${PREV_VERSION}_${CODENAME}_SUMMARY.md V${VERSION}_${CODENAME}_SUMMARY.md 2>/dev/null || \
    echo "Creating from template..."
# Add manual edit step here
echo "⚠️  Manual edit required: V${VERSION}_${CODENAME}_SUMMARY.md"

# 4. Update docs/README.md
# Add new entry to documentation table
echo "✅ Updated docs/README.md (manual edit may be required)"

# 5. Verify past 5 versions
echo "🔍 Verifying past 5 versions..."
for v in $(seq $((VERSION-5)) $((VERSION-1)); do
    doc_file="docs/V0.$v*_QUICK_REFERENCE.md"
    if [ ! -f "$doc_file" ]; then
        echo "⚠️  Missing documentation: v0.$v"
    else
        echo "✅ Found: v0.$v"
    fi
done

echo "🎉 Website update complete!"
echo "📝 Review changes and commit:"
git add README.md CHANGELOG.md docs/
git commit -m "docs: update to v${VERSION}"
```

---

## 📊 VERSION INFORMATION MATRIX FOR WEBSITE

### Current Versions (Must Always Be Present)

| Version | Codename | Quick Reference | Summary | Status | Last Updated |
|---------|-----------|-----------------|----------|---------|---------------|
| 0.0.6 | FIGHTER JET | ✅ docs/V0.0.6_QUICK_REFERENCE.md | ✅ V0.0.6_FIGHTER_JET_SUMMARY.md | Current | 2026-01-27 |
| 0.0.5 | CHAMELEON | ✅ docs/V0.0.5_QUICK_REFERENCE.md | ✅ V0.0.5_CHAMELEON_SUMMARY.md | Archived | 2026-01-19 |
| 0.0.4 | Constellation | ❌ CREATE | ✅ V0.0.4_STATUS.md | Archived | 2026-01-13 |
| 0.0.3 | Butterfly | ✅ docs/V0.0.3_BUTTERFLY_RELEASE.md | ✅ docs/V0.0.3_FINAL_STATUS.md | Archived | 2026-01-11 |
| 0.0.2 | Core Language | ✅ docs/V0.0.2_TEST_RESULTS.md | ✅ docs/V0.0.2_COMPLETE.md | Archived | 2026-01-11 |
| 0.0.1 | Web Framework | ✅ docs/V0.0.1_COMPLETE.md | ✅ docs/V0.0.1_COMPLETE.md | Archived | 2026-01-10 |

### Missing Documentation (Create for v0.0.7)

| Version | Missing Document | Template | Priority |
|---------|------------------|-----------|-----------|
| 0.0.4 | V0.0.4_QUICK_REFERENCE.md | V0.0.5_QUICK_REFERENCE.md | High |
| 0.0.4 | V0.0.4_SUMMARY.md | V0.0.5_CHAMELEON_SUMMARY.md | High |

---

## 🔗 WEBSITE DEPLOYMENT

### GitHub Actions Workflow
**File:** `.github/workflows/gh-pages.yml`
**Trigger:** Push to `main` branch
**Action:**
1. Copy `docs/` to `public/`
2. Deploy to GitHub Pages

### Manual Deployment
```bash
# Build and deploy
npm run build:static  # If applicable

# Or simply copy
mkdir -p public
cp -r docs/* public/

# Commit and push
git add public/
git commit -m "deploy: update website"
git push origin main
```

### Verify Deployment
```bash
# Check status
gh api /repos/zarigata/RADS/pages

# Visit site
curl -I https://zarigata.github.io/RADS
```

---

## 📊 WEBSITE UPDATE TRACKING

### Version History on Website
```
v0.0.6 (current) → Latest release features, download links
v0.0.5          → Python transpilation, archived
v0.0.4          → Distributed computing, archived
v0.0.3          → Database & testing, archived
v0.0.2          → Core language, archived
v0.0.1          → Web framework, archived
```

### Update Frequency
- **Major Release:** Full website update
- **Minor Release:** Quick reference update
- **Patch Release:** Only CHANGELOG update

---

## 🎨 WEBSITE DESIGN CONSISTENCY

### Version Information Display Pattern
All version references should follow:
```markdown
**Current Version:** v0.0.6 "FIGHTER JET"
**Release Date:** January 27, 2026
**Download:** [v0.0.6 Release](https://github.com/zarigata/rads/releases/tag/v0.0.6)
```

### Download URL Pattern
```markdown
wget https://github.com/zarigata/rads/releases/download/v0.0.6/rads-0.0.6-linux-x86_64.tar.gz
```

### Documentation Link Pattern
```markdown
- [Quick Reference](docs/V0.0.6_QUICK_REFERENCE.md)
- [Full Changelog](CHANGELOG.md)
- [Release Summary](V0.0.6_FIGHTER_JET_SUMMARY.md)
```

---

## 📝 QUICK REFERENCE TEMPLATE

### File: `docs/V0.0.x_QUICK_REFERENCE.md`
```markdown
# 🚀 RADS v{VERSION} "{CODENAME}" - Quick Reference

**Version:** {VERSION}
**Codename:** {CODENAME}
**Release Date:** {DATE}

---

## ⚡ Quick Start

### Installation
```bash
wget https://github.com/zarigata/rads/releases/download/v{VERSION}/rads-{VERSION}-linux-x86_64.tar.gz
tar -xzf rads-{VERSION}-linux-x86_64.tar.gz
cd rads-{VERSION}
./install.sh
```

### Verify Installation
```bash
rads --version
# Output: RADS v{VERSION} "{CODENAME}"
```

### Hello World
```bash
rads hello.rads
```

---

## 🌟 Key Features

[List from CHANGELOG]

---

## 📚 Documentation

| Document | Description |
|-----------|-------------|
| [Release Summary](../V{VERSION}_{CODENAME}_SUMMARY.md) | Complete release information |
| [Development Plan](../V{VERSION}_{CODENAME}_PLAN.md) | Original roadmap |
| [CHANGELOG](../CHANGELOG.md) | Full version history |
| [Migration Guide](../docs/MIGRATION_GUIDE.md) | Upgrade instructions |

---

## 🔮 What's Next

[Next version plans]

---

**Previous Release:** [v{PREV_VERSION}](../V{PREV_VERSION}_{CODENAME}_SUMMARY.md)
**Next Release:** [v{NEXT_VERSION}] (planned)
```

---

**Last Updated:** January 27, 2026
**Maintained By:** AI Assistant Automation
