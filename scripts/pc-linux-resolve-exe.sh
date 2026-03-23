#!/usr/bin/env bash
# Print absolute path to the Linux AnimalCrossing binary for helper scripts.
# Resolution order:
#   1. COZY_PC_EXE if set and executable
#   2. pc/build32/bin/AnimalCrossing (default ./build_pc.sh output)
#   3. pc/build32-native/bin/AnimalCrossing (e.g. manual cmake when build32 is root-owned)
set -euo pipefail
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$script_dir/.." && pwd)"

if [ -n "${COZY_PC_EXE:-}" ]; then
    if [ ! -x "$COZY_PC_EXE" ]; then
        echo "COZY_PC_EXE is set but not an executable file: $COZY_PC_EXE" >&2
        exit 1
    fi
    printf '%s\n' "$COZY_PC_EXE"
    exit 0
fi

for cand in \
    "$repo/pc/build32/bin/AnimalCrossing" \
    "$repo/pc/build32-native/bin/AnimalCrossing"
do
    if [ -x "$cand" ]; then
        printf '%s\n' "$cand"
        exit 0
    fi
done

echo "No AnimalCrossing binary found. Run ./build_pc.sh from the repo root, or set" >&2
echo "COZY_PC_EXE=/path/to/AnimalCrossing" >&2
exit 1
