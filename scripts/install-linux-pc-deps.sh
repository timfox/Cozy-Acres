#!/bin/bash
# Optional helper: install Debian/Ubuntu packages needed to build the PC port on 64-bit Linux.
# Requires sudo. See README.md for manual steps and Fedora notes.

set -euo pipefail

sudo dpkg --add-architecture i386 2>/dev/null || true
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  pkg-config \
  gcc-multilib \
  g++-multilib \
  libsdl2-dev:i386 \
  libgl1-mesa-dev:i386

echo "Dependencies installed. From the repo root run: ./build_pc.sh"
