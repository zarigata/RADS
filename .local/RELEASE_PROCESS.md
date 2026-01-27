# 🚀 RELEASE PROCESS - LOCAL ONLY

**Purpose:** Complete workflow for creating and publishing RADS releases
**Usage:** Follow this guide from planning through post-release
**Status:** INTERNAL - DO NOT PUBLISH

---

## 📋 Release Process Overview

### Phases

1. **Planning Phase** - Define version, features, timeline
2. **Development Phase** - Implement features, fix bugs, test
3. **Pre-Release Phase** - Documentation, packaging, QA
4. **Release Phase** - Create GitHub release, deploy, announce
5. **Post-Release Phase** - Monitor, cleanup, plan next version

---

## 📅 Planning Phase

### Version Determination

**Checklist:**
- [ ] Review current version in `VERSION_TRACKING.md`
- [ ] Determine next version number:
  - Major version: Breaking changes (x.0.0)
  - Minor version: New features (0.x.0)
  - Patch version: Bug fixes (0.0.x)
- [ ] Choose codename (theme-based)
  - Speed/performance themes (v0.0.6: FIGHTER JET)
  - Nature/animal themes (v0.0.5: CHAMELEON, v0.0.4: Constellation)
  - Other creative themes
- [ ] Set target release date

**Output:** Update `VERSION_TRACKING.md` with planned version

### Feature Planning

**Checklist:**
- [ ] Review previous version's incomplete features
- [ ] Review GitHub issues and feature requests
- [ ] Gather community feedback
- [ ] Prioritize features based on impact/effort
- [ ] Create `V{VERSION}_{CODENAME}_PLAN.md`
- [ ] Update roadmap documents

**Output:** Detailed plan document with milestones

### Timeline Planning

**Checklist:**
- [ ] Estimate development time for each feature
- [ ] Schedule code review periods
- [ ] Set buffer time for testing and fixes
- [ ] Plan documentation time
- [ ] Set release date with buffer
- [ ] Create milestone tracking

**Output:** Project timeline with dates

---

## 🔨 Development Phase

### Feature Implementation

**Process:**

For each feature in plan:

1. **Implement**
   - [ ] Write feature code
   - [ ] Write unit tests
   - [ ] Update relevant documentation

2. **Test**
   - [ ] Run unit tests
   - [ ] Manual testing
   - [ ] Cross-platform test (Linux, macOS)
   - [ ] Integration testing

3. **Review**
   - [ ] Code review (self or peer)
   - [ ] Performance test
   - [ ] Security check

**Tracking:** Use issue tracker or kanban board

### Bug Fixes

**Process:**

1. **Identify**
   - [ ] Review existing issues
   - [ ] Monitor bug reports
   - [ ] Check for regressions

2. **Prioritize**
   - [ ] Critical: Security, data loss
   - [ ] High: Major functionality broken
   - [ ] Medium: Minor issues, usability
   - [ ] Low: Cosmetic, non-blocking

3. **Fix**
   - [ ] Implement fix
   - [ ] Write test for fix
   - [ ] Verify fix resolves issue
   - [ ] No new bugs introduced

### Documentation Updates (During Development)

**Checklist:**
- [ ] Update CHANGELOG.md with new features
- [ ] Document breaking changes as they occur
- [ ] Update plan document with progress
- [ ] Update examples for new features
- [ ] Update API documentation

---

## 📦 Pre-Release Phase

### Code Quality Checks

**Checklist:**
- [ ] All tests passing (`make test`)
- [ ] Code review completed
- [ ] No compiler warnings (or acceptable)
- [ ] No memory leaks (valgrind/sanitizers)
- [ ] Security audit (if applicable)
- [ ] Performance benchmarks met targets
- [ ] Cross-platform compatibility verified

**Commands:**
```bash
# Run all tests
make test

# Check for memory leaks (Linux)
valgrind --leak-check=full ./bin/rads --version

# Security check (if tool available)
# (Use security scanning tools)
```

### Documentation Completion

**Checklist:**

#### Release Documentation
- [ ] `CHANGELOG.md` complete entry for version
  - [ ] Version header
  - [ ] New features
  - [ ] Breaking changes
  - [ ] Bug fixes
  - [ ] Migration guide
