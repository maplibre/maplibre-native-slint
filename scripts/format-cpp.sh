#!/usr/bin/env bash
set -euo pipefail

# Format all C++ sources, excluding vendor/, build/, build-ninja/, and .git/
# Usage: scripts/format-cpp.sh

if ! command -v clang-format >/dev/null 2>&1; then
  echo "clang-format not found in PATH" >&2
  exit 127
fi

# Build file list while excluding directories via -prune, then print each
find . \
  -type d \( -path ./vendor -o -path ./build -o -path ./build-ninja -o -path ./.git \) -prune \
  -o \( -name "*.cpp" -o -name "*.hpp" \) -print0 \
| while IFS= read -r -d '' f; do
    echo "format: $f"
    clang-format -i "$f"
  done

echo "C++ sources formatted (exclusions: vendor/, build/, build-ninja/, .git/)."
