#!/usr/bin/env bash
# Convenience launcher: starts yadr-server with the bundled frontend.
# Extra CLI args are forwarded (e.g. --port 9000, --bind 0.0.0.0).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build/backend/yadr-server"
[[ -x "$BIN" ]] || BIN="${ROOT}/build/yadr-server"
WEB="$ROOT/build/web"

if [[ ! -x "$BIN" ]]; then
    echo "yadr-server not built. Run ./scripts/build.sh first." >&2
    exit 1
fi

exec "$BIN" --web-root "$WEB" "$@"
