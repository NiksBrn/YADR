#!/usr/bin/env bash
# Configure + build the project (backend + frontend) into ./build.
# Usage: ./scripts/build.sh [Release|Debug]
set -euo pipefail

BUILD_TYPE="${1:-Release}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"

cd "$ROOT"
cmake -S . -B build -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
cmake --build build -j"$(nproc)"

echo
echo "Built:"
echo "  ./build/yadr-server"
echo "  ./build/web/"
echo
echo "Run with: ./scripts/run.sh"
