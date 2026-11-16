#!/bin/sh
# ==============================================================================
# AxisOS 2.1 - Pure C Wayland Desktop Session Launcher
# Launches native wlroots axis-compositor with pure C desktop shell and GTK 3 apps
# ==============================================================================

set -u

# 1. Setup XDG runtime directory safely without touching /run/user/1000/systemd/inaccessible
UID_NUM=$(id -u)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/${UID_NUM}}"
if [ ! -d "$XDG_RUNTIME_DIR" ] || [ ! -w "$XDG_RUNTIME_DIR" ]; then
    export XDG_RUNTIME_DIR="/tmp/runtime-axis-${UID_NUM}"
    mkdir -p "$XDG_RUNTIME_DIR" 2>/dev/null || true
    chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true
fi

# 2. Configure libseat / logind session backend
# When launched via systemd PAMName=axisos-autologin, systemd-logind owns the VT/DRM master on tty1.
# Query loginctl if XDG_SESSION_ID is not present in service environment.
if [ -z "${XDG_SESSION_ID:-}" ] && command -v loginctl >/dev/null 2>&1; then
    XDG_SESSION_ID=$(loginctl session-status 2>/dev/null | head -n1 | awk '{print $1}')
    [ -n "$XDG_SESSION_ID" ] && export XDG_SESSION_ID
fi

if [ -d "/run/systemd/seats" ] || pgrep -x systemd-logind >/dev/null 2>&1; then
    export LIBSEAT_BACKEND=logind
elif [ -w "/run/seatd.sock" ]; then
    export LIBSEAT_BACKEND=seatd
else
    export LIBSEAT_BACKEND=builtin
fi

export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP="AxisOS:LiquidNodes"
export GDK_BACKEND=wayland
export WLR_RENDERER_ALLOW_SOFTWARE=1
export WLR_NO_HARDWARE_CURSORS=1
export MOZ_ENABLE_WAYLAND=1
export OZONE_PLATFORM=wayland
export SDL_VIDEODRIVER=wayland

# Auto-detect hypervisors with broken/unsupported host EGL (e.g. VirtualBox VMSVGA without 3D, VMware, QEMU)
# and select pixman CPU rasterization up front to prevent EGL initialization crashes.
IS_HYPERVISOR=0
if [ "${WLR_RENDERER:-}" = "pixman" ]; then
    IS_HYPERVISOR=1
elif grep -qa "hypervisor" /proc/cpuinfo 2>/dev/null; then
    IS_HYPERVISOR=1
elif command -v systemd-detect-virt >/dev/null 2>&1 && systemd-detect-virt --quiet; then
    IS_HYPERVISOR=1
elif grep -qa -E "VirtualBox|VMware|QEMU|KVM|Bochs|innotek" /sys/class/dmi/id/* 2>/dev/null; then
    IS_HYPERVISOR=1
elif grep -qa -E "VirtualBox|VMware|QEMU|KVM|Bochs|innotek" /sys/devices/virtual/dmi/id/* 2>/dev/null; then
    IS_HYPERVISOR=1
elif grep -qa -E "vmwgfx|vboxvideo|bochs|qxl|cirrus|virtio_gpu" /proc/modules 2>/dev/null; then
    IS_HYPERVISOR=1
elif [ -e /sys/class/drm/card0/device/driver ] && readlink /sys/class/drm/card0/device/driver 2>/dev/null | grep -qE "vmwgfx|vboxvideo|qxl|bochs|virtio"; then
    IS_HYPERVISOR=1
fi

if [ "$IS_HYPERVISOR" -eq 1 ]; then
    echo "[AxisOS] Hypervisor / virtual graphics detected; defaulting to pixman software renderer."
    export WLR_RENDERER=pixman
    export WLR_RENDERER_ALLOW_SOFTWARE=1
    export WLR_NO_HARDWARE_CURSORS=1
    export LIBGL_ALWAYS_SOFTWARE=1
fi

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

# 7. Configure Desktop Shell Component (waybar taskbar + optional axis-shell topbar)
SHELL_CMD=""
if command -v waybar >/dev/null 2>&1; then
    SHELL_CMD="waybar &"
fi
if [ -x "/usr/local/bin/axis-shell" ]; then
    SHELL_CMD="$SHELL_CMD /usr/local/bin/axis-shell &"
elif [ -x "/usr/bin/axis-shell" ]; then
    SHELL_CMD="$SHELL_CMD /usr/bin/axis-shell &"
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
    RET=0
    "$COMPOSITOR_BIN" -s "$STARTUP_CMD" || RET=$?
    if [ $RET -ne 0 ]; then
        echo "[AxisOS] Compositor exited ($RET). Retrying with pixman software rendering fallback..."
        export WLR_RENDERER=pixman
        export WLR_RENDERER_ALLOW_SOFTWARE=1
        export WLR_NO_HARDWARE_CURSORS=1
        RET=0
        "$COMPOSITOR_BIN" -s "$STARTUP_CMD" || RET=$?
        if [ $RET -ne 0 ]; then
            echo "[AxisOS] Compositor failed ($RET). Launching fallback kiosk..."
            if command -v cage >/dev/null 2>&1; then
                exec cage -s -- /usr/local/bin/$STARTUP_APP
            fi
        fi
    fi
elif command -v cage >/dev/null 2>&1; then
    echo "[AxisOS] Fallback: Launching cage with $STARTUP_APP..."
    exec cage -s -- /usr/local/bin/$STARTUP_APP
fi
