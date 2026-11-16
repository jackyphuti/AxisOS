#!/usr/bin/env bash
# ==============================================================================
# AxisOS Game Mode Session Launcher (gamescope + Steam Big Picture / Game Station)
# Alternative session running directly under gamescope micro-compositor.
# ==============================================================================

set -u

UID_NUM=$(id -u)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/${UID_NUM}}"
if [ ! -d "$XDG_RUNTIME_DIR" ] || [ ! -w "$XDG_RUNTIME_DIR" ]; then
    export XDG_RUNTIME_DIR="/tmp/runtime-axis-${UID_NUM}"
    mkdir -p "$XDG_RUNTIME_DIR" 2>/dev/null || true
    chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true
fi

# Session backend
if [ -w "/run/seatd.sock" ] || [ -S "/run/seatd.sock" ]; then
    export LIBSEAT_BACKEND=seatd
else
    export LIBSEAT_BACKEND=logind
fi

export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP="gamescope:Steam"
export SDL_VIDEODRIVER=wayland
export GDK_BACKEND=wayland

# Ensure PipeWire audio is up
if command -v pipewire >/dev/null 2>&1; then
    pgrep -x pipewire >/dev/null || pipewire &
    pgrep -x pipewire-pulse >/dev/null || pipewire-pulse &
    pgrep -x wireplumber >/dev/null || wireplumber &
fi

# Enable GameBoost (performance governor & sysctl tuning)
if [ -x "/usr/local/bin/axis-game-boost" ]; then
    /usr/local/bin/axis-game-boost &
fi

echo "[AxisOS Game Mode] Starting gamescope console session..."

# Detect display resolution if drm is available
RES_W=1920
RES_H=1080
REFRESH=60

if [ -f /sys/class/drm/card0/modes ]; then
    MODE_STR=$(head -n 1 /sys/class/drm/card0/modes 2>/dev/null || true)
    if [ -n "$MODE_STR" ]; then
        RES_W=$(echo "$MODE_STR" | cut -d'x' -f1)
        RES_H=$(echo "$MODE_STR" | cut -d'x' -f2)
    fi
fi

# Determine target launcher: Steam Big Picture or AxisOS Game Station Hub
GAME_CMD=""
if command -v steam >/dev/null 2>&1; then
    GAME_CMD="steam -gamepadui -steamos3"
elif [ -x "/usr/local/bin/axis-game-runner" ]; then
    GAME_CMD="/usr/local/bin/axis-game-runner"
else
    GAME_CMD="retroarch || lutris"
fi

# Launch gamescope
if command -v gamescope >/dev/null 2>&1; then
    exec gamescope \
        -f \
        -e \
        -w "$RES_W" -h "$RES_H" \
        -r "$REFRESH" \
        --mangoapp \
        --hdr-enabled \
        -- $GAME_CMD
else
    echo "[AxisOS Game Mode] gamescope not found, falling back to direct runner..."
    exec $GAME_CMD
fi
