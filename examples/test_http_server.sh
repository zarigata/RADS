#!/usr/bin/env bash
set -euo pipefail
BASE="http://localhost:8080"

echo "[1] GET /"
curl -i "$BASE/" || true

echo "\n[2] GET /api"
curl -i "$BASE/api" || true

echo "\n[3] GET /static/index.html"
curl -i "$BASE/static/index.html" || true

echo "\n[4] GET /missing"
curl -i "$BASE/missing" || true
