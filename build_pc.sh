#!/bin/bash
# build_pc.sh - Build the Animal Crossing PC port
#
# Windows: run from MSYS2 MINGW32 (32-bit).
# Linux:   run from any shell; requires a 32-bit toolchain and 32-bit SDL2/OpenGL dev.
#
# Usage:
#   ./build_pc.sh
#   Place your disc image (.ciso/.iso/.gcm) in pc/build32/bin/rom/
#   Run the binary from pc/build32/bin/
#
# Optional: CMAKE_RECONFIGURE=1 to force CMake reconfigure.
# Optional: EXTRA_CMAKE_ARGS="..." extra flags passed to cmake (one line).

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/pc/build32"
BIN_DIR="$BUILD_DIR/bin"
PC_DIR="$SCRIPT_DIR/pc"
TOOLCHAIN_FILE="$PC_DIR/cmake/Toolchain-linux32.cmake"

# --- Linux: i386 SDL/Mesa .deb packages exist for amd64 multiarch, not for ARM64 ---
linux_host_can_use_i386_apt_packages() {
    if command -v dpkg >/dev/null 2>&1; then
        local d
        d="$(dpkg --print-architecture 2>/dev/null || true)"
        case "$d" in
            arm64|armhf|armel|riscv64|ppc64el|s390x) return 1 ;;
        esac
    fi
    case "$(uname -m)" in
        x86_64|i686|i386) ;;
        aarch64|armv8*|armv7l|armv6l|riscv64|ppc64le) return 1 ;;
        *) return 1 ;;
    esac
    if command -v dpkg >/dev/null 2>&1; then
        local d
        d="$(dpkg --print-architecture 2>/dev/null || true)"
        if [ -n "$d" ] && [ "$d" != "amd64" ] && [ "$d" != "i386" ]; then
            return 1
        fi
    fi
    return 0
}

# --- Linux: locate sdl2.pc for i386 (multilib / multiarch) ---
linux_find_sdl2_pkgconfig() {
    local d
    for d in \
        /usr/lib/i386-linux-gnu/pkgconfig \
        /usr/lib32/pkgconfig \
        /usr/lib/i686-linux-gnu/pkgconfig; do
        if [ -f "$d/sdl2.pc" ]; then
            printf '%s' "$d"
            return 0
        fi
    done
    return 1
}

linux_gcc_m32_works() {
    local tmp
    tmp="$(mktemp -d)"
    trap 'rm -rf "$tmp"' RETURN
    printf 'int main(void){return 0;}\n' >"$tmp/t.c"
    gcc -m32 -o "$tmp/t" "$tmp/t.c" 2>/dev/null
}

linux_cmake_build() {
    local generator="$1"
    shift
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    if [ ! -f CMakeCache.txt ] || [ "${CMAKE_RECONFIGURE:-}" = 1 ]; then
        echo "=== Configuring CMake ($generator) ==="
        # shellcheck disable=SC2206
        EXTRA_CMAKE=( )
        if [ -n "${EXTRA_CMAKE_ARGS:-}" ]; then
            read -r -a EXTRA_CMAKE <<< "${EXTRA_CMAKE_ARGS}"
        fi
        cmake "$PC_DIR" -G "$generator" "$@" "${EXTRA_CMAKE[@]}"
    fi

    echo "=== Building PC port ==="
    if [ "$generator" = "Ninja" ]; then
        ninja -j"$(nproc)"
    else
        make -j"$(nproc)"
    fi
}

