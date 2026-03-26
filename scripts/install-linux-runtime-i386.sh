#!/usr/bin/env bash
# Install 32-bit SDL2 + OpenGL libraries so pc/build32/bin/AnimalCrossing can start.
# Only works on x86_64 Debian/Ubuntu (multiarch i386). On ARM64, use an x86_64 PC or emulation.

set -euo pipefail

case "$(uname -m)" in
    aarch64|armv8*|armv7l|armv6l|riscv64|ppc64le)
        echo "This host is $(uname -m). AnimalCrossing is a 32-bit x86 program."
        echo "Installing libsdl2-2.0-0:i386 here will not help — you need the x86 .so files."
        echo "Run the game on a real x86_64 Linux machine (install this script there), or use unsupported qemu-i386/box86."
        exit 1
        ;;
esac

d="$(dpkg --print-architecture 2>/dev/null || true)"
case "$d" in
    arm64|armhf|armel|riscv64|ppc64el|s390x)
        echo "dpkg architecture is $d — same as above: run on x86_64 Linux."
        exit 1
        ;;
esac

sudo dpkg --add-architecture i386 2>/dev/null || true
sudo apt-get update

if ! apt-cache show libsdl2-2.0-0:i386 >/dev/null 2>&1; then
    echo ""
    echo "apt cannot see libsdl2-2.0-0:i386 (no i386 index for Ubuntu main)."
    echo "On Ubuntu 22.04+, DEB822 .sources often omit i386 from Architectures:."
    echo "Fix:  sudo ./scripts/fix-ubuntu-apt-i386.sh"
    echo "Then: sudo apt install libsdl2-2.0-0:i386 libgl1:i386"
    echo "Details: ./scripts/diagnose-apt-i386.sh"
    exit 1
fi

sudo apt-get install -y libsdl2-2.0-0:i386 libgl1:i386 libcurl4:i386

echo "Done. From pc/build32/bin run: ./AnimalCrossing"
