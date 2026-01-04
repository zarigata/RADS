#!/bin/bash
# RADS Universal Installer 🚀
# "90s/2000s Vibes, Modern Performance"

set -e

echo "--------------------------------------------------"
echo "🛸 RADS BUILD & INSTALLER v0.1.0-alpha"
echo "--------------------------------------------------"

# Configurable install prefix (default: /usr/local)
RADS_PREFIX="${RADS_PREFIX:-/usr/local}"
BIN_DIR="${RADS_PREFIX}/bin"

# Allow skipping sudo (useful for local user installs/tests)
USE_SUDO=${USE_SUDO:-yes}
if [ "$USE_SUDO" = "yes" ]; then
    SUDO_CMD="sudo"
else
    SUDO_CMD=""
fi

# Check for build tools
if ! command -v gcc &> /dev/null; then
    echo "⚠️ Error: gcc not found. Please install a C compiler."
    exit 1
fi

if ! command -v make &> /dev/null; then
    echo "⚠️ Error: make not found. Please install build-essential."
    exit 1
fi

echo "📦 Step 1: Compiling RADS Core..."
make clean && make

echo "🛠️ Step 2: Compiling RPM Package Manager..."
gcc -D_POSIX_C_SOURCE=200809L -Isrc tools/rpm/rpm.c -o tools/rpm/rpm

echo "💾 Step 3: Installing to ${BIN_DIR} ..."
if [ "$USE_SUDO" = "yes" ]; then
    sudo mkdir -p "$BIN_DIR"
else
    mkdir -p "$BIN_DIR"
fi

# Install binaries
if [ "$USE_SUDO" = "yes" ]; then
    sudo install -m 755 rads "$BIN_DIR/"
    sudo install -m 755 tools/rpm/rpm "$BIN_DIR/"
else
    install -m 755 rads "$BIN_DIR/"
    install -m 755 tools/rpm/rpm "$BIN_DIR/"
fi

echo ""
echo "✨ RADS installed! Ensure ${BIN_DIR} is on your PATH."
echo "🚀 Try running: ${BIN_DIR}/rads --help"
echo "--------------------------------------------------"
