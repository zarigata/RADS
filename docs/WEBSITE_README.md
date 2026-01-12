# RADS Website Documentation

## Overview

This directory contains the complete RADS programming language website with comprehensive documentation, examples, and feature showcases.

## Website Structure

### Main Pages

- **`index.html`** - Homepage with feature showcase, code samples, and use cases
- **`guide.html`** - Getting started guide with installation instructions
- **`documentation.html`** - Complete language documentation hub
- **`examples.html`** - Comprehensive code examples for all features
- **`compare.html`** - Comparison with other languages and tools
- **`releases.html`** - Version history and release notes

### Styles

- **`style.css`** - Main stylesheet with Frutiger Aero/Y2K design system
- **`docs-style.css`** - Additional styles for documentation pages

### Assets

- **`assets/branding/`** - Logo, emblem, and favicon files
  - `logo.png` - Main RADS logo
  - `emblem.png` - RADS emblem
  - `logotype.png` - RADS logotype
  - `favicon.svg` - Website favicon (SVG format)

## Features Documented

### Language Features
- ✅ Variables and data types (primitives, arrays, strings)
- ✅ Functions (parameters, return values, multiple returns)
- ✅ Control flow (if/elif/else, switch)
- ✅ Loops (loop/while, cruise/for)
- ✅ Structs and custom data types
- ✅ Enums and enumerations
- ✅ Async/await for asynchronous programming
- ✅ Error handling with Result types

### Built-in Capabilities
- ✅ HTTP server with routing and static files
- ✅ HTTP client for API requests
- ✅ Web Engine (JavaScript execution, HTML/CSS parsing)
- ✅ File I/O and stream processing
- ✅ Media processing (audio and image)
- ✅ Plugin system (RPM - RADS Plugin Manager)
- ✅ Memory management (stack/heap, turbo keyword)

### Code Examples
- ✅ Hello World
- ✅ Variables and types
- ✅ Functions and return values
- ✅ Control flow and conditionals
- ✅ Loops and iteration
- ✅ Structs and custom types
- ✅ HTTP server implementation
- ✅ Web Engine usage
- ✅ File I/O operations
- ✅ Media processing

## Design System

The website uses a **Frutiger Aero / Y2K Premium** design aesthetic:

### Colors
- Primary Blue: `#0078D7`
- Primary Hover: `#005A9E`
- Secondary Green: `#4CAF50`
- Accent Cyan: `#00F2FF`
- Glass Background: `rgba(255, 255, 255, 0.45)`

### Typography
- Main Font: **Outfit** (Google Fonts)
- Code Font: **Fira Code** (Google Fonts)

### Visual Effects
- Glassmorphism (backdrop blur effects)
- Gradient backgrounds
- Floating animations
- Smooth transitions
- Box shadows for depth

## Navigation Structure

All pages include consistent navigation:
1. Home
2. Guide
3. Docs (Documentation hub)
4. Examples (Code samples)
5. Compare (Language comparison)
6. Releases (Version history)

## Responsive Design

The website is fully responsive with breakpoints:
- Desktop: 1200px+ (full layout)
- Tablet: 768px-1199px (adjusted grid)
- Mobile: <768px (single column, simplified navigation)

## Browser Compatibility

- ✅ Modern browsers (Chrome, Firefox, Safari, Edge)
- ✅ Mobile browsers (iOS Safari, Chrome Mobile)
- ✅ Favicon support (SVG format with PNG fallback)

## Asset Requirements

### Required Files
- Logo files in `assets/branding/`
- Emblem for footer and hero sections
- Favicon (SVG format for modern browsers)

### Font Loading
- Google Fonts (Outfit, Fira Code)
- Preconnect for performance optimization

## Development Notes

### Adding New Pages
1. Create HTML file in `/docs` directory
2. Include favicon link: `<link rel="icon" type="image/svg+xml" href="assets/branding/favicon.svg">`
3. Add navigation with all 6 main links
4. Use consistent header/footer structure
5. Link `style.css` and `docs-style.css` if needed

### Code Highlighting
Code samples use inline styles for syntax highlighting:
- Comments: `#5C6370`
- Keywords: `#C678DD`
- Functions: `#61AFEF`
- Strings: `#98C379`
- Numbers: `#D19A66`
- Types: `#E5C07B`

### Glass Card Components
Use the `.glass-card` class for content sections:
```html
<div class="glass-card">
    <!-- Content here -->
</div>
```

### Code Windows
Use the `.code-window` structure for code samples:
```html
<div class="code-window">
    <div class="window-header">
        <div class="window-dots">
            <div class="dot red"></div>
            <div class="dot yellow"></div>
            <div class="dot green"></div>
        </div>
        <span>filename.rads</span>
    </div>
    <pre class="pre-container"><code>
        <!-- Code here -->
    </code></pre>
</div>
```

## Performance Optimizations

- ✅ Font preconnect for faster loading
- ✅ Optimized images (PNG format)
- ✅ CSS animations with GPU acceleration
- ✅ Minimal JavaScript (only for copy buttons)
- ✅ Efficient CSS selectors

## Accessibility

- ✅ Semantic HTML structure
- ✅ Alt text for all images
- ✅ Proper heading hierarchy
- ✅ Keyboard navigation support
- ✅ Sufficient color contrast

## Future Enhancements

- [ ] Dark mode toggle
- [ ] Interactive code playground
- [ ] Search functionality
- [ ] Multi-language support
- [ ] Video tutorials section
- [ ] Community showcase
- [ ] Blog/news section

## Maintenance

### Regular Updates
- Keep examples synchronized with language features
- Update version numbers in releases page
- Add new features to documentation as they're implemented
- Maintain consistent styling across all pages

### Testing Checklist
- [ ] All navigation links work
- [ ] All asset images load correctly
- [ ] Favicon displays in browser tabs
- [ ] Responsive design works on mobile
- [ ] Code samples are accurate and runnable
- [ ] No broken links or 404 errors

## Contact & Support

For website issues or suggestions:
- GitHub: https://github.com/zarigata/rads
- Report issues in the repository

---

**Last Updated**: 2026-01-12  
**Website Version**: 1.0  
**RADS Version**: 0.1.0-alpha
