#!/bin/bash
# Optional helper: install Debian/Ubuntu packages needed to build the PC port on 64-bit Linux.
# Requires sudo. See README.md for manual steps and Fedora notes.
#
# Prefers host gcc/g++ with multilib (-m32). If apt cannot resolve gcc-multilib
# (some images report pkgProblemResolver breaks), falls back to gcc-i686-linux-gnu
# plus the same 32-bit SDL2/OpenGL packages — matching ./build_pc.sh behavior.

set -euo pipefail

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
