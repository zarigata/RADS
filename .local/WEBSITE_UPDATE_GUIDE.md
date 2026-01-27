# 🌐 WEBSITE UPDATE GUIDE - LOCAL ONLY

**Purpose:** Guide for updating RADS website and documentation on every release
**Usage:** Follow this guide when preparing website updates for a new version
**Status:** INTERNAL - DO NOT PUBLISH

---

## 📂 Website Structure

### Directory Layout

```
docs/
├── index.html              # Homepage (shows latest version)
├── releases.html           # All release notes
├── guide.html              # User guide
├── compare.html           # Language comparison
├── documentation.html      # API documentation
├── examples.html          # Code examples
├── playground.html        # Interactive code editor
├── style.css             # Global stylesheet
└── VERSION_TRACKING.md    # (Local only) Version mapping
```

### GitHub Pages Deployment

**Repository:** zarigata/rads
**Branch:** gh-pages
**URL:** https://zarigata.github.io/RADS/

**Workflow:** `.github/workflows/gh-pages.yml`
**Trigger:** Push to main branch (docs/ folder)

---

## 🎯 Version Information Updates

### Files Requiring Version Updates

| File | Version Location | Pattern | Update Command |
|-------|-----------------|----------|----------------|
| `index.html` | Badge, hero section | Find `0.0.6`, replace with new version |
| `index.html` | Download buttons | Update `v0.0.6` links |
| `releases.html` | Add new entry | Copy template, update with v0.0.7 info |
| `documentation.html` | Version footer | Update version reference |
| `guide.html` | Version examples | Update code examples |
| `README.md` | Version badge | Update badge URL |
| `CHANGELOG.md` | Version header | Add new section |

### Search & Replace Pattern

```bash
# Find all version references in docs/
grep -r "0\.0\.6" docs/ --include="*.html"

# Replace version (example: 0.0.6 → 0.0.7)
find docs/ -name "*.html" -exec sed -i 's/0\.0\.6/0.0.7/g' {} \;

# Verify replacements
grep -r "0\.0\.7" docs/ --include="*.html"
```

---

## 📝 Homepage Updates (index.html)

### Hero Section

**Location:** Top of page, prominent display

**Updates Required:**
1. **Version Badge**
   ```html
   <!-- Before -->
   <span class="version-badge">v0.0.6</span>
   
   <!-- After -->
   <span class="version-badge">v0.0.7</span>
   ```

2. **Download Buttons**
   ```html
   <!-- Before -->
   <a href="https://github.com/zarigata/rads/releases/download/v0.0.6/rads-0.0.6-linux-x86_64.tar.gz" 
      class="btn-download">
       Download v0.0.6
   </a>
   
   <!-- After -->
   <a href="https://github.com/zarigata/rads/releases/download/v0.0.7/rads-0.0.7-linux-x86_64.tar.gz" 
      class="btn-download">
       Download v0.0.7
   </a>
   ```

3. **Codename Display**
   ```html
   <!-- Before -->
   <h2>FIGHTER JET - Mach Speed Performance</h2>
   
   <!-- After -->
   <h2>{NEW_CODENAME} - {Theme Description}</h2>
   ```

### Feature Cards

**Location:** Features section

**Updates Required:**
- Add new feature cards for v0.0.7 capabilities
- Update existing cards if features enhanced
- Remove deprecated feature cards (if any)

**Template:**
```html
<div class="feature-card">
  <h3>Feature Name</h3>
  <p>Feature description</p>
  <code>code_example.rads</code>
  <a href="documentation.html#feature">Learn More →</a>
</div>
```

### Quick Start Section

**Updates Required:**
- Update version in code examples
- Update download link
- Verify installation commands work

---

## 📄 Releases Page (releases.html)

### Add New Release Entry

**Location:** In reverse chronological order (newest first)

**Template:**
```html
<section class="release">
  <div class="release-header">
    <h3>🛩️ RADS v0.0.7 "{Codename}"</h3>
    <span class="release-date">Released: January 27, 2026</span>
  </div>
  
  <div class="release-body">
    <h4>What's New</h4>
    <ul>
      <li><strong>Feature Name:</strong> Description</li>
      <li><strong>Feature Name:</strong> Description</li>
      <li><strong>Performance:</strong> {Benchmark results}</li>
    </ul>
    
    <h4>Breaking Changes</h4>
    {List breaking changes or "None"}
    
    <h4>Bug Fixes</h4>
    <ul>
      <li>Fixed issue description</li>
      <li>Fixed issue description</li>
    </ul>
    
    <div class="release-downloads">
      <a href="https://github.com/zarigata/rads/releases/download/v0.0.7/rads-0.0.7-linux-x86_64.tar.gz" 
         class="btn-download">
        Download v0.0.7
      </a>
      <a href="https://github.com/zarigata/rads/releases/tag/v0.0.7" 
         class="btn-github">
        GitHub Release
      </a>
    </div>
    
    <div class="release-notes">
      <h4>Documentation</h4>
      <ul>
        <li><a href="V0.0.7_{CODENAME}_SUMMARY.md">Release Summary</a></li>
        <li><a href="CHANGELOG.md#0.0.7">Full Changelog</a></li>
        <li><a href="V0.0.7_QUICK_REFERENCE.md">Quick Reference</a></li>
      </ul>
    </div>
  </div>
</section>
```

