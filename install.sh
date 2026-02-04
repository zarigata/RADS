#!/bin/bash
#
# RADS Universal Installation Script
# Works on: Linux (all distros), macOS, Windows (WSL/Git Bash/MSYS2)
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/zarigata/RADS/main/install.sh | bash
#   wget -qO- https://raw.githubusercontent.com/zarigata/RADS/main/install.sh | bash
#
# Options:
#   -y, --yes       Skip confirmation prompts
#   -s, --source    Force build from source (skip binary download)
#   -b, --binary    Force binary download (skip build)
#   -d, --dir DIR   Install to custom directory (default: /usr/local/bin)
#   -h, --help      Show this help message
#

set -e

# ============================================================================
# Configuration
# ============================================================================
VERSION="0.0.7"
REPO_OWNER="zarigata"
REPO_NAME="RADS"
GITHUB_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}"
RAW_URL="https://raw.githubusercontent.com/${REPO_OWNER}/${REPO_NAME}/main"
RELEASES_URL="${GITHUB_URL}/releases"
INSTALL_DIR="${INSTALL_DIR:-/usr/local/bin}"
FORCE_SOURCE=false
FORCE_BINARY=false
AUTO_YES=false

# ============================================================================
# Colors and Output
# ============================================================================
if [[ -t 1 ]]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    BLUE='\033[0;34m'
    CYAN='\033[0;36m'
    MAGENTA='\033[0;35m'
    BOLD='\033[1m'
    NC='\033[0m'
else
    RED=''
    GREEN=''
    YELLOW=''
    BLUE=''
    CYAN=''
    MAGENTA=''
    BOLD=''
    NC=''
fi

info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
success() { echo -e "${GREEN}[OK]${NC} $1"; }
warn()    { echo -e "${YELLOW}[WARN]${NC} $1"; }
error()   { echo -e "${RED}[ERROR]${NC} $1" >&2; }
step()    { echo -e "${CYAN}==>${NC} ${BOLD}$1${NC}"; }

# ============================================================================
# Banner
# ============================================================================
print_banner() {
    echo ""
    echo -e "${MAGENTA}╔═══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${MAGENTA}║${NC}                                                           ${MAGENTA}║${NC}"
    echo -e "${MAGENTA}║${NC}   ${BOLD}${CYAN}RADS${NC} - Rapid Asynchronous Data Server               ${MAGENTA}║${NC}"
    echo -e "${MAGENTA}║${NC}   ${GREEN}v${VERSION} \"DARK MOON\"${NC}                                    ${MAGENTA}║${NC}"
    echo -e "${MAGENTA}║${NC}                                                           ${MAGENTA}║${NC}"
    echo -e "${MAGENTA}║${NC}   ${BLUE}https://github.com/zarigata/RADS${NC}                       ${MAGENTA}║${NC}"
    echo -e "${MAGENTA}╚═══════════════════════════════════════════════════════════╝${NC}"
    echo ""
}

# ============================================================================
# Help
# ============================================================================
print_help() {
    echo "RADS Installation Script v${VERSION}"
    echo ""
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -y, --yes       Skip confirmation prompts"
    echo "  -s, --source    Force build from source"
    echo "  -b, --binary    Force binary download"
    echo "  -d, --dir DIR   Install to custom directory (default: /usr/local/bin)"
    echo "  -h, --help      Show this help message"
    echo ""
    echo "Environment Variables:"
    echo "  INSTALL_DIR     Installation directory"
    echo "  UV_INCLUDE      libuv include path"
    echo "  UV_LIB          libuv library path"
}

# ============================================================================
# Parse Arguments
# ============================================================================
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -y|--yes)
                AUTO_YES=true
                shift
                ;;
            -s|--source)
                FORCE_SOURCE=true
                shift
                ;;
            -b|--binary)
                FORCE_BINARY=true
                shift
                ;;
            -d|--dir)
                INSTALL_DIR="$2"
                shift 2
                ;;
            -h|--help)
                print_help
                exit 0
                ;;
            *)
                error "Unknown option: $1"
                print_help
                exit 1
                ;;
        esac
    done
}

