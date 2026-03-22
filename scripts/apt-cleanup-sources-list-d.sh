#!/usr/bin/env bash
# Move stray files out of /etc/apt/sources.list.d/ that trigger apt warnings, e.g.:
#   Ignoring file 'ubuntu.sources.bak-before-i386-...' ... invalid filename extension
#
# Default (safe): only ubuntu DEB822 / i386-fix leftovers:
#   *before-i386*, *.sources.bak, *.sources.bak.*
#
# Optional --all-bak: also move *.list.bak, *~, *.old, *.dpkg-old, *.dpkg-dist.
# Restores: copy or mv files back from /var/backups/apt-sources/ if needed.
#
# Run: sudo ./scripts/apt-cleanup-sources-list-d.sh
#      sudo ./scripts/apt-cleanup-sources-list-d.sh --all-bak
set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "Run as root: sudo $0 [--all-bak]"
    exit 1
fi

all_bak=0
if [ "${1:-}" = "--all-bak" ]; then
    all_bak=1
elif [ -n "${1:-}" ]; then
    echo "Usage: sudo $0 [--all-bak]" >&2
    exit 1
fi

dest=/var/backups/apt-sources
mkdir -p "$dest"
shopt -s nullglob
moved=0

move_if_file() {
    local f="$1"
    [ -f "$f" ] || return 0
    echo "Moving: $f -> $dest/"
    mv -n "$f" "$dest/"
    moved=1
}

if [ "$all_bak" -eq 1 ]; then
    echo "Mode: --all-bak (includes *.list.bak and editor temp files)"
    for f in /etc/apt/sources.list.d/*; do
        base="$(basename "$f")"
        case "$base" in
            *.bak*|*~|*.old|*.dpkg-old|*.dpkg-dist)
                move_if_file "$f"
                ;;
        esac
    done
else
    echo "Mode: default (only ubuntu .sources backups / *before-i386*)"
    for f in /etc/apt/sources.list.d/*; do
        base="$(basename "$f")"
        case "$base" in
            *before-i386*)
                move_if_file "$f"
                ;;
            *.sources.bak)
                move_if_file "$f"
                ;;
            *.sources.bak.*)
                move_if_file "$f"
                ;;
        esac
    done
fi

if [ "$moved" -eq 0 ]; then
    echo "No matching files in /etc/apt/sources.list.d/"
else
    echo "Done. Run: apt update"
    echo "Restored copies are under: $dest"
fi
