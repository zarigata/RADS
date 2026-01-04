#!/bin/bash
# RADS Universal Installer 🚀
# "90s/2000s Vibes, Modern Performance"

set -e

echo "--------------------------------------------------"
echo "🛸 RADS BUILD & INSTALLER v0.1.0-alpha"
echo "--------------------------------------------------"

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

echo "💾 Step 3: Installing system-wide..."
sudo make install

echo ""
echo "✨ RADS is now global! Blast code from anywhere with 'rads'."
echo "🚀 Try running: rads --help"
echo "--------------------------------------------------"
