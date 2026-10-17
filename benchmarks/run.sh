#!/usr/bin/env bash
# benchmarks/run.sh — thin wrapper (see run.py for the real runner)
set -e
cd "$(dirname "$0")/.."
exec python3 benchmarks/run.py "$@"
