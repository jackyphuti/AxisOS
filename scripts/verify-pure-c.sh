#!/usr/bin/env bash
# ==============================================================================
# AxisOS Pure C11 Strict Linkage Verification Runner
# ==============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

echo "=== Verifying Pure C Strict Linkage Across All Binaries ==="

# Compile verification tool if binary does not exist or if source is newer
if [ ! -x "$ROOT_DIR/scripts/verify-pure-c" ] || [ "$ROOT_DIR/scripts/verify-pure-c.c" -nt "$ROOT_DIR/scripts/verify-pure-c" ]; then
    gcc -std=gnu11 -O2 "$ROOT_DIR/scripts/verify-pure-c.c" -o "$ROOT_DIR/scripts/verify-pure-c"
fi

exec "$ROOT_DIR/scripts/verify-pure-c"
