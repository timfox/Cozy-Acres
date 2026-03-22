#!/usr/bin/env bash
# Why can't apt install libsdl2-2.0-0:i386? Run this and read the output.
set -uo pipefail

echo "=== CPU / dpkg ==="
echo "uname -m:                 $(uname -m)"
echo "dpkg --print-architecture: $(dpkg --print-architecture 2>/dev/null || echo 'n/a')"
echo "foreign architectures:     $(dpkg --print-foreign-architectures 2>/dev/null | tr '\n' ' ')"
echo ""

echo "=== apt candidate for 32-bit SDL2 (what install needs) ==="
apt-cache policy libsdl2-2.0-0:i386 2>/dev/null || true
echo ""

echo "=== Same for native (sanity check — should exist on any Ubuntu desktop) ==="
apt-cache policy libsdl2-2.0-0:"$(dpkg --print-architecture 2>/dev/null || echo amd64)" 2>/dev/null | head -15
echo ""

echo "=== Legacy deb lines (often empty on Ubuntu 22.04+) ==="
grep -hE '^deb |^deb-src ' /etc/apt/sources.list /etc/apt/sources.list.d/*.list 2>/dev/null | grep -E 'ubuntu\.com|ubuntu-ports' || true
echo ""

echo "=== DEB822 .sources Architectures= (must include i386 for :i386 packages) ==="
shopt -s nullglob
for sf in /etc/apt/sources.list.d/*.sources; do
    [ -f "$sf" ] || continue
    grep -qE 'archive\.ubuntu\.com|security\.ubuntu\.com' "$sf" || continue
    echo "--- $sf ---"
    grep -E '^URIs:|^Suites:|^Architectures:' "$sf" || true
done
echo ""

echo "=== apt-config APT::Architectures (if set without i386, apt ignores dpkg foreign i386) ==="
apt-config dump 2>/dev/null | grep '^APT::Architectures' || echo '(default: follow dpkg foreign archs)'
echo ""

echo "=== APT architecture snippets in apt.conf.d ==="
grep -rhs 'Architectures' /etc/apt/apt.conf /etc/apt/apt.conf.d/* 2>/dev/null | head -15 || true
echo ""

echo "--- Interpretation ---"
NATIVE="$(dpkg --print-architecture 2>/dev/null || true)"
if [ "$NATIVE" = "arm64" ] || [ "$(uname -m)" = "aarch64" ]; then
    echo "You are on ARM64. Ubuntu does not provide usable :i386 multiarch here like on an Intel/AMD PC."
    echo "Run AnimalCrossing on x86_64 Linux, or build with ./scripts/docker-build-linux-amd64.sh and run the binary there."
    exit 0
fi

if [ "$NATIVE" != "amd64" ]; then
    echo "Native dpkg arch is $NATIVE (not amd64). i386 game libs expect an amd64 Ubuntu install."
    exit 0
fi

echo "If libsdl2-2.0-0:i386 has no Candidate on amd64:"
echo "  1) Ubuntu .sources often have 'Architectures: amd64' or 'amd64 arm64' WITHOUT i386."
echo "     Fix: sudo ./scripts/fix-ubuntu-apt-i386.sh"
echo "  2) Legacy deb lines need [arch=amd64,i386] on archive.ubuntu.com entries."
echo "Then: sudo apt update && sudo apt install libsdl2-2.0-0:i386 libgl1:i386"