# ============================================================================
# System Detection
# ============================================================================
detect_os() {
    OS=""
    DISTRO=""
    ARCH=$(uname -m)

    case "$OSTYPE" in
        linux-gnu*|linux*)
            OS="linux"
            # Detect Linux distribution
            if [ -f /etc/os-release ]; then
                . /etc/os-release
                DISTRO="$ID"
            elif [ -f /etc/debian_version ]; then
                DISTRO="debian"
            elif [ -f /etc/redhat-release ]; then
                DISTRO="rhel"
            elif [ -f /etc/arch-release ]; then
                DISTRO="arch"
            elif [ -f /etc/alpine-release ]; then
                DISTRO="alpine"
            else
                DISTRO="unknown"
            fi
            ;;
        darwin*)
            OS="macos"
            DISTRO="macos"
            ;;
        msys*|cygwin*|mingw*)
            OS="windows"
            DISTRO="windows"
            ;;
        *)
            # Try uname
            case "$(uname -s)" in
                Linux*)
                    OS="linux"
                    DISTRO="unknown"
                    ;;
                Darwin*)
                    OS="macos"
                    DISTRO="macos"
                    ;;
                MINGW*|MSYS*|CYGWIN*)
                    OS="windows"
                    DISTRO="windows"
                    ;;
                *)
                    error "Unsupported operating system: $OSTYPE ($(uname -s))"
                    exit 1
                    ;;
            esac
            ;;
    esac

    # Normalize architecture
    case "$ARCH" in
        x86_64|amd64)
            ARCH="x86_64"
            ;;
        aarch64|arm64)
            ARCH="aarch64"
            ;;
        armv7l|armhf)
            ARCH="armv7"
            ;;
        i386|i686)
            ARCH="i386"
            ;;
    esac

    info "Detected: ${BOLD}${OS}${NC} (${DISTRO}) on ${BOLD}${ARCH}${NC}"
}

# ============================================================================
# Dependency Installation
# ============================================================================
check_command() {
    command -v "$1" &> /dev/null
}

get_package_manager() {
    if check_command apt-get; then
        echo "apt"
    elif check_command dnf; then
        echo "dnf"
    elif check_command yum; then
        echo "yum"
    elif check_command pacman; then
        echo "pacman"
    elif check_command apk; then
        echo "apk"
    elif check_command zypper; then
        echo "zypper"
    elif check_command brew; then
        echo "brew"
    elif check_command pkg; then
        echo "pkg"
    else
        echo "unknown"
    fi
}

install_dependencies() {
    step "Checking and installing dependencies..."

    local pkg_manager=$(get_package_manager)
    local need_install=false
    local missing_deps=()

    # Check for required tools
    if ! check_command gcc && ! check_command clang; then
        missing_deps+=("compiler")
        need_install=true
    fi

    if ! check_command make; then
        missing_deps+=("make")
        need_install=true
    fi

    if ! check_command git; then
        missing_deps+=("git")
        need_install=true
    fi

    # Check for libuv
    local has_libuv=false
    if pkg-config --exists libuv 2>/dev/null; then
        has_libuv=true
    elif [[ "$OS" == "macos" ]] && brew list libuv &>/dev/null 2>&1; then
        has_libuv=true
    elif [[ -f /usr/include/uv.h ]] || [[ -f /usr/local/include/uv.h ]]; then
        has_libuv=true
    fi

    if ! $has_libuv; then
        missing_deps+=("libuv")
        need_install=true
    fi

    # Check for readline
    local has_readline=false
    if [[ -f /usr/include/readline/readline.h ]] || \
       [[ -f /usr/local/include/readline/readline.h ]] || \
       [[ -f /opt/homebrew/include/readline/readline.h ]]; then
        has_readline=true
    fi

    if ! $has_readline; then
        missing_deps+=("readline")
        need_install=true
    fi

    # Check for sqlite3
    local has_sqlite=false
    if pkg-config --exists sqlite3 2>/dev/null || \
       [[ -f /usr/include/sqlite3.h ]] || \
       [[ -f /usr/local/include/sqlite3.h ]]; then
        has_sqlite=true
    fi

    if ! $has_sqlite; then
        missing_deps+=("sqlite3")
        need_install=true
    fi

    if $need_install; then
        warn "Missing dependencies: ${missing_deps[*]}"

        if [[ "$AUTO_YES" != "true" ]]; then
            echo ""
            read -p "Install missing dependencies? [Y/n] " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Nn]$ ]]; then
                error "Cannot continue without dependencies."
                exit 1
            fi
        fi

        info "Installing dependencies using ${pkg_manager}..."

        case "$pkg_manager" in
            apt)
                sudo apt-get update -qq
                sudo apt-get install -y build-essential git libuv1-dev libreadline-dev libsqlite3-dev pkg-config
                ;;
            dnf)
                sudo dnf install -y gcc make git libuv-devel readline-devel sqlite-devel pkgconfig
                ;;
            yum)
                sudo yum install -y gcc make git libuv-devel readline-devel sqlite-devel pkgconfig
                ;;
            pacman)
                sudo pacman -Sy --noconfirm base-devel git libuv readline sqlite
                ;;
            apk)
                sudo apk add --no-cache build-base git libuv-dev readline-dev sqlite-dev pkgconfig
                ;;
            zypper)
                sudo zypper install -y gcc make git libuv-devel readline-devel sqlite3-devel pkg-config
                ;;
            brew)
                brew install libuv readline sqlite3 pkg-config
                ;;
            pkg)
                sudo pkg install -y libuv readline sqlite3 pkgconf
                ;;
            *)
                error "Unknown package manager. Please install manually:"
                echo "  - GCC or Clang compiler"
                echo "  - GNU Make"
                echo "  - Git"
                echo "  - libuv development files"
                echo "  - readline development files"
                echo "  - sqlite3 development files"
                exit 1
                ;;
        esac

        success "Dependencies installed successfully"
    else
        success "All dependencies are already installed"
    fi
}

