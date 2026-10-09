#!/usr/bin/env bash
# ==============================================================================
# AxisOS Wi-Fi Captive Portal Detection & Web Login Redirect Handler
# ==============================================================================

set -e

PORTAL_URL="http://connectivity-check.ubuntu.com/"

# Check NetworkManager connectivity state
CONN_STATE="$(nmcli networking connectivity check 2>/dev/null || echo "unknown")"

if [ "$CONN_STATE" = "portal" ]; then
    echo "[AxisOS Network] Captive portal detected! Launching browser for web login..."
    # Notify user via desktop notification if desktop session active
    if command -v notify-send >/dev/null 2>&1; then
        notify-send -u normal -i network-wireless "Wi-Fi Login Required" "A web portal login is required to access the internet. Opening login page..." || true
    fi
    # Launch browser pointing to the portal check URL which triggers the network redirection
    if command -v chromium >/dev/null 2>&1; then
        chromium "$PORTAL_URL" &
    elif command -v firefox >/dev/null 2>&1; then
        firefox "$PORTAL_URL" &
    elif command -v x-www-browser >/dev/null 2>&1; then
        x-www-browser "$PORTAL_URL" &
    elif command -v xdg-open >/dev/null 2>&1; then
        xdg-open "$PORTAL_URL" &
    fi
else
    echo "[AxisOS Network] Connectivity state: $CONN_STATE"
fi
