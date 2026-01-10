# RADS Documentation

Welcome to the RADS documentation directory.

## Contents

### Language Specification
- **LANGUAGE_SPEC.md** - Complete language specification
- **RADStar.md** - RADStar plugin manager documentation

### Web Documentation
- **index.html** - Main documentation homepage
- **guide.html** - User guide and tutorials
- **compare.html** - Language comparison
- **releases.html** - Release notes

### Roadmap
- **roadmap/PROJECT_STATE.md** - Current project state
- **roadmap/ENHANCEMENT_ROADMAP.md** - Future enhancements

### Style
- **style.css** - Documentation styling

## Building Documentation

The HTML documentation can be viewed directly in a web browser:
```bash
firefox docs/index.html
```

Or serve with a simple HTTP server:
```bash
cd docs
python3 -m http.server 8000
```

Then visit http://localhost:8000

## Contributing

When adding documentation:
1. Follow the existing style and formatting
2. Update the index.html with new content
3. Keep language spec synchronized with implementation