# ============================================================================
# Download Binary
# ============================================================================
try_download_binary() {
    if [[ "$FORCE_SOURCE" == "true" ]]; then
        return 1
    fi

    step "Checking for pre-built binary..."

    local binary_name="rads-${VERSION}-${OS}-${ARCH}"
    local tarball="${binary_name}.tar.gz"
    local download_url="${RELEASES_URL}/download/v${VERSION}/${tarball}"

    # Check if release exists
    if ! check_command curl && ! check_command wget; then
        warn "Neither curl nor wget found, cannot download binary"
        return 1
    fi

    local http_code=""
    if check_command curl; then
        http_code=$(curl -sI -o /dev/null -w "%{http_code}" "$download_url" 2>/dev/null || echo "000")
    else
        http_code=$(wget --spider -S "$download_url" 2>&1 | grep "HTTP/" | tail -1 | awk '{print $2}' || echo "000")
    fi

    if [[ "$http_code" != "200" && "$http_code" != "302" ]]; then
        warn "Pre-built binary not available for ${OS}-${ARCH}"
        return 1
    fi

    info "Downloading ${tarball}..."

    local tmp_dir=$(mktemp -d)
    trap "rm -rf $tmp_dir" EXIT

    if check_command curl; then
        curl -fsSL "$download_url" -o "${tmp_dir}/${tarball}"
    else
        wget -q "$download_url" -O "${tmp_dir}/${tarball}"
    fi

    info "Extracting..."
    tar -xzf "${tmp_dir}/${tarball}" -C "$tmp_dir"

    # Install binaries
    local extract_dir="${tmp_dir}/${binary_name}"
    if [[ ! -d "$extract_dir" ]]; then
        extract_dir="${tmp_dir}/rads-${VERSION}"
    fi
    if [[ ! -d "$extract_dir" ]]; then
        extract_dir="$tmp_dir"
    fi

    install_binaries "$extract_dir"

    return 0
}

# ============================================================================
# Build from Source
# ============================================================================
build_from_source() {
    step "Building from source..."

    local tmp_dir=$(mktemp -d)
    local src_dir="${tmp_dir}/RADS"

    info "Cloning repository..."
    git clone --depth 1 --branch "v${VERSION}" "${GITHUB_URL}.git" "$src_dir" 2>/dev/null || \
    git clone --depth 1 "${GITHUB_URL}.git" "$src_dir"

    cd "$src_dir"

    info "Building RADS..."

    # Determine number of parallel jobs
    local jobs=2
    if check_command nproc; then
        jobs=$(nproc)
    elif check_command sysctl; then
        jobs=$(sysctl -n hw.ncpu 2>/dev/null || echo 2)
    fi

    if make -j"$jobs"; then
        success "Build completed successfully"
    else
        error "Build failed"
        cd /
        rm -rf "$tmp_dir"
        exit 1
    fi

    install_binaries "${src_dir}/bin"

    cd /
    rm -rf "$tmp_dir"
}

