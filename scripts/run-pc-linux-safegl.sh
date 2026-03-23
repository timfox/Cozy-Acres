#!/usr/bin/env bash
# 32-bit GLX on machines with AMDGPU's /opt/amdgpu i386 stack can segfault inside
# libLLVM.so.*amdgpu during dlopen (see glXChooseVisual -> libGLX). LIBGL_ALWAYS_SOFTWARE
# alone does not help because that stack is still loaded first.
#
# Also: pc_main used to set DRI_PRIME=1 on every Linux run; that can force the AMDGPU
# GLX path. Rebuild after the pc_main.c fix, or export DRI_PRIME=0 below (overrides
# old binaries because setenv(..., 0) does not replace an existing variable).
#
# This wrapper: X11 video (Wayland + i386 SDL is often broken), Mesa GLX, llvmpipe,
# system i386 LD path before /opt/amdgpu. We do NOT set SDL_VIDEO_X11_FORCE_EGL:
# forced EGL on i386 has crashed some setups; classic GLX is fine once AMDGPU i386 DRI is gone.
set -euo pipefail
script_dir="$(cd "$(dirname "$0")" && pwd)"
exe="$("$script_dir/pc-linux-resolve-exe.sh")"
cd "$(dirname "$exe")"
export SDL_VIDEODRIVER=x11
unset SDL_VIDEO_X11_FORCE_EGL 2>/dev/null || true
export DRI_PRIME=0
export __NV_PRIME_RENDER_OFFLOAD=0
export __GLX_VENDOR_LIBRARY_NAME=mesa
export LIBGL_ALWAYS_SOFTWARE=1
export GALLIUM_DRIVER=llvmpipe
export MESA_LOADER_DRIVER_OVERRIDE=llvmpipe
export LIBGL_DRIVERS_PATH=/usr/lib/i386-linux-gnu/dri
export LD_LIBRARY_PATH="/lib/i386-linux-gnu:/usr/lib/i386-linux-gnu"
exec "$exe" "$@"
