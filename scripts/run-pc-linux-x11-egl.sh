#!/usr/bin/env bash
# Try X11 + EGL for GL context (avoids glXChooseVisual). Useful when i386 GLX + libLLVM
# crashes during window creation (e.g. some Ubuntu 25.10 setups). If this fails on AMDGPU,
# use run-pc-linux-safegl.sh instead.
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"
exe="$repo/pc/build32/bin/AnimalCrossing"
if [ ! -x "$exe" ]; then
    echo "Missing or non-executable: $exe" >&2
    echo "Build with ./build_pc.sh." >&2
    exit 1
fi
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}"
export PC_X11_EGL=1
exec "$exe" "$@"
