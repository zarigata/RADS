# RADS Registry - Repository Setup Guide

**Purpose:** Complete guide for setting up the RADS Package Registry repositories on GitHub.

---

## Repository Structure

We need 2 separate repositories:

```
1. rads-registry          (Frontend - GitHub Pages)
2. rads-packages          (Database - GitHub Issues + Releases)
```

---

## Repository 1: rads-registry

**Purpose:** Frontend web application (Next.js)
**Hosting:** GitHub Pages
**URL:** `https://registry.rads-lang.org`

### Setup Steps

#### 1. Create Repository

```bash
# Via GitHub UI:
# 1. Go to https://github.com/new
# 2. Repository name: rads-registry
# 3. Public repository
# 4. Initialize with: Add a README file
# 5. Create repository
```

#### 2. Clone and Initialize Next.js

```bash
# Clone the repository
git clone git@github.com:zarigata/rads-registry.git
cd rads-registry

# Create Next.js app (App Router, TypeScript, Tailwind, ESLint)
npx create-next-app@latest . --typescript --tailwind --eslint --app --src-dir --import-alias "@/*"

# Remove default files
rm -rf public app/*.tsx app/*.css
```

#### 3. Install Dependencies

```bash
# Install shadcn/ui components (professional UI library)
npx shadcn@latest init -y -d

# Install additional dependencies
npm install swr axios jose
npm install -D @types/node

# Install shadcn components we'll need
npx shadcn@latest add button card input search select badge avatar
npx shadcn@latest add tabs dropdown-menu dialog
npx shadcn@latest add form label textarea skeleton
```

#### 4. Configure GitHub Pages

Create `.github/workflows/deploy.yml`:

```yaml
name: Deploy to GitHub Pages

on:
  push:
    branches: [main]
  workflow_dispatch:

permissions:
  contents: read
  pages: write
  id-token: write

concurrency:
  group: "pages"
  cancel-in-progress: false

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Setup Node.js
        uses: actions/setup-node@v4
        with:
          node-version: 20

      - name: Install dependencies
        run: npm ci

      - name: Build
        run: npm run build
        env:
          NEXT_PUBLIC_GITHUB_CLIENT_ID: ${{ secrets.GITHUB_CLIENT_ID }}
          NEXT_PUBLIC_API_BASE: https://api.registry.rads-lang.org

      - name: Upload artifact
        uses: actions/upload-pages-artifact@v3
        with:
          path: ./out

  deploy:
    runs-on: ubuntu-latest
    needs: build
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    steps:
      - name: Deploy to GitHub Pages
        id: deployment
        uses: actions/deploy-pages@v4
```

#### 5. Configure Next.js for Static Export

Update `next.config.mjs`:

```javascript
/** @type {import('next').NextConfig} */
const nextConfig = {
  output: 'export',
  images: {
    unoptimized: true,
  },
  trailingSlash: true,
}

export default nextConfig
```

#### 6. Create Project Structure

```bash
# Create directory structure
mkdir -p src/app/{packages,publish,login,user,docs}
mkdir -p src/app/api/{packages,search,auth,stats}
mkdir -p src/components/{ui,layout,package}
mkdir -p src/lib/{auth,api,utils}
mkdir -p src/types

# Create environment variables file
cat > .env.local << EOF
NEXT_PUBLIC_GITHUB_CLIENT_ID=your_client_id_here
GITHUB_CLIENT_SECRET=your_client_secret_here
NEXT_PUBLIC_API_BASE=https://api.registry.rads-lang.org
NEXT_PUBLIC_REGISTRY_URL=https://registry.rads-lang.org
JWT_SECRET=your_jwt_secret_here
EOF
```

#### 7. GitHub Secrets

Add these secrets to repository (Settings → Secrets and variables → Actions):

```
GITHUB_CLIENT_ID          - From GitHub App
GITHUB_CLIENT_SECRET      - From GitHub App
JWT_SECRET               - Random 64-character string
```

#### 8. Enable GitHub Pages

1. Go to repository Settings
2. Pages section
3. Build and deployment:
   - Source: GitHub Actions
4. Save

#### 9. Custom Domain (Optional)

1. Add `CNAME` file in `public/`:
   ```
   registry.rads-lang.org
   ```

2. Configure DNS records for `registry.rads-lang.org`

---

## Repository 2: rads-packages

**Purpose:** Package database (GitHub Issues) + Storage (GitHub Releases)
**URL:** `https://github.com/zarigata/rads-packages`

### Setup Steps

#### 1. Create Repository

```bash
# Via GitHub UI:
# 1. Go to https://github.com/new
# 2. Repository name: rads-packages
# 3. Public repository
# 4. Initialize with: Add a README file
# 5. Create repository
```

