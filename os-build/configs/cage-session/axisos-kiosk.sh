#!/bin/sh
# ==============================================================================
# AxisOS 2.1 - Pure C Wayland Desktop Session Launcher
# Launches native wlroots axis-compositor with pure C desktop shell and GTK 3 apps
# ==============================================================================

set -eu

# 1. Setup XDG runtime directory safely without touching /run/user/1000/systemd/inaccessible
UID_NUM=$(id -u)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/${UID_NUM}}"
if [ ! -d "$XDG_RUNTIME_DIR" ] || [ ! -w "$XDG_RUNTIME_DIR" ]; then
    export XDG_RUNTIME_DIR="/tmp/runtime-axis-${UID_NUM}"
    mkdir -p "$XDG_RUNTIME_DIR" 2>/dev/null || true
    chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true
fi

# 2. Configure libseat / logind session backend
# When launched via systemd PAMName=login, XDG_SESSION_ID is set and logind owns the VT/DRM master.
# Only force LIBSEAT_BACKEND=seatd if no logind session is active and /run/seatd.sock is writable.
if [ -z "${XDG_SESSION_ID:-}" ] && [ -w "/run/seatd.sock" ]; then
    export LIBSEAT_BACKEND=seatd
else
    unset LIBSEAT_BACKEND
fi

export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP="AxisOS:LiquidNodes"
export GDK_BACKEND=wayland
export WLR_RENDERER_ALLOW_SOFTWARE=1
export WLR_NO_HARDWARE_CURSORS=1
export LIBGL_ALWAYS_SOFTWARE=1
export MOZ_ENABLE_WAYLAND=1
export OZONE_PLATFORM=wayland
export SDL_VIDEODRIVER=wayland

# Unset parent display server variables
unset WAYLAND_DISPLAY
unset DISPLAY

# 3. Initialize and unmute physical audio devices (PipeWire / ALSA)
alsactl init 2>/dev/null || true
amixer sset Master unmute 100% 2>/dev/null || true
amixer -c 0 sset Master unmute 100% 2>/dev/null || true

if command -v pipewire >/dev/null 2>&1; then
    pgrep -x pipewire >/dev/null || pipewire &
    pgrep -x pipewire-pulse >/dev/null || pipewire-pulse &
    pgrep -x wireplumber >/dev/null || wireplumber &
fi

# 4. User directory layout and configs
mkdir -p "$HOME/.config/axisos" "$HOME/.cache" "$HOME/Desktop" "$HOME/Downloads" 2>/dev/null || true
for cfg in waybar wofi mako foot; do
    if [ ! -d "$HOME/.config/$cfg" ] && [ -d "/etc/xdg/$cfg" ]; then
        mkdir -p "$HOME/.config/$cfg"
        cp -r /etc/xdg/$cfg/* "$HOME/.config/$cfg/" 2>/dev/null || true
    fi
done

# 5. Determine Wallpaper
WALLPAPER="/usr/share/backgrounds/axisos/01-axisos-signature-core.png"
[ ! -f "$WALLPAPER" ] && WALLPAPER="/usr/share/backgrounds/axis-wallpaper.png"
[ ! -f "$WALLPAPER" ] && WALLPAPER="/usr/share/backgrounds/axisos/03-axisos-arctic-light.jpg"

# 6. Determine Startup App (with GDK_BACKEND=wayland)
if [ -f "/var/lib/axisos/first-boot-pending" ] && command -v axis-oobe >/dev/null 2>&1; then
    STARTUP_APP="axis-oobe"
elif grep -q "axisos.autoinstall=1" /proc/cmdline 2>/dev/null; then
    STARTUP_APP="axis-installer"
else
    STARTUP_APP="axis-welcome"
fi

# 7. Configure Desktop Shell Component (axis-shell or waybar + wofi)
SHELL_CMD=""
if [ -x "/usr/local/bin/axis-shell" ]; then
    SHELL_CMD="/usr/local/bin/axis-shell &"
elif [ -x "/usr/bin/axis-shell" ]; then
    SHELL_CMD="/usr/bin/axis-shell &"
elif command -v waybar >/dev/null 2>&1; then
    SHELL_CMD="waybar &"
fi

STARTUP_CMD="swaybg -i '$WALLPAPER' -m fill & mako & $SHELL_CMD (sleep 1 && GDK_BACKEND=wayland $STARTUP_APP) &"

# 8. Execute Pure C wlroots Compositor with Software Rendering Fallback
COMPOSITOR_BIN=""
if [ -x "/usr/local/bin/axis-compositor" ]; then
    COMPOSITOR_BIN="/usr/local/bin/axis-compositor"
elif [ -x "/usr/bin/axis-compositor" ]; then
    COMPOSITOR_BIN="/usr/bin/axis-compositor"
fi

if [ -n "$COMPOSITOR_BIN" ]; then
    echo "[AxisOS] Launching Native Pure C wlroots axis-compositor..."
    "$COMPOSITOR_BIN" -s "$STARTUP_CMD"
    RET=$?
    if [ $RET -ne 0 ]; then
        echo "[AxisOS] Compositor exited ($RET). Retrying with pixman software rendering fallback..."
        export WLR_RENDERER=pixman
        export WLR_RENDERER_ALLOW_SOFTWARE=1
        export WLR_NO_HARDWARE_CURSORS=1
        export LIBGL_ALWAYS_SOFTWARE=1
        "$COMPOSITOR_BIN" -s "$STARTUP_CMD"
        RET=$?
        if [ $RET -ne 0 ]; then
            echo "[AxisOS] Compositor failed. Launching fallback kiosk..."
            if command -v cage >/dev/null 2>&1; then
                exec cage -s -- /usr/local/bin/$STARTUP_APP
            fi
        fi
    fi
elif command -v cage >/dev/null 2>&1; then
    echo "[AxisOS] Fallback: Launching cage with $STARTUP_APP..."
    exec cage -s -- /usr/local/bin/$STARTUP_APP
fi
