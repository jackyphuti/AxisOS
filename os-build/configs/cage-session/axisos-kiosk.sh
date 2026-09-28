#!/usr/bin/env bash
# ==============================================================================
# AxisOS Production Kiosk Launcher
# Compatible with both Real Hardware (Intel/AMD/Nvidia) and Virtual Machines (QEMU/KVM)
# ==============================================================================

# Setup XDG runtime directory with required 0700 permissions
UID_NUM=$(id -u)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/${UID_NUM}}"
mkdir -p "$XDG_RUNTIME_DIR" 2>/dev/null || true
chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true

# Display and compositor environment
export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=AxisOS
unset WAYLAND_DISPLAY
unset DISPLAY
export WLR_BACKENDS=drm
export OZONE_PLATFORM=wayland
export MOZ_ENABLE_WAYLAND=1
export GDK_BACKEND=wayland
export QT_QPA_PLATFORM=wayland
export WLR_LIBINPUT_NO_DEVICES=1
export WLR_RENDERER_ALLOW_SOFTWARE=1
export LIBSEAT_BACKEND=seatd

# Create user directories
mkdir -p /home/axis/.config/chromium 2>/dev/null || true
mkdir -p /home/axis/.cache 2>/dev/null || true

# Start system daemon if not already responding
if ! curl -s -f http://127.0.0.1:3000/api/system-info >/dev/null 2>&1; then
    /usr/bin/node /usr/local/bin/axisos-daemon.cjs >> /home/axis/.daemon.log 2>&1 &
fi

# Wait for local daemon to be healthy
URL="http://127.0.0.1:3000"
for i in {1..60}; do
    if curl -s -f http://127.0.0.1:3000/api/system-info >/dev/null 2>&1; then
        break
    fi
    sleep 0.2
done

CHROME_BIN=$(command -v chromium || command -v chromium-browser || true)

if [ -n "$CHROME_BIN" ]; then
    # Launch Cage with Chromium in kiosk mode
    exec cage -s -d -- "$CHROME_BIN" \
        --kiosk \
        --ozone-platform=wayland \
        --enable-features=UseOzonePlatform,WaylandWindowDecorations \
        --no-sandbox \
        --disable-dev-shm-usage \
        --disable-gpu-sandbox \
        --in-process-gpu \
        --ignore-gpu-blocklist \
        --enable-gpu-rasterization \
        --enable-zero-copy \
        --allow-file-access-from-files \
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
        --app="$URL" >> /home/axis/.kiosk.log 2>&1
else
    echo "Error: Chromium not found in PATH." >&2
    exec cage -s -- xterm >> /home/axis/.kiosk.log 2>&1
fi