### Archive Old Releases

**Location:** Bottom of page

**Action:** Move releases older than 3 versions to archive section

**Archive Template:**
```html
<section class="release-archive">
  <h4>Archive (Older Releases)</h4>
  <details>
    <summary>Show Archived Releases</summary>
    <div class="archived-releases">
      <!-- v0.0.5 -->
      <div class="release-archived">
        <h5>🎭 RADS v0.0.5 "CHAMELEON"</h5>
        <p>Released: January 19, 2026</p>
        <a href="V0.0.5_CHAMELEON_SUMMARY.md">Summary</a>
        <a href="https://github.com/zarigata/rads/releases/tag/v0.0.5">Release</a>
      </div>
      
      <!-- v0.0.4 -->
      <div class="release-archived">
        <h5>🌌 RADS v0.0.4 "Constellation"</h5>
        <p>Released: January 13, 2026</p>
        <a href="V0.0.4_CONSTELLATION_SUMMARY.md">Summary</a>
        <a href="https://github.com/zarigata/rads/releases/tag/v0.0.4">Release</a>
      </div>
    </div>
  </details>
</section>
```

---

## 📚 Documentation Updates

### Create Quick Reference Guide

**File:** `docs/V{VERSION}_QUICK_REFERENCE.md`

**Sections Required:**
1. Quick Start (installation, first program)
2. New Features Overview (table)
3. Detailed usage for each new feature
4. Performance benchmarks
5. Migration guide from previous version
6. Known limitations
7. What's next (planned features)
8. Links to full documentation

**Template:** Use `docs/V0.0.6_QUICK_REFERENCE.md` as template

### Update Documentation Hub

**File:** `docs/README.md`

**Updates Required:**
- Add new version to version list
- Update documentation status table
- Add new quick reference guide link
- Update "last updated" timestamps

### Update Examples Guide

**File:** `examples/README.md`

**Updates Required:**
- Add v0.0.7 section with new examples
- Update learning paths
- Add difficulty ratings for new examples
- Update feature tables

---

## 🎨 Style & Visual Updates

### Color Themes

**Version-Based Themes:**

| Version | Codename | Primary Color | Secondary Color | Accent Color |
|---------|-----------|---------------|-----------------|--------------|
| 0.0.6 | FIGHTER JET | #1a1a2a (jet black) | #2d2d2d (dark gray) | #4a90e2 (sky blue) |
| 0.0.5 | CHAMELEON | #1e3a8a (chameleon green) | #2e5c46 (bright green) | #7cb342 (vibrant green) |
| 0.0.4 | Constellation | #1a237e (space blue) | #2e3a52 (stellar purple) | #4a90e2 (comet blue) |
| 0.0.3 | Butterfly | #e91e63 (butterfly orange) | #ff6b35 (wing orange) | #ffd93d (light yellow) |

**Update Process:**
1. Add new version color scheme to `docs/style.css`
2. Use CSS variables for easy switching:
   ```css
   :root {
       --primary-color: #1a1a2a;
       --secondary-color: #2d2d2d;
       --accent-color: #4a90e2;
   }
   ```
3. Apply colors to version-specific sections

### Hero Banners

**Updates Required:**
- Create new hero banner for version codename
- Add version-specific background graphics
- Update tagline/slogan

**Location:** Top of `index.html`, `guide.html`

---

## 🔍 Testing Website Updates

### Local Testing

**Method 1: Python HTTP Server**
```bash
cd docs/
python3 -m http.server 8000

# Open in browser
firefox http://localhost:8000
```

**Method 2: Node.js Serve**
```bash
cd docs/
npx serve

# Opens automatically at http://localhost:3000
```

**Method 3: PHP Built-in Server**
```bash
cd docs/
php -S localhost:8000
```

### Testing Checklist

- [ ] Homepage loads without errors
- [ ] Version badge shows correct version
- [ ] Download links work (test actual download)
- [ ] Release notes page displays correctly
- [ ] All internal links work
- [ ] Code examples formatted properly
- [ ] Mobile responsive (test on phone)
- [ ] Different browsers tested:
  - [ ] Chrome/Chromium
  - [ ] Firefox
  - [ ] Safari (if available)
  - [ ] Edge (if available)