#### 2. Configure Labels

Create these repository labels (Settings → Labels):

```
package                # Color: 1f883d (green)
status:published      # Color: 0e8a16 (dark green)
status:yanked        # Color: cf222e (red)
status:deprecated     # Color: 9e6a03 (yellow)
category:database    # Color: 0969da (blue)
category:web         # Color: 0969da (blue)
category:utils       # Color: 0969da (blue)
category:media       # Color: 0969da (blue)
category:networking  # Color: 0969da (blue)
license:MIT          # Color: 8a2be2 (purple)
license:Apache-2.0   # Color: 8a2be2 (purple)
license:GPL-3.0      # Color: 8a2be2 (purple)
license:BSD-3        # Color: 8a2be2 (purple)
```

#### 3. Create GitHub App for OAuth

**Step 1: Create GitHub App**
1. Go to: https://github.com/settings/apps
2. Click "New GitHub App"
3. Fill in details:
   - **GitHub App name:** RADS Registry
   - **Homepage URL:** `https://registry.rads-lang.org`
   - **Callback URL:** `https://registry.rads-lang.org/api/auth/callback`
   - **Webhook URL:** Leave blank
4. Permissions:
   - Repository permissions:
     - Issues: Read & Write
     - Contents: Read & Write
   - User permissions:
     - Read all user data
5. Click "Create GitHub App"

**Step 2: Generate Private Key**
1. Scroll to "Private keys"
2. Click "Generate a private key"
3. Download `rads-registry.<timestamp>.pem`
4. Store securely (this is needed for server-side authentication)

**Step 3: Get Credentials**
1. Note down:
   - **App ID:** Displayed at top of app page
   - **Client ID:** Displayed in "General" section
   - **Client Secret:** Click "Generate a new client secret" to get this

**Step 4: Add to GitHub Secrets**
Add these secrets to `rads-registry` repository:
```
GITHUB_APP_ID         - From step 3
GITHUB_CLIENT_ID      - From step 3
GITHUB_CLIENT_SECRET  - From step 3
GITHUB_PRIVATE_KEY    - Content of .pem file (as single line)
```

#### 4. Create Initial README

Create `README.md` in `rads-packages`:

```markdown
# RADS Package Registry Database

This repository contains the RADS Package Registry database.

## Structure

- **Issues:** Package metadata (database records)
- **Releases:** Package files (storage)

## Package Metadata Format

Each package is a GitHub Issue with:
- **Title:** `package-name: Display Name`
- **Labels:** `package`, `category:xxx`, `license:xxx`, `status:published`
- **Body:** JSON metadata with version info, download URLs, stats

## Publishing Packages

Packages are published via:
1. GitHub Release in package repository
2. Automated workflow updates this repository's Issues

See [rads-registry](https://github.com/zarigata/rads-registry) for full documentation.
```

---

## Integration: Linking Repositories

### 1. GitHub Actions for Package Publishing

Create `.github/workflows/publish.yml` in package repositories:

```yaml
name: Publish to RADS Registry

on:
  release:
    types: [published]

permissions:
  contents: read
  issues: write

jobs:
  publish:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Update Registry Database
        uses: actions/github-script@v7
        with:
          github-token: ${{ secrets.GH_REGISTRY_TOKEN }}
          script: |
            // Parse package.rads
            // Create/update issue in zarigata/rads-packages
            // Add release info to metadata
```

**Note:** `GH_REGISTRY_TOKEN` is a PAT (Personal Access Token) with `issues:write` permission on `rads-packages` repository.

### 2. API Gateway (GitHub Actions)

The `rads-registry` repository will use GitHub Actions as serverless functions. These will be triggered by:

1. **Repository dispatch events:** For API calls
2. **Webhooks:** For package updates
3. **Scheduled runs:** For statistics aggregation

---

## Verification Checklist

### rads-registry
- [ ] Repository created
- [ ] Next.js app initialized
- [ ] Tailwind CSS configured
- [ ] shadcn/ui components installed
- [ ] GitHub Actions workflow created
- [ ] GitHub Pages enabled
- [ ] Environment variables set
- [ ] GitHub App credentials added as secrets
- [ ] Build succeeds locally: `npm run build`
- [ ] Deploy succeeds to GitHub Pages

### rads-packages
- [ ] Repository created
- [ ] Labels configured
- [ ] GitHub App created
- [ ] OAuth credentials obtained
- [ ] README documentation added

---

## Next Steps

After repository setup:
1. Start building frontend pages (Task 5)
2. Implement API endpoints (Task 4)
3. Add authentication flow (Task 6)
4. Integrate with rstar CLI (Task 7)
5. Test publishing workflow (Task 8)

---

**Last Updated:** January 26, 2026
**Version:** 1.0
