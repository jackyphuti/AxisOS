#!/bin/bash
# AxisOS Background Remote OTA Update Checker & Notifier
# Author: Jacky Mpoka <jackympoka22@gmail.com>
# Checks remote repository for newer packages and presents a desktop notification.

set -euo pipefail

APP_NAME="AxisOS Software"
REPO_URL="https://raw.githubusercontent.com/jackyphuti/AxisOS/gh-pages/apt"
FALLBACK_URL="https://jackyphuti.github.io/AxisOS/apt"
CACHE_DIR="${XDG_CACHE_HOME:-$HOME/.cache}/axisos"
LAST_NOTIFIED_FILE="${CACHE_DIR}/last-update-notified"
FORCE=0

if [ "${1:-}" = "--force" ] || [ "${1:-}" = "-f" ] || [ "${1:-}" = "--check-now" ]; then
    FORCE=1
fi

# Ensure user runtime environment for notifications
if [ -z "${WAYLAND_DISPLAY:-}" ] && [ -z "${DISPLAY:-}" ]; then
    # Look for active GUI session if running without display env
    if [ -n "${XDG_RUNTIME_DIR:-}" ]; then
        if [ -S "${XDG_RUNTIME_DIR}/wayland-0" ]; then
            export WAYLAND_DISPLAY="wayland-0"
        elif [ -S "/tmp/.X11-unix/X0" ]; then
            export DISPLAY=":0"
        fi
    fi
fi

# Determine appropriate icon
ICON="/usr/share/pixmaps/axisos-logo.png"
if [ ! -f "$ICON" ]; then
    ICON="system-software-update"
fi

launch_terminal_upgrade() {
    local cmd="pkexec /usr/local/bin/axis-sysupgrade; echo ''; echo 'Press Enter to close...'; read -r"
    if command -v axis-terminal >/dev/null 2>&1; then
        axis-terminal -e bash -c "$cmd" &
    elif command -v gnome-terminal >/dev/null 2>&1; then
        gnome-terminal -- bash -c "$cmd" &
    elif command -v kgx >/dev/null 2>&1; then
        kgx -e bash -c "$cmd" &
    elif command -v x-terminal-emulator >/dev/null 2>&1; then
        x-terminal-emulator -e bash -c "$cmd" &
    fi
}

# Fetch remote Packages manifest
fetch_manifest() {
    local content=""
    content=$(curl -sSL --connect-timeout 4 --max-time 12 "${REPO_URL}/Packages" 2>/dev/null || true)
    if [ -z "$content" ]; then
        content=$(curl -sSL --connect-timeout 4 --max-time 12 "${FALLBACK_URL}/Packages" 2>/dev/null || true)
    fi
    echo "$content"
}

MANIFEST=$(fetch_manifest)
if [ -z "$MANIFEST" ]; then
    # No network connection or repository unreachable; exit quietly
    exit 0
fi

# Parse remote version for axisos-desktop-suite
REMOTE_VER=$(echo "$MANIFEST" | awk '/^Package: axisos-desktop-suite$/ {flag=1; next} /^Package:/ {flag=0} flag && /^Version:/ {print $2; exit}')
if [ -z "$REMOTE_VER" ]; then
    exit 0
fi

# Determine locally installed version
LOCAL_VER=$(dpkg-query -W -f='${Version}' axisos-desktop-suite 2>/dev/null || true)
if [ -z "$LOCAL_VER" ]; then
    LOCAL_VER="0.0.0"
fi

# Compare versions using dpkg
UPDATE_AVAILABLE=0
if dpkg --compare-versions "$REMOTE_VER" gt "$LOCAL_VER"; then
    UPDATE_AVAILABLE=1
fi

if [ "$UPDATE_AVAILABLE" -eq 0 ]; then
    if [ "$FORCE" -eq 1 ]; then
        if command -v notify-send >/dev/null 2>&1; then
            notify-send \
                --app-name="$APP_NAME" \
                --icon="$ICON" \
                --urgency=low \
                "AxisOS is Up to Date" \
                "Current version: $LOCAL_VER. No updates are required at this time." || true
        fi
    fi
    exit 0
fi

# Throttle notifications: do not spam more than once per 12 hours for the same version
mkdir -p "$CACHE_DIR"
if [ "$FORCE" -eq 0 ] && [ -f "$LAST_NOTIFIED_FILE" ]; then
    read -r CACHED_VER CACHED_TIME < "$LAST_NOTIFIED_FILE" 2>/dev/null || true
    CURRENT_TIME=$(date +%s)
    if [ "$CACHED_VER" = "$REMOTE_VER" ]; then
        ELAPSED=$(( CURRENT_TIME - CACHED_TIME ))
        if [ "$ELAPSED" -lt 43200 ]; then
            # Notified less than 12 hours ago for this version
            exit 0
        fi
    fi
fi

# Record notification timestamp
echo "$REMOTE_VER $(date +%s)" > "$LAST_NOTIFIED_FILE" 2>/dev/null || true

# Display notification to user
if command -v notify-send >/dev/null 2>&1; then
    if notify-send --help 2>&1 | grep -q -- "--action"; then
        ACTION=$(notify-send \
            --app-name="$APP_NAME" \
            --icon="$ICON" \
            --urgency=normal \
            --action="update=Update Now" \
            --action="later=Later" \
            "AxisOS System Update Available" \
            "A new system update ($REMOTE_VER) is ready to install.\nClick 'Update Now' to apply with snapshot rollback protection." 2>/dev/null || true)

        if [ "$ACTION" = "update" ]; then
            launch_terminal_upgrade
        fi
    else
        notify-send \
            --app-name="$APP_NAME" \
            --icon="$ICON" \
            --urgency=normal \
            "AxisOS System Update Available" \
            "A new system update ($REMOTE_VER) is ready to install.\nRun 'axis-sysupgrade' or launch System Update to install." 2>/dev/null || true
    fi
fi
