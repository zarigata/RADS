# GitHub Action for Automated Package Publishing

**Status:** Ready
**Date:** January 26, 2026

---

## Overview

This GitHub Action automatically publishes your RADS package to the RADS Registry when you create a GitHub Release. It:

1. ✅ Validates `package.rads` manifest
2. ✅ Creates package archive (.tar.gz)
3. ✅ Updates the rads-packages database (GitHub Issues)
4. ✅ Adds package metadata (version, download URL, etc.)

---

## Prerequisites

Before using this workflow, ensure:

### 1. RADS Registry Running
- `rads-registry` repository created and deployed
- `rads-packages` repository created
- GitHub App configured for OAuth

### 2. GitHub Access Token
Generate a Personal Access Token (PAT) with permissions:
- `issues:write` - To update rads-packages database
- `repo:read` - To read package repository

Create at: https://github.com/settings/tokens
Name it: `RADS Registry Publisher`

### 3. package.rads File

Your package repository must have a `package.rads` file in root:

```rads
name = "my-awesome-package"
version = "1.0.0"
description = "An awesome RADS package"
author = "your-github-username"
license = "MIT"
```

---

## Installation

### Step 1: Add Workflow to Package Repository

Create file: `.github/workflows/publish-to-registry.yml`

```bash
mkdir -p .github/workflows
```

Copy the workflow from the Rads repository:
```bash
# Or copy from:
# /path/to/Rads/.github/workflows/publish-to-registry.yml
```

### Step 2: Add GitHub Secret

Add `GH_REGISTRY_TOKEN` secret to your package repository:

1. Go to your package repository on GitHub
2. Settings → Secrets and variables → Actions
3. Click "New repository secret"
4. Name: `GH_REGISTRY_TOKEN`
5. Value: Your PAT from Prerequisites
6. Click "Add secret"

---

## Usage

### Publishing a New Version

When you're ready to release a new version:

1. **Update version in package.rads**
   ```rads
   version = "1.1.0"
   ```

2. **Commit and push changes**
   ```bash
   git add package.rads
   git commit -m "Bump version to 1.1.0"
   git push origin main
   ```

3. **Create GitHub Release**
   - Go to your repository on GitHub
   - Click "Releases"
   - Click "Create a new release"
   - Tag version: `v1.1.0`
   - Title: `v1.1.0`
   - Description: Release notes
   - Click "Publish release"

4. **Workflow runs automatically**
   - The workflow validates `package.rads`
   - Creates package archive
   - Updates rads-packages database
   - Your package appears on registry.rads-lang.org!

---

## Workflow Process

```
GitHub Release Triggered
         │
         ▼
┌────────────────────────────────┐
│ 1. Checkout package repo    │
│ 2. Validate package.rads    │
│    - name exists              │
│    - version exists            │
│    - semver format           │
│ 3. Get release info         │
│ 4. Create package metadata    │
│ 5. Create package archive    │
│ 6. Upload archive artifact  │
│ 7. Update rads-packages    │
│    - Find existing issue      │
│    - Update or create issue   │
└────────────────────────────────┘
         │
         ▼
  Package appears on registry!
```

---

## package.rads Format

### Required Fields

```rads
name = "package-name"        # Required: Lowercase, hyphens, numbers
version = "1.0.0"           # Required: Semver (x.y.z)
description = "Package description"  # Required: Brief description
author = "github-username"    # Required: GitHub username
```

### Optional Fields

```rads
license = "MIT"            # Optional: Package license (default: MIT)
homepage = "https://..."     # Optional: Package website URL
keywords = ["db", "sql"]   # Optional: Array of keywords
```

### Example package.rads

```rads
name = "database-sqlite"
version = "1.2.0"
description = "SQLite3 database integration for RADS with full CRUD operations"
author = "rads-team"
license = "MIT"
homepage = "https://github.com/rads/database-sqlite"
keywords = ["database", "sqlite", "sql", "crud"]
```

---

## Validation Rules

The workflow enforces these rules:

### 1. package.rads Must Exist
```bash
Error: package.rads not found
Exit: 1
```

### 2. Required Fields Present
```bash
Error: name is required in package.rads
Exit: 1
```

### 3. Semver Format
Valid formats: `1.0.0`, `2.3.1`, `10.20.30`
Invalid formats: `1.0`, `v1.0.0`, `1.0.0-beta`

```bash
Error: version must be in semantic versioning format (x.y.z)
Exit: 1
```

### 4. Name Format
Valid: `my-package`, `awesome-lib-2026`, `db_sqlite`
Invalid: `MyPackage`, `awesome@lib`, `db.sqlite`

---

## Metadata Created

The workflow creates a GitHub Issue in `rads-packages` with this structure:

