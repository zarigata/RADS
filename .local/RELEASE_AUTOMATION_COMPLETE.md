# 🎯 COMPLETE VERSION MANAGEMENT SYSTEM
**Purpose:** Master reference for all automation files
**Usage:** For understanding the complete version update workflow
**Status:** INTERNAL - DO NOT PUBLISH

---

## 📁 FILE STRUCTURE

All automation files are in `.local/` directory:

```
.local/
├── VERSION_TRACKING.md              # Main version status and locations
├── VERSION_PLACEMENTS.md           # All files with version strings
├── GITHUB_RELEASE_WORKFLOW.md      # GitHub Actions for releases
├── WEBSITE_VERSION_UPDATE.md       # Website documentation updates
├── ENVIRONMENT_CLEANUP.md          # Cleanup policies and scripts
└── README.md                      # This file (overview)
```

---

## 🔄 COMPLETE WORKFLOW

### 1. Pre-Release Phase
**Files:** `VERSION_TRACKING.md`, `VERSION_PLACEMENTS.md`

**Actions:**
1. Read `VERSION_TRACKING.md` to understand current version
2. Review `VERSION_PLACEMENTS.md` for all files to update
3. Plan new version number and codename
4. Create `V{VERSION}_{CODENAME}_PLAN.md`

### 2. Development Phase
**Files:** `VERSION_PLACEMENTS.md`

**Actions:**
1. Update version strings in source code
2. Update version strings in documentation
3. Test all changes
4. Update `CHANGELOG.md` incrementally

### 3. Release Phase
**Files:** `GITHUB_RELEASE_WORKFLOW.md`, `VERSION_PLACEMENTS.md`

**Actions:**
1. Create `V{VERSION}_{CODENAME}_SUMMARY.md`
2. Finalize `CHANGELOG.md` entry
3. Update all version placements
4. Create git tag `v{VERSION}`
5. Push tag to GitHub (triggers automated release)

### 4. Post-Release Phase
**Files:** `WEBSITE_VERSION_UPDATE.md`, `ENVIRONMENT_CLEANUP.md`

**Actions:**
1. Update website documentation
2. Verify past 5 versions present
3. Run cleanup script
4. Archive previous release
5. Update `VERSION_TRACKING.md`

---

## 📊 AUTOMATION MATRIX

| Task | Input File | Output | Automation | Frequency |
|-------|-------------|---------|-------------|------------|
| Find version placements | VERSION_PLACEMENTS.md | grep search results | Manual/As needed |
| Create GitHub release | GITHUB_RELEASE_WORKFLOW.md | GitHub Release | Every release |
| Update website | WEBSITE_VERSION_UPDATE.md | Updated docs/ | Every release |
| Cleanup environment | ENVIRONMENT_CLEANUP.md | Clean repo | Every release/monthly |
| Track versions | VERSION_TRACKING.md | Version history | Continuous |

---

## 🚀 QUICK START GUIDE

### For AI Assistant: New Release

```bash
# 1. Read version tracking
read .local/VERSION_TRACKING.md

# 2. Review version placements
read .local/VERSION_PLACEMENTS.md

# 3. Update all files (use automation or manual)
# See VERSION_PLACEMENTS.md for complete list

# 4. Create release summary
# Use template from GITHUB_RELEASE_WORKFLOW.md

# 5. Tag and push
git tag v0.0.7 -m "Release v0.0.7"
git push origin v0.0.7

# 6. Monitor GitHub Actions
gh run list --workflow=release.yml

# 7. Update website
# Follow WEBSITE_VERSION_UPDATE.md

# 8. Run cleanup
.local/cleanup.sh 0.0.7
```

### For Human: Understanding Automation

1. **Version Tracking:** Read `.local/VERSION_TRACKING.md` to see current state
2. **Find Versions:** Use `.local/VERSION_PLACEMENTS.md` to locate all version strings
3. **Create Release:** Follow `.local/GITHUB_RELEASE_WORKFLOW.md` for automated release
4. **Update Website:** Follow `.local/WEBSITE_VERSION_UPDATE.md` for documentation updates
5. **Clean Up:** Use `.local/ENVIRONMENT_CLEANUP.md` to clean old files

---

## 🎯 VERIFICATION CHECKLIST

### After Release Complete

- [ ] All version strings updated (check with VERSION_PLACEMENTS.md)
- [ ] GitHub release created (check with GITHUB_RELEASE_WORKFLOW.md)
- [ ] Website updated (check with WEBSITE_VERSION_UPDATE.md)
- [ ] Past 5 versions documented (check VERSION_TRACKING.md)
- [ ] Environment cleaned (check with ENVIRONMENT_CLEANUP.md)
- [ ] VERSION_TRACKING.md updated
- [ ] All automation files still present
- [ ] No broken links or references

