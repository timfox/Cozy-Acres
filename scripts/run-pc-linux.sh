#!/usr/bin/env bash
# Run the 32-bit PC build on Linux with stable defaults: X11 (not Wayland), Mesa GLX,
# no prime offload, system i386 libs before /opt/amdgpu. Use this after removing
# amdgpu-lib32 if you want GPU-accelerated Mesa (not llvmpipe).
#
# This still uses the *host* i386 Mesa/LLVM stack. If you SIGSEGV right after
# "[Keybindings] Loaded" (fault in libLLVM.so / glXChooseVisual), this script will not
# fix it — try ./scripts/docker-run-pc-ubuntu2404.sh or README "Segmentation fault".
set -euo pipefail
script_dir="$(cd "$(dirname "$0")" && pwd)"
exe="$("$script_dir/pc-linux-resolve-exe.sh")"
export SDL_VIDEODRIVER=x11
export DRI_PRIME=0
export __NV_PRIME_RENDER_OFFLOAD=0
export __GLX_VENDOR_LIBRARY_NAME=mesa
export LD_LIBRARY_PATH="/lib/i386-linux-gnu:/usr/lib/i386-linux-gnu"
exec "$exe" "$@"