### Title
```
package-name: Package Display Name
```

### Body (JSON)
```json
{
  "name": "package-name",
  "displayName": "Package Display Name",
  "description": "Package description",
  "version": "1.0.0",
  "author": "github-username",
  "repository": "https://github.com/user/repo",
  "homepage": "",
  "keywords": [],
  "license": "MIT",
  "readme": "Release notes from GitHub Release",
  "latestVersion": "1.0.0",
  "versions": [
    {
      "version": "1.0.0",
      "releaseDate": "2026-01-26T20:00:00Z",
      "releaseUrl": "https://github.com/user/repo/releases/tag/v1.0.0",
      "downloadUrl": "https://github.com/user/repo/releases/download/v1.0.0/package-name-1.0.0.tar.gz",
      "checksum": ""
    }
  ],
  "stats": {
    "downloads": 0,
    "stars": 0,
    "lastUpdated": "2026-01-26T20:00:00Z"
  }
}
```

### Labels
- `package` - All packages have this
- `category:<category>` - Optional (if package.rads has category field)
- `license:<license>` - License type
- `status:published` - Published packages

---

## Troubleshooting

### Workflow Fails: "package.rads not found"
**Problem:** No `package.rads` file in repository root

**Solution:** Create `package.rads` file with required fields

### Workflow Fails: "Invalid version format"
**Problem:** Version doesn't follow semver (x.y.z)

**Solution:** Update version to format like `1.0.0` or `2.3.1`

### Workflow Fails: "Error: name is required"
**Problem:** package.rads missing required field

**Solution:** Add all required fields: name, version, description, author

### Package Not Appearing in Registry
**Problem:** Workflow succeeded but package not visible

**Solution:**
1. Check `rads-packages` repository for the Issue
2. Verify labels are correct (`package`, `status:published`)
3. Clear cache on registry.rads-lang.org
4. Wait 1-2 minutes for GitHub Actions to complete

### Permission Denied
**Problem:** `GH_REGISTRY_TOKEN` doesn't have correct permissions

**Solution:**
1. Regenerate PAT with `issues:write` and `repo:read`
2. Update secret in package repository
3. Run workflow manually

---

## Testing the Workflow

### Manual Trigger (Testing Only)

You can manually trigger the workflow for testing:

```bash
# Using GitHub CLI
gh workflow run "Publish to RADS Registry"
```

### Check Workflow Logs

1. Go to your package repository
2. Actions tab
3. Select "Publish to RADS Registry" workflow
4. View logs for each step

---

## Best Practices

### 1. Semantic Versioning
Follow semver strictly:
- `MAJOR.MINOR.PATCH` format
- `1.0.0` → Initial release
- `1.1.0` → New feature (backward compatible)
- `1.1.1` → Bug fix (backward compatible)
- `2.0.0` → Breaking changes

### 2. Release Notes
Always include release notes:
- New features
- Bug fixes
- Breaking changes
- Migration guide (if breaking)

### 3. Pre-release Versions
Use semver pre-release tags:
- `1.0.0-alpha.1`
- `1.0.0-beta.2`
- `1.0.0-rc.1`

### 4. Package Structure

Exclude from package archive:
- `.git/` directory
- `.md` files (except README)
- `tests/` directory (unless tests should be bundled)
- Development files

Include in package archive:
- Source code files (.rads, .c, .h)
- README.md
- package.rads (manifest)
- Dependencies (if any)

---

## Advanced Usage

### Multiple Package Categories

Update package.rads:

```rads
name = "my-package"
version = "1.0.0"
description = "..."
author = "username"
category = "database"  # Add category for registry filtering
```

### Custom License

```rads
license = "Apache-2.0"  # Use any SPDX identifier
```

### Package Homepage

```rads
homepage = "https://my-package-docs.com"  # Link to documentation
```

---

## Example: Publishing a Complete Package

```bash
# 1. Update version
echo 'version = "1.0.0"' > package.rads

# 2. Commit
git add package.rads
git commit -m "Initial release: v1.0.0"

# 3. Push
git push origin main

# 4. Create GitHub Release (via GitHub UI or CLI)
gh release create v1.0.0 \
  --title "v1.0.0" \
  --notes "Initial release of my package" \
  --target main

# 5. Workflow runs automatically
# 6. Check Actions tab for logs
# 7. Visit registry.rads-lang.org to verify
```

---

## Next Steps

After workflow is working:

1. ✅ Your package appears on registry.rads-lang.org
2. ✅ Users can search and install your package
3. ✅ Version history tracked automatically
4. ✅ Download statistics available (when implemented)

---

**Last Updated:** January 26, 2026
**Status:** Ready for Use

*"Publish your RADS package with ease!"* 🚀
