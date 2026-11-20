#!/usr/bin/env bash
# linux/build.sh — build xenia-app (Linux arm64 desktop) from the embedded xenia tree.
#
# Run this from the repo root on a native aarch64 Linux host (or inside a
# distrobox/toolbox container on one). No Android toolchain needed.
#
# Prerequisites: clang-21/lld-21/g++-14, cmake, ninja, pkg-config, plus the GTK3/X11/
# ALSA/PulseAudio/dbus/curl dev libs listed in linux/README.md.
#
# Output: emulator-core/src/main/cpp/xenia/build/bin/Linux/Release/xenia-app

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
XENIA_DIR="$REPO_ROOT/emulator-core/src/main/cpp/xenia"

# Allow caller to override the compiler (default: clang-21 / clang++-21).
export CC="${CC:-clang-21}"
export CXX="${CXX:-clang++-21}"

echo "=== xenia Linux arm64 build ==="
echo "    xenia dir : $XENIA_DIR"
echo "    CC        : $CC"
echo "    CXX       : $CXX"

cd "$XENIA_DIR"

# Download Slang shader compiler for the host arch (cached under .slang/).
echo "--- fetching Slang shader compiler ---"
./xenia-build.py slang

# Configure + build in Release.  On a native aarch64 host the top CMakeLists
# auto-detects XE_TARGET_AARCH64; the 'default' Ninja Multi-Config preset
# applies.  Cap simultaneous link jobs so they don't exhaust RAM on the Odin 3
# (each link can use 2–4 GiB for ThinLTO codegen).
echo "--- building xenia-app (Release) ---"
XENDROID_NINJA_JOBS="${XENDROID_NINJA_JOBS:-$(nproc)}" \
XENDROID_THINLTO_JOBS="${XENDROID_THINLTO_JOBS:-4}" \
./xenia-build.py build --config=Release

BINARY="$XENIA_DIR/build/bin/Linux/Release/xenia-app"
if [ ! -f "$BINARY" ]; then
    echo "ERROR: expected binary not found at $BINARY" >&2
    exit 1
fi

echo ""
echo "=== Build complete ==="
echo "    $(file "$BINARY")"
echo "    $(du -sh "$BINARY")"
echo ""
echo "Run:"
echo "    $BINARY /path/to/game.iso"
