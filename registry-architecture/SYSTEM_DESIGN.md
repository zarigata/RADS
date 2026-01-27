# RADS Package Registry - System Design Document

**Version:** 1.0
**Status:** Design Phase
**Date:** January 26, 2026

---

## Executive Summary

The RADS Package Registry is a comprehensive package management platform designed for the RADS ecosystem. It provides npm-style functionality including package discovery, installation, publishing, and user management. The registry is built entirely on GitHub infrastructure (GitHub Pages + GitHub Actions) to minimize costs and leverage existing tooling.

### Key Features
- **Full npm-style registry**: Browse, search, install packages
- **User authentication**: GitHub OAuth integration
- **Package publishing**: Automated via GitHub Actions
- **Version management**: Semantic versioning support
- **Download statistics**: Track package popularity
- **GitHub-native**: All packages stored in GitHub Releases

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        User Layer                              │
├─────────────────────────────────────────────────────────────────┤
│  CLI (rstar)      │  Web UI (registry.rads-lang.org)        │
└─────────┬───────────┴───────────────────┬─────────────────────┘
          │                               │
          │ HTTP/REST API                 │ GitHub OAuth
          ▼                               ▼
┌─────────────────────────────────────────────────────────────────┐
│                      GitHub Actions API                        │
│  (Serverless functions triggered by webhooks/API Gateway)      │
└─────────────────────────┬───────────────────────────────────────┘
                          │
          ┌───────────────┼───────────────┐
          │               │               │
          ▼               ▼               ▼
┌──────────────────┐ ┌────────────────┐ ┌────────────────┐
│  GitHub Pages   │ │  GitHub Issues │ │ GitHub Releases│
│  (Frontend)     │ │ (Package DB)   │ │ (Package Store)│
└─────────────────┘ └────────────────┘ └────────────────┘
```

---

## Core Components

### 1. Frontend (GitHub Pages)

**Technology Stack:**
- **Framework:** Next.js 14 (App Router)
- **Styling:** Tailwind CSS
- **UI Components:** shadcn/ui (professional, accessible)
- **State Management:** React Context + SWR for data fetching
- **Build:** Static export for GitHub Pages

**Pages:**
- `/` - Homepage (featured packages, quick start)
- `/packages` - Package browser with search/filters
- `/packages/[name]` - Package detail page
- `/packages/[name]/versions` - Version history
- `/publish` - Package publishing form (requires auth)
- `/login` - GitHub OAuth login page
- `/user/[username]` - User profile and packages
- `/docs` - Registry documentation

**Features:**
- Real-time search (debounced, 300ms)
- Faceted search (category, license, keywords)
- Package comparison view
- Download statistics charts
- GitHub badge integration
- Dark mode support
- Responsive design (mobile-first)

---

### 2. Backend API (GitHub Actions)

**Architecture:**
- **Serverless Functions:** GitHub Actions as HTTP endpoints
- **Runtime:** Node.js 20
- **Trigger:** Repository dispatch events + scheduled workflows
- **Response Format:** JSON (application/json)

**API Endpoints:**

```
GET  /api/packages           - List all packages (paginated)
GET  /api/packages/:name    - Get package metadata
GET  /api/packages/:name/versions - List package versions
GET  /api/search?q=keyword   - Search packages
POST /api/publish           - Publish new package (auth required)
POST /api/yank              - Yank/unpublish package version
GET  /api/stats/:name       - Get package statistics
GET  /api/user/:username   - Get user packages
POST /api/auth/login        - GitHub OAuth callback
GET  /api/auth/status       - Check authentication status
```

**Implementation:**
Each endpoint is a GitHub Actions workflow:
```yaml
# .github/workflows/api/search-packages.yml
name: Search Packages API
on:
  repository_dispatch:
    types: [api-search]
jobs:
  search:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/github-script@v7
        with:
          script: |
            // Query GitHub Issues for packages
            // Return JSON response
```

---

### 3. Database (GitHub Issues)

**Rationale:**
- Free, unlimited storage
- GitHub API built-in
- Version history automatically maintained
- Collaborative editing via PRs
- Search functionality included

**Schema:**

Each package is a GitHub Issue with specific labels and metadata:

```yaml
Title: "package-name: Package Display Name"
Labels:
  - "package"
  - "category:database"
  - "license:MIT"
  - "status:published"

Body:
```json
{
  "name": "package-name",
  "displayName": "Package Display Name",
  "description": "Package description",
  "version": "1.0.0",
  "author": "github-username",
  "repository": "https://github.com/user/repo",
  "homepage": "https://package-site.com",
  "keywords": ["database", "sql"],
  "license": "MIT",
  "readme": "Full README content...",
  "latestVersion": "1.0.0",
  "versions": [
    {
      "version": "1.0.0",
      "releaseDate": "2026-01-26",
      "releaseUrl": "https://github.com/user/repo/releases/tag/v1.0.0",
      "downloadUrl": "https://github.com/user/repo/releases/download/v1.0.0/package.tar.gz",
      "checksum": "sha256:abc123..."
    }
  ],
  "stats": {
    "downloads": 1234,
    "stars": 56,
    "lastUpdated": "2026-01-26"
  }
}
```

