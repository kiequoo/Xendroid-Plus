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
INJECT_CMAKE="$REPO_ROOT/linux/zerofg_inject.cmake"

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
python3 ./xenia-build.py slang

# Configure manually so we can inject the zerofg::zerofg target.
# xenia-build.py's pass_args (--) only reach cmake --build, not cmake configure.
echo "--- configuring ---"
cmake -S . -B build -G "Ninja Multi-Config" \
    -DCMAKE_C_COMPILER="$CC" \
    -DCMAKE_CXX_COMPILER="$CXX" \
    -DCMAKE_PROJECT_xenia_INCLUDE="$INJECT_CMAKE"

# Build Release. Cap link/ThinLTO jobs so they don't exhaust RAM on the Odin 3.
echo "--- building xenia-app (Release) ---"
XENDROID_NINJA_JOBS="${XENDROID_NINJA_JOBS:-$(nproc)}" \
XENDROID_THINLTO_JOBS="${XENDROID_THINLTO_JOBS:-4}" \
python3 ./xenia-build.py build --config=Release --no_configure

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
