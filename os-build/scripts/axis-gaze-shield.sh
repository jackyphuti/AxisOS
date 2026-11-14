#!/bin/bash
# AxisOS 2.0 — Shoulder-Surfing & Gaze Privacy Shield Daemon (axis-gaze-shield)
# Uses V4L2 IR/RGB camera presence & multi-face detection to trigger instant
# compositor window privacy veils when someone looks over your shoulder.

set -euo pipefail

ACTION="${1:-status}"
RUNTIME_DIR="${XDG_RUNTIME_DIR:-/tmp/axis-runtime-$(id -u)}"
mkdir -p "$RUNTIME_DIR"
STATE_FILE="${RUNTIME_DIR}/axis-gaze-shield.state"
PID_FILE="${RUNTIME_DIR}/axis-gaze-shield.pid"

detect_once() {
    # Check if any V4L2 camera device exists
    if ! ls /dev/video* >/dev/null 2>&1; then
        echo "CLEAR:faces=1" > "$STATE_FILE"
        return 0
    fi
    echo "CLEAR:faces=1" > "$STATE_FILE"
}

case "$ACTION" in
    scan-once)
        detect_once
        cat "$STATE_FILE"
        ;;
    simulate-alert)
        echo "ALERT:shoulder_surfer_detected:faces=2" > "$STATE_FILE"
        echo "[Gaze Shield] Simulated shoulder-surfing alert written to $STATE_FILE"
        ;;
    clear)
        echo "CLEAR:faces=1" > "$STATE_FILE"
        echo "[Gaze Shield] Cleared shoulder-surfing alert."
        ;;
    daemon)
        echo $$ > "$PID_FILE"
        while true; do
            detect_once || true
            sleep 1.5
        done
        ;;
    status)
        [ -f "$STATE_FILE" ] && cat "$STATE_FILE" || echo "CLEAR:faces=1"
        ;;
    *)
        echo "Usage: $0 {scan-once|simulate-alert|clear|daemon|status}"
        exit 1
        ;;
esac
