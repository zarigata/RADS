# ✅ RELEASE CHECKLIST - LOCAL ONLY

**Purpose:** Comprehensive checklist for creating and publishing RADS releases
**Usage:** Follow step-by-step when preparing a release
**Status:** INTERNAL - DO NOT PUBLISH

---

## 🎯 Pre-Release Phase

### Development Verification

- [ ] All planned features implemented
- [ ] All tests passing (run `make test`)
- [ ] No critical bugs remaining
- [ ] Documentation updated for new features
- [ ] Examples created for new features
- [ ] Code reviewed (or self-reviewed)
- [ ] Breaking changes documented

### Version Planning

- [ ] Version number determined (following semantic versioning)
- [ ] Codename chosen (theme-based)
- [ ] Release date set
- [ ] Release notes drafted
- [ ] Breaking changes identified
- [ ] Migration guide updated (if needed)
- [ ] CHANGELOG.md updated with all changes

### Documentation Updates

#### Root Documentation
- [ ] `README.md` updated with new version
- [ ] Version badge updated to show new version
- [ ] Download links updated to new version
- [ ] Features section updated with new capabilities
- [ ] Roadmap section updated (new version as current)

#### Release Documentation
- [ ] `V{VERSION}_{CODENAME}_SUMMARY.md` created
  - [ ] Executive summary
  - [ ] Feature descriptions
  - [ ] Usage examples
  - [ ] Performance benchmarks
  - [ ] Architecture highlights
  - [ ] Migration guide
- [ ] `V{VERSION}_{CODENAME}_PLAN.md` moved to archive
- [ ] `CHANGELOG.md` updated with complete entry
  - [ ] Version header added
  - [ ] New features listed
  - [ ] Breaking changes noted
  - [ ] Bug fixes listed
  - [ ] Migration guide included
- [ ] `VERSION_TRACKING.md` updated
  - [ ] New version added to matrix
  - [ ] Previous version marked as historical
  - [ ] Files to update listed

#### Example Documentation
- [ ] `examples/README.md` updated
  - [ ] New examples added
  - [ ] Learning paths updated
  - [ ] Difficulty ratings assigned
  - [ ] v0.0.{VERSION} section created

#### API/Technical Documentation
- [ ] API documentation updated
- [ ] New functions documented
- [ ] Deprecation warnings added (if any)
- [ ] Type signatures verified

### Source Code Updates

- [ ] Version strings updated in `src/core/main.c`
  - [ ] `--version` displays new version
  - [ ] Welcome message updated (if needed)
- [ ] `tools/rads-lsp/Cargo.toml` version updated
- [ ] Any build files with version updated
- [ ] Installation scripts updated (`install.sh`)
  - [ ] Download URLs to new version
  - [ ] Version checks updated

### Testing

- [ ] Unit tests passing (`make test`)
- [ ] Integration tests passing
- [ ] Manual testing completed
- [ ] Performance benchmarks run and documented
- [ ] Memory leak checks passed
- [ ] Cross-platform compatibility verified (Linux, macOS)
- [ ] Examples verified to run correctly
- [ ] Installation script tested

---

## 🚦 Build Phase

### Build Verification

- [ ] Clean build completed (`make clean && make`)
- [ ] Binary created successfully
- [ ] Version verified in binary (`./rads --version`)
- [ ] All tools built (rads, rstar, rpm, rads-mask)
- [ ] LSP server built (`cd tools/rads-lsp && cargo build`)
- [ ] No compiler warnings (or acceptable ones)
- [ ] Binary size reasonable (check for bloat)

### Release Artifacts

- [ ] Release tarball created
  ```bash
  tar -czf rads-{version}-linux-x86_64.tar.gz \
    --exclude=".git" \
    --exclude="build/" \
    --exclude="bin/" \
    --exclude="*.md" \
    .
  ```
