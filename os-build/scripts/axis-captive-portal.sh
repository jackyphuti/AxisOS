#!/usr/bin/env bash
# ==============================================================================
# AxisOS Wi-Fi Captive Portal Detection & Web Login Redirect Handler
# ==============================================================================

PORTAL_URL="http://connectivitycheck.gstatic.com/generate_204"

# If launched interactively by user (from desktop or application launcher)
if [ -n "$DISPLAY" ] || [ -n "$WAYLAND_DISPLAY" ]; then
    echo "[AxisOS Network] Opening web browser for Wi-Fi portal login..."
    if command -v chromium >/dev/null 2>&1; then
        exec chromium --new-window "$PORTAL_URL"
    elif command -v firefox >/dev/null 2>&1; then
        exec firefox --new-window "$PORTAL_URL"
    elif command -v x-www-browser >/dev/null 2>&1; then
        exec x-www-browser "$PORTAL_URL"
    elif command -v xdg-open >/dev/null 2>&1; then
        exec xdg-open "$PORTAL_URL"
    fi
    exit 0
fi

# If launched from NetworkManager Dispatcher (runs in system environment as root)
sleep 2
CONN_STATE="$(nmcli networking connectivity check 2>/dev/null || echo "unknown")"

if [ "$CONN_STATE" = "portal" ]; then
    echo "[AxisOS Network] Captive portal detected! Launching browser for web login..."

    # Find active GUI user
    ACTIVE_USER=$(who 2>/dev/null | awk '{print $1}' | head -n 1)
    [ -z "$ACTIVE_USER" ] && ACTIVE_USER="axis"
    USER_ID=$(id -u "$ACTIVE_USER" 2>/dev/null || echo 1000)

    if [ -d "/run/user/$USER_ID" ]; then
        # Send desktop notification
        su - "$ACTIVE_USER" -c "DISPLAY=:0 WAYLAND_DISPLAY=wayland-0 XDG_RUNTIME_DIR=/run/user/$USER_ID notify-send -u critical -i network-wireless 'Wi-Fi Login Required' 'Opening web login page for network authentication...'" 2>/dev/null || true
        # Launch Chromium window pointing to the captive portal trigger URL
        su - "$ACTIVE_USER" -c "DISPLAY=:0 WAYLAND_DISPLAY=wayland-0 XDG_RUNTIME_DIR=/run/user/$USER_ID chromium --new-window '$PORTAL_URL'" >/dev/null 2>&1 &
    fi
else
    echo "[AxisOS Network] Connectivity state: $CONN_STATE"
fi
