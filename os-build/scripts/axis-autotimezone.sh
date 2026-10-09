#!/usr/bin/env bash
# ==============================================================================
# AxisOS Automatic Location & Timezone Synchronizer
# Uses network IP geolocation to detect location, configure system timezone,
# and enable NTP network clock synchronization.
# ==============================================================================

set -e

# Try multiple geolocation services
TZ_DETECTED=""

# Attempt 1: ipapi.co
if command -v curl >/dev/null 2>&1; then
    TZ_DETECTED="$(curl -s --max-time 4 https://ipapi.co/timezone 2>/dev/null || true)"
fi

# Attempt 2: worldtimeapi.org fallback
if [ -z "$TZ_DETECTED" ] || [ "${#TZ_DETECTED}" -gt 40 ]; then
    if command -v curl >/dev/null 2>&1; then
        TZ_DETECTED="$(curl -s --max-time 4 http://worldtimeapi.org/api/ip 2>/dev/null | grep -o '"timezone":"[^"]*"' | cut -d'"' -f4 || true)"
    fi
fi

# Apply timezone if valid
if [ -n "$TZ_DETECTED" ] && [ -f "/usr/share/zoneinfo/$TZ_DETECTED" ]; then
    echo "[AxisOS Timezone] Auto-detected timezone: $TZ_DETECTED"
    timedatectl set-timezone "$TZ_DETECTED" 2>/dev/null || true
fi

# Always ensure NTP network clock synchronization is enabled
timedatectl set-ntp true 2>/dev/null || true
echo "[AxisOS Timezone] NTP clock synchronization enabled."
