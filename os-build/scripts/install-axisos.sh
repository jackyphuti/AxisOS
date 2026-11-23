#!/usr/bin/env bash
# ==============================================================================
# AxisOS Calamares Installer Launcher
# Runs Calamares with full root PATH (/usr/sbin:/sbin), display auth & logging
# ==============================================================================

set -u

# Preserve live fstab if present
if [ -f /etc/fstab ] && [ ! -f /etc/fstab.orig.calamares ]; then
    sudo cp -f /etc/fstab /etc/fstab.orig.calamares 2>/dev/null || true
fi

# Full root PATH so unsquashfs, grub-install, parted, mkfs.*, efibootmgr are always found
FULL_PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
export PATH="$FULL_PATH"
export QT_AUTO_SCREEN_SCALE_FACTOR=1

# Allow root user to access current X11 / Xwayland display
if command -v xhost >/dev/null 2>&1; then
    xhost +si:localuser:root >/dev/null 2>&1 || true
fi

# Resolve current Xauthority cleanly (handles both X11 and KWin XWayland)
AUTH_FILE="${XAUTHORITY:-$HOME/.Xauthority}"
DISP="${DISPLAY:-:0}"

# Launch Calamares with full environment and log output for diagnostics
if [ "$(id -u)" -eq 0 ]; then
    env PATH="$FULL_PATH" DISPLAY="$DISP" XAUTHORITY="$AUTH_FILE" QT_AUTO_SCREEN_SCALE_FACTOR=1 \
        calamares -d 2>&1 | tee /tmp/calamares.log
elif sudo -n true 2>/dev/null; then
    sudo env PATH="$FULL_PATH" DISPLAY="$DISP" XAUTHORITY="$AUTH_FILE" \
        XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/0}" \
        QT_AUTO_SCREEN_SCALE_FACTOR=1 \
        calamares -d 2>&1 | tee /tmp/calamares.log
elif command -v pkexec >/dev/null 2>&1; then
    pkexec env PATH="$FULL_PATH" DISPLAY="$DISP" XAUTHORITY="$AUTH_FILE" \
        QT_AUTO_SCREEN_SCALE_FACTOR=1 \
        calamares -d 2>&1 | tee /tmp/calamares.log
else
    sudo env PATH="$FULL_PATH" DISPLAY="$DISP" XAUTHORITY="$AUTH_FILE" \
        QT_AUTO_SCREEN_SCALE_FACTOR=1 \
        calamares -d 2>&1 | tee /tmp/calamares.log
fi

# Revoke elevated display authorization
if command -v xhost >/dev/null 2>&1; then
    xhost -si:localuser:root >/dev/null 2>&1 || true
fi

# Restore fstab
if [ -f /etc/fstab.orig.calamares ]; then
    sudo cp -f /etc/fstab.orig.calamares /etc/fstab 2>/dev/null || true
fi
