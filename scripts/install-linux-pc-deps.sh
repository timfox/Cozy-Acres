#!/bin/bash
# Optional helper: install Debian/Ubuntu packages needed to build the PC port on 64-bit Linux.
# Requires sudo. See README.md for manual steps and Fedora notes.
#
# Prefers host gcc/g++ with multilib (-m32). If apt cannot resolve gcc-multilib
# (some images report pkgProblemResolver breaks), falls back to gcc-i686-linux-gnu
# plus the same 32-bit SDL2/OpenGL packages — matching ./build_pc.sh behavior.

set -euo pipefail

# 32-bit x86 dev packages (libsdl2-dev:i386, etc.) are published for multiarch on amd64.
# On ARM64 they are not available the same way; build inside linux/amd64 instead.
HOST_ARCH="$(dpkg --print-architecture 2>/dev/null || true)"
if [ -z "$HOST_ARCH" ]; then
    case "$(uname -m)" in
        x86_64) HOST_ARCH=amd64 ;;
        i686|i386) HOST_ARCH=i386 ;;
        aarch64) HOST_ARCH=arm64 ;;
        armv7l) HOST_ARCH=armhf ;;
        *) HOST_ARCH="" ;;
    esac
fi
if [ "$HOST_ARCH" != "amd64" ] && [ "$HOST_ARCH" != "i386" ]; then
    echo "This PC port is built as 32-bit x86 (i686). Your system is ${HOST_ARCH:-unknown} ($(uname -m))."
    echo "apt cannot install libsdl2-dev:i386 / libgl1-mesa-dev:i386 here the way it does on x86_64."
    echo ""
    echo "Use a container that emulates x86_64 userspace, from the repo root:"
    echo "  ./scripts/docker-build-linux-amd64.sh"
    echo ""
    echo "Or build on an actual x86_64 Linux machine or VM."
    exit 1
fi

sudo dpkg --add-architecture i386 2>/dev/null || true
sudo apt-get update

COMMON_PKGS=(
  build-essential
  cmake
  ninja-build
  pkg-config
  libsdl2-dev:i386
  libgl1-mesa-dev:i386
)

sudo apt-get install -y "${COMMON_PKGS[@]}"

if sudo apt-get install -y gcc-multilib g++-multilib; then
  echo "Installed multilib gcc/g++ (-m32)."
else
  echo ""
  echo "Multilib install failed (common when apt has held/broken dependencies)."
  echo "Installing i686 cross compiler instead; ./build_pc.sh will use Toolchain-linux32.cmake."
  echo ""
  sudo apt-get install -y gcc-i686-linux-gnu g++-i686-linux-gnu
fi

echo "Dependencies installed. From the repo root run: ./build_pc.sh"
