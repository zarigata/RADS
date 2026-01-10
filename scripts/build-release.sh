#!/bin/bash
set -e

# RADS Release Build Script
# Creates distributable packages for RADS

VERSION="0.1.0-alpha"
PROJECT_NAME="rads"
BUILD_DATE=$(date +%Y%m%d)

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m'

info() { echo -e "${BLUE}ℹ${NC} $1"; }
success() { echo -e "${GREEN}✓${NC} $1"; }
warning() { echo -e "${YELLOW}⚠${NC} $1"; }

# Detect platform
detect_platform() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        PLATFORM="linux"
        ARCH=$(uname -m)
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        PLATFORM="macos"
        ARCH=$(uname -m)
    else
        PLATFORM="unknown"
        ARCH=$(uname -m)
    fi

    info "Platform: $PLATFORM-$ARCH"
}

# Clean and build
build_clean() {
    info "Cleaning previous builds..."
    make clean > /dev/null 2>&1 || true

    info "Building RADS (release mode)..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)

    success "Build complete"
}

# Create release directory structure
create_release_structure() {
    RELEASE_DIR="release/${PROJECT_NAME}-${VERSION}-${PLATFORM}-${ARCH}"

    info "Creating release structure..."

    rm -rf "$RELEASE_DIR"
    mkdir -p "$RELEASE_DIR"/{bin,docs,examples,tools}

    # Copy binaries
    cp bin/rads "$RELEASE_DIR/bin/"
    cp bin/rstar "$RELEASE_DIR/bin/"

    # Strip binaries (reduce size)
    strip "$RELEASE_DIR/bin/rads" 2>/dev/null || true
    strip "$RELEASE_DIR/bin/rstar" 2>/dev/null || true

    # Copy documentation
    cp README.md "$RELEASE_DIR/" 2>/dev/null || echo "# RADS v$VERSION" > "$RELEASE_DIR/README.md"
    cp LICENSE "$RELEASE_DIR/" 2>/dev/null || true
    cp -r docs "$RELEASE_DIR/"

    # Copy examples
    cp -r examples "$RELEASE_DIR/"

    # Copy installation script
    cp install.sh "$RELEASE_DIR/"

    # Create version file
    cat > "$RELEASE_DIR/VERSION" <<EOF
RADS Version: $VERSION
Platform: $PLATFORM-$ARCH
Build Date: $(date '+%Y-%m-%d %H:%M:%S')
Build Host: $(hostname)
Compiler: $(gcc --version | head -n1 || clang --version | head -n1)
EOF

    success "Release structure created: $RELEASE_DIR"
}

# Create tarball
create_tarball() {
    TARBALL_NAME="${PROJECT_NAME}-${VERSION}-${PLATFORM}-${ARCH}.tar.gz"

    info "Creating tarball..."

    cd release
    tar -czf "$TARBALL_NAME" "$(basename "$RELEASE_DIR")"
    cd ..

    TARBALL_PATH="release/$TARBALL_NAME"
    TARBALL_SIZE=$(du -h "$TARBALL_PATH" | cut -f1)
    TARBALL_SHA256=$(sha256sum "$TARBALL_PATH" | cut -d' ' -f1)

    success "Tarball created: $TARBALL_PATH ($TARBALL_SIZE)"
    info "SHA256: $TARBALL_SHA256"

    # Create checksum file
    echo "$TARBALL_SHA256  $TARBALL_NAME" > "$TARBALL_PATH.sha256"
}

# Create installation instructions
create_install_instructions() {
    TARBALL_NAME="${PROJECT_NAME}-${VERSION}-${PLATFORM}-${ARCH}.tar.gz"

    cat > "release/INSTALL.txt" <<EOF
RADS v$VERSION - Installation Instructions
==========================================

Quick Install:
--------------
1. Extract the tarball:
   tar -xzf $TARBALL_NAME

2. Run the installer:
   cd ${PROJECT_NAME}-${VERSION}-${PLATFORM}-${ARCH}
   ./install.sh

3. Verify installation:
   rads --version
   rstar help

Manual Install:
---------------
1. Extract the tarball
2. Copy binaries to your PATH:
   sudo cp bin/rads /usr/local/bin/
   sudo cp bin/rstar /usr/local/bin/

3. Create plugin directory:
   mkdir -p ~/.rads/plugins

Getting Started:
----------------
- Start REPL: rads
- Run example: rads examples/01-basics/hello_world.rads
- View docs: docs/REPL_GUIDE.md

For more information, visit:
https://github.com/yourusername/rads

SHA256 Checksum:
----------------
To verify the download integrity:
  sha256sum -c $TARBALL_NAME.sha256

Build Information:
------------------
Platform: $PLATFORM-$ARCH
Version: $VERSION
Build Date: $(date '+%Y-%m-%d')

Enjoy RADS! 🚀
EOF

    success "Created INSTALL.txt"
}

# Main release build process
main() {
    echo ""
    echo "╔═══════════════════════════════════════╗"
    echo "║   RADS Release Builder v$VERSION      ║"
    echo "╚═══════════════════════════════════════╝"
    echo ""

    detect_platform
    build_clean
    create_release_structure
    create_tarball
    create_install_instructions

    echo ""
    echo "╔═══════════════════════════════════════╗"
    echo "║   Release Build Complete! 🎉         ║"
    echo "╚═══════════════════════════════════════╝"
    echo ""
    echo "Release package:"
    echo "  📦 release/${PROJECT_NAME}-${VERSION}-${PLATFORM}-${ARCH}.tar.gz"
    echo ""
    echo "To test installation:"
    echo "  cd release"
    echo "  tar -xzf ${PROJECT_NAME}-${VERSION}-${PLATFORM}-${ARCH}.tar.gz"
    echo "  cd ${PROJECT_NAME}-${VERSION}-${PLATFORM}-${ARCH}"
    echo "  ./install.sh"
    echo ""
}

# Run
main "$@"
