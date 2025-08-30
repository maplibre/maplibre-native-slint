#!/usr/bin/env bash
set -euo pipefail
BUILD_DIR=${1:-build}
CONFIG=${2:-Debug}
GEN=${GEN:-Ninja}

if [[ ! -d "$BUILD_DIR" ]]; then
  cmake -S . -B "$BUILD_DIR" -G "$GEN" -DCMAKE_BUILD_TYPE=$CONFIG -DMLN_WITH_METAL=ON
fi
cmake --build "$BUILD_DIR" --target maplibre-slint-example -j
if [[ -x "$BUILD_DIR/maplibre-slint-example" ]]; then
  exec "$BUILD_DIR/maplibre-slint-example"
fi
if [[ -x "$BUILD_DIR/$CONFIG/maplibre-slint-example" ]]; then
  exec "$BUILD_DIR/$CONFIG/maplibre-slint-example"
fi
echo "Executable not found in expected locations" >&2
exit 1
