#!/usr/bin/env bash
# Build this project the supported way for your CPU:
#   x86_64 / i686 → native ./build_pc.sh (after install-linux-pc-deps.sh)
#   ARM / other   → ./scripts/docker-build-linux-amd64.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"
case "$(uname -m)" in
    x86_64|i686|i386)
        exec ./build_pc.sh
        ;;
    *)
        echo "Non-x86 host ($(uname -m)): building inside linux/amd64 via Docker/Podman..."
        exec ./scripts/docker-build-linux-amd64.sh
        ;;
esac
