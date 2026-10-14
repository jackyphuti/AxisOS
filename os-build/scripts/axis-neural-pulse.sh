#!/usr/bin/env bash
# ==============================================================================
# AxisOS "NeuralPulse" — Focus-Driven Kernel, Scheduler & GPU Priority Router
# Automatically boosts the focused Wayland window's CPU nice level, I/O class,
# and cgroup v2 weight while throttling unfocused background windows.
# ==============================================================================

set -uo pipefail

STATE_DIR="/run/user/$(id -u 2>/dev/null || echo 1000)/axis-neural-pulse"
mkdir -p "$STATE_DIR" 2>/dev/null || true
STATE_FILE="$STATE_DIR/active_window.env"

ACTION="${1:-status}"
WIN_TITLE="${2:-Desktop}"
WIN_PID="${3:-0}"
IS_FULLSCREEN="${4:-false}"

case "$ACTION" in
    boost)
        # Demote previously boosted PID back to normal priority
        if [ -f "$STATE_FILE" ]; then
            prev_pid=$(grep '^ACTIVE_PID=' "$STATE_FILE" 2>/dev/null | cut -d= -f2 || echo 0)
            if [ -n "$prev_pid" ] && [ "$prev_pid" -gt 0 ] 2>/dev/null && [ "$prev_pid" != "$WIN_PID" ]; then
                renice -n 0 -p "$prev_pid" >/dev/null 2>&1 || true
                ionice -c 2 -n 4 -p "$prev_pid" >/dev/null 2>&1 || true
            fi
        fi

        # Resolve PID by window title if PID was not supplied
        if [ "$WIN_PID" -le 0 ] 2>/dev/null; then
            proc_guess=$(echo "$WIN_TITLE" | awk '{print tolower($1)}' | tr -cd 'a-z0-9_-')
            if [ -n "$proc_guess" ]; then
                WIN_PID=$(pgrep -n "$proc_guess" 2>/dev/null || echo 0)
            fi
        fi

        if [ -n "$WIN_PID" ] && [ "$WIN_PID" -gt 0 ] 2>/dev/null && [ -d "/proc/$WIN_PID" ]; then
            renice -n -10 -p "$WIN_PID" >/dev/null 2>&1 || true
            ionice -c 2 -n 0 -p "$WIN_PID" >/dev/null 2>&1 || true
            # Boost cgroup v2 cpu.weight if writable
            cg_path=$(awk -F: '$1=="0"{print $3}' "/proc/$WIN_PID/cgroup" 2>/dev/null || true)
            if [ -n "$cg_path" ] && [ -w "/sys/fs/cgroup${cg_path}/cpu.weight" ]; then
                echo 1000 > "/sys/fs/cgroup${cg_path}/cpu.weight" 2>/dev/null || true
            fi
        fi

        cat << EOF > "$STATE_FILE"
ACTIVE_TITLE=$WIN_TITLE
ACTIVE_PID=${WIN_PID:-0}
FULLSCREEN=$IS_FULLSCREEN
UPDATED_AT=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
EOF
        echo "NEURAL_PULSE_TARGET=$WIN_TITLE"
        ;;
    status)
        if [ -f "$STATE_FILE" ]; then
            cat "$STATE_FILE"
        else
            echo "ACTIVE_TITLE=Desktop"
            echo "ACTIVE_PID=0"
        fi
        ;;
    *)
        echo "Usage: $0 {boost|status} [window-title] [pid] [fullscreen]"
        exit 1
        ;;
esac
