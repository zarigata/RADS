# RADS Package Registry - User Guide

**Version:** 1.0
**Last Updated:** January 26, 2026

---

## Welcome to RADS Package Registry

The RADS Package Registry is the central hub for discovering, installing, and publishing RADS packages. This guide will help you get started quickly.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Browsing Packages](#browsing-packages)
3. [Searching Packages](#searching-packages)
4. [Installing Packages](#installing-packages)
5. [Publishing Packages](#publishing-packages)
6. [Package Authentication](#package-authentication)
7. [Troubleshooting](#troubleshooting)
8. [Best Practices](#best-practices)

---

## Quick Start

### Step 1: Install RADS (if not already installed)

```bash
# Using pre-built binary (Linux)
wget https://github.com/zarigata/rads/releases/download/v0.0.5/rads-0.0.5-linux-x86_64.tar.gz
tar -xzf rads-0.0.5-linux-x86_64.tar.gz
cd rads-0.0.5
./install.sh

# Verify installation
rads --version
rstar help
```

### Step 2: Browse Available Packages

Visit: `https://registry.rads-lang.org/packages`

Browse by category:
- **Database** - SQLite, PostgreSQL, MySQL connectors
- **Web** - HTTP servers, web frameworks
- **Utils** - Helper libraries, utilities
- **Media** - Audio, image, video processing
- **Networking** - TCP/UDP, WebSocket libraries
- **Testing** - Test frameworks, mocks
- **CLI** - Command-line tools

### Step 3: Install Your First Package

```bash
# Search for a package
rstar search "database"

# Install a specific package
rstar install database-sqlite

# Verify installation
rstar list
```

---

## Browsing Packages

### Registry Homepage

Visit `https://registry.rads-lang.org` to see:
- **Featured Packages** - Curated popular packages
- **Quick Start Guide** - Installation basics
- **Recent Packages** - Latest additions

### Package Browser

Navigate to `/packages` for full browsing experience:

**Features:**
- 📦 View all packages (paginated, 50 per page)
- 🔍 Real-time search (300ms debounce)
- 🏷️ Filter by category
- 📄 Filter by license
- 📊 Sort by relevance, downloads, stars, or updated
- 📱 Responsive design (mobile, tablet, desktop)
- 🌙 Dark mode support

**Package Cards Display:**
- Package name and description
- Category badge
- Keywords (up to 5 shown)
- Download count
- GitHub stars
- Last updated date
- View details link

### Filtering Options

**Categories:**
- Database
- Web
- Utils
- Media
- Networking
- Testing
- CLI
- Other

**Licenses:**
- MIT
- Apache-2.0
- GPL-3.0
- BSD-3
- ISC

**Sort Options:**
- Relevance (default)
- Downloads (most downloaded first)
- Stars (most starred first)
- Recently Updated (newest first)

---

## Searching Packages

### Using the Registry Web UI

1. **Basic Search**
   - Type in search bar
   - Press Enter or wait 300ms (auto-search)
   - Results appear below

2. **Filtered Search**
   - Select category dropdown
   - Select license filter
   - Choose sort order
   - Results update automatically

3. **Active Filters**
   - See current filters as badges
   - Click × on any filter to remove
   - Click "Clear all" to reset

### Using rstar CLI

```bash
# Basic search
rstar search "database"

# Search with category filter
rstar search "database" --category database

# Search with license filter
rstar search "web" --license MIT

# Combine filters
rstar search "web" --category web --license Apache-2.0
```

### Search Tips

- **Use Specific Terms:** "sqlite" works better than "database"
- **Combine Keywords:** "http web server" narrows results
- **Check Categories:** Filter by category for faster results
- **Use Sort:** Sort by downloads for popular packages

---

## Installing Packages

### Using rstar CLI

#### Basic Installation

```bash
# Install latest version
rstar install database-sqlite

# Install specific version
rstar install database-sqlite@1.2.0

# Verbose output (see installation steps)
rstar install database-sqlite --verbose
```

#### Installation Process

```
1. ✅ Search package in registry
2. ✅ Download package tarball
3. ✅ Verify checksums
4. ✅ Extract to ~/.rads/plugins/
5. ✅ Update plugins.lock
6. ✅ Installation complete
```

#### Listing Installed Packages

```bash
# List all installed plugins
rstar list

# Output example:
# database-sqlite @ 1.2.0 [/home/user/.rads/plugins/database-sqlite] (entry=main.rads)
# web-framework @ 2.0.1 [/home/user/.rads/plugins/web-framework] (entry=main.rads)
```

#### Running Installed Packages

```bash
# Run package with rads
rstar run database-sqlite

# Run with additional arguments
rstar run web-framework --server --port 8080
```

### Using Package in RADS Code

Once installed, import and use:

```rads
import database-sqlite

blast main() {
    // Use package
    turbo db = database_sqlite.open(":memory:");
    db.execute("CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
    
    echo("Package loaded successfully!");
}
```

---

## Publishing Packages

### Prerequisites

Before publishing, ensure you have:

1. **RADS Registry Account**
   - GitHub account
   - Login at registry.rads-lang.org/login

2. **GitHub Repository**
   - Package source code repository
   - `package.rads` manifest file in root

3. **GitHub Access Token (PAT)**
   - Generate at: https://github.com/settings/tokens
   - Permissions: `repo:read`, `issues:write`
   - Add as secret: `GH_REGISTRY_TOKEN`

### Creating package.rads

Required fields:
```rads
name = "my-package"
version = "1.0.0"
description = "Brief description of your package"
author = "your-github-username"
```

Optional fields:
```rads
license = "MIT"        # Package license (default: MIT)
homepage = "https://..."  # Package documentation URL
keywords = ["keyword1", "keyword2"]  # Comma-separated
```

Example package.rads:
```rads
name = "my-awesome-lib"
version = "1.0.0"
description = "An awesome library for RADS that does amazing things"
author = "rads-dev"
license = "MIT"
homepage = "https://github.com/rads-dev/my-awesome-lib"
keywords = ["utils", "helper", "awesome"]
```

### Publishing via GitHub Workflow

**Recommended Method:** Automated publishing via GitHub Release

1. **Add Workflow to Repository**
   - Create `.github/workflows/publish-to-registry.yml`
   - Copy from Rads repository workflow
   - Add GitHub secret: `GH_REGISTRY_TOKEN`

2. **Update Version and Push**
   ```bash
   # Update version
   echo 'version = "1.0.0"' > package.rads
   
   # Commit and push
   git add package.rads
   git commit -m "Bump version to 1.0.0"
   git push origin main
   ```

3. **Create GitHub Release**
   ```bash
   # Using GitHub CLI
   gh release create v1.0.0 --title "v1.0.0" --notes "Release notes"
   
   # Or via GitHub UI
   # Go to Releases → Create a new release
   # Tag: v1.0.0
   # Title: v1.0.0
   # Description: Release notes
   ```

4. **Automatic Publishing**
   - GitHub Action triggers automatically
   - Package appears on registry.rads-lang.org
   - Metadata updated in rads-packages repository

See: [PUBLISHING_WORKFLOW.md](../registry-architecture/PUBLISHING_WORKFLOW.md) for full details.

### Publishing via Web Form

**Alternative Method:** Manual publishing via web UI

1. **Login to Registry**
   - Visit: https://registry.rads-lang.org/login
   - Click "Sign in with GitHub"
   - Authorize RADS Registry app

2. **Navigate to Publish Page**
   - Click "Publish Package" in navbar
   - Fill package form

3. **Fill Package Information**
   - Package Name (required, lowercase, hyphens only)
   - Display Name (optional, defaults to package name)
   - Description (required)
   - Version (required, semver format: x.y.z)
   - Repository URL (required, GitHub repo link)
   - Homepage URL (optional, documentation site)
   - Category (optional, dropdown selection)
   - License (optional, default: MIT)
   - Keywords (optional, comma-separated)
   - README (required, Markdown supported)

4. **Submit Package**
   - Click "Publish Package" button
   - Wait for success message
   - Redirect to package detail page

### Package Verification

After publishing:

1. **Check Package Page**
   - Visit: https://registry.rads-lang.org/packages/your-package
   - Verify metadata displays correctly
   - Check version and download URL

2. **Test Installation**
   ```bash
   rstar install your-package
   rstar list
   rstar run your-package
   ```

3. **Check Database**
   - Visit rads-packages GitHub repository
   - Verify Issue created with correct labels
   - Check metadata JSON in Issue body

---

## Package Authentication

### GitHub OAuth Login

The RADS Registry uses GitHub OAuth 2.0 for authentication:

**Benefits:**
- No need to create separate account
- Use existing GitHub credentials
- Secure, industry-standard authentication
- Manage access in GitHub settings

### Login Process

1. **Visit Login Page**
   - Navigate to: https://registry.rads-lang.org/login
   - Click "Sign in with GitHub"

2. **Authorize RADS Registry**
   - GitHub prompts for authorization
   - Review permissions requested:
     - `read:user` - Read public profile
     - `read:org` - Read organization membership
     - `repo` - Read and write repository access

3. **Grant Permissions**
   - Click "Authorize zarigata/rads-registry"
   - Redirect back to registry

4. **Session Active**
   - JWT token generated
   - Session stored in browser
   - Valid for 24 hours
   - Auto-renewal available

### Protected Actions

**Authentication Required For:**
- 📝 Publishing packages
- ✅ (Protected pages coming soon: managing your packages)
- ✅ (Protected pages coming soon: viewing analytics)

**No Authentication Required For:**
- Browsing packages
- Viewing package details
- Searching packages
- Reading documentation

### Session Management

**Login Duration:** 24 hours
**Renewal:** Automatic when expired
**Logout:** Clear browser cookies or sign out button

---

## Troubleshooting

### Installation Issues

#### Problem: "package.rads not found"

**Cause:** Repository missing required manifest file

**Solution:**
```rads
# Create package.rads in repository root
name = "my-package"
version = "1.0.0"
description = "Package description"
author = "github-username"
```

#### Problem: "version must be in semantic versioning format"

**Cause:** Version doesn't follow x.y.z format

**Solution:**
```rads
# Invalid:
version = "1.0"
version = "v1.0.0"
version = "1.0.0-beta"

# Valid:
version = "1.0.0"
version = "2.3.1"
version = "10.20.30"
```

#### Problem: "Package not found in registry"

**Cause:** Package not published or search query incorrect

**Solution:**
- Verify package name spelling
- Check if package is published on registry
- Try browsing all packages instead of search
- Contact package maintainer

### Publishing Issues

#### Problem: "Workflow failed: GH_REGISTRY_TOKEN not found"

**Cause:** GitHub secret not configured

**Solution:**
1. Go to package repository on GitHub
2. Settings → Secrets and variables → Actions
3. Click "New repository secret"
4. Name: `GH_REGISTRY_TOKEN`
5. Value: Your PAT with `issues:write`, `repo:read` permissions
6. Save secret
7. Re-run workflow

#### Problem: "Package not appearing in registry after release"

**Cause:** Workflow failed or timing delay

**Solution:**
1. Check Actions tab in package repository
2. View "Publish to RADS Registry" workflow logs
3. Fix any errors shown
4. Workflow will auto-retry on next push

#### Problem: "Error: User X is not authorized to publish packages as Y"

**Cause:** package.rads `author` doesn't match GitHub username

**Solution:**
```rads
# Update package.rads to match your GitHub username
author = "your-correct-github-username"

# Commit and push changes
git add package.rads
git commit -m "Fix author"
git push origin main

# Create new release
```

### Authentication Issues

#### Problem: "Authentication Failed"

**Cause:** OAuth callback error or missing code

**Solution:**
- Clear browser cookies
- Try logging in again
- Ensure network connectivity
- Check GitHub status page

#### Problem: "Token expired"

**Cause:** 24-hour session timeout

**Solution:**
- Logout (if possible)
- Login again to get fresh token
- Session automatically renews on next login

---

## Best Practices

### For Package Authors

**1. Semantic Versioning**
- Follow strict semver: `MAJOR.MINOR.PATCH`
- Increment appropriately:
  - MAJOR: Breaking changes
  - MINOR: New features (backward compatible)
  - PATCH: Bug fixes (backward compatible)

**2. Clear Descriptions**
- One sentence summary
- What the package does
- Target use cases
- Include key features

**3. Accurate Keywords**
- 3-5 relevant keywords
- Use common terms
- Avoid overly specific terms
- Consider search behavior

**4. Proper Licensing**
- Choose OSI-approved license
- Include LICENSE file in repository
- Specify in package.rads
- Be consistent with dependencies

**5. Documentation**
- Complete README.md
- Installation instructions
- Usage examples
- API documentation (if applicable)
- Include quick start examples

### For Package Users

**1. Search Effectively**
- Use specific terms
- Filter by category
- Check license compatibility
- Verify compatibility with your RADS version

**2. Review Before Installing**
- Check package description
- Verify keywords match your needs
- Check last updated date
- Read reviews or issues (when available)

**3. Manage Versions**
- Pin specific versions for stability
- Update regularly for bug fixes
- Test updates in development environment first
- Use semantic version ranges when supported

**4. Report Issues**
- Report bugs to package maintainers
- Open issues in package repository
- Provide clear reproduction steps
- Include environment information

### Security Best Practices

**1. Verify Package Integrity**
- Check download checksums
- Verify source code before running
- Use published releases only
- Report suspicious packages

**2. Use Trusted Packages**
- Check package author
- Verify GitHub repository
- Check for recent activity
- Read code reviews (if available)

**3. Keep Dependencies Updated**
- Update regularly for security patches
- Monitor security advisories
- Use version pinning when needed
- Test updates before production use

---

## Getting Help

### Documentation

- **User Guide:** You are here
- **Package Author Guide:** [AUTHOR_GUIDE.md](AUTHOR_GUIDE.md)
- **API Documentation:** [API_REFERENCE.md](API_REFERENCE.md)
- **CLI Documentation:** [CLI_GUIDE.md](CLI_GUIDE.md)

### Community

- **GitHub Issues:** https://github.com/zarigata/rads/issues
- **GitHub Discussions:** https://github.com/zarigata/rads/discussions
- **Documentation Issues:** Report docs issues here

### Quick Links

- **Registry:** https://registry.rads-lang.org
- **RADS Project:** https://github.com/zarigata/rads
- **rads-packages Database:** https://github.com/zarigata/rads-packages

---

**Last Updated:** January 26, 2026
**Version:** 1.0

*"Discover, install, and publish RADS packages with ease!"* 🚀
