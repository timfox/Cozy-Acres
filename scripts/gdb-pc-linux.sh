#!/usr/bin/env bash
# Debug the 32-bit PC binary with gdb (64-bit gdb can debug i386 ELFs).
# Usage: ./scripts/gdb-pc-linux.sh [--verbose] [other args passed to the game]
set -euo pipefail
script_dir="$(cd "$(dirname "$0")" && pwd)"
exe="$("$script_dir/pc-linux-resolve-exe.sh")"
cd "$(dirname "$exe")"
echo "Debugging: $exe" >&2
echo "Tip: use this path or this script — not gdb /pc/build32/... (leading /pc is wrong)." >&2
echo "After (gdb) starts: run  — after crash: bt  — quit with q" >&2
echo "Do not type \"gdb /path\" inside gdb (undefined command). To change exe: file /full/path/..." >&2
exec gdb -q \
    -ex "set pagination off" \
    --args "$exe" "$@"
