#!/usr/bin/env bash
# Try X11 + EGL for GL context (avoids glXChooseVisual). Useful when i386 GLX + libLLVM
# crashes during window creation (e.g. some Ubuntu 25.10 setups). If this fails on AMDGPU,
# use run-pc-linux-safegl.sh instead.
set -euo pipefail
script_dir="$(cd "$(dirname "$0")" && pwd)"
exe="$("$script_dir/pc-linux-resolve-exe.sh")"
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}"
export PC_X11_EGL=1
exec "$exe" "$@"
