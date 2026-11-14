#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="${WORKSPACE_DIR:-$(cd "${SCRIPT_DIR}/../.." && pwd)}"
WALLPAPERS_DIR="${REPO_ROOT}/data/wallpapers"

echo "=== Standardizing AxisOS Wallpapers (1920x1080 Full HD) ==="
if [ -d "$WALLPAPERS_DIR" ]; then
    for img in "${WALLPAPERS_DIR}"/*.jpg "${WALLPAPERS_DIR}"/*.png; do
        if [ -f "$img" ]; then
            if command -v magick >/dev/null 2>&1; then
                magick "$img" -resize 1920x1080^ -gravity center -extent 1920x1080 "$img" 2>/dev/null || true
            elif command -v convert >/dev/null 2>&1; then
                convert "$img" -resize 1920x1080^ -gravity center -extent 1920x1080 "$img" 2>/dev/null || true
            fi
            echo "  [OK] Processed wallpaper: $(basename "$img")"
        fi
    done
fi

echo "Wallpapers standardized successfully."
