#!/usr/bin/env bash
# ==============================================================================
# AxisOS Linux - Live-Build Environment & Hybrid ISO Generator
# Runs inside the Debian builder container or native live-build environment
# ==============================================================================
set -e

WORKSPACE_DIR="/workspace"
CHROOT_CONFIG_DIR="$WORKSPACE_DIR/os-build/configs"
SCRIPTS_DIR="$WORKSPACE_DIR/os-build/scripts"

echo "=================================================="
echo "      AxisOS Chroot & Bootable ISO Generator"
echo "=================================================="

cd /build

# Clean any previous build artifacts
lb clean --purge || true

# Configure live-build with live-boot parameters and UEFI+BIOS hybrid bootloader
lb config \
    --distribution bookworm \
    --architecture amd64 \
    --archive-areas "main contrib non-free non-free-firmware" \
    --bootloader "syslinux,grub-efi" \
    --bootappend-live "boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash" \
    --security true \
    --updates true \
    --iso-application "AxisOS Linux 1.0 Horizon" \
    --iso-publisher "AxisOS Project" \
    --iso-volume "AXISOS_LIVE" \
    --memtest none

# 1. Package lists
mkdir -p config/package-lists
cp "$CHROOT_CONFIG_DIR/live-build/package-lists/axisos.list.chroot" config/package-lists/

# 2. Deploy custom shell bundle into /opt/axisos-shell
mkdir -p config/includes.chroot/opt/axisos-shell
if [ -d "$WORKSPACE_DIR/shell/dist" ]; then
    cp -r "$WORKSPACE_DIR/shell/dist" config/includes.chroot/opt/axisos-shell/
fi
if [ -d "$WORKSPACE_DIR/shell/electron" ]; then
    cp -r "$WORKSPACE_DIR/shell/electron" config/includes.chroot/opt/axisos-shell/
fi
cp "$WORKSPACE_DIR/shell/package.json" config/includes.chroot/opt/axisos-shell/ 2>/dev/null || true

# 3. System binaries & scripts
mkdir -p config/includes.chroot/usr/local/bin
cp "$SCRIPTS_DIR/axisos-install.sh" config/includes.chroot/usr/local/bin/axisos-installer.sh
cp "$CHROOT_CONFIG_DIR/cage-session/axisos-daemon.cjs" config/includes.chroot/usr/local/bin/axisos-daemon.cjs
cp "$CHROOT_CONFIG_DIR/cage-session/axisos-kiosk.sh" config/includes.chroot/usr/local/bin/axisos-kiosk.sh
chmod +x config/includes.chroot/usr/local/bin/axisos-installer.sh
chmod +x config/includes.chroot/usr/local/bin/axisos-daemon.cjs
chmod +x config/includes.chroot/usr/local/bin/axisos-kiosk.sh

# 4. Systemd services
mkdir -p config/includes.chroot/etc/systemd/system
cp "$CHROOT_CONFIG_DIR/cage-session/axisos.service" config/includes.chroot/etc/systemd/system/
cp "$CHROOT_CONFIG_DIR/cage-session/axisos-daemon.service" config/includes.chroot/etc/systemd/system/

# Enable services in graphical target and multi-user target
mkdir -p config/includes.chroot/etc/systemd/system/graphical.target.wants
mkdir -p config/includes.chroot/etc/systemd/system/multi-user.target.wants
ln -sf /etc/systemd/system/axisos.service config/includes.chroot/etc/systemd/system/graphical.target.wants/axisos.service
ln -sf /etc/systemd/system/axisos-daemon.service config/includes.chroot/etc/systemd/system/multi-user.target.wants/axisos-daemon.service

# 5. Sudoers & Polkit rules for live session
mkdir -p config/includes.chroot/etc/sudoers.d
echo "axis ALL=(ALL) NOPASSWD: ALL" > config/includes.chroot/etc/sudoers.d/axis
chmod 0440 config/includes.chroot/etc/sudoers.d/axis

mkdir -p config/includes.chroot/etc/polkit-1/rules.d
cat << 'EOF' > config/includes.chroot/etc/polkit-1/rules.d/49-axisos.rules
// Allow axis user and sudo group to perform power operations and storage management
polkit.addRule(function(action, subject) {
    if ((action.id.indexOf("org.freedesktop.login1.") === 0 ||
         action.id.indexOf("org.freedesktop.udisks2.") === 0) &&
        (subject.isInGroup("sudo") || subject.user === "axis")) {
        return polkit.Result.YES;
    }
});
EOF

# 6. Automatic background updates configuration
mkdir -p config/includes.chroot/etc/apt/apt.conf.d
cat << 'EOF' > config/includes.chroot/etc/apt/apt.conf.d/20auto-upgrades
APT::Periodic::Update-Package-Lists "1";
APT::Periodic::Download-Upgradeable-Packages "1";
APT::Periodic::AutocleanInterval "7";
APT::Periodic::Unattended-Upgrade "1";
EOF

# 6. Execute live-build
echo "=== Running lb build ==="
lb build

# 7. Copy output ISO
OUTPUT_ISO=$(ls live-image-amd64.hybrid.iso 2>/dev/null || ls *.iso 2>/dev/null | head -n1)
if [ -n "$OUTPUT_ISO" ] && [ -f "$OUTPUT_ISO" ]; then
    cp "$OUTPUT_ISO" "$WORKSPACE_DIR/axisos-live-amd64.iso"
    echo "=================================================="
    echo " SUCCESS: AxisOS Bootable Hybrid ISO generated!"
    echo " Location: $WORKSPACE_DIR/axisos-live-amd64.iso"
    echo "=================================================="
else
    echo "Error: Output ISO was not generated."
    exit 1
fi
