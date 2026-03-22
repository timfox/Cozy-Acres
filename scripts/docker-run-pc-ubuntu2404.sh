#!/usr/bin/env bash
# Build and run the 32-bit PC binary inside Ubuntu 24.04 so it loads *container* i386
# GL libraries (not the host's /lib/i386-linux-gnu). You still need host X11 for a window.
#
# Requires: Docker or Podman. For X11 auth (Wayland/Xwayland), a cookie file must be visible
# inside the container — this script mounts ${XAUTHORITY:-~/.Xauthority} when it exists.
# You may still need: xhost +local:root   (or for Podman: xhost +local:)
#
# Note: 24.04's i386 Mesa can still be a recent line (e.g. 25.2.x) with the same class of
# libLLVM issues as newer Ubuntu; if it still SIGSEGVs, try an older LTS snapshot, a VM, or
# track distro fixes — not all "24.04" stacks differ from 25.10.
#
# Usage from repo root:
#   ./scripts/docker-run-pc-ubuntu2404.sh
#   ./scripts/docker-run-pc-ubuntu2404.sh --verbose
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

if ! command -v "$engine" >/dev/null 2>&1; then
    echo "Install Docker or Podman, or set CONTAINER_ENGINE to the binary name." >&2
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
    echo "Warning: no Xauthority file found; set XAUTHORITY or create ~/.Xauthority" >&2
    docker_x11_auth=( -e "XAUTHORITY=${XAUTHORITY:-}" )
fi

tty_flags=(-i)
if [ -t 1 ]; then
    tty_flags+=(-t)
fi

echo "Using $engine image $image (repo -> /work)"
echo "Host X11: $DISPLAY"

exec "$engine" run --rm "${tty_flags[@]}" \
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
dpkg --add-architecture i386
apt-get update -qq
apt-get install -y -qq --no-install-recommends \
    ca-certificates \
    cmake \
    g++-multilib \
    gcc-multilib \
    libc6-dev \
    ninja-build \
    pkg-config \
    libsdl2-dev:i386 \
    libgl1-mesa-dev:i386
export PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig
export PKG_CONFIG_PATH=
cd /work
./build_pc.sh
chown -R ${host_uid}:${host_gid} /work/pc/build32 2>/dev/null || true
exec /work/pc/build32/bin/AnimalCrossing "\$@"
SCRIPT
