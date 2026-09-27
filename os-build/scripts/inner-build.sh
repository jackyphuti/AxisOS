#!/usr/bin/env bash
set -e

WORKSPACE_DIR="/workspace"
CHROOT_CONFIG_DIR="$WORKSPACE_DIR/os-build/configs"

echo "=== AxisOS Chroot & ISO Generator ==="
cd /build

lb config \
    --distribution bookworm \
    --architecture amd64 \
    --archive-areas "main contrib non-free non-free-firmware" \
    --bootloader grub-efi \
    --security true \
    --updates true \
    --iso-application "AxisOS Linux 1.0" \
    --iso-publisher "AxisOS Project" \
    --iso-volume "AXISOS_LIVE" \
    --memtest none

# Copy package lists
mkdir -p config/package-lists
cp "$CHROOT_CONFIG_DIR/live-build/package-lists/axisos.list.chroot" config/package-lists/

# Copy custom shell files into chroot /opt/axisos-shell
mkdir -p config/includes.chroot/opt/axisos-shell
cp -r "$WORKSPACE_DIR/shell/dist/"* config/includes.chroot/opt/axisos-shell/

# Copy systemd service and kiosk launcher
mkdir -p config/includes.chroot/etc/systemd/system
mkdir -p config/includes.chroot/usr/local/bin
cp "$CHROOT_CONFIG_DIR/cage-session/axisos.service" config/includes.chroot/etc/systemd/system/
cp "$CHROOT_CONFIG_DIR/cage-session/axisos-kiosk.sh" config/includes.chroot/usr/local/bin/
chmod +x config/includes.chroot/usr/local/bin/axisos-kiosk.sh

# Enable axisos session service
mkdir -p config/includes.chroot/etc/systemd/system/graphical.target.wants
ln -sf /etc/systemd/system/axisos.service config/includes.chroot/etc/systemd/system/graphical.target.wants/axisos.service

echo "=== Running lb build ==="
lb build

if [ -f live-image-amd64.hybrid.iso ]; then
    cp live-image-amd64.hybrid.iso "$WORKSPACE_DIR/axisos-live-amd64.iso"
    echo "=== AxisOS Live ISO built successfully: $WORKSPACE_DIR/axisos-live-amd64.iso ==="
fi