---

## 🔗 QUICK REFERENCES

### File Dependencies
```
VERSION_TRACKING.md
  → Depends on: None
  → Updates: VERSION_PLACEMENTS.md

VERSION_PLACEMENTS.md
  → Depends on: VERSION_TRACKING.md
  → Updates: None (reference only)

GITHUB_RELEASE_WORKFLOW.md
  → Depends on: VERSION_PLACEMENTS.md, VERSION_TRACKING.md
  → Updates: VERSION_TRACKING.md (after release)

WEBSITE_VERSION_UPDATE.md
  → Depends on: VERSION_TRACKING.md, CHANGELOG.md
  → Updates: docs/ directory

ENVIRONMENT_CLEANUP.md
  → Depends on: VERSION_TRACKING.md
  → Updates: release/ directory
```

### Common Patterns
All files follow consistent structure:
- **Purpose:** Clear statement of what file does
- **Status:** INTERNAL warning
- **Checklists:** Actionable items
- **Commands:** Copy-paste ready
- **Tables:** Easy reference
- **Templates:** Reusable patterns

---

## 📝 NOTES

### Version Number Format
- Semantic versioning: MAJOR.MINOR.PATCH
- Example: 0.0.6
- RADS is pre-1.0 (0.x.x series)

### Codename Themes
Each version has a thematic codename:
- v0.0.1: Web Framework (Foundation)
- v0.0.2: Core Language (Stability)
- v0.0.3: Butterfly (Metamorphosis)
- v0.0.4: Constellation (Distributed)
- v0.0.5: CHAMELEON (Transpilation)
- v0.0.6: FIGHTER JET (Performance)

### Release Schedule
- Current: Every ~2-3 weeks
- Goal: Frequent, stable releases
- Backward compatibility maintained unless announced

---

## ⚡ ONE-COMMAND RELEASE

### When Everything is Ready
```bash
# Single command to trigger entire workflow
make release VERSION=0.0.7 CODENAME="NEXT_THEME"
```

This should:
1. Update all version strings
2. Create release summary
3. Tag the release
4. Push to GitHub
5. Trigger GitHub Actions
6. Update website
7. Clean up environment

---

## 🐛 TROUBLESHOOTING

### Issue: Version strings not updated
**Solution:** Check VERSION_PLACEMENTS.md for complete list
**Command:** `grep -r "0\.0\.[0-9]" --include="*.md" .`

### Issue: GitHub release not created
**Solution:** Check GitHub Actions logs
**Command:** `gh run list --workflow=release.yml`

### Issue: Website not updated
**Solution:** Check gh-pages workflow
**Command:** `gh run list --workflow=gh-pages.yml`

### Issue: Files deleted by cleanup
**Solution:** Check git history
**Command:** `git checkout HEAD -- <file>`

---

## 📚 ADDITIONAL RESOURCES

### Related Documentation
- [Main README](../README.md)
- [CHANGELOG](../CHANGELOG.md)
- [Release Planning](../V0.0.6_FIGHTER_JET_PLAN.md)
- [Release Summary](../V0.0.6_FIGHTER_JET_SUMMARY.md)

### External Resources
- [Semantic Versioning](https://semver.org/)
- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [GitHub Releases API](https://docs.github.com/en/rest/releases)

---

## 🎓 MAINTENANCE

### When to Update This File
- After adding new automation file
- After changing automation workflows
- After modifying file structure
- After any major changes to version management

### File Maintenance Checklist
- [ ] All sections up to date
- [ ] All links working
- [ ] All commands tested
- [ ] All tables accurate
- [ ] All templates correct

---

**Created:** January 27, 2026
**Last Updated:** January 27, 2026
**Maintained By:** AI Assistant Automation
**Purpose:** Central reference for all version automation

---

## 📝 SUMMARY

This system provides:

1. ✅ **VERSION_TRACKING.md** - Central version status
2. ✅ **VERSION_PLACEMENTS.md** - Complete file inventory
3. ✅ **GITHUB_RELEASE_WORKFLOW.md** - Automated releases
4. ✅ **WEBSITE_VERSION_UPDATE.md** - Documentation updates
5. ✅ **ENVIRONMENT_CLEANUP.md** - Environment hygiene
6. ✅ **RELEASE_AUTOMATION_COMPLETE.md** - Master reference

All files are LOCAL ONLY and should NOT be published to the website or GitHub releases.