- [ ] SHA256 checksum generated
  ```bash
  sha256sum rads-{version}-linux-x86_64.tar.gz > rads-{version}-SHA256.txt
  ```
- [ ] Binary checksums created (BIN-CHECKSUMS.txt)
- [ ] README included in tarball
- [ ] LICENSE included in tarball
- [ ] Installation script included in tarball
- [ ] Artifacts tested (extract and run)

---

## 📝 Release Notes Preparation

### Release Notes Structure

- [ ] Title clear and descriptive
- [ ] Version number prominent
- [ ] Codename displayed with emoji
- [ ] Summary paragraph (what this release is about)
- [ ] New features section with descriptions
- [ ] Breaking changes clearly marked
- [ ] Bug fixes listed
- [ ] Known limitations noted
- [ ] Performance benchmarks included
- [ ] Migration guide link (if applicable)
- [ ] Download links correct
- [ ] Documentation links provided

### Release Notes Template

```markdown
# 🛩️ RADS v0.0.7 "{Codename}" Released

**Release Date:** {Date}
**Status:** ✅ Production Ready

## What's New

### Major Features
- **Feature Name:** Description
  - Details...
  - Impact...

### Performance Improvements
- {Improvement details}
- {Benchmarks}

### Bug Fixes
- Fixed {issue description}
- {Technical details}

### Documentation
- Updated {documentation files}
- Added {new guides}

## Installation

### Quick Install
```bash
wget {download-url}
tar -xzf rads-0.0.7-linux-x86_64.tar.gz
cd rads-0.0.7
./install.sh
```

### Upgrade from v0.0.6
```bash
# Follow migration guide
# See docs/MIGRATION_GUIDE.md
```

## Breaking Changes
- {List breaking changes}
- {Migration steps}

## Documentation
- [Full Documentation](link)
- [Migration Guide](link)
- [Examples](link)

## Downloads
- [Linux x86_64](download-url)
- [SHA256 Checksum](checksum-url)

## What's Next
- {Planned features for v0.0.8}
```

---

## 🌐 Website Updates

### Documentation Files

- [ ] `docs/index.html` version updated
- [ ] `docs/releases.html` new release added
- [ ] `docs/README.md` updated
- [ ] `docs/V{VERSION}_QUICK_REFERENCE.md` created
- [ ] All HTML files with version references updated

### Visual Updates

- [ ] Homepage version badge updated
- [ ] Download links updated
- [ ] Feature cards updated
- [ ] Screenshot examples added (if new UI)
- [ ] Code examples updated
- [ ] Release notes page created

### Website Testing

- [ ] Local testing completed (`python3 -m http.server docs/`)
- [ ] All links work
- [ ] Version information correct
- [ ] No broken images or resources
- [ ] Mobile responsive verified
- [ ] Browser compatibility checked

---

## 📦 GitHub Release

### Preparation

- [ ] Git tag created (`git tag v{version}`)
- [ ] Tag pushed to remote (`git push origin v{version}`)
- [ ] Release notes prepared (see above)
- [ ] Assets ready (tarball, checksums)
- [ ] Release title finalized
- [ ] Release type selected (pre-release or stable)

### Create Release

- [ ] GitHub release created
  - [ ] Tag selected: v{version}
  - [ ] Title formatted: "v{version} {Codename}"
  - [ ] Release notes pasted
  - [ ] Assets uploaded:
    - [ ] Tarball (linux-x86_64.tar.gz)
    - [ ] SHA256 checksum file
    - [ ] Binary checksums file
  - [ ] Release set as latest (if stable)
  - [ ] Pre-release checkbox unchecked (if stable)

### Verification

- [ ] Release visible on GitHub
- [ ] Download links work
- [ ] Version tag linked correctly
- [ ] Release notes display properly
- [ ] Assets downloadable

---

## 🔗 Registry & Package Management

### Package Registry (rads-packages)

