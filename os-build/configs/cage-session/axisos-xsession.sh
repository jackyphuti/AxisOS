#!/usr/bin/env bash
# ==============================================================================
# AxisOS Production X11 Desktop Session
# ==============================================================================

UID_NUM=$(id -u)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/${UID_NUM}}"
mkdir -p "$XDG_RUNTIME_DIR" 2>/dev/null || true
chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true

export XDG_SESSION_TYPE=x11
export XDG_CURRENT_DESKTOP=AxisOS
export DISPLAY="${DISPLAY:-:0}"

USER_HOME="${HOME:-/home/$(id -un)}"
mkdir -p "$USER_HOME/.config/chromium" "$USER_HOME/.cache" 2>/dev/null || true

# Start AxisOS background system daemon if not responding
if ! curl -s -f http://127.0.0.1:3000/api/system-info >/dev/null 2>&1; then
    /usr/bin/node /usr/local/bin/axisos-daemon.cjs >> "$USER_HOME/.daemon.log" 2>&1 &
fi

# Wait for local daemon to be healthy
for i in {1..50}; do
    if curl -s -f http://127.0.0.1:3000/api/system-info >/dev/null 2>&1; then
        break
    fi
    sleep 0.2
done

# Disable screen blanking & DPMS
xset s off 2>/dev/null || true
xset -dpms 2>/dev/null || true
xset s noblank 2>/dev/null || true

# Set clean slate background and standard arrow cursor
xsetroot -solid "#020617" 2>/dev/null || true
xsetroot -cursor_name left_ptr 2>/dev/null || true

# Start Openbox window manager
openbox &

# Launch Chromium in fullscreen kiosk mode loading AxisOS Shell
CHROME_BIN=$(command -v chromium || command -v chromium-browser || true)

exec "$CHROME_BIN" \
    --kiosk \
    --no-sandbox \
    --disable-dev-shm-usage \
    --disable-gpu-sandbox \
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
    --user-data-dir="$USER_HOME/.config/chromium" \
    --window-position=0,0 \
    --start-maximized \
    --app="http://127.0.0.1:3000" >> "$USER_HOME/.kiosk.log" 2>&1
