#!/usr/bin/env bash
# ==============================================================================
# AxisOS Calamares Installer Launcher
# Runs Calamares with proper xhost permissions, Qt scaling, and root elevation
# ==============================================================================

set -euo pipefail

# Preserve live fstab if present
if [ -f /etc/fstab ] && [ ! -f /etc/fstab.orig.calamares ]; then
    sudo cp -f /etc/fstab /etc/fstab.orig.calamares 2>/dev/null || true
fi

# High-DPI UI scaling for modern displays
export QT_AUTO_SCREEN_SCALE_FACTOR=1

# Allow root user to access current X11 / Xwayland display
if command -v xhost >/dev/null 2>&1; then
    xhost +si:localuser:root >/dev/null 2>&1 || true
fi

# Launch Calamares via pkexec or sudo
if [ "$(id -u)" -eq 0 ]; then
    calamares -d 2>&1 | tee /tmp/calamares.log || calamares
else
    if command -v pkexec >/dev/null 2>&1; then
        pkexec env DISPLAY="${DISPLAY:-:0}" XAUTHORITY="${XAUTHORITY:-$HOME/.Xauthority}" calamares -d 2>&1 | tee /tmp/calamares.log || pkexec calamares
    else
        sudo calamares -d 2>&1 | tee /tmp/calamares.log || sudo calamares
    fi
fi

# Revoke elevated display authorization
if command -v xhost >/dev/null 2>&1; then
    xhost -si:localuser:root >/dev/null 2>&1 || true
fi

# Restore fstab
if [ -f /etc/fstab.orig.calamares ]; then
    sudo cp -f /etc/fstab.orig.calamares /etc/fstab 2>/dev/null || true
fi
