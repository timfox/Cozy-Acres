#!/usr/bin/env bash
# Gather versions for a distro bug report when the PC binary crashes in i386 Mesa/LLVM
# (e.g. glXChooseVisual -> libLLVM.so -> ManagedStatic). Paste the output into Launchpad.
#
# Run from the repo: ./scripts/collect-pc-linux-gl-bug-info.sh
# No root required.
set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"
exe="$repo/pc/build32/bin/AnimalCrossing"

echo "=== OS ==="
uname -a 2>/dev/null || true
if command -v lsb_release >/dev/null 2>&1; then
    lsb_release -a 2>/dev/null || true
fi

echo ""
echo "=== dpkg architectures ==="
if command -v dpkg >/dev/null 2>&1; then
    echo "native: $(dpkg --print-architecture 2>/dev/null || echo n/a)"
    echo "foreign: $(dpkg --print-foreign-architectures 2>/dev/null | tr '\n' ' ')"
else
    echo "(dpkg not found)"
fi

echo ""
echo "=== Installed i386 GL / LLVM / SDL (dpkg -l, filtered) ==="
if command -v dpkg >/dev/null 2>&1; then
    dpkg -l 2>/dev/null | grep -E '^ii' | grep i386 | grep -iE 'llvm|mesa|libgl1|libglx|gallium|libsdl2' || echo "(no matching rows)"
else
    echo "(skipped)"
fi

echo ""
echo "=== apt-cache policy (if apt exists; names vary by release) ==="
if command -v apt-cache >/dev/null 2>&1; then
    for p in \
        libllvm20:i386 \
        libllvm20.1:i386 \
        libllvm19:i386 \
        libgl1-mesa-dri:i386 \
        libglx-mesa0:i386 \
        mesa-libgallium:i386 \
        libsdl2-2.0-0:i386; do
        if apt-cache show "$p" >/dev/null 2>&1; then
            echo "--- $p ---"
            apt-cache policy "$p" 2>/dev/null | head -8 || true
        fi
    done
else
    echo "(apt-cache not found)"
fi

echo ""
if [ -x "$exe" ]; then
    echo "=== ldd $exe (filtered) ==="
    ldd "$exe" 2>/dev/null | grep -E 'LLVM|GL|EGL|SDL|drm|X11|xcb|gbm' || ldd "$exe" 2>/dev/null | tail -25
else
    echo "=== ldd (skipped: not executable: $exe) ==="
fi

echo ""
echo "=== gdb one-liner (for your notes) ==="
echo "  ./scripts/gdb-pc-linux.sh --verbose"
echo "  (gdb) run"
echo "  (gdb) bt"