# ============================================================================
# Install Binaries
# ============================================================================
install_binaries() {
    local bin_dir="$1"

    step "Installing to ${INSTALL_DIR}..."

    # Check if we need sudo
    local SUDO=""
    if [[ ! -w "$INSTALL_DIR" ]]; then
        if check_command sudo; then
            SUDO="sudo"
            warn "Requesting sudo access for installation to $INSTALL_DIR"
        else
            error "Cannot write to $INSTALL_DIR and sudo is not available"
            error "Try: INSTALL_DIR=~/.local/bin $0"
            exit 1
        fi
    fi

    # Create install dir if needed
    $SUDO mkdir -p "$INSTALL_DIR"

    # Install each binary
    local binaries=("rads" "rstar" "radpkg" "rads-mask")
    local installed=()

    for bin in "${binaries[@]}"; do
        local src="${bin_dir}/${bin}"
        if [[ -f "$src" ]]; then
            $SUDO install -m 755 "$src" "${INSTALL_DIR}/${bin}"
            installed+=("$bin")
            success "Installed ${bin} -> ${INSTALL_DIR}/${bin}"
        elif [[ -f "${bin_dir}/bin/${bin}" ]]; then
            $SUDO install -m 755 "${bin_dir}/bin/${bin}" "${INSTALL_DIR}/${bin}"
            installed+=("$bin")
            success "Installed ${bin} -> ${INSTALL_DIR}/${bin}"
        fi
    done

    if [[ ${#installed[@]} -eq 0 ]]; then
        error "No binaries found to install"
        exit 1
    fi
}

# ============================================================================
# Setup
# ============================================================================
setup_directories() {
    local rads_home="$HOME/.rads"

    mkdir -p "${rads_home}/plugins"
    mkdir -p "${rads_home}/packages"

    success "Created RADS home directory: ${rads_home}"
}

# ============================================================================
# Verify Installation
# ============================================================================
verify_installation() {
    step "Verifying installation..."

    local all_good=true

    # Check if binaries are accessible
    if check_command rads; then
        local ver=$(rads --version 2>/dev/null || echo "unknown")
        success "rads: ${ver}"
    else
        warn "rads not in PATH"
        all_good=false
    fi

    if check_command rstar; then
        success "rstar: OK"
    else
        warn "rstar not in PATH"
    fi

    if ! $all_good; then
        echo ""
        warn "Add the following to your shell profile (.bashrc, .zshrc, etc.):"
        echo ""
        echo "    export PATH=\"${INSTALL_DIR}:\$PATH\""
        echo ""
    fi
}

# ============================================================================
# Post-Install Message
# ============================================================================
print_success() {
    echo ""
    echo -e "${GREEN}╔═══════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║${NC}                                                           ${GREEN}║${NC}"
    echo -e "${GREEN}║${NC}   ${BOLD}Installation Complete!${NC}                                 ${GREEN}║${NC}"
    echo -e "${GREEN}║${NC}                                                           ${GREEN}║${NC}"
    echo -e "${GREEN}╚═══════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "${BOLD}Quick Start:${NC}"
    echo ""
    echo -e "  ${CYAN}rads${NC}                    # Start interactive REPL"
    echo -e "  ${CYAN}rads hello.rads${NC}         # Run a RADS program"
    echo -e "  ${CYAN}rads --version${NC}          # Show version"
    echo -e "  ${CYAN}rstar --help${NC}            # Package manager help"
    echo ""
    echo -e "${BOLD}Documentation:${NC}"
    echo ""
    echo -e "  Website:   ${BLUE}https://zarigata.github.io/RADS/${NC}"
    echo -e "  GitHub:    ${BLUE}https://github.com/zarigata/RADS${NC}"
    echo ""
    echo -e "${BOLD}Example Program:${NC}"
    echo ""
    echo -e "  ${MAGENTA}blast${NC} main() {"
    echo -e "      echo(${GREEN}\"Hello, RADS World!\"${NC});"
    echo -e "  }"
    echo ""
}

# ============================================================================
# Main
# ============================================================================
main() {
    parse_args "$@"
    print_banner
    detect_os
    install_dependencies

    # Try binary download first, fall back to source
    if [[ "$FORCE_BINARY" == "true" ]]; then
        if ! try_download_binary; then
            error "Binary download requested but failed"
            exit 1
        fi
    elif [[ "$FORCE_SOURCE" == "true" ]]; then
        build_from_source
    else
        if ! try_download_binary; then
            info "Falling back to building from source..."
            build_from_source
        fi
    fi

    setup_directories
    verify_installation
    print_success
}

main "$@"
