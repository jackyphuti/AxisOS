#!/usr/bin/env bash
# ==============================================================================
# AxisOS Wayland Kiosk Session Launcher
# Runs directly on DRM/KMS hardware via Cage (lightweight Wayland compositor)
# ==============================================================================

export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=AxisOS
export WAYLAND_DISPLAY=wayland-0
export OZONE_PLATFORM=wayland
export MOZ_ENABLE_WAYLAND=1
export GDK_BACKEND=wayland
export QT_QPA_PLATFORM=wayland

# Wait briefly for axisos-daemon to be available
URL="file:///opt/axisos-shell/index.html"
for i in {1..20}; do
    if curl -s -f http://127.0.0.1:3000/api/system-info >/dev/null 2>&1; then
        URL="http://127.0.0.1:3000"
        break
    fi
    sleep 0.5
done

# Launch Cage compositor with hardware acceleration
if command -v electron >/dev/null 2>&1 && [ -f /opt/axisos-shell/electron/main.cjs ]; then
    exec cage -- electron /opt/axisos-shell/electron/main.cjs \
        --kiosk \
        --ozone-platform=wayland \
        --enable-features=UseOzonePlatform,WaylandWindowDecorations
elif command -v chromium >/dev/null 2>&1; then
    exec cage -- chromium \
        --kiosk \
        --ozone-platform=wayland \
        --enable-features=UseOzonePlatform,WaylandWindowDecorations \
        --enable-gpu-rasterization \
        --enable-zero-copy \
        --ignore-gpu-blocklist \
        --app="$URL" \
        --noerrdialogs \
        --disable-infobars \
        --disable-session-crashed-bubble \
        --disable-features=Translate \
        --no-first-run \
        --check-for-update-interval=31536000 \
        --password-store=basic
else
    echo "Error: Neither Chromium nor Electron found in PATH." >&2
    exit 1
fi
