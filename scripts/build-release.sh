#!/bin/bash
#
# RADS Release Build Script
# Builds release packages for distribution
#
# Usage: ./scripts/build-release.sh [version]
#

set -e

VERSION="${1:-0.0.7}"
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
RELEASE_DIR="${PROJECT_ROOT}/release/rads-${VERSION}"
DIST_DIR="${PROJECT_ROOT}/dist"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
success() { echo -e "${GREEN}[OK]${NC} $1"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $1"; }
error()   { echo -e "${RED}[ERROR]${NC} $1" >&2; }

# Detect platform
detect_platform() {
    local os=""
    local arch=$(uname -m)

    case "$(uname -s)" in
        Linux*)  os="linux" ;;
        Darwin*) os="macos" ;;
        MINGW*|MSYS*|CYGWIN*) os="windows" ;;
        *) error "Unsupported OS"; exit 1 ;;
    esac

    case "$arch" in
        x86_64|amd64) arch="x86_64" ;;
        aarch64|arm64) arch="aarch64" ;;
        armv7l) arch="armv7" ;;
    esac

    echo "${os}-${arch}"
}

# Clean previous build
clean_release() {
    info "Cleaning previous release..."
    rm -rf "${RELEASE_DIR}"
    mkdir -p "${RELEASE_DIR}"
    mkdir -p "${DIST_DIR}"
}

# Build the project
build_project() {
    info "Building RADS v${VERSION}..."
    cd "${PROJECT_ROOT}"

    make clean || true

    local jobs=2
    if command -v nproc &>/dev/null; then
        jobs=$(nproc)
    elif command -v sysctl &>/dev/null; then
        jobs=$(sysctl -n hw.ncpu 2>/dev/null || echo 2)
    fi

    if make -j"$jobs"; then
        success "Build completed"
    else
        error "Build failed"
        exit 1
    fi
}

# Copy files to release directory
prepare_release() {
    info "Preparing release directory..."

    # Create directory structure
    mkdir -p "${RELEASE_DIR}/bin"
    mkdir -p "${RELEASE_DIR}/docs"
    mkdir -p "${RELEASE_DIR}/examples"

    # Copy binaries
    for bin in rads rstar rpm rads-mask; do
        if [[ -f "${PROJECT_ROOT}/bin/${bin}" ]]; then
            cp "${PROJECT_ROOT}/bin/${bin}" "${RELEASE_DIR}/bin/"
            success "Copied ${bin}"
        else
            warn "Binary not found: ${bin}"
        fi
    done

    # Copy documentation
    cp "${PROJECT_ROOT}/README.md" "${RELEASE_DIR}/"
    cp "${PROJECT_ROOT}/LICENSE" "${RELEASE_DIR}/" 2>/dev/null || echo "MIT License" > "${RELEASE_DIR}/LICENSE"
    cp "${PROJECT_ROOT}/CHANGELOG.md" "${RELEASE_DIR}/" 2>/dev/null || true
    cp "${PROJECT_ROOT}/install.sh" "${RELEASE_DIR}/"
    chmod +x "${RELEASE_DIR}/install.sh"

    # Copy essential docs
    for doc in LANGUAGE_SPEC.md REPL_GUIDE.md PACKAGE_MANAGER.md; do
        if [[ -f "${PROJECT_ROOT}/docs/${doc}" ]]; then
            cp "${PROJECT_ROOT}/docs/${doc}" "${RELEASE_DIR}/docs/"
        fi
    done

    # Copy examples
    if [[ -d "${PROJECT_ROOT}/examples" ]]; then
        cp -r "${PROJECT_ROOT}/examples/"* "${RELEASE_DIR}/examples/" 2>/dev/null || true
    fi

    success "Release directory prepared"
}

# Create release tarball
create_tarball() {
    local platform=$(detect_platform)
    local tarball_name="rads-${VERSION}-${platform}.tar.gz"
    local tarball_path="${DIST_DIR}/${tarball_name}"

    info "Creating tarball: ${tarball_name}"

    cd "${PROJECT_ROOT}/release"
    tar -czvf "${tarball_path}" "rads-${VERSION}"

    success "Created: ${tarball_path}"
    echo "${tarball_path}"
}

# Generate checksums
generate_checksums() {
    info "Generating checksums..."

    cd "${DIST_DIR}"

    # SHA256 checksums
    if command -v sha256sum &>/dev/null; then
        sha256sum rads-*.tar.gz > CHECKSUMS-SHA256.txt
    elif command -v shasum &>/dev/null; then
        shasum -a 256 rads-*.tar.gz > CHECKSUMS-SHA256.txt
    fi

    # Individual binary checksums
    cd "${RELEASE_DIR}/bin"
    if command -v sha256sum &>/dev/null; then
        sha256sum * > "${DIST_DIR}/BIN-CHECKSUMS-SHA256.txt"
    elif command -v shasum &>/dev/null; then
        shasum -a 256 * > "${DIST_DIR}/BIN-CHECKSUMS-SHA256.txt"
    fi

    success "Checksums generated"
}

# Print release info
print_summary() {
    echo ""
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN} Release v${VERSION} Build Complete!${NC}"
    echo -e "${GREEN}═══════════════════════════════════════════════════════════${NC}"
    echo ""
    echo "Release files:"
    ls -la "${DIST_DIR}/"
    echo ""
    echo "Release contents:"
    ls -la "${RELEASE_DIR}/"
    ls -la "${RELEASE_DIR}/bin/"
    echo ""
    echo "To upload to GitHub:"
    echo "  gh release create v${VERSION} ${DIST_DIR}/rads-*.tar.gz --title \"RADS v${VERSION} DARK MOON\" --notes-file RELEASE_NOTES.md"
    echo ""
}

# Main
main() {
    echo ""
    echo -e "${BLUE}╔═══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC}  RADS Release Builder v${VERSION}                             ${BLUE}║${NC}"
    echo -e "${BLUE}╚═══════════════════════════════════════════════════════════╝${NC}"
    echo ""

    clean_release
    build_project
    prepare_release
    create_tarball
    generate_checksums
    print_summary
}

main "$@"