- [ ] `V{VERSION}_{CODENAME}_SUMMARY.md` created
  - [ ] Executive summary
  - [ ] Feature descriptions with examples
  - [ ] Performance benchmarks
  - [ ] Architecture highlights
  - [ ] Migration guide
  - [ ] Links to documentation
- [ ] `V{VERSION}_QUICK_REFERENCE.md` created

#### Website Documentation
- [ ] `docs/index.html` version updated
- [ ] `docs/releases.html` new release added
- [ ] `docs/README.md` updated
- [ ] `examples/README.md` updated
- [ ] All HTML files version references updated

#### Source Documentation
- [ ] Version strings updated in code
  - [ ] `src/core/main.c`
  - [ ] `tools/rads-lsp/Cargo.toml`
  - [ ] Any other version files
- [ ] Code comments updated for new features
- [ ] Function signatures documented

### Build and Packaging

**Checklist:**

#### Clean Build
```bash
# Remove old build artifacts
make clean

# Verify clean
ls build/  # Should be empty or not exist
```

- [ ] Clean build completed

#### Production Build
```bash
# Build release version
make

# Verify binary
ls -lh bin/rads
./bin/rads --version  # Should show new version
```

- [ ] Binary created successfully
- [ ] Version verified in binary
- [ ] Binary size reasonable (<5 MB)
- [ ] All tools built (rstar, rpm, rads-mask)

#### Release Archive Creation

```bash
# Create release directory
mkdir -p release/artifacts

# Create tarball
tar -czf release/artifacts/rads-{version}-linux-x86_64.tar.gz \
  --exclude=".git" \
  --exclude="build/" \
  --exclude="bin/" \
  --exclude=".local/" \
  --exclude="release/artifacts/" \
  --exclude="tests/" \
  --exclude="*.o" \
  --exclude="*.a" \
  .

# Create checksums
cd release/artifacts
sha256sum rads-{version}-linux-x86_64.tar.gz > rads-{version}-SHA256.txt

# Create binary checksums
for file in rads rstar rpm rads-mask; do
    if [ -f "$file" ]; then
        sha256sum "$file" >> rads-{version}-BIN-CHECKSUMS.txt
    fi
done
cd ../..
```

- [ ] Release tarball created
- [ ] SHA256 checksum generated
- [ ] Binary checksums created
- [ ] README included in tarball (verify with tar -tzf)
- [ ] LICENSE included in tarball (verify with tar -tzf)
- [ ] Installation script included (if needed)

#### Installation Script Verification

```bash
# Extract and test
cd /tmp
rm -rf rads-test
mkdir rads-test
cd rads-test
tar -xzf /path/to/release/artifacts/rads-{version}-linux-x86_64.tar.gz

# Run installation
./install.sh

# Verify installation
which rads
rads --version  # Should show new version
```

- [ ] Installation script works
- [ ] Binary installs correctly
- [ ] Version correct after install

### Quality Assurance

**Testing Checklist:**

- [ ] Binary runs on fresh system
- [ ] All examples work correctly
- [ ] REPL starts and functions
- [ ] Compiler works on all test files
- [ ] LSP server builds and connects
- [ ] No crashes on standard operations
- [ ] Memory usage within limits
- [ ] No obvious performance regressions
- [ ] Error messages clear and helpful

**Platform Testing:**

- [ ] Linux x86_64 tested
- [ ] macOS x86_64 tested (if applicable)
- [ ] Linux ARM tested (if applicable)
- [ ] Cross-version upgrade tested (install v0.0.6 over v0.0.5)

---

## 🌐 Release Phase

### Git Preparation

**Checklist:**

#### Branch Verification
```bash
# Ensure on correct branch
git branch  # Should show main or master

# Pull latest
git pull origin main
```

- [ ] On main/master branch
- [ ] Branch is clean (no uncommitted changes)
- [ ] Latest changes pulled

#### Create Release Tag
```bash
# Tag release version
git tag -a v{version} -m "Release v{version} {codename}"

# Example:
git tag -a v0.0.7 -m "Release v0.0.7 JETSTREAM"

# Verify tag created
git tag -l
```

- [ ] Tag created with correct format
- [ ] Tag message includes version and codename
- [ ] Tag signed (if GPG signing used)

#### Push Tag
```bash
# Push tag to remote
git push origin v{version}

# Verify pushed
git ls-remote --tags origin
```

- [ ] Tag pushed to origin
- [ ] Tag visible on GitHub

