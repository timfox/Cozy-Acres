#!/usr/bin/env bash
# Run the PC port on Linux from the repo root: correct working directory (rom/, shaders/),
# stable X11 env, and a hint if the binary is still PIE (rebuild with ./build_pc.sh for -no-pie).
#
# Usage (from cozyacres repo root):
#   ./scripts/play-native-linux.sh
#   ./scripts/play-native-linux.sh --verbose
#
# Prereqs: ./build_pc.sh ; disc image in pc/build32/bin/rom/
set -euo pipefail
script_dir="$(cd "$(dirname "$0")" && pwd)"
exe="$("$script_dir/pc-linux-resolve-exe.sh")"
if file "$exe" 2>/dev/null | grep -q 'pie executable'; then
    echo "This binary is PIE. For Ubuntu 25.x + Mesa + AMD/Intel, pull latest sources and run ./build_pc.sh" >&2
    echo "(non-PIE link avoids many libLLVM crashes during OpenGL init)." >&2
fi
exec "$script_dir/run-pc-linux.sh" "$@"
