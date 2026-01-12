#!/bin/bash
# Wrapper script to run constellation prototype from project root

cd "$(dirname "$0")/../.."  # Go to project root

if [ ! -f "rads" ]; then
    echo "Error: rads executable not found in project root"
    exit 1
fi

# Run the spawner with updated path
prototype/constellation/simple_spawner "$@"
