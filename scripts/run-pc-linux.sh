#!/usr/bin/env bash
# Run the 32-bit PC build on Linux with stable defaults: X11 (not Wayland), Mesa GLX,
# no prime offload, system i386 libs before /opt/amdgpu. Use this after removing
# amdgpu-lib32 if you want GPU-accelerated Mesa (not llvmpipe).
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"
exe="$repo/pc/build32/bin/AnimalCrossing"
if [ ! -x "$exe" ]; then
    echo "Missing or non-executable: $exe" >&2
    echo "Build with ./build_pc.sh. If pc/build32 is root-owned from Docker: sudo rm -rf pc/build32 && ./build_pc.sh" >&2
    exit 1
fi
export SDL_VIDEODRIVER=x11
export DRI_PRIME=0
export __NV_PRIME_RENDER_OFFLOAD=0
export __GLX_VENDOR_LIBRARY_NAME=mesa
export LD_LIBRARY_PATH="/lib/i386-linux-gnu:/usr/lib/i386-linux-gnu"
exec "$exe" "$@"
