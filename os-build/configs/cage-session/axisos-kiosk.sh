#!/usr/bin/env bash
# ==============================================================================
# AxisOS Wayland Kiosk Session Launcher
# Runs directly on DRM/KMS hardware via Cage (lightweight Wayland compositor)
# ==============================================================================

# Ensure standard user runtime directory exists with strict permissions
UID_NUM=$(id -u)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/${UID_NUM}}"
if [ ! -d "$XDG_RUNTIME_DIR" ]; then
    mkdir -p "$XDG_RUNTIME_DIR" 2>/dev/null || true
    chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true
fi

# Modern Wayland & wlroots display environment
export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=AxisOS
export WAYLAND_DISPLAY=wayland-0
export OZONE_PLATFORM=wayland
export MOZ_ENABLE_WAYLAND=1
export GDK_BACKEND=wayland
export QT_QPA_PLATFORM=wayland
export WLR_LIBINPUT_NO_DEVICES=1
export WLR_RENDERER_ALLOW_SOFTWARE=1

# Wait for local AxisOS daemon to be healthy
URL="file:///opt/axisos-shell/dist/index.html"
for i in {1..30}; do
    if curl -s -f http://127.0.0.1:3000/api/system-info >/dev/null 2>&1; then
        URL="http://127.0.0.1:3000"
        break
    fi
    sleep 0.2
done

# Launch Cage compositor with shell application
if command -v electron >/dev/null 2>&1 && [ -f /opt/axisos-shell/electron/main.cjs ]; then
    exec cage -- electron /opt/axisos-shell/electron/main.cjs \
        --kiosk \
        --ozone-platform=wayland \
        --enable-features=UseOzonePlatform,WaylandWindowDecorations \
        --no-sandbox \
        --disable-dev-shm-usage
elif command -v chromium >/dev/null 2>&1 || command -v chromium-browser >/dev/null 2>&1; then
    CHROME_BIN=$(command -v chromium || command -v chromium-browser)
    mkdir -p /home/axis/.config/chromium 2>/dev/null || true
    exec cage -- "$CHROME_BIN" \
        --kiosk \
        --ozone-platform=wayland \
        --enable-features=UseOzonePlatform,WaylandWindowDecorations \
        --no-sandbox \
        --disable-dev-shm-usage \
        --disable-gpu-sandbox \
        --ignore-gpu-blocklist \
        --enable-gpu-rasterization \
        --enable-zero-copy \
        --disable-features=Translate,OptimizationHints,MediaRouter \
        --noerrdialogs \
        --disable-infobars \
        --disable-session-crashed-bubble \
        --no-first-run \
        --no-default-browser-check \
        --check-for-update-interval=31536000 \
        --password-store=basic \
        --user-data-dir=/home/axis/.config/chromium \
        --window-position=0,0 \
        --window-size=1280,800 \
        --start-maximized \
        --app="$URL"
else
    echo "Error: Neither Chromium nor Electron found in PATH." >&2
    # Fallback shell inside cage so session does not panic or enter rapid restart loop
    exec cage -- xterm 2>/dev/null || sleep infinity
fi