- [ ] Images and assets load
- [ ] No console errors (check DevTools)
- [ ] Search functionality works (if present)
- [ ] Navigation menu functional

### Cross-Reference Check

**Action:** Verify links to external documentation

```bash
# Check for broken links
grep -oP 'href="[^"]*"' docs/*.html | sort -u

# Test each link manually or with link checker
```

---

## 📤 GitHub Actions Deployment

### Trigger Deployment

**File:** `.github/workflows/gh-pages.yml`

**Process:**
1. Commit all website updates to main branch
2. Push to GitHub
3. GitHub Actions automatically deploys to gh-pages branch
4. Site updates within 1-3 minutes

**To Trigger Manually:**
```bash
# Push changes
git add docs/
git commit -m "docs: update to v0.0.7"
git push origin main

# Monitor workflow
gh workflow view gh-pages
```

### Verification

**Check deployment:**
```bash
# Wait for workflow to complete
gh run list --workflow=gh-pages

# View logs if needed
gh run view <run-id>

# Verify site is live
curl -I https://zarigata.github.io/RADS/
```

---

## 🔄 Update Workflow

### Step-by-Step Process

1. **Preparation Phase**
   - [ ] Identify all files needing updates
   - [ ] Review VERSION_TRACKING.md for list
   - [ ] Gather new version information
   - [ ] Prepare new content (copy-edit-paste templates)

2. **Update Phase**
   - [ ] Update `index.html` (homepage)
   - [ ] Update `releases.html` (add new release)
   - [ ] Create `V{VERSION}_QUICK_REFERENCE.md`
   - [ ] Update `docs/README.md`
   - [ ] Update `examples/README.md`
   - [ ] Update `style.css` (if new color theme)
   - [ ] Verify all links

3. **Testing Phase**
   - [ ] Start local HTTP server
   - [ ] Test in multiple browsers
   - [ ] Check mobile responsiveness
   - [ ] Verify all links work
   - [ ] Check for console errors

4. **Deployment Phase**
   - [ ] Commit all changes
   - [ ] Push to GitHub
   - [ ] Monitor GitHub Actions workflow
   - [ ] Verify deployment completes
   - [ ] Test live site

5. **Post-Deployment Phase**
   - [ ] Verify site is live
   - [ ] Test download links work
   - [ ] Check for any deployment errors
   - [ ] Update VERSION_TRACKING.md (deployment complete)

---

## 🔍 Quality Assurance

### Content Quality

- [ ] Spelling checked
- [ ] Grammar verified
- [ ] Technical accuracy confirmed
- [ ] Code examples tested
- [ ] Links verified (no 404s)
- [ ] Consistent terminology used
- [ ] Formatting consistent across pages

### Accessibility

- [ ] Alt text on all images
- [ ] Heading hierarchy correct (h1 > h2 > h3)
- [ ] Color contrast WCAG AA compliant
- [ ] Keyboard navigation works
- [ ] Screen reader compatible
- [ ] Forms properly labeled (if any)

### Performance

- [ ] Page load time < 3 seconds
- [ ] Images optimized (compressed)
- [ ] CSS/JS minified (if applicable)
- [ ] Caching headers set (via GitHub Pages)
- [ ] No blocking scripts

---

## 📞 Rollback Plan

### If Deployment Fails

1. Revert commit: `git revert HEAD`
2. Push: `git push origin main`
3. GitHub Actions automatically reverts deployment

### If Critical Issues Found Post-Deployment

1. Immediately update docs/ (hotfix)
2. Commit and push
3. GitHub Actions auto-deploys
4. Verify fix deployed

---

## 📊 Deployment Checklist

### Pre-Deployment

- [ ] All website files updated
- [ ] Local testing completed
- [ ] Cross-browser testing done
- [ ] Mobile verified
- [ ] Links checked
- [ ] No console errors
- [ ] Ready to deploy

### Deployment

- [ ] Changes committed
- [ ] Pushed to main branch
- [ ] GitHub Actions triggered
- [ ] Workflow running
- [ ] Monitoring in progress

### Post-Deployment

- [ ] Deployment successful
- [ ] Live site verified
- [ ] Download links tested
- [ ] All pages loading correctly
- [ ] No console errors on live site
- [ ] Social media/announcement ready

---

## 🔗 Related Files

- `VERSION_TRACKING.md` - Complete version information
- `RELEASE_CHECKLIST.md` - Full release preparation guide
- `RELEASE_PROCESS.md` - Complete release workflow

---

**Last Updated:** January 27, 2026
**Maintained By:** Website Maintainer
**Next Review:** Before v0.0.7 website updates

---

*This file is for internal use only. Do not publish to public documentation.*
