#!/usr/bin/env bash
# ==============================================================================
# AxisOS Nobara Edition - Native Wayland Desktop Session Launcher
# Runs custom wlroots compositor (axis-compositor) with native Waybar, Swaybg,
# Mako notifications, and Nobara Driver & Codec Manager.
# ==============================================================================

# Setup XDG runtime directory with required 0700 permissions
UID_NUM=$(id -u)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/${UID_NUM}}"
mkdir -p "$XDG_RUNTIME_DIR" 2>/dev/null || true
chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true

# Display and compositor environment
export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=Nobara-Axis
unset WAYLAND_DISPLAY
unset DISPLAY
export OZONE_PLATFORM=wayland
export MOZ_ENABLE_WAYLAND=1
export GDK_BACKEND=wayland
export QT_QPA_PLATFORM=wayland
export SDL_VIDEODRIVER=wayland
export CLUTTER_BACKEND=wayland
export WLR_RENDERER_ALLOW_SOFTWARE=1
export WLR_NO_HARDWARE_CURSORS=1

# Initialize and unmute physical audio devices (PipeWire / ALSA)
alsactl init 2>/dev/null || true
amixer sset Master unmute 100% 2>/dev/null || true
amixer -c 0 sset Master unmute 100% 2>/dev/null || true
amixer sset Speaker unmute 100% 2>/dev/null || true
amixer sset Headphone unmute 100% 2>/dev/null || true
amixer sset PCM unmute 100% 2>/dev/null || true

# Ensure user audio daemons are running
if command -v pipewire >/dev/null 2>&1; then
    pgrep -x pipewire >/dev/null || pipewire &
    pgrep -x pipewire-pulse >/dev/null || pipewire-pulse &
    pgrep -x wireplumber >/dev/null || wireplumber &
fi

# Ensure user directories exist
mkdir -p "$HOME/.config" "$HOME/.cache" "$HOME/Desktop" "$HOME/Downloads" 2>/dev/null || true

# Copy default Waybar and Wofi configs to user home if not present
if [ ! -d "$HOME/.config/waybar" ]; then
    mkdir -p "$HOME/.config/waybar"
    cp -r /etc/xdg/waybar/* "$HOME/.config/waybar/" 2>/dev/null || true
fi
if [ ! -d "$HOME/.config/wofi" ]; then
    mkdir -p "$HOME/.config/wofi"
    cp -r /etc/xdg/wofi/* "$HOME/.config/wofi/" 2>/dev/null || true
fi
if [ ! -d "$HOME/.config/mako" ]; then
    mkdir -p "$HOME/.config/mako"
    cp -r /etc/xdg/mako/* "$HOME/.config/mako/" 2>/dev/null || true
fi

# Background wallpaper path
WALLPAPER="/usr/share/backgrounds/nobara-gaming.png"
if [ ! -f "$WALLPAPER" ]; then
    WALLPAPER="/usr/share/backgrounds/axis-wallpaper.png"
fi

# Define Nobara desktop startup suite
STARTUP_CMD="swaybg -i '$WALLPAPER' -m fill & mako & waybar & nobara-welcome &"

echo "[AxisOS] Launching Native Nobara wlroots Wayland Compositor..." > "$HOME/.axis-session.log"

if [ -x "/usr/local/bin/axis-compositor" ]; then
    exec /usr/local/bin/axis-compositor -s "$STARTUP_CMD" >> "$HOME/.axis-session.log" 2>&1
elif [ -x "/usr/bin/axis-compositor" ]; then
    exec /usr/bin/axis-compositor -s "$STARTUP_CMD" >> "$HOME/.axis-session.log" 2>&1
else
    echo "Warning: axis-compositor not found, falling back to cage with terminal" >> "$HOME/.axis-session.log"
    exec cage -s -- foot >> "$HOME/.axis-session.log" 2>&1
fi