**Issue Labels:**
- `package` - All packages have this label
- `category:<category>` - Database, web, utils, etc.
- `license:<license>` - MIT, Apache-2.0, GPL, etc.
- `status:<status>` - published, yanked, deprecated

---

### 4. Package Storage (GitHub Releases)

**Storage Strategy:**
- Packages are released as tar.gz files
- Each version is a GitHub Release
- Release assets contain:
  - `package.tar.gz` - Package source code
  - `package.rads` - Package manifest
  - `checksums.txt` - SHA256 checksums
  - `package.radsmeta` - Plugin metadata (if applicable)

**Release Tag Format:**
```
<package-name>-v<version>
# Example:
database-sqlite-v1.0.0
```

**Release Assets:**
```
database-sqlite-1.0.0.tar.gz
database-sqlite-1.0.0.rads
database-sqlite-1.0.0.checksums.txt
```

---

### 5. Authentication (GitHub OAuth)

**Flow:**

```
1. User clicks "Login with GitHub"
2. Redirect to: https://github.com/login/oauth/authorize
   - client_id: <GitHub App ID>
   - scope: read:user, read:org, repo
   - state: <random token>
3. GitHub redirects to: https://registry.rads-lang.org/auth/callback
   - code: <authorization code>
   - state: <random token>
4. Backend exchanges code for access token
5. Token stored in encrypted session cookie
6. User authenticated
```

**GitHub App Configuration:**
- **Homepage URL:** `https://registry.rads-lang.org`
- **Callback URL:** `https://registry.rads-lang.org/api/auth/callback`
- **Scopes:** `read:user`, `read:org`, `repo`

**Session Management:**
- JWT token signed with secret
- Stored in httpOnly cookie
- Expires in 24 hours
- Refresh token available

---

### 6. CLI Integration (rstar)

**Enhanced Commands:**

```bash
# Install from registry
rstar install package-name@version

# Search registry
rstar search "database"
rstar search --category database --license MIT

# Publish package
rstar publish
# Creates GitHub Release
# Updates GitHub Issue (metadata)
# Validates package manifest

# View package info
rstar info package-name

# List installed packages
rstar list

# Uninstall package
rstar uninstall package-name

# Update package
rstar update package-name

# Login to registry
rstar login
# Opens browser for OAuth

# Check auth status
rstar whoami
```

**Installation Flow:**
```bash
rstar install database-sqlite

# Steps:
1. Query /api/packages/database-sqlite
2. Get latest version from response
3. Download tar.gz from GitHub Releases
4. Verify checksums
5. Extract to ~/.rads/plugins/database-sqlite/
6. Update plugins.lock
7. Run post-install hooks (if any)
```

---

### 7. GitHub Action for Publishing

**Workflow:**

```yaml
# .github/workflows/publish-package.yml
name: Publish RADS Package

on:
  release:
    types: [published]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Validate package.rads
        run: |
          # Parse package.rads
          # Validate required fields
          # Check version format (semver)

  update-registry:
    needs: validate
    runs-on: ubuntu-latest
    steps:
      - uses: actions/github-script@v7
        with:
          github-token: ${{ secrets.GITHUB_TOKEN }}
          script: |
            // Create or update GitHub Issue
            // Update package metadata
            // Add release URL to versions array
            // Increment download stats
```

**Manual Publishing:**
```bash
# In package repository
rstar publish
# Prompts for:
# - Package name
# - Version
# - Description
# - Category
# - License
# Creates GitHub Release
# Triggers update-registry workflow
```

---

## Data Flow Diagrams

### Package Publishing Flow

```
┌─────────┐     ┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│  Developer    │  GitHub Repo       │ GitHub Action      │ Registry DB     │
│  (rstar)  │─────▶  Releases  │─────▶  Validation │─────▶  Issue      │
└─────────┘     └─────────────┘     └──────────────┘     └─────────────┘
                     │                   │                      │
                     ▼                   ▼                      ▼
              .tar.gz file        Validate manifest     Update metadata
              uploaded          Check semver         Add version info
```

### Package Installation Flow

```
┌─────────┐     ┌─────────────┐     ┌──────────────┐     ┌─────────────┐
│  User    │     │  rstar CLI │     │  GitHub API  │     │  GitHub      │
│         │─────▶            │─────▶              │─────▶  Releases    │
└─────────┘     └─────────────┘     └──────────────┘     └─────────────┘
  install           search pkg         get latest         download
  pkg              get metadata       version            tar.gz
                                                                 │
                                                                 ▼
                                                           Extract to
                                                        ~/.rads/plugins/
```

---

## Security Considerations

### 1. Package Verification
- **Checksum validation:** SHA256 for all downloads
- **Signature verification:** Optional PGP signatures
- **Source verification:** Validate package.rads matches registry

### 2. Authentication
- **OAuth 2.0:** Industry-standard GitHub OAuth
- **Token storage:** Encrypted cookies, server-side secrets
- **CSRF protection:** State parameter in OAuth flow
- **Rate limiting:** GitHub API rate limits apply

