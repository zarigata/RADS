#!/bin/bash
set -e

# RADS Installation Script
# Installs RADS and RADStar package manager

VERSION="0.1.0-alpha"
INSTALL_DIR="${INSTALL_DIR:-/usr/local/bin}"
PREFIX="${PREFIX:-/usr/local}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored messages
info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

success() {
    echo -e "${GREEN}✓${NC} $1"
}

warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

error() {
    echo -e "${RED}✗${NC} $1"
}

# Print banner
print_banner() {
    echo ""
    echo -e "${BLUE}╔═══════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC}   🚀 RADS Installation Script       ${BLUE}║${NC}"
    echo -e "${BLUE}║${NC}   Rapid Asynchronous Data Server    ${BLUE}║${NC}"
    echo -e "${BLUE}║${NC}   Version: $VERSION              ${BLUE}║${NC}"
    echo -e "${BLUE}╚═══════════════════════════════════════╝${NC}"
    echo ""
}

# Detect OS
detect_os() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        OS="linux"
        info "Detected OS: Linux"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
        info "Detected OS: macOS"
    elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]]; then
        OS="windows"
        info "Detected OS: Windows (via MSYS/Cygwin)"
    else
        error "Unsupported operating system: $OSTYPE"
        exit 1
    fi
}

# Check for required dependencies
check_dependencies() {
    info "Checking dependencies..."

    local missing=0

    # Check for GCC/Clang
    if command -v gcc &> /dev/null; then
        success "GCC found: $(gcc --version | head -n1)"
    elif command -v clang &> /dev/null; then
        success "Clang found: $(clang --version | head -n1)"
    else
        error "C compiler not found (gcc or clang required)"
        missing=1
    fi

    # Check for make
    if command -v make &> /dev/null; then
        success "Make found: $(make --version | head -n1)"
    else
        error "Make not found"
        missing=1
    fi

    # Check for libuv
    if pkg-config --exists libuv 2>/dev/null; then
        success "libuv found: $(pkg-config --modversion libuv)"
    elif [[ "$OS" == "macos" ]] && brew list libuv &>/dev/null; then
        success "libuv found via Homebrew"
    else
        warning "libuv not found (required for async I/O)"
        info "Install with:"
        if [[ "$OS" == "linux" ]]; then
            echo "    sudo apt-get install libuv1-dev  # Debian/Ubuntu"
            echo "    sudo dnf install libuv-devel     # Fedora"
            echo "    sudo pacman -S libuv             # Arch Linux"
        elif [[ "$OS" == "macos" ]]; then
            echo "    brew install libuv"
        fi
        missing=1
    fi

    if [[ $missing -eq 1 ]]; then
        error "Missing dependencies. Please install them and try again."
        exit 1
    fi

    echo ""
}

# Build RADS
build_rads() {
    info "Building RADS..."

    if [[ ! -f "Makefile" ]]; then
        error "Makefile not found. Are you in the RADS directory?"
        exit 1
    fi

    # Clean previous builds
    make clean > /dev/null 2>&1 || true

    # Build with parallel jobs
    if make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2); then
        success "RADS built successfully"
    else
        error "Build failed"
        exit 1
    fi

    echo ""
}

# Install RADS
install_rads() {
    info "Installing RADS to $INSTALL_DIR..."

    # Check if we need sudo
    if [[ -w "$INSTALL_DIR" ]]; then
        SUDO=""
    else
        SUDO="sudo"
        warning "Requesting sudo access for installation to $INSTALL_DIR"
    fi

    # Install binaries
    $SUDO install -m 755 bin/rads "$INSTALL_DIR/rads"
    $SUDO install -m 755 bin/rstar "$INSTALL_DIR/rstar"

    success "Installed rads to $INSTALL_DIR/rads"
    success "Installed rstar to $INSTALL_DIR/rstar"

    echo ""
}

# Create plugin directory
setup_plugin_dir() {
    local plugin_dir="$HOME/.rads/plugins"

    if [[ ! -d "$plugin_dir" ]]; then
        mkdir -p "$plugin_dir"
        success "Created plugin directory: $plugin_dir"
    else
        info "Plugin directory already exists: $plugin_dir"
    fi

    echo ""
}

# Test installation
test_installation() {
    info "Testing installation..."

    if command -v rads &> /dev/null; then
        success "rads is in PATH"
        rads --version 2>/dev/null || echo "  RADS v$VERSION"
    else
        warning "rads not in PATH. Add $INSTALL_DIR to your PATH:"
        echo "    export PATH=\"$INSTALL_DIR:\$PATH\""
    fi

    if command -v rstar &> /dev/null; then
        success "rstar is in PATH"
    else
        warning "rstar not in PATH"
    fi

    echo ""
}

# Print post-install instructions
print_instructions() {
    echo -e "${GREEN}╔═══════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║${NC}   Installation Complete! 🎉          ${GREEN}║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════╝${NC}"
    echo ""
    echo "Quick Start:"
    echo ""
    echo "  1. Start the REPL:"
    echo -e "     ${BLUE}rads${NC}"
    echo ""
    echo "  2. Run an example:"
    echo -e "     ${BLUE}rads examples/01-basics/hello_world.rads${NC}"
    echo ""
    echo "  3. View package manager:"
    echo -e "     ${BLUE}rstar help${NC}"
    echo ""
    echo "Documentation:"
    echo "  - REPL Guide: docs/REPL_GUIDE.md"
    echo "  - Package Manager: docs/PACKAGE_MANAGER.md"
    echo "  - Examples: examples/"
    echo ""
    echo "Need help? Visit: https://github.com/yourusername/rads"
    echo ""
}

# Main installation flow
main() {
    print_banner
    detect_os
    check_dependencies
    build_rads

    # Ask for installation (or use -y flag)
    if [[ "$1" != "-y" ]] && [[ "$1" != "--yes" ]]; then
        read -p "Install to $INSTALL_DIR? [Y/n] " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]] && [[ -n $REPLY ]]; then
            info "Skipping system installation. Binaries available in ./bin/"
            setup_plugin_dir
            echo ""
            echo "To use RADS from this directory:"
            echo "  ./bin/rads"
            echo "  ./bin/rstar"
            exit 0
        fi
    fi

    install_rads
    setup_plugin_dir
    test_installation
    print_instructions
}

# Run main function
main "$@"
