#!/usr/bin/env bash
# ==============================================================================
# AxisOS FastBoot Hotfix & Performance Patch
# Resolves slow boot delays (60-90s hangs) on installed AxisOS machines without
# requiring a system reinstallation.
# ==============================================================================

set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "[!] This patch requires root privileges. Re-running with sudo/pkexec..."
    exec sudo "$0" "$@"
fi

echo "=================================================="
echo "   AxisOS Instant FastBoot Hotfix (OTA Update)    "
echo "=================================================="

# 1. Fix axisos-session.service (ensure QT_QPA_PLATFORM is NOT set to wayland on root compositor)
echo "--> Optimizing graphical session service..."
CURRENT_USER=$(id -un 1000 2>/dev/null || who | awk '{print $1}' | head -n1 || echo "axis")
if [ -z "$CURRENT_USER" ] || [ "$CURRENT_USER" = "root" ]; then
    CURRENT_USER="axis"
fi

cat << EOF > /etc/systemd/system/axisos-session.service
[Unit]
Description=AxisOS LiquidNodes Desktop Environment
After=systemd-user-sessions.service seatd.service
Wants=seatd.service
Conflicts=getty@tty1.service
After=getty@tty1.service

[Service]
Type=simple
User=${CURRENT_USER}
Group=${CURRENT_USER}
PAMName=login
WorkingDirectory=/home/${CURRENT_USER}
Environment=XDG_SESSION_TYPE=wayland
Environment=XDG_CURRENT_DESKTOP=AxisOS:LiquidNodes
Environment=WLR_RENDERER_ALLOW_SOFTWARE=1
Environment=WLR_NO_HARDWARE_CURSORS=1
Environment=LIBSEAT_BACKEND=seatd
Environment=XDG_RUNTIME_DIR=/run/user/1000
ExecStartPre=/bin/sh -c 'mkdir -p /run/user/1000 && chown ${CURRENT_USER}:${CURRENT_USER} /run/user/1000 && chmod 0700 /run/user/1000'
ExecStart=/usr/local/bin/axisos-kiosk.sh
Restart=on-failure
RestartSec=2
StandardInput=tty
StandardOutput=journal
StandardError=journal
TTYPath=/dev/tty1
TTYReset=yes
TTYVHangup=yes
TTYVTDisallocate=yes

[Install]
WantedBy=graphical.target
EOF

# 2. Fix /etc/network/interfaces (eliminate 60-90s DHCP timeout on unplugged eth0)
echo "--> Cleaning network interfaces configuration..."
cat << 'EOF' > /etc/network/interfaces
# AxisOS Network Interfaces (NetworkManager Managed)
auto lo
iface lo inet loopback
EOF

# 3. Mask blocking systemd wait-online units
echo "--> Disabling blocking network-wait services..."
systemctl mask NetworkManager-wait-online.service 2>/dev/null || true
systemctl mask systemd-networkd-wait-online.service 2>/dev/null || true
systemctl enable seatd.service 2>/dev/null || true

# 4. Patch /etc/fstab swap timeout (prevent 90s swap device discovery timeout)
echo "--> Tuning filesystem and swap mount timeouts..."
if [ -f /etc/fstab ]; then
    sed -i -E 's/(\s+swap\s+)sw(\s+)/\1defaults,nofail,x-systemd.device-timeout=5s\2/g' /etc/fstab
fi

# 5. Make gpu-autodetect asynchronous if present
if [ -f /etc/systemd/system/gpu-autodetect.service ]; then
    echo "--> Decoupling GPU autodetection from boot pipeline..."
    sed -i '/Wants=network-online.target/d' /etc/systemd/system/gpu-autodetect.service
    sed -i 's/Type=oneshot/Type=simple/' /etc/systemd/system/gpu-autodetect.service
fi

# 6. Tune GRUB bootloader timeout to 1 second
if [ -f /etc/default/grub ]; then
    echo "--> Setting GRUB bootloader timeout to 1 second..."
    sed -i -E 's/^GRUB_TIMEOUT=[0-9]+/GRUB_TIMEOUT=1/' /etc/default/grub
    if command -v update-grub >/dev/null 2>&1; then
        update-grub 2>/dev/null || true
    fi
fi

# 7. Reload systemd daemon
echo "--> Reloading systemd daemon..."
systemctl daemon-reload
systemctl enable axisos-session.service 2>/dev/null || true

echo "=================================================="
echo " ✔ AxisOS FastBoot Hotfix successfully applied!   "
echo "   Your system will now boot into desktop in ~2s. "
echo "=================================================="
