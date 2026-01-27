# 🚀 GITHUB RELEASE AUTOMATION
**Purpose:** Automated GitHub release creation and publishing
**Usage:** For CI/CD automation via GitHub Actions
**Status:** INTERNAL - DO NOT PUBLISH

---

## 📋 WORKFLOW OVERVIEW

### Current Setup
RADS has GitHub Actions for:
1. **Registry Publishing** - `.github/workflows/publish-to-registry.yml`
2. **Website Deployment** - `.github/workflows/gh-pages.yml`
3. **Registry Website Deploy** - `rads-registry/.github/workflows/deploy.yml`

### Missing
- ❌ Automated GitHub Release creation workflow
- ❌ Automated release notes generation
- ❌ Automated asset upload
- ❌ Automated version tagging

---

## 🔧 REQUIRED GITHUB ACTION: Create Release

### File to Create: `.github/workflows/release.yml`

```yaml
name: Create GitHub Release

on:
  push:
    tags:
      - 'v*'

permissions:
  contents: write

jobs:
  create-release:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout code
        uses: actions/checkout@v4

      - name: Generate Release Notes
        id: release-notes
        run: |
          VERSION=${GITHUB_REF#refs/tags/v}
          echo "version=$VERSION" >> $GITHUB_OUTPUT

          # Extract changelog entry
          awk "/## \[$VERSION\]/,/^## /" CHANGELOG.md | head -n -1 > release_notes.md
          tail -n +2 release_notes.md > ${VERSION}_RELEASE_NOTES.md

      - name: Create Release
        uses: actions/create-release@v1
        with:
          tag_name: ${{ github.ref }}
          release_name: Release ${{ steps.release-notes.outputs.version }}
          body_path: ${{ steps.release-notes.outputs.version }}_RELEASE_NOTES.md
          draft: false
          prerelease: false
```

---

## 📊 VERSION DATA TRACKING

### Extract Version Information from Files

The release workflow should extract:
1. **Version number** from `VERSION_TRACKING.md`
2. **Codename** from `VERSION_TRACKING.md`
3. **Release date** from CHANGELOG
4. **Changelog entry** from CHANGELOG.md

### Required Metadata
- Version: MAJOR.MINOR.PATCH
- Codename: Thematic name
- Release Date: YYYY-MM-DD
- Feature List: From CHANGELOG
- Breaking Changes: From CHANGELOG
- Migration Guide: Link to docs

---

## 🔄 AUTOMATED RELEASE PROCESS

### Step 1: Tag Creation
```bash
# Determine next version
NEXT_VERSION=$(grep "Planned Version" .local/VERSION_TRACKING.md | grep -oE '[0-9]+\.[0-9]+\.[0-9]+')

# Create tag
git tag -a v${NEXT_VERSION} -m "Release v${NEXT_VERSION}"
git push origin v${NEXT_VERSION}
```

### Step 2: Automatic Actions Trigger
When tag is pushed:
1. GitHub Actions `release.yml` triggers
2. Extracts changelog from CHANGELOG.md
3. Generates release notes
4. Creates GitHub release with:
   - Tag name
   - Release title
   - Release body
   - Assets (if any)

### Step 3: Post-Release
1. Archive previous release files
2. Update VERSION_TRACKING.md
3. Trigger website deployment
4. Update package registry (if applicable)

---

## 📦 ASSET UPLOAD AUTOMATION

### Build Artifacts
```yaml
- name: Build Release Artifacts
  run: |
    make clean
    make release

    # Create archive
    tar -czf rads-${VERSION}-linux-x86_64.tar.gz \
      --exclude='.git' \
      --exclude='*.md' \
      --exclude='tests/' \
      --exclude='.local/' \
      rads

    # Generate checksums
    sha256sum rads-${VERSION}-linux-x86_64.tar.gz > rads-${VERSION}-SHA256.txt
```

### Upload to Release
```yaml
- name: Upload Release Assets
  uses: softprops/action-gh-release@v1
  with:
    files: |
      rads-${VERSION}-linux-x86_64.tar.gz
      rads-${VERSION}-SHA256.txt
    generate_release_notes: false
```

---

## 🎯 RELEASE CHECKLIST

### Before Release
- [ ] All tests passing
- [ ] CHANGELOG.md updated
- [ ] V{VERSION}_SUMMARY.md created
- [ ] Version strings updated across codebase
- [ ] Documentation updated
- [ ] Assets built and tested

### During Release (Automated)
- [ ] Tag created and pushed
- [ ] GitHub Actions triggered
- [ ] Release notes generated
- [ ] Assets uploaded
- [ ] Release published

### After Release
- [ ] Version tracking updated
- [ ] Previous release archived
- [ ] Website deployed
- [ ] Registry updated
- [ ] Announcement published

---

## 🔍 VERIFICATION

### Check Release Was Created
```bash
# List releases
gh release list

# View specific release
gh release view v0.0.7

# Download release assets
gh release download v0.0.7
```

### Monitor GitHub Actions
```bash
# List workflow runs
gh run list --workflow=release.yml

# View specific run
gh run view <run-id>
```

---

## 📝 RELEASE NOTES TEMPLATE

### Auto-Generated Format
```markdown
# 🚀 RADS v{VERSION} "{CODENAME}"

**Release Date:** {DATE}
**Status:** ✅ Production Ready

---

## ✨ What's New

{FEATURES FROM CHANGELOG}

---

## 🐛 Bug Fixes

{BUG FIXES FROM CHANGELOG}

---

## ⚠️ Breaking Changes

{BREAKING CHANGES FROM CHANGELOG}

---

## 📦 Installation

```bash
wget https://github.com/zarigata/rads/releases/download/v{VERSION}/rads-{VERSION}-linux-x86_64.tar.gz
tar -xzf rads-{VERSION}-linux-x86_64.tar.gz
cd rads-{VERSION}
./install.sh
```

---

## 📚 Documentation

- [Full Changelog](CHANGELOG.md)
- [Release Summary](V{VERSION}_{CODENAME}_SUMMARY.md)
- [Quick Reference](docs/V{VERSION}_QUICK_REFERENCE.md)
- [Migration Guide](docs/MIGRATION_GUIDE.md)

---

## 🔮 What's Next

{NEXT VERSION PLANS}

---

**Previous Releases:** [v{PREV_VERSION}]({PREV_RELEASE_URL})
```

---

## 🎨 CODENAME THEMES

| Version | Codename | Theme |
|---------|-----------|--------|
| 0.0.1 | Web Framework | Foundation |
| 0.0.2 | Core Language | Language Stability |
| 0.0.3 | Butterfly | Metamorphosis |
| 0.0.4 | Constellation | Distributed Computing |
| 0.0.5 | CHAMELEON | Transpilation/Masking |
| 0.0.6 | FIGHTER JET | Speed/Performance |
| 0.0.7 | TBD | Suggest next theme |

---

## 🔗 RELATED WORKFLOWS

| Workflow | File | Purpose | Trigger |
|----------|--------|---------|----------|
| Release Creation | `.github/workflows/release.yml` | ✅ CREATE THIS | Tag push |
| Registry Publish | `.github/workflows/publish-to-registry.yml` | ✅ EXISTS | Release published |
| Website Deploy | `.github/workflows/gh-pages.yml` | ✅ EXISTS | Push to main |
| Registry Deploy | `rads-registry/.github/workflows/deploy.yml` | ✅ EXISTS | Push to main |

---

**Last Updated:** January 27, 2026
**Maintained By:** AI Assistant Automation
