#!/bin/bash
set -e

echo "Building RADS..."
gcc -Wall -Wextra -std=c11 -O2 src/*.c -o rads

echo "Build successful!"
echo ""
echo "Testing hello_world.rads..."
./rads examples/hello_world.rads
