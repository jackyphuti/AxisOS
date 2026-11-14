#!/usr/bin/env bash
# ==============================================================================
# AxisOS Bootable Hybrid ISO Generator Entrypoint
# ==============================================================================
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec bash "$SCRIPT_DIR/os-build/scripts/build-iso.sh" "$@"
