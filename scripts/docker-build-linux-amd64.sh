#!/usr/bin/env bash
# Build the PC port inside a linux/amd64 container (x86_64 userspace).
# Use this on ARM64 hosts (Apple Silicon, Ampere, Raspberry Pi 64-bit, etc.) where
# apt cannot install libsdl2-dev:i386 on the host — those packages target x86 multiarch.
#
# Requires Docker or Podman with binfmt/QEMU for foreign platforms (Docker Desktop
# and most Podman setups on ARM provide this).
#
# The resulting binary is 32-bit x86 (i686). Run it on x86_64 Linux, or use qemu-user
# (e.g. qemu-i386) on ARM if you only need to test execution.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

ENGINE=""
if command -v docker >/dev/null 2>&1; then
    ENGINE=docker
elif command -v podman >/dev/null 2>&1; then
    ENGINE=podman
else
    echo "Install Docker or Podman, then re-run this script."
    exit 1
fi

echo "Using $ENGINE with --platform linux/amd64 ..."
# No -t: works from CI and non-TTY environments (avoid "input device is not a TTY").
exec "$ENGINE" run --rm --platform linux/amd64 \
    -v "$ROOT:/src" \
    -w /src \
    -e COZY_BUILD_UID="$(id -u)" \
    -e COZY_BUILD_GID="$(id -g)" \
    ubuntu:24.04 \
    bash -c '
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq dpkg-dev
dpkg --add-architecture i386
apt-get update -qq
apt-get install -y -qq \
  build-essential cmake ninja-build pkg-config \
  gcc-i686-linux-gnu g++-i686-linux-gnu \
  libsdl2-dev:i386 libgl1-mesa-dev:i386 libcurl4-openssl-dev:i386
./build_pc.sh
if [ -d /src/pc/build32 ]; then
  chown -R "${COZY_BUILD_UID}:${COZY_BUILD_GID}" /src/pc/build32
fi
echo ""
echo "=== Container build finished ==="
echo "Output: pc/build32/bin/AnimalCrossing (32-bit x86 ELF)"
echo "Copy the repo or just that binary to an x86_64 Linux machine to run, or use qemu-i386."
'
