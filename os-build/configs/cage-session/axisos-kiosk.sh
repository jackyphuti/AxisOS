#!/usr/bin/env bash
# AxisOS Wayland Kiosk Session Launcher
# Runs directly on DRM/KMS hardware via Cage (lightweight Wayland compositor)

export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=AxisOS
export WAYLAND_DISPLAY=wayland-0
export OZONE_PLATFORM=wayland

# If running production build served locally or from file:
if command -v chromium >/dev/null 2>&1; then
    exec cage -- chromium \
        --kiosk \
        --ozone-platform=wayland \
        --enable-features=UseOzonePlatform \
        --app="file:///opt/axisos-shell/index.html" \
        --noerrdialogs \
        --disable-infobars \
        --no-first-run
elif command -v electron >/dev/null 2>&1; then
    exec cage -- electron /opt/axisos-shell/electron/main.cjs --kiosk --ozone-platform=wayland
else
    echo "Error: Neither Chromium nor Electron found in path."
    exit 1
fi
