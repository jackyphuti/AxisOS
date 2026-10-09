#!/usr/bin/env bash
# NetworkManager Dispatcher: Check for Captive Portal on connection up
IFACE="$1"
ACTION="$2"

if [ "$ACTION" = "up" ]; then
    /usr/local/bin/axis-captive-portal.sh &
fi
