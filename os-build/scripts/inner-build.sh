#!/usr/bin/env bash
# ==============================================================================
# AxisOS Linux - Live-Build Environment & Hybrid ISO Generator
# Runs inside the Debian builder container or native live-build environment
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="${WORKSPACE_DIR:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
CHROOT_CONFIG_DIR="$WORKSPACE_DIR/os-build/configs"
SCRIPTS_DIR="$WORKSPACE_DIR/os-build/scripts"
BUILD_DIR="${BUILD_DIR:-/build}"

echo "=================================================="
echo "      AxisOS Chroot & Bootable ISO Generator"
echo "=================================================="
echo "Workspace: $WORKSPACE_DIR"
echo "Build Dir: $BUILD_DIR"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Apply live-build patches for Debian 12 Bookworm compatibility
if [ -f /usr/lib/live/build/lb_chroot_archives ]; then
    sed -i 's|/updates|-security|g' /usr/lib/live/build/lb_chroot_archives 2>/dev/null || true
fi
if [ -f "$SCRIPTS_DIR/patch-live-build.py" ]; then
    python3 "$SCRIPTS_DIR/patch-live-build.py"
fi

# Prepare host rsvg wrapper
cp "$SCRIPTS_DIR/rsvg-wrapper.sh" /usr/bin/rsvg
chmod +x /usr/bin/rsvg

# Setup real syslinux/isolinux bootloader binaries in live-build data
rm -f /usr/share/live/build/bootloaders/isolinux/isolinux.bin
rm -f /usr/share/live/build/bootloaders/isolinux/vesamenu.c32
rm -f /usr/share/live/build/bootloaders/syslinux/vesamenu.c32

cp /usr/lib/ISOLINUX/isolinux.bin /usr/share/live/build/bootloaders/isolinux/isolinux.bin 2>/dev/null || true
cp /usr/lib/syslinux/modules/bios/vesamenu.c32 /usr/share/live/build/bootloaders/isolinux/vesamenu.c32 2>/dev/null || true
cp /usr/lib/syslinux/modules/bios/ldlinux.c32 /usr/share/live/build/bootloaders/isolinux/ldlinux.c32 2>/dev/null || true
cp /usr/lib/syslinux/modules/bios/libutil.c32 /usr/share/live/build/bootloaders/isolinux/libutil.c32 2>/dev/null || true
cp /usr/lib/syslinux/modules/bios/libcom32.c32 /usr/share/live/build/bootloaders/isolinux/libcom32.c32 2>/dev/null || true

cp /usr/lib/syslinux/modules/bios/vesamenu.c32 /usr/share/live/build/bootloaders/syslinux/vesamenu.c32 2>/dev/null || true
cp /usr/lib/syslinux/modules/bios/ldlinux.c32 /usr/share/live/build/bootloaders/syslinux/ldlinux.c32 2>/dev/null || true
cp /usr/lib/syslinux/modules/bios/libutil.c32 /usr/share/live/build/bootloaders/syslinux/libutil.c32 2>/dev/null || true
cp /usr/lib/syslinux/modules/bios/libcom32.c32 /usr/share/live/build/bootloaders/syslinux/libcom32.c32 2>/dev/null || true

# Save existing downloaded deb packages to save download time
mkdir -p /tmp/axisos-deb-cache
cp -r "$BUILD_DIR"/cache/packages.* /tmp/axisos-deb-cache/ 2>/dev/null || true

# Clean any previous broken stages to ensure deterministic build
lb clean --purge || true
rm -rf "$BUILD_DIR"/chroot "$BUILD_DIR"/binary "$BUILD_DIR"/.build "$BUILD_DIR"/config "$BUILD_DIR"/.lock

