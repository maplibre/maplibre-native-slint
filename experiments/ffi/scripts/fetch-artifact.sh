#!/usr/bin/env bash
# Fetches a prebuilt maplibre-native-ffi artifact into ffi/third_party.
#
# The prebuilt package is self-contained: a 14 MB libmaplibre-native-c.so whose
# only shared dependencies are libc, libm, libpthread and libdl. Nothing here
# builds maplibre-native, which is the whole point of trying this route.
set -euo pipefail

VERSION="${MLN_FFI_VERSION:-core/v0.202608.0}"
TARGET="${MLN_FFI_TARGET:-linux-x64-egl}"

here="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
dest="$here/third_party"
name="maplibre-native-c-$TARGET"

mkdir -p "$dest"
if [ -d "$dest/$name" ]; then
    echo "already present: $dest/$name"
    exit 0
fi

gh release download "$VERSION" \
    --repo maplibre/maplibre-native-ffi \
    --pattern "$name.tar.gz" \
    --dir "$dest" --clobber
tar xzf "$dest/$name.tar.gz" -C "$dest"
echo "extracted: $dest/$name"
