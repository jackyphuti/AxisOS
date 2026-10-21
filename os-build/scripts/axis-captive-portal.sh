#!/usr/bin/env bash
# ==============================================================================
# AxisOS Wi-Fi Captive Portal Detection & Web Login Redirect Handler
# ==============================================================================

PROBE_URLS=(
    "http://connectivitycheck.gstatic.com/generate_204"
    "http://detectportal.firefox.com/canonical.html"
    "http://www.msftconnecttest.com/connecttest.txt"
    "http://neverssl.com"
)

TARGET_URL=""

# Probe for captive portal redirect URL
for url in "${PROBE_URLS[@]}"; do
    REDIRECT_LOC=$(curl -s -I -m 3 "$url" 2>/dev/null | grep -i '^Location:' | awk '{print $2}' | tr -d '\r\n')
    if [ -n "$REDIRECT_LOC" ] && [[ "$REDIRECT_LOC" =~ ^https?:// ]]; then
        TARGET_URL="$REDIRECT_LOC"
        break
    fi
done

if [ -z "$TARGET_URL" ]; then
    TARGET_URL="http://neverssl.com"
fi

launch_browser() {
    local url="$1"
    echo "[AxisOS Network] Opening web browser for Wi-Fi portal login -> $url"
    if command -v chromium >/dev/null 2>&1; then
        exec chromium --new-window --no-sandbox "$url"
    elif command -v firefox >/dev/null 2>&1; then
        exec firefox --new-window "$url"
    elif command -v x-www-browser >/dev/null 2>&1; then
        exec x-www-browser "$url"
    elif command -v xdg-open >/dev/null 2>&1; then
        exec xdg-open "$url"
    fi
}

# If launched interactively by user (from desktop, settings, or top bar)
if [ -n "$DISPLAY" ] || [ -n "$WAYLAND_DISPLAY" ]; then
    launch_browser "$TARGET_URL"
    exit 0
fi

# If launched from NetworkManager Dispatcher (runs in system environment as root)
sleep 2
CONN_STATE="$(nmcli networking connectivity check 2>/dev/null || echo "unknown")"

if [ "$CONN_STATE" = "portal" ] || [ "$CONN_STATE" = "limited" ]; then
    echo "[AxisOS Network] Captive portal detected! Launching browser for web login..."

    ACTIVE_USER=$(who 2>/dev/null | awk '{print $1}' | head -n 1)
    [ -z "$ACTIVE_USER" ] && ACTIVE_USER="axis"
    USER_ID=$(id -u "$ACTIVE_USER" 2>/dev/null || echo 1000)

    if [ -d "/run/user/$USER_ID" ]; then
        su - "$ACTIVE_USER" -c "DISPLAY=:0 WAYLAND_DISPLAY=wayland-0 XDG_RUNTIME_DIR=/run/user/$USER_ID notify-send -u critical -i network-wireless 'Wi-Fi Web Login Required' 'Opening browser to complete network authentication...'" 2>/dev/null || true
        su - "$ACTIVE_USER" -c "DISPLAY=:0 WAYLAND_DISPLAY=wayland-0 XDG_RUNTIME_DIR=/run/user/$USER_ID /usr/local/bin/axis-captive-portal.sh" >/dev/null 2>&1 &
    fi
else
    echo "[AxisOS Network] Connectivity state: $CONN_STATE"
fi
