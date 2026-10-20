#!/usr/bin/env bash
# Builds the Linux client (preset linux-clang: clang + libc++, bgfx, SDL3) into ~/build/tclient-linux.
#
#   ./Client/build-linux.sh             # configure if needed, then build TClient
#   ./Client/build-linux.sh --deps      # first install the build packages (Debian/Ubuntu, sudo)
#   LLVM=19 ./Client/build-linux.sh     # another clang/lld version than 18
#
# Run the client from the game folder, with a config.ini next to the binary:
#   cp Game/config.ini ~/build/tclient-linux/bin/
#   cd Game && ~/build/tclient-linux/bin/TClient 127.0.0.1 4816
set -euo pipefail

LLVM="${LLVM:-18}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$HOME/build/tclient-linux"

if [[ "${1:-}" == "--deps" ]]; then
    # clang/lld/libc++ of $LLVM (Debian 12: add apt.llvm.org first), and SDL3's X11/Wayland/audio headers.
    sudo apt-get install -y \
        "clang-$LLVM" "lld-$LLVM" "libc++-$LLVM-dev" "libc++abi-$LLVM-dev" cmake ninja-build zlib1g-dev \
        libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxinerama-dev \
        libxtst-dev libxrender-dev libxkbcommon-dev libxkbcommon-x11-dev libwayland-dev wayland-protocols \
        libdecor-0-dev libegl-dev libgl-dev libdrm-dev libgbm-dev libudev-dev libdbus-1-dev \
        libpulse-dev libasound2-dev libpipewire-0.3-dev
fi

if ! command -v "clang++-$LLVM" >/dev/null; then
    echo "clang++-$LLVM not found: run with --deps, or set LLVM to the installed version." >&2
    exit 1
fi

cd "$ROOT/Client"
if [[ ! -f "$BUILD/build.ninja" ]]; then
    cmake --preset linux-clang \
        -DCMAKE_C_COMPILER="clang-$LLVM" \
        -DCMAKE_CXX_COMPILER="clang++-$LLVM" \
        -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++ --ld-path=/usr/bin/ld.lld-$LLVM"
fi

cmake --build "$BUILD" --target TClient
echo "Built $BUILD/bin/TClient"