### Create GitHub Release

**Via GitHub Web UI:**

1. Navigate to: https://github.com/zarigata/rads/releases/new
2. Select tag: `v{version}`
3. Title: `v{version} {Codename}`
4. Description: Paste release notes from CHANGELOG
5. Attach assets:
   - [ ] rads-{version}-linux-x86_64.tar.gz
   - [ ] rads-{version}-SHA256.txt
   - [ ] rads-{version}-BIN-CHECKSUMS.txt
6. Select release type:
   - [ ] Pre-release (if beta/alpha)
   - [ ] Latest release (if stable)
7. Click "Publish release"

**Via GitHub CLI (gh):**

```bash
# Create release
gh release create v{version} \
  --title "v{version} {Codename}" \
  --notes-file RELEASE_NOTES.md \
  --attach release/artifacts/rads-{version}-linux-x86_64.tar.gz \
  --attach release/artifacts/rads-{version}-SHA256.txt \
  --attach release/artifacts/rads-{version}-BIN-CHECKSUMS.txt \
  --latest \
  --generate-notes

# Verify release
gh release view v{version}
```

- [ ] Release created on GitHub
- [ ] All assets uploaded
- [ ] Release notes displayed correctly
- [ ] Download links work
- [ ] Release marked as latest (if stable)

### Trigger GitHub Actions

**Automatic:**
- Release event triggers `.github/workflows/publish-to-registry.yml`
- Check workflow status: `gh workflow list`

**Manual Trigger (if needed):**
```bash
# Trigger registry publish
gh workflow run publish-to-registry.yml -f version={version}
```

- [ ] Registry workflow triggered
- [ ] Workflow completed successfully
- [ ] Package published to rads-packages

### Website Deployment

**Process:**

1. **Update Website Files**
   - Follow `WEBSITE_UPDATE_GUIDE.md`
   - All files updated and tested locally

2. **Commit and Push**
   ```bash
   git add docs/
   git commit -m "docs: update website for v{version} release"
   git push origin main
   ```

3. **Verify Deployment**
   - [ ] GitHub Actions workflow running
   - [ ] Workflow completed successfully
   - [ ] Website live at https://zarigata.github.io/RADS/
   - [ ] Version information correct
   - [ ] All pages accessible

**Manual Rollback (if needed):**
```bash
# Revert website commit
git revert HEAD
git push origin main
```

---

## 📊 Post-Release Phase

### Monitoring

**Checklist:**

#### First 24 Hours
- [ ] Monitor GitHub release download count
- [ ] Watch GitHub issues for bug reports
- [ ] Check social media mentions
- [ ] Respond to community feedback

#### First Week
- [ ] Compile user feedback
- [ ] Track critical bugs found
- [ ] Monitor performance reports
- [ ] Document common issues

#### First Month
- [ ] Collect all feedback
- [ ] Analyze feature adoption
- [ ] Track migration challenges
- [ ] Prepare lessons learned

### Support

**Checklist:**

- [ ] Respond to all GitHub issues
- [ ] Update documentation based on questions
- [ ] Fix critical bugs immediately
- [ ] Release hotfix if needed (v0.0.7.1)

**Hotfix Process:**

1. Fix critical bug
2. Update version to patch (0.0.7.1)
3. Run through Pre-Release Phase (abbreviated)
4. Create patch release
5. Update CHANGELOG.md with hotfix notes

---

## 🗂️ Archive and Cleanup

### Archive Previous Release

**Follow `CLEANUP_GUIDE.md`**

**Checklist:**
- [ ] Previous release moved to `release/rads-{previous-version}/`
- [ ] Archive docs moved to `release/rads-{previous-version}/archive/`
- [ ] Old release binaries deleted from root
- [ ] `VERSION_TRACKING.md` updated
- [ ] Clean build artifacts removed

### Cleanup Old Files

**Checklist:**
- [ ] Temporary files deleted
- [ ] Backup files removed
- [ ] Old release archives deleted (> 3 versions)
- [ ] Build directory cleaned
- [ ] .local/ organized (archive very old files)

### Verification

**Checklist:**
- [ ] All important files preserved
- [ ] No critical data lost
- [ ] Repository size reasonable
- [ ] Git status clean (except untracked .local/)
- [ ] Archives verified (can be extracted)

---

## 📝 Final Documentation Updates

