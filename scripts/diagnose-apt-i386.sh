#!/usr/bin/env bash
# Why can't apt install libsdl2-2.0-0:i386? Run this and read the output.
set -u

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

echo "=== Ubuntu archive lines (look for [arch=arm64] only, or missing i386) ==="
grep -hE '^deb |^deb-src ' /etc/apt/sources.list /etc/apt/sources.list.d/*.list 2>/dev/null | grep -E 'ubuntu\.com|ubuntu-ports' || true
echo ""

echo "=== APT architecture config (if set, can hide i386) ==="
grep -rhs 'Architectures\|Architecture' /etc/apt/apt.conf /etc/apt/apt.conf.d 2>/dev/null | head -20 || echo '(none found)'
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

echo "If libsdl2-2.0-0:i386 has no Candidate, fix sources: main Ubuntu lines must allow i386, e.g.:"
echo "  deb [arch=amd64,i386] http://archive.ubuntu.com/ubuntu questing main universe"
echo "Do not use only [arch=arm64] or only ubuntu-ports for main packages on an amd64 PC."
echo "Then: sudo dpkg --add-architecture i386 && sudo apt update && sudo apt install libsdl2-2.0-0:i386 libgl1:i386"
