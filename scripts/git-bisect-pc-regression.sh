#!/usr/bin/env bash
# Git bisect helper for Linux PC regressions ("used to look fine, got worse").
#
#   start [GOOD] [BAD]   Begin bisect (defaults: GOOD=16bc2be BAD=HEAD)
#   log [GOOD] [BAD]     List commits in range (newest first)
#   stats [GOOD] [BAD]   Diffstat for high-risk render/physics paths only
#   status               Show bisect log (or say if none)
#   reset                git bisect reset
#   help                 This summary
#
# After `start`: build, run, then  git bisect good  |  git bisect bad
# Clean rebuild often needed: rm -rf pc/build32 && cmake … && ninja -C pc/build32
#
set -euo pipefail
cd "$(dirname "$0")/.."

RISKY='pc/src/pc_gx_texture.c pc/src/pc_gx.c pc/src/pc_gx_tev.c pc/src/pc_main.c
  pc/src/pc_settings.c pc/src/pc_texture_pack.c pc/src/pc_vi.c
  src/game/m_play.c src/game/m_actor.c src/static/libforest/emu64/emu64.c
  src/system/sys_matrix.c'

def_good=16bc2be
def_bad=HEAD

cmd="${1:-start}"

usage() {
  sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
}

require_ancestor() {
  local GOOD="$1" BAD="$2"
  if ! git merge-base --is-ancestor "$GOOD" "$BAD" 2>/dev/null; then
    echo "error: $GOOD is not an ancestor of $BAD (same branch only)." >&2
    exit 1
  fi
}

bisect_start() {
  local GOOD="${1:-$def_good}"
  local BAD="${2:-$def_bad}"
  require_ancestor "$GOOD" "$BAD"
  echo "Starting bisect: good=$GOOD bad=$BAD"
  echo "About $(git rev-list --count "$GOOD..$BAD") commits in range."
  git bisect start "$BAD" "$GOOD"
  echo ""
  echo "Midpoint: $(git rev-parse --short HEAD) $(git log -1 --oneline)"
  echo "Build, test, then: git bisect good  OR  git bisect bad"
  echo "Done: ./scripts/git-bisect-pc-regression.sh reset"
  echo ""
  echo "tip: ./scripts/git-bisect-pc-regression.sh log"
  echo "tip: ./scripts/git-bisect-pc-regression.sh stats"
}

case "$cmd" in
help|-h|--help)
  usage
  exit 0
  ;;
reset)
  git bisect reset
  exit 0
  ;;
status)
  if [[ -f .git/BISECT_LOG ]]; then
    git bisect log
  else
    echo "No bisect in progress (.git/BISECT_LOG missing)."
  fi
  exit 0
  ;;
log)
  GOOD="${2:-$def_good}"
  BAD="${3:-$def_bad}"
  require_ancestor "$GOOD" "$BAD"
  echo "# $(git rev-list --count "$GOOD..$BAD") commits: $GOOD .. $BAD (newest first)"
  git log --oneline "$GOOD..$BAD"
  ;;
stats)
  GOOD="${2:-$def_good}"
  BAD="${3:-$def_bad}"
  require_ancestor "$GOOD" "$BAD"
  git diff --stat "$GOOD..$BAD" -- $RISKY
  ;;
start)
  shift || true
  bisect_start "${1:-$def_good}" "${2:-$def_bad}"
  ;;
*)
  bisect_start "$cmd" "${2:-$def_bad}"
  ;;
esac
