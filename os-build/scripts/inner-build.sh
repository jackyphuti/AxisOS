#!/usr/bin/env bash
# ==============================================================================
# AxisOS Linux - Live-Build Environment & Hybrid ISO Generator (Pure C)
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="${WORKSPACE_DIR:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
CHROOT_CONFIG_DIR="$WORKSPACE_DIR/os-build/configs"
SCRIPTS_DIR="$WORKSPACE_DIR/os-build/scripts"
BUILD_DIR="${BUILD_DIR:-$WORKSPACE_DIR/os-build/work}"

echo "=================================================="
echo "   AxisOS Chroot & Bootable ISO Generator (Pure C)"
echo "=================================================="
echo "Workspace: $WORKSPACE_DIR"
echo "Build Dir: $BUILD_DIR"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 1. Setup live-build base configuration
if [ ! -d "$BUILD_DIR/auto" ]; then
    mkdir -p "$BUILD_DIR/auto"
fi

# Clean previous build artifacts
if command -v lb >/dev/null 2>&1; then
    lb clean --purge || true
fi

mkdir -p "$BUILD_DIR/config/package-lists"
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/local/bin"
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/applications"
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/skel/.config"

# 2. Copy Package Lists
if [ -d "$CHROOT_CONFIG_DIR/package-lists" ]; then
    cp -r "$CHROOT_CONFIG_DIR"/package-lists/*.list.chroot "$BUILD_DIR/config/package-lists/" 2>/dev/null || true
fi
if [ -d "$CHROOT_CONFIG_DIR/live-build/package-lists" ]; then
    cp -r "$CHROOT_CONFIG_DIR"/live-build/package-lists/*.list.chroot "$BUILD_DIR/config/package-lists/" 2>/dev/null || true
fi

# 3. Copy Precompiled Pure C Applications & Shell
echo "--> Staging Pure C Applications into chroot filesystem..."
for app_dir in "$WORKSPACE_DIR"/os-build/apps/*; do
    if [ -d "$app_dir" ]; then
        app_name="$(basename "$app_dir")"
        if [ -x "$app_dir/$app_name" ]; then
            cp -f "$app_dir/$app_name" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
        fi
        if [ -f "$app_dir/$app_name.desktop" ]; then
            cp -f "$app_dir/$app_name.desktop" "$BUILD_DIR/config/includes.chroot/usr/share/applications/"
        fi
    fi
done

if [ -x "$WORKSPACE_DIR/os-build/apps/axis-settings/axisos-settings-panel" ]; then
    cp -f "$WORKSPACE_DIR/os-build/apps/axis-settings/axisos-settings-panel" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
fi

# 4. Copy Desktop Shell (shell-c)
if [ -x "$WORKSPACE_DIR/shell-c/axisos-shell" ]; then
    cp -f "$WORKSPACE_DIR/shell-c/axisos-shell" "$BUILD_DIR/config/includes.chroot/usr/local/bin/axis-shell"
    ln -sf axis-shell "$BUILD_DIR/config/includes.chroot/usr/local/bin/LiquidNodesOS" || true
fi

# 5. Copy Updater and Package Manager
if [ -x "$WORKSPACE_DIR/os-build/updater/axis-update" ]; then
    cp -f "$WORKSPACE_DIR/os-build/updater/axis-update" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
fi
if [ -x "$WORKSPACE_DIR/pkg-mgr/bin/axis" ]; then
    cp -f "$WORKSPACE_DIR/pkg-mgr/bin/axis" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
fi

# 6. Copy Wayland Kiosk Session configuration
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/wayland-sessions"
if [ -f "$CHROOT_CONFIG_DIR/wayland/liquidnodes.desktop" ]; then
    cp -f "$CHROOT_CONFIG_DIR/wayland/liquidnodes.desktop" "$BUILD_DIR/config/includes.chroot/usr/share/wayland-sessions/"
fi
if [ -f "$CHROOT_CONFIG_DIR/cage-session/axisos-kiosk.sh" ]; then
    cp -f "$CHROOT_CONFIG_DIR/cage-session/axisos-kiosk.sh" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
    chmod +x "$BUILD_DIR/config/includes.chroot/usr/local/bin/axisos-kiosk.sh"
fi

# 7. Run live-build config & build
if command -v lb >/dev/null 2>&1; then
    lb config \
        --distribution bookworm \
        --architecture amd64 \
        --archive-areas "main contrib non-free non-free-firmware" \
        --linux-flavours amd64 \
        --binary-images iso-hybrid \
        --bootloader syslinux \
        --iso-application "AxisOS Pure C Linux" \
        --iso-publisher "AxisOS Team" \
        --iso-volume "AxisOS_Live" \
        --memtest none \
        --verbose

    lb build 2>&1 | tee "$BUILD_DIR/build.log"
    cp -f "$BUILD_DIR"/live-image-amd64.hybrid.iso "$WORKSPACE_DIR/axisos-live-amd64.iso" 2>/dev/null || true
fi

echo "=================================================="
echo "   AxisOS Chroot & Build Pipeline Completed!      "
echo "=================================================="
