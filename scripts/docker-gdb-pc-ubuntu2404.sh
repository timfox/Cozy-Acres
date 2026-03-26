#!/usr/bin/env bash
# Run AnimalCrossing under gdb inside Ubuntu 24.04 (same stack idea as
# docker-run-pc-ubuntu2404.sh) and print backtraces after SIGSEGV — for Mesa/i386
# LLVM bug reports (Launchpad, etc.).
#
# Requires: Docker or Podman, host DISPLAY + X11 socket (see docker-run-pc-ubuntu2404.sh).
#
# Usage from repo root:
#   ./scripts/docker-gdb-pc-ubuntu2404.sh
#   ./scripts/docker-gdb-pc-ubuntu2404.sh --verbose
#   ./scripts/docker-gdb-pc-ubuntu2404.sh --build --verbose   # build in container first
#
# Blocks until gdb finishes (after apt setup on first run). Run in its own shell invocation.
#
# For symbols in the game binary (not Mesa), rebuild on the host with:
#   COZY_PC_GDB_SYMBOLS=1 ./build_pc.sh
#
# Environment:
#   CONTAINER_ENGINE=docker   (default) or podman
#   UBUNTU_IMAGE=ubuntu:24.04
set -euo pipefail

repo="$(cd "$(dirname "$0")/.." && pwd)"
engine="${CONTAINER_ENGINE:-docker}"
image="${UBUNTU_IMAGE:-ubuntu:24.04}"
host_uid="$(id -u)"
host_gid="$(id -g)"

build_first=0
if [ "${1:-}" = "--build" ]; then
    build_first=1
    shift
fi

if [ "${1:-}" = "-h" ] || [ "${1:-}" = "--help" ]; then
    sed -n '2,20p' "$0" | sed 's/^# \{0,1\}//'
    exit 0
fi

if ! command -v "$engine" >/dev/null 2>&1; then
    echo "Install Docker or Podman, or set CONTAINER_ENGINE." >&2
    exit 1
fi

if [ -z "${DISPLAY:-}" ]; then
    echo "DISPLAY is not set; start an X11 session or export DISPLAY=:0" >&2
    exit 1
fi

xauth_file="${XAUTHORITY:-}"
if [ -z "$xauth_file" ] || [ ! -f "$xauth_file" ]; then
    if [ -f "${HOME}/.Xauthority" ]; then
        xauth_file="${HOME}/.Xauthority"
    fi
fi

docker_x11_auth=()
if [ -n "$xauth_file" ] && [ -f "$xauth_file" ]; then
    docker_x11_auth=( -v "$xauth_file:/tmp/host-xauth:ro" -e XAUTHORITY=/tmp/host-xauth )
    echo "X11 auth: mount $xauth_file -> /tmp/host-xauth"
else
    echo "Warning: no Xauthority file found; set XAUTHORITY if gdb run cannot open DISPLAY" >&2
    docker_x11_auth=( -e "XAUTHORITY=${XAUTHORITY:-}" )
fi

# No -t: docker's stdin is this heredoc (pipe), not a TTY (see docker-run-pc-ubuntu2404.sh).

echo "Using $engine image $image (repo -> /work)"
echo "Host X11: $DISPLAY"
echo "Starting container — first run may pull the image; apt can take a few minutes."
echo "=== gdb batch (all threads) ==="

exec "$engine" run --rm -i \
    --network host \
    -e "DISPLAY=$DISPLAY" \
    "${docker_x11_auth[@]}" \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    -v "$repo:/work" \
    -w /work \
    "$image" \
    bash -s -- "$@" <<SCRIPT
set -euo pipefail
export DEBIAN_FRONTEND=noninteractive
echo "=== [cozy-pc docker] dpkg --add-architecture i386 ==="
dpkg --add-architecture i386
echo "=== [cozy-pc docker] apt-get update ==="
apt-get update

if [ "${build_first}" = 1 ]; then
    echo "=== [cozy-pc docker] apt-get install build dependencies (--build) ==="
    apt-get install -y -qq --no-install-recommends \
        ca-certificates \
        cmake \
        g++-multilib \
        gcc-multilib \
        libc6-dev \
        ninja-build \
        pkg-config \
        libsdl2-dev:i386 \
        libgl1-mesa-dev:i386 \
        libcurl4-openssl-dev:i386
    export PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig
    export PKG_CONFIG_PATH=
    cd /work
    ./build_pc.sh
    chown -R ${host_uid}:${host_gid} /work/pc/build32 2>/dev/null || true
fi

echo "=== [cozy-pc docker] apt-get install gdb + i386 GL runtime ==="
apt-get install -y -qq --no-install-recommends \
    ca-certificates \
    gdb \
    libc6:i386 \
    libgcc-s1:i386 \
    libstdc++6:i386 \
    libsdl2-2.0-0:i386 \
    libgl1:i386 \
    libgl1-mesa-dri:i386 \
    libglx-mesa0:i386

exe=/work/pc/build32/bin/AnimalCrossing
if [ ! -x "\$exe" ]; then
    echo "No executable: \$exe" >&2
    echo "Build first: ./scripts/docker-run-pc-ubuntu2404.sh  or  ./scripts/docker-gdb-pc-ubuntu2404.sh --build" >&2
    exit 1
fi

export SDL_VIDEODRIVER=x11
cd /work/pc/build32/bin
echo "=== [cozy-pc docker] gdb run (backtrace after SIGSEGV) ==="
gdb -q --batch \
    -ex 'set pagination off' \
    -ex 'set confirm off' \
    -ex run \
    -ex 'thread apply all bt' \
    -ex quit \
    --args "\$exe" "\$@"
SCRIPT