- [ ] Package metadata prepared (`package.rads`)
- [ ] Version in package.rads matches release
- [ ] Description updated
- [ ] Dependencies listed (if any)
- [ ] Package published to registry
- [ ] Registry updated with new version

### Update Scripts

- [ ] `.github/workflows/publish-to-registry.yml` verified
- [ ] Workflow triggers on release
- [ ] Workflow tested with dry-run

---

## 🗂️ Archive Management

### Archive Previous Release

- [ ] Previous release moved to `release/rads-{previous-version}/`
- [ ] Archive subfolder maintained
- [ ] Old binaries removed from root
- [ ] Historical docs preserved
- [ ] Checksum files for old releases archived

### Clean Environment

- [ ] Old release archives compressed (if very old)
- [ ] Duplicate or obsolete files removed
- [ ] README files cleaned (if old)
- [ ] Build artifacts removed (`make clean`)
- [ ] Test artifacts cleaned
- [ ] Temporary files removed

### Retention Policy

- [ ] Last 3 release folders kept
- [ ] All .md documentation preserved
- [ ] CHANGELOG.md preserved (full history)
- [ ] Only release binary for latest version in root
- [ ] Old versions archived with proper documentation

---

## 📊 Post-Release Phase

### Announcement

- [ ] Announcement tweet/post drafted
- [ ] Reddit post created
- [ ] Hacker News submission (if appropriate)
- [ ] Blog post written (if applicable)
- [ ] Discord/Slack announcement prepared
- [ ] Email to mailing list (if exists)

### Monitoring

- [ ] Release monitoring (download counts)
- [ ] Issue tracker monitored for bug reports
- [ ] Performance benchmarks from users collected
- [ ] Documentation feedback gathered

### Update Documentation

- [ ] `VERSION_TRACKING.md` updated (release complete)
- [ ] Next version planning started
- [ ] Known issues documented
- [ ] User feedback compiled

---

## 🔍 Quality Assurance

### Pre-Release QA

- [ ] Code review completed
- [ ] Security audit (if applicable)
- [ ] Performance regression tests passed
- [ ] Cross-platform testing completed
  - [ ] Linux x86_64
  - [ ] macOS x86_64
  - [ ] (if other platforms supported)
- [ ] Installation testing completed
  - [ ] Clean install
  - [ ] Upgrade from previous version

### Release Verification

- [ ] Download link verified
- [ ] Installation verified on clean system
- [ ] Version verification works
- [ ] Basic examples run successfully
- [ ] Documentation links correct
- [ ] No broken links in docs

---

## 📞 Emergency Rollback Plan

### If Critical Bug Found

1. [ ] Pull release (delete from GitHub)
2. [ ] Delete tag (`git tag -d v{version}` && git push --delete origin v{version}`)
3. [ ] Fix critical bug
4. [ ] Run through pre-release checklist again
5. [ ] Republish release

### Rollback Criteria

- [ ] Critical security vulnerability
- [ ] Data loss potential
- [ ] Corrupt installation
- [ ] Major functionality broken
- [ ] Within 24 hours of release

---

## 📈 Metrics to Track

After Release, Monitor:

- [ ] GitHub release downloads (first 24 hours, first week)
- [ ] Package registry installs
- [ ] GitHub stars increase
- [ ] New issues opened (bug reports)
- [ ] Documentation feedback
- [ ] Community forum discussions
- [ ] Social media mentions
- [ ] Performance reports from users

---

## ✅ Final Sign-Off

Before publishing release, confirm:

- [ ] All checklist items completed
- [ ] All tests passing
- [ ] All documentation updated
- [ ] All artifacts created
- [ ] Release notes reviewed
- [ ] Website updates committed and pushed
- [ ] Team approval received
- [ ] Ready to publish

**Release Manager Signature:** _________________
**Date:** _______________

---

**Last Updated:** January 27, 2026
**Maintained By:** Release Manager
**Next Review:** Before v0.0.7 release

---

*This file is for internal use only. Do not publish to public documentation.*
