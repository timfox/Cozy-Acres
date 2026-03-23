#!/usr/bin/env bash
# Run the 32-bit PC build on Linux with stable defaults: X11 (not Wayland), Mesa GLX,
# no prime offload, system i386 libs before /opt/amdgpu. Use this after removing
# amdgpu-lib32 if you want GPU-accelerated Mesa (not llvmpipe).
#
# This still uses the *host* i386 Mesa/LLVM stack. If you SIGSEGV right after
# "[Keybindings] Loaded" (fault in libLLVM.so / glXChooseVisual), rebuild with current
# ./build_pc.sh (-no-pie) or see README "Segmentation fault".
#
# We do not set __GLX_VENDOR_LIBRARY_NAME here so the game can pick NVIDIA i386 GLX when
# installed; use export __GLX_VENDOR_LIBRARY_NAME=mesa to force Mesa on hybrid systems.
set -euo pipefail
script_dir="$(cd "$(dirname "$0")" && pwd)"
exe="$("$script_dir/pc-linux-resolve-exe.sh")"
cd "$(dirname "$exe")"
export SDL_VIDEODRIVER=x11
export DRI_PRIME=0
export __NV_PRIME_RENDER_OFFLOAD=0
export LD_LIBRARY_PATH="/lib/i386-linux-gnu:/usr/lib/i386-linux-gnu"
exec "$exe" "$@"