# Restore package cache into fresh build directory
mkdir -p "$BUILD_DIR"/cache
cp -r /tmp/axisos-deb-cache/* "$BUILD_DIR"/cache/ 2>/dev/null || true

# Configure live-build with live-boot parameters and UEFI+BIOS hybrid bootloader
lb config \
    --mode debian \
    --distribution bookworm \
    --architecture amd64 \
    --archive-areas "main contrib non-free non-free-firmware" \
    --mirror-bootstrap "http://deb.debian.org/debian/" \
    --parent-mirror-bootstrap "http://deb.debian.org/debian/" \
    --mirror-chroot "http://deb.debian.org/debian/" \
    --parent-mirror-chroot "http://deb.debian.org/debian/" \
    --mirror-chroot-security "http://deb.debian.org/debian-security/" \
    --parent-mirror-chroot-security "http://deb.debian.org/debian-security/" \
    --mirror-binary "http://deb.debian.org/debian/" \
    --parent-mirror-binary "http://deb.debian.org/debian/" \
    --mirror-binary-security "http://deb.debian.org/debian-security/" \
    --parent-mirror-binary-security "http://deb.debian.org/debian-security/" \
    --cache true \
    --cache-packages true \
    --cache-stages none \
    --bootloader syslinux \
    --bootappend-live "boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash" \
    --security true \
    --iso-application "AxisOS Linux 1.0 Horizon" \
    --iso-publisher "AxisOS Project" \
    --iso-volume "AXISOS_LIVE" \
    --firmware-chroot false \
    --firmware-binary false \
    --memtest none

# Bootloader custom template setup
mkdir -p config/bootloaders/isolinux
cp -r /usr/share/live/build/bootloaders/isolinux/* config/bootloaders/isolinux/
mkdir -p config/bootloaders/syslinux
cp -r /usr/share/live/build/bootloaders/syslinux/* config/bootloaders/syslinux/

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
mkdir -p config/includes.chroot/usr/bin
mkdir -p config/includes.chroot/etc/axis
mkdir -p config/includes.chroot/var/lib/axis/cache

# Deploy Axis Package Manager CLI
cp "$WORKSPACE_DIR/pkg-mgr/bin/axis" config/includes.chroot/usr/bin/axis
chmod +x config/includes.chroot/usr/bin/axis
cp "$WORKSPACE_DIR/pkg-mgr/configs/axis.conf" config/includes.chroot/etc/axis/axis.conf

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

# Inject rsvg wrapper and syslinux module compatibility into chroot
mkdir -p config/includes.chroot/usr/bin
cp "$SCRIPTS_DIR/rsvg-wrapper.sh" config/includes.chroot/usr/bin/rsvg
chmod +x config/includes.chroot/usr/bin/rsvg

mkdir -p config/includes.chroot/usr/lib/syslinux
cp /usr/lib/ISOLINUX/isolinux.bin config/includes.chroot/usr/lib/syslinux/isolinux.bin 2>/dev/null || true
cp /usr/lib/syslinux/modules/bios/*.c32 config/includes.chroot/usr/lib/syslinux/ 2>/dev/null || true

# Also sync directly to existing chroot if available
if [ -d "$BUILD_DIR/chroot" ]; then
    cp "$SCRIPTS_DIR/rsvg-wrapper.sh" "$BUILD_DIR/chroot/usr/bin/rsvg" 2>/dev/null || true
    chmod +x "$BUILD_DIR/chroot/usr/bin/rsvg" 2>/dev/null || true
    mkdir -p "$BUILD_DIR/chroot/usr/lib/syslinux"
    cp /usr/lib/ISOLINUX/isolinux.bin "$BUILD_DIR/chroot/usr/lib/syslinux/isolinux.bin" 2>/dev/null || true
    cp /usr/lib/syslinux/modules/bios/*.c32 "$BUILD_DIR/chroot/usr/lib/syslinux/" 2>/dev/null || true
    which isohybrid && cp $(which isohybrid) "$BUILD_DIR/chroot/usr/bin/isohybrid" 2>/dev/null || true
fi

# Ensure isohybrid binary is packaged into chroot
if which isohybrid >/dev/null 2>&1; then
    cp $(which isohybrid) config/includes.chroot/usr/bin/isohybrid 2>/dev/null || true
    chmod +x config/includes.chroot/usr/bin/isohybrid 2>/dev/null || true
fi

# 7. Execute live-build
echo "=== Running lb build ==="
lb build

# 7. Copy output ISO
OUTPUT_ISO=$(ls live-image-amd64.hybrid.iso binary.hybrid.iso chroot/binary.hybrid.iso *.iso 2>/dev/null | head -n1)
if [ -n "$OUTPUT_ISO" ] && [ -f "$OUTPUT_ISO" ]; then
    which isohybrid >/dev/null 2>&1 && isohybrid "$OUTPUT_ISO" 2>/dev/null || true
    cp "$OUTPUT_ISO" "$WORKSPACE_DIR/axisos-live-amd64.iso"
    echo "=================================================="
    echo " SUCCESS: AxisOS Bootable Hybrid ISO generated!"
    echo " Location: $WORKSPACE_DIR/axisos-live-amd64.iso"
    echo "=================================================="
else
    echo "Error: Output ISO was not generated."
    exit 1
fi