build_linux() {
    if ! linux_host_can_use_i386_apt_packages; then
        echo "Error: This machine ($(uname -m); dpkg: $(dpkg --print-architecture 2>/dev/null || echo n/a)) cannot use Ubuntu/Debian i386 multiarch dev packages."
        echo ""
        echo "The PC port is always 32-bit x86 (i686). libsdl2-dev:i386 and libgl1-mesa-dev:i386 are for x86_64 hosts only."
        echo "A 32-bit x86 AnimalCrossing binary also cannot load ARM libSDL2 — the \"libSDL2-2.0.so.0: No such file\" error on ARM is expected."
        echo ""
        echo "What to do:"
        echo "  1. Build inside an emulated x86_64 environment:"
        echo "       ./scripts/docker-build-linux-amd64.sh"
        echo "  2. Copy the repo (or just pc/build32/bin/) to a real x86_64 Linux PC."
        echo "  3. There install runtime libs and run:"
        echo "       ./scripts/install-linux-pc-deps.sh --runtime"
        echo "       pc/build32/bin/AnimalCrossing"
        exit 1
    fi

    local SDL_PC
    SDL_PC="$(linux_find_sdl2_pkgconfig || true)"

    local CMAKE_ARGS=()
    local GEN="Unix Makefiles"
    if command -v ninja >/dev/null 2>&1; then
        GEN="Ninja"
    fi

    if command -v i686-linux-gnu-gcc >/dev/null 2>&1 \
        && command -v i686-linux-gnu-g++ >/dev/null 2>&1; then
        echo "=== Using i686-linux-gnu cross compiler ==="
        CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE")
    elif linux_gcc_m32_works; then
        echo "=== Using host gcc/g++ with -m32 (multilib) ==="
        CMAKE_ARGS+=(
            -DCMAKE_C_FLAGS=-m32
            -DCMAKE_CXX_FLAGS=-m32
            -DCMAKE_EXE_LINKER_FLAGS=-m32
        )
    else
        echo "Error: No 32-bit C/C++ toolchain found."
        echo ""
        echo "This project must be built as 32-bit (see pc/CMakeLists.txt)."
        if linux_host_can_use_i386_apt_packages; then
            echo "On Debian/Ubuntu (x86_64), run:"
            echo "  ./scripts/install-linux-pc-deps.sh"
            echo ""
            echo "Or manually:"
            echo "  sudo dpkg --add-architecture i386   # if needed"
            echo "  sudo apt update"
            echo "  sudo apt install gcc-multilib g++-multilib libsdl2-dev:i386 \\"
            echo "                 libgl1-mesa-dev:i386 pkg-config"
            echo ""
            echo "Or: sudo apt install gcc-i686-linux-gnu g++-i686-linux-gnu libsdl2-dev:i386 libgl1-mesa-dev:i386"
        fi
        exit 1
    fi

    if [ -n "$SDL_PC" ]; then
        export PKG_CONFIG_LIBDIR="$SDL_PC"
        export PKG_CONFIG_PATH=""
        echo "=== Using SDL2 from PKG_CONFIG_LIBDIR=$PKG_CONFIG_LIBDIR ==="
    else
        echo "Warning: Could not find sdl2.pc under common i386 pkg-config paths."
        echo "If CMake fails to find SDL2, set PKG_CONFIG_LIBDIR to the directory"
        echo "that contains sdl2.pc for 32-bit (e.g. /usr/lib/i386-linux-gnu/pkgconfig)."
    fi

    linux_cmake_build "$GEN" "${CMAKE_ARGS[@]}"
}

build_msys2_mingw32() {
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    if [ ! -f CMakeCache.txt ] || [ "${CMAKE_RECONFIGURE:-}" = 1 ]; then
        echo "=== Configuring CMake (MinGW Makefiles) ==="
        EXTRA_CMAKE=( )
        if [ -n "${EXTRA_CMAKE_ARGS:-}" ]; then
            read -r -a EXTRA_CMAKE <<< "${EXTRA_CMAKE_ARGS}"
        fi
        cmake "$PC_DIR" -G "MinGW Makefiles" "${EXTRA_CMAKE[@]}"
    fi

    echo "=== Building PC port ==="
    mingw32-make -j"$(nproc)"
}

# --- Main ---

mkdir -p "$BIN_DIR/rom"
mkdir -p "$BIN_DIR/texture_pack"
mkdir -p "$BIN_DIR/save"

case "$(uname -s)" in
Linux)
    build_linux
    EXE_NAME="AnimalCrossing"
    ;;
MINGW*|MSYS*)
    if [ "${MSYSTEM:-}" != "MINGW32" ]; then
        echo "Error: On Windows, run this script from MSYS2 MINGW32 (32-bit), not MINGW64."
        exit 1
    fi
    build_msys2_mingw32
    EXE_NAME="AnimalCrossing.exe"
    ;;
*)
    echo "Error: Unsupported OS: $(uname -s)"
    echo "Use Linux natively, or Windows with MSYS2 MINGW32."
    exit 1
    ;;
esac

echo ""
echo "=== Build complete! ==="
echo ""
echo "Place your Animal Crossing disc image (.ciso/.iso/.gcm) in:"
echo "  pc/build32/bin/rom/"
echo ""
echo "Run: pc/build32/bin/$EXE_NAME"
