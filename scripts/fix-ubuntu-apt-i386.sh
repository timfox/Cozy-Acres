#!/usr/bin/env bash
# Ubuntu 22.04+ often uses .deb822 sources (*.sources) with:
#   Architectures: amd64
# or
#   Architectures: amd64 arm64
# without i386. Then apt never downloads binary-i386/Packages — every :i386 install fails.
# This script adds i386 to those lines for official Ubuntu URIs only (archive / security).
#
# Run: sudo ./scripts/fix-ubuntu-apt-i386.sh
# Then: sudo apt update && sudo apt install libsdl2-2.0-0:i386 libgl1:i386

set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root: sudo $0"
    exit 1
fi

shopt -s nullglob
patched=0
for f in /etc/apt/sources.list.d/*.sources /etc/apt/sources.list; do
    [ -f "$f" ] || continue
    grep -qE 'archive\.ubuntu\.com|security\.ubuntu\.com' "$f" || continue
    if ! grep -q '^Architectures:' "$f"; then
        continue
    fi
    if grep -q '^Architectures:.*\bi386\b' "$f"; then
        echo "Already has i386: $f"
        continue
    fi
    echo "Patching: $f"
    cp -a "$f" "${f}.bak-before-i386-$(date +%Y%m%d%H%M%S)"
    sed -i \
        -e 's/^Architectures: amd64$/Architectures: amd64 i386/' \
        -e 's/^Architectures: amd64 arm64$/Architectures: amd64 i386 arm64/' \
        -e 's/^Architectures: arm64 amd64$/Architectures: amd64 i386 arm64/' \
        "$f"
    patched=1
done

if [ "$patched" -eq 0 ]; then
    echo "No matching .sources needed changes (or i386 already listed)."
    echo "If :i386 is still missing, run: apt-config dump | grep APT::Architectures"
    echo "If that lists only amd64/arm64, add i386, e.g. create /etc/apt/apt.conf.d/52i386.conf with:"
    echo '  APT::Architectures { "amd64"; "i386"; "arm64"; };'
fi

apt-get update
echo ""
echo "Next:"
echo "  apt-cache policy libsdl2-2.0-0:i386"
echo "  apt install -y libsdl2-2.0-0:i386 libgl1:i386"