### 3. Authorization
- **Write permissions:** Only package owners can update
- **Publish permissions:** Validated via GitHub repo access
- **Admin functions:** Only registry maintainers (GitHub Org members)

### 4. Package Safety
- **Malicious packages:** Report mechanism, takedown process
- **Supply chain attacks:** Dependency verification, SBOM support (future)
- **Code review:** PR-based metadata updates require review

---

## Performance Optimization

### 1. Caching
- **GitHub Pages:** CDN caching (1 hour)
- **API responses:** GitHub API has built-in caching
- **Search results:** Implement search index (Algolia/Meilisearch in future)

### 2. Pagination
- **Package list:** 50 packages per page
- **Versions:** All versions (typically <20)
- **Search results:** 100 per page

### 3. CDN Distribution
- **Frontend:** GitHub Pages CDN (Fastly)
- **Package downloads:** GitHub releases CDN
- **Static assets:** GitHub Pages

---

## Monitoring & Analytics

### 1. Download Statistics
- **Tracked:** Per-version, per-package downloads
- **Source:** GitHub Release download counts
- **Display:** Download badges, charts on package pages

### 2. Usage Analytics
- **Search queries:** Most searched terms
- **Install trends:** Popular packages
- **User growth:** Registered users over time

### 3. Error Tracking
- **API errors:** Logged via GitHub Actions logs
- **Frontend errors:** Sentry integration (optional)
- **Package validation failures:** Reported to maintainer

---

## Scalability

### Current Limits (GitHub Free Tier)
- **API rate limit:** 5,000 requests/hour (authenticated)
- **Issues:** Unlimited per repository
- **Releases:** Unlimited
- **Storage:** 1GB soft limit, 100GB per release

### Scaling Strategy
- **Phase 1:** GitHub-native (current design)
- **Phase 2:** Add external search (Algolia/Meilisearch)
- **Phase 3:** Dedicated database (PostgreSQL) if needed
- **Phase 4:** Custom API server (if Actions become limiting)

---

## Migration Path

### From Current rstar
- **v0.1.0:** Current skeleton (no registry)
- **v0.2.0:** Add HTTP fetch from GitHub Releases
- **v0.3.0:** Full registry integration with OAuth
- **v0.4.0:** Search and browse capabilities
- **v1.0.0:** Production-ready package manager

### For Package Authors
- **Manual migration:** Upload existing packages to GitHub Releases
- **Automated migration:** CLI tool to bulk publish
- **Gradual adoption:** Keep old workflow while testing new registry

---

## Cost Analysis

### GitHub Free Tier (Current Plan)
- **Hosting:** $0 (GitHub Pages)
- **Storage:** $0 (GitHub Releases)
- **API:** $0 (GitHub Actions, rate-limited)
- **Domain:** $12/year (registry.rads-lang.org)

### Potential Future Costs
- **Domain:** $12/year
- **SSL:** $0 (Let's Encrypt)
- **Search service:** $0-50/month (Algolia free tier → paid)
- **Database:** $0-25/month (Supabase free tier → paid)

---

## Implementation Timeline

### Phase 1: Foundation (Week 1-2)
- ✅ Architecture and design
- ✅ Repository setup
- ✅ Frontend scaffolding
- ✅ Basic API endpoints (search, list)

### Phase 2: Core Features (Week 3-4)
- ✅ GitHub OAuth authentication
- ✅ Package publishing workflow
- ✅ GitHub Issues as database
- ✅ rstar CLI integration

### Phase 3: UI/UX Polish (Week 5-6)
- ✅ Package detail pages
- ✅ User profiles
- ✅ Search and filtering
- ✅ Download statistics

### Phase 4: Testing & Launch (Week 7-8)
- ✅ End-to-end testing
- ✅ Performance optimization
- ✅ Documentation
- ✅ Public launch

---

## Success Metrics

### Technical Metrics
- **API response time:** <200ms (95th percentile)
- **Page load time:** <2s (3G connection)
- **Uptime:** >99.9%
- **Package installation time:** <30s (average)

### Adoption Metrics
- **Registered users:** 100+ in first month
- **Published packages:** 50+ in first month
- **Downloads:** 1,000+ in first month
- **Active maintainers:** 10+ in first month

---

## Conclusion

This design provides a fully-featured, npm-style package registry for RADS using entirely free GitHub infrastructure. The architecture is:

- **Cost-effective:** $12/year for domain only
- **Scalable:** Can grow to thousands of packages
- **Maintainable:** Leverages existing GitHub tooling
- **Professional:** Industry-standard features and UX
- **Secure:** OAuth, checksums, access controls

The registry will significantly improve the RADS developer experience and accelerate ecosystem growth.

---

**Next Steps:**
1. Review and approve this design
2. Create GitHub repositories (frontend, docs)
3. Begin frontend development (Phase 1)
4. Implement API endpoints (Phase 2)
5. Integrate with rstar CLI (Phase 3)
6. Testing and launch (Phase 4)

---

**Document Version:** 1.0
**Last Updated:** January 26, 2026
**Author:** RADS Registry Team
