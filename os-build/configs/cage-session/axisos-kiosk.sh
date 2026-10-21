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
export XDG_CURRENT_DESKTOP=AxisOS-LiquidNodes
unset WAYLAND_DISPLAY
unset DISPLAY
export OZONE_PLATFORM=wayland
export MOZ_ENABLE_WAYLAND=1
export GDK_BACKEND=wayland
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
WALLPAPER="/usr/share/backgrounds/axisos/01-axisos-signature-core.png"
if [ ! -f "$WALLPAPER" ]; then
    WALLPAPER="/usr/share/backgrounds/nobara-gaming.png"
fi
if [ ! -f "$WALLPAPER" ]; then
    WALLPAPER="/usr/share/backgrounds/axis-wallpaper.png"
fi

# Check if running in Live ISO environment
if [ -f "/etc/axisos-live" ] || [ -f "/etc/axisos-autologin" ] || [ -d "/run/live" ] || grep -q "boot=live" /proc/cmdline 2>/dev/null; then
    IS_LIVE=1
else
    IS_LIVE=0
fi

# Check if Out-of-Box Experience (OOBE) setup is pending
FIRST_BOOT_FLAG="/var/lib/axisos/first-boot-pending"
if [ -f "$FIRST_BOOT_FLAG" ] && command -v axis-oobe >/dev/null 2>&1; then
    echo "[AxisOS] First-boot pending: launching Out-of-Box Experience (OOBE)..." >> "$HOME/.axis-session.log"
    STARTUP_CMD="swaybg -i '$WALLPAPER' -m fill & mako & axis-oobe"
elif [ "$IS_LIVE" -eq 1 ]; then
    echo "[AxisOS] Live ISO boot: launching Desktop with AxisOS Installer..." >> "$HOME/.axis-session.log"
    STARTUP_CMD="swaybg -i '$WALLPAPER' -m fill & mako & waybar & (sleep 2 && /usr/local/bin/axis-installer) &"
else
    # Show the Welcome Center once for each newly installed user.
    WELCOME_SHOWN_FLAG="$HOME/.config/axisos/welcome-shown"
    STARTUP_CMD="swaybg -i '$WALLPAPER' -m fill & mako & waybar & if [ ! -f '$WELCOME_SHOWN_FLAG' ]; then mkdir -p '$(dirname "$WELCOME_SHOWN_FLAG")'; touch '$WELCOME_SHOWN_FLAG'; axis-welcome; fi &"
fi

if [ -S "/run/seatd.sock" ]; then
    export LIBSEAT_BACKEND=seatd
fi

export QT_QPA_PLATFORM=wayland

# Primary Production Compositor: Native wlroots axis-compositor
# Fully supports software rendering (WLR_RENDERER_ALLOW_SOFTWARE=1) on any hardware, GPU, or VM
echo "[AxisOS] Launching Native wlroots axis-compositor..." >> "$HOME/.axis-session.log"
if [ -x "/usr/local/bin/axis-compositor" ]; then
    exec /usr/local/bin/axis-compositor -s "$STARTUP_CMD" >> "$HOME/.axis-session.log" 2>&1
elif [ -x "/usr/bin/axis-compositor" ]; then
    exec /usr/bin/axis-compositor -s "$STARTUP_CMD" >> "$HOME/.axis-session.log" 2>&1
elif command -v cage >/dev/null 2>&1; then
    echo "[AxisOS] Fallback: Launching cage kiosk with installer..." >> "$HOME/.axis-session.log"
    exec cage -s -- /usr/local/bin/axis-installer >> "$HOME/.axis-session.log" 2>&1
else
    echo "[AxisOS] Ultimate fallback: launching xterm..." >> "$HOME/.axis-session.log"
    exec xterm >> "$HOME/.axis-session.log" 2>&1
fi