### Update Tracking Files

**Files to Update:**
- [ ] `VERSION_TRACKING.md` - Mark current as previous, new as current
- [ ] `RELEASE_CHECKLIST.md` - All items checked for this release
- [ ] `WEBSITE_UPDATE_GUIDE.md` - Notes for next release improvements

### Plan Next Version

**Checklist:**
- [ ] Review this release's issues and feedback
- [ ] Identify incomplete features for next version
- [ ] Gather feature requests from community
- [ ] Create `V{NEXT_VERSION}_{CODENAME}_PLAN.md`
- [ ] Update `VERSION_TRACKING.md` with planned version

---

## 📊 Release Metrics

### Track These Metrics

| Metric | How to Track | Target |
|--------|--------------|--------|
| Release Downloads | GitHub release stats | N/A (monitor) |
| Website Visits | GitHub Pages analytics | N/A (monitor) |
| Issues Created (first week) | GitHub issues | < 10 critical |
| Issues Resolved (first week) | GitHub issues | > 90% |
| Stars Gained | GitHub stars | N/A (monitor) |
| Forks | GitHub forks | N/A (monitor) |
| Registry Installs | rads-packages stats | N/A (monitor) |

### Post-Mortem

**After 1 Week, Create:**

**Questions to Answer:**
1. What went well?
2. What could be improved?
3. Any unexpected issues?
4. User feedback themes?
5. Technical debt incurred?
6. What should we do differently next time?

**Output:** Update this `RELEASE_PROCESS.md` with lessons learned

---

## 🔄 Release Process Timeline

### Typical Timeline

| Phase | Duration | Notes |
|--------|---------|-------|
| Planning | 2-3 days | Version, features, timeline |
| Development | 2-4 weeks | Feature implementation, bug fixes |
| Pre-Release | 3-5 days | Documentation, packaging, QA |
| Release | 1 day | GitHub release, website deployment |
| Post-Release | 1-2 weeks | Monitoring, support, cleanup |

**Total:** 3-6 weeks depending on scope

---

## 📞 Emergency Procedures

### If Critical Bug Found Before Release

1. **Stop Release Process**
2. **Fix Critical Bug**
3. **Run Through Pre-Release Phase Again**
4. **Resume Release Process**

### If Critical Bug Found After Release

1. **Assess Impact**
   - Data loss risk?
   - Security vulnerability?
   - Major functionality broken?

2. **Take Action**
   - If YES: Immediate hotfix release (v{version}.1)
   - If NO: Schedule for next release

3. **Communicate**
   - Post announcement about issue
   - Provide workaround if available
   - Give timeline for fix

### If Release Website Fails

1. **Cancel GitHub Release** (if not published)
2. **Delete Git Tag**
   ```bash
   git tag -d v{version}
   git push origin :refs/tags/v{version}
   ```
3. **Fix Issue**
4. **Retry Release Process**

---

## 📋 Complete Release Checklist

### Final Verification Before Publishing

- [ ] All planned features implemented
- [ ] All tests passing
- [ ] Code reviewed
- [ ] Documentation complete
- [ ] Examples created/updated
- [ ] Release notes written
- [ ] Breaking changes documented
- [ ] Migration guide created
- [ ] Version strings updated in code
- [ ] Clean build completed
- [ ] Release artifacts created
- [ ] Checksums generated
- [ ] Installation tested
- [ ] Quality assurance passed
- [ ] Git tag created
- [ ] Tag pushed to origin
- [ ] Release notes prepared
- [ ] Ready to create GitHub release

### After Publishing

- [ ] GitHub release created
- [ ] Assets uploaded
- [ ] Release visible to public
- [ ] Download links verified
- [ ] Website updated
- [ ] Website deployed
- [ ] Previous release archived
- [ ] Cleanup completed
- [ ] Monitoring started
- [ ] Next version planned

---

## 🔗 Related Files

- `VERSION_TRACKING.md` - Current version status
- `RELEASE_CHECKLIST.md` - Detailed release checklist
- `WEBSITE_UPDATE_GUIDE.md` - Website update procedures
- `CLEANUP_GUIDE.md` - Cleanup policies

---

**Last Updated:** January 27, 2026
**Maintained By:** Release Manager
**Next Review:** After v0.0.7 release completion

---

*This file is for internal use only. Do not publish to public documentation.*
