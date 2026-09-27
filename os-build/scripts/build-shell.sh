#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SHELL_DIR="$SCRIPT_DIR/../../shell"

echo "=== Building AxisOS Shell ==="
cd "$SHELL_DIR"
npm run build

echo "=== AxisOS Shell built successfully in $SHELL_DIR/dist ==="
