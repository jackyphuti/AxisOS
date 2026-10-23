#!/usr/bin/env bash
# ==============================================================================
# AxisOS 2.1 - Native Wayland Desktop Session Launcher
# Primary Display Server: LiquidNodesOS (Native C++20 / Qt 6 Wayland Compositor)
# Fallback Display Server: axis-compositor (Native C wlroots Wayland Compositor)
# ==============================================================================

# 1. Setup XDG runtime directory with required 0700 permissions
UID_NUM=$(id -u)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/${UID_NUM}}"
if ! mkdir -p "$XDG_RUNTIME_DIR" 2>/dev/null; then
    export XDG_RUNTIME_DIR="/tmp/runtime-axis-${UID_NUM}"
    mkdir -p "$XDG_RUNTIME_DIR"
fi
chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true

# 2. Configure seatd / libseat and display environment
if [ -S "/run/seatd.sock" ]; then
    export LIBSEAT_BACKEND=seatd
fi

export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP="AxisOS:LiquidNodes"
export WLR_RENDERER_ALLOW_SOFTWARE=1
export WLR_NO_HARDWARE_CURSORS=1
export MOZ_ENABLE_WAYLAND=1
export OZONE_PLATFORM=wayland
export SDL_VIDEODRIVER=wayland

# IMPORTANT: Unset QT_QPA_PLATFORM and WAYLAND_DISPLAY before launching the root
# compositor so LiquidNodesOS binds directly to DRM/KMS (eglfs) or Framebuffer (linuxfb)
# instead of searching for a non-existent parent Wayland server!
unset WAYLAND_DISPLAY
unset DISPLAY
unset QT_QPA_PLATFORM

# 3. Initialize and unmute physical audio devices (PipeWire / ALSA)
alsactl init 2>/dev/null || true
amixer sset Master unmute 100% 2>/dev/null || true
amixer -c 0 sset Master unmute 100% 2>/dev/null || true

if command -v pipewire >/dev/null 2>&1; then
    pgrep -x pipewire >/dev/null || pipewire &
    pgrep -x pipewire-pulse >/dev/null || pipewire-pulse &
    pgrep -x wireplumber >/dev/null || wireplumber &
fi

# 4. Ensure user directories and default unified Dark Mode theme config exist
mkdir -p "$HOME/.config/axisos" "$HOME/.cache" "$HOME/Desktop" "$HOME/Downloads" 2>/dev/null || true
if [ ! -f "$HOME/.config/axisos/theme.json" ]; then
    cat << 'EOF' > "$HOME/.config/axisos/theme.json"
{
    "theme": "dark",
    "isDark": true,
    "accent": "#7d95ff"
}
EOF
fi

# Copy default Waybar, Wofi, and Mako configs for fallback wlroots session
for cfg in waybar wofi mako foot; do
    if [ ! -d "$HOME/.config/$cfg" ] && [ -d "/etc/xdg/$cfg" ]; then
        mkdir -p "$HOME/.config/$cfg"
        cp -r /etc/xdg/$cfg/* "$HOME/.config/$cfg/" 2>/dev/null || true
    fi
done

# 5. Primary Compositor: Native C++20 / Qt 6 LiquidNodesOS Wayland Compositor
if [ -x "/usr/local/bin/LiquidNodesOS" ]; then
    echo "[AxisOS] Launching Native Qt 6 LiquidNodesOS Wayland Compositor..." >> "$HOME/.axis-session.log"
    exec /usr/local/bin/LiquidNodesOS >> "$HOME/.axis-session.log" 2>&1
elif [ -x "/usr/bin/LiquidNodesOS" ]; then
    echo "[AxisOS] Launching Native Qt 6 LiquidNodesOS Wayland Compositor..." >> "$HOME/.axis-session.log"
    exec /usr/bin/LiquidNodesOS >> "$HOME/.axis-session.log" 2>&1
fi

# 6. Secondary Fallback Compositor: Native C wlroots axis-compositor
WALLPAPER="/usr/share/backgrounds/axisos/01-axisos-signature-core.png"
[ ! -f "$WALLPAPER" ] && WALLPAPER="/usr/share/backgrounds/axis-wallpaper.png"

if [ -f "/var/lib/axisos/first-boot-pending" ] && command -v axis-oobe >/dev/null 2>&1; then
    STARTUP_CMD="swaybg -i '$WALLPAPER' -m fill & mako & waybar & (sleep 1 && QT_QPA_PLATFORM=wayland axis-oobe) &"
elif grep -q "axisos.autoinstall=1" /proc/cmdline 2>/dev/null; then
    STARTUP_CMD="swaybg -i '$WALLPAPER' -m fill & mako & waybar & (sleep 1 && QT_QPA_PLATFORM=wayland axis-installer) &"
else
    STARTUP_CMD="swaybg -i '$WALLPAPER' -m fill & mako & waybar & (sleep 1 && QT_QPA_PLATFORM=wayland axis-welcome) &"
fi

if [ -x "/usr/local/bin/axis-compositor" ]; then
    echo "[AxisOS] Launching Native wlroots axis-compositor..." >> "$HOME/.axis-session.log"
    exec /usr/local/bin/axis-compositor -s "$STARTUP_CMD" >> "$HOME/.axis-session.log" 2>&1
elif [ -x "/usr/bin/axis-compositor" ]; then
    exec /usr/bin/axis-compositor -s "$STARTUP_CMD" >> "$HOME/.axis-session.log" 2>&1
elif command -v cage >/dev/null 2>&1; then
    echo "[AxisOS] Fallback: Launching cage with axis-welcome..." >> "$HOME/.axis-session.log"
    exec cage -s -- /usr/local/bin/axis-welcome >> "$HOME/.axis-session.log" 2>&1
fi
