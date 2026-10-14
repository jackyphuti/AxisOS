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
    python3 - "$STATE_FILE" << 'PYEOF'
import sys, os, glob

state_file = sys.argv[1]
# Check if any V4L2 camera device exists
video_devs = sorted(glob.glob("/dev/video*"))
if not video_devs:
    with open(state_file, "w") as f:
        f.write("CLEAR:faces=1\n")
    sys.exit(0)

try:
    import cv2
    cap = cv2.VideoCapture(0)
    ok, frame = cap.read()
    cap.release()
    if ok and frame is not None:
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        cascade = cv2.CascadeClassifier(cv2.data.haarcascades + "haarcascade_frontalface_default.xml")
        faces = cascade.detectMultiScale(gray, 1.2, 5)
        count = len(faces)
        if count > 1:
            with open(state_file, "w") as f:
                f.write(f"ALERT:shoulder_surfer_detected:faces={count}\n")
            sys.exit(0)
        elif count == 0:
            with open(state_file, "w") as f:
                f.write("AWAY:no_gaze_detected:faces=0\n")
            sys.exit(0)
except Exception:
    pass

with open(state_file, "w") as f:
    f.write("CLEAR:faces=1\n")
PYEOF
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
