#!/usr/bin/env bash
# ==============================================================================
# AxisOS Linux - Live-Build Environment & Hybrid ISO Generator (Pure C)
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="${WORKSPACE_DIR:-$(cd "$SCRIPT_DIR/../.." && pwd)}"
CHROOT_CONFIG_DIR="$WORKSPACE_DIR/os-build/configs"
SCRIPTS_DIR="$WORKSPACE_DIR/os-build/scripts"
BUILD_DIR="${BUILD_DIR:-$WORKSPACE_DIR/os-build/work}"

echo "=================================================="
echo "   AxisOS Chroot & Bootable ISO Generator (Pure C)"
echo "=================================================="
echo "Workspace: $WORKSPACE_DIR"
echo "Build Dir: $BUILD_DIR"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 1. Setup live-build base configuration
if [ ! -d "$BUILD_DIR/auto" ]; then
    mkdir -p "$BUILD_DIR/auto"
fi

# Clean previous build artifacts (preserve package cache for speed)
if command -v lb >/dev/null 2>&1; then
    lb clean || true
fi

# 1b. Compile 100% Pure C binaries natively within Debian Bookworm environment
if command -v gcc >/dev/null 2>&1 && [ -f "$WORKSPACE_DIR/os-build/scripts/compile-native-apps.sh" ]; then
    echo "--> [Container Pre-Build] Compiling 100% Pure C binaries natively for Debian 12 Bookworm..."
    bash "$WORKSPACE_DIR/os-build/scripts/compile-native-apps.sh"
fi

mkdir -p "$BUILD_DIR/config/package-lists"
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/local/bin"
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/applications"
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/skel/.config"

# 2. Copy Package Lists
if [ -d "$CHROOT_CONFIG_DIR/package-lists" ]; then
    cp -r "$CHROOT_CONFIG_DIR"/package-lists/*.list.chroot "$BUILD_DIR/config/package-lists/" 2>/dev/null || true
fi
if [ -d "$CHROOT_CONFIG_DIR/live-build/package-lists" ]; then
    cp -r "$CHROOT_CONFIG_DIR"/live-build/package-lists/*.list.chroot "$BUILD_DIR/config/package-lists/" 2>/dev/null || true
fi

# 3. Copy Precompiled Pure C Applications & Shell
echo "--> Staging Pure C Applications into chroot filesystem..."
for app_dir in "$WORKSPACE_DIR"/os-build/apps/*; do
    if [ -d "$app_dir" ]; then
        app_name="$(basename "$app_dir")"
        if [ -x "$app_dir/$app_name" ]; then
            cp -f "$app_dir/$app_name" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
        fi
        if [ -f "$app_dir/$app_name.desktop" ]; then
            cp -f "$app_dir/$app_name.desktop" "$BUILD_DIR/config/includes.chroot/usr/share/applications/"
        fi
    fi
done

if [ -x "$WORKSPACE_DIR/os-build/apps/axis-settings/axisos-settings-panel" ]; then
    cp -f "$WORKSPACE_DIR/os-build/apps/axis-settings/axisos-settings-panel" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
    ln -sf axisos-settings-panel "$BUILD_DIR/config/includes.chroot/usr/local/bin/axis-settings" || true
fi

if [ -x "$WORKSPACE_DIR/os-build/apps/nobara-welcome/nobara-welcome" ]; then
    cp -f "$WORKSPACE_DIR/os-build/apps/nobara-welcome/nobara-welcome" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
    ln -sf nobara-welcome "$BUILD_DIR/config/includes.chroot/usr/local/bin/axis-hardware-manager" || true
fi

# 4. Copy Desktop Shell (shell-c) & Wayland Compositor (compositor)
if [ -x "$WORKSPACE_DIR/shell-c/axisos-shell" ]; then
    cp -f "$WORKSPACE_DIR/shell-c/axisos-shell" "$BUILD_DIR/config/includes.chroot/usr/local/bin/axisos-shell"
    chmod 755 "$BUILD_DIR/config/includes.chroot/usr/local/bin/axisos-shell"
    ln -sf axisos-shell "$BUILD_DIR/config/includes.chroot/usr/local/bin/axis-shell" || true
    ln -sf axisos-shell "$BUILD_DIR/config/includes.chroot/usr/local/bin/LiquidNodesOS" || true
fi

if [ -x "$WORKSPACE_DIR/compositor/axis-compositor" ]; then
    cp -f "$WORKSPACE_DIR/compositor/axis-compositor" "$BUILD_DIR/config/includes.chroot/usr/local/bin/axis-compositor"
fi

# 5. Copy System Installation Engine & Real Disk Partitioner
if [ -x "$WORKSPACE_DIR/os-build/scripts/axis-disk-info" ]; then
    cp -f "$WORKSPACE_DIR/os-build/scripts/axis-disk-info" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
fi
if [ -f "$WORKSPACE_DIR/os-build/scripts/axisos-installer.sh" ]; then
    cp -f "$WORKSPACE_DIR/os-build/scripts/axisos-installer.sh" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
    chmod +x "$BUILD_DIR/config/includes.chroot/usr/local/bin/axisos-installer.sh"
    ln -sf axisos-installer.sh "$BUILD_DIR/config/includes.chroot/usr/local/bin/axisos-install.sh" || true
fi

# 5b. Copy AxisOS System Helper Daemons & Tools
for helper in axis-game-boost axis-game-runner axis-update-notifier axis-autotimezone gpu-autodetect axis-biometrics axis-fastboot-patch axis-captive-portal axis-rewind; do
    if [ -x "$WORKSPACE_DIR/os-build/scripts/$helper" ]; then
        cp -f "$WORKSPACE_DIR/os-build/scripts/$helper" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
        ln -sf "$helper" "$BUILD_DIR/config/includes.chroot/usr/local/bin/$helper.sh" || true
    fi
done

# 6. Copy Updater and Package Manage
if [ -x "$WORKSPACE_DIR/os-build/updater/axis-update" ]; then
    cp -f "$WORKSPACE_DIR/os-build/updater/axis-update" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
fi
if [ -x "$WORKSPACE_DIR/pkg-mgr/bin/axis" ]; then
    cp -f "$WORKSPACE_DIR/pkg-mgr/bin/axis" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
fi
if [ -x "$WORKSPACE_DIR/pkg-mgr/bin/axis_repo_builder" ]; then
    cp -f "$WORKSPACE_DIR/pkg-mgr/bin/axis_repo_builder" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
fi

# 7. Copy Design Theme, Wallpapers, and Icons
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/axisos/theme"
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/axis/theme"
if [ -d "$WORKSPACE_DIR/data/theme" ]; then
    cp -rf "$WORKSPACE_DIR"/data/theme/* "$BUILD_DIR/config/includes.chroot/usr/share/axisos/theme/" 2>/dev/null || true
    cp -rf "$WORKSPACE_DIR"/data/theme/* "$BUILD_DIR/config/includes.chroot/usr/share/axis/theme/" 2>/dev/null || true
fi

mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/backgrounds/axisos"
if [ -d "$WORKSPACE_DIR/data/wallpapers" ]; then
    cp -rf "$WORKSPACE_DIR"/data/wallpapers/* "$BUILD_DIR/config/includes.chroot/usr/share/backgrounds/axisos/" 2>/dev/null || true
fi

mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/icons"
if [ -d "$WORKSPACE_DIR/data/icons" ]; then
    cp -rf "$WORKSPACE_DIR"/data/icons/* "$BUILD_DIR/config/includes.chroot/usr/share/icons/" 2>/dev/null || true
fi

# 7b. Copy Desktop Configurations & Emulation MIME types
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/applications"
if [ -d "$WORKSPACE_DIR/os-build/configs/desktop" ]; then
    cp -rf "$WORKSPACE_DIR"/os-build/configs/desktop/*.desktop "$BUILD_DIR/config/includes.chroot/usr/share/applications/" 2>/dev/null || true
fi

mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/mime/packages"
if [ -f "$WORKSPACE_DIR/os-build/configs/desktop/axisos-emulation.xml" ]; then
    cp -f "$WORKSPACE_DIR/os-build/configs/desktop/axisos-emulation.xml" "$BUILD_DIR/config/includes.chroot/usr/share/mime/packages/"
fi

# 7c. Pre-configure User Emulation Tree in /etc/skel
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/bios/ps1" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/bios/ps2" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/bios/ps3" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/bios/psp" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/games/ps1" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/games/ps2" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/games/ps3" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/games/psp"

# 8. Copy PAM, Polkit, Udev Rules, and Systemd Services
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/pam.d"
if [ -d "$WORKSPACE_DIR/data/pam" ]; then
    cp -rf "$WORKSPACE_DIR"/data/pam/* "$BUILD_DIR/config/includes.chroot/etc/pam.d/" 2>/dev/null || true
fi

mkdir -p "$BUILD_DIR/config/includes.chroot/etc/udev/rules.d"
if [ -d "$WORKSPACE_DIR/os-build/configs/udev" ]; then
    cp -rf "$WORKSPACE_DIR"/os-build/configs/udev/* "$BUILD_DIR/config/includes.chroot/etc/udev/rules.d/" 2>/dev/null || true
fi

mkdir -p "$BUILD_DIR/config/includes.chroot/etc/systemd/system"
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/systemd/system/graphical.target.wants"
if [ -d "$WORKSPACE_DIR/os-build/configs/systemd" ]; then
    cp -rf "$WORKSPACE_DIR"/os-build/configs/systemd/*.service "$BUILD_DIR/config/includes.chroot/etc/systemd/system/" 2>/dev/null || true
fi
if [ -d "$WORKSPACE_DIR/os-build/configs/systemd/user" ]; then
    mkdir -p "$BUILD_DIR/config/includes.chroot/etc/systemd/user"
    cp -rf "$WORKSPACE_DIR"/os-build/configs/systemd/user/* "$BUILD_DIR/config/includes.chroot/etc/systemd/user/" 2>/dev/null || true
fi
if [ -f "$WORKSPACE_DIR/os-build/configs/systemd/zram-generator.conf" ]; then
    mkdir -p "$BUILD_DIR/config/includes.chroot/etc/systemd"
    cp -f "$WORKSPACE_DIR/os-build/configs/systemd/zram-generator.conf" "$BUILD_DIR/config/includes.chroot/etc/systemd/" 2>/dev/null || true
fi
if [ -f "$WORKSPACE_DIR/os-build/configs/cage-session/axisos.service" ]; then
    cp -f "$WORKSPACE_DIR/os-build/configs/cage-session/axisos.service" "$BUILD_DIR/config/includes.chroot/etc/systemd/system/"
    ln -sf /etc/systemd/system/axisos.service "$BUILD_DIR/config/includes.chroot/etc/systemd/system/graphical.target.wants/axisos.service" || true
fi

# Deploy GTK 3 Global Dark Theme settings
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/gtk-3.0"
if [ -f "$WORKSPACE_DIR/os-build/configs/gtk-3.0/settings.ini" ]; then
    cp -f "$WORKSPACE_DIR/os-build/configs/gtk-3.0/settings.ini" "$BUILD_DIR/config/includes.chroot/etc/gtk-3.0/settings.ini"
fi

# Polkit rules for hardware & desktop administrative privileges
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/polkit-1/rules.d"
cat << 'EOF' > "$BUILD_DIR/config/includes.chroot/etc/polkit-1/rules.d/10-axisos-admin.rules"
polkit.addRule(function(action, subject) {
    if (subject.isInGroup("sudo") || subject.user == "axis") {
        return polkit.Result.YES;
    }
});
EOF

mkdir -p "$BUILD_DIR/config/includes.chroot/etc/xdg"
if [ -d "$WORKSPACE_DIR/os-build/configs/wayland" ]; then
    for cfg in foot mako waybar wofi; do
        if [ -d "$WORKSPACE_DIR/os-build/configs/wayland/$cfg" ]; then
            cp -rf "$WORKSPACE_DIR/os-build/configs/wayland/$cfg" "$BUILD_DIR/config/includes.chroot/etc/xdg/"
        fi
    done
fi

# 9. Copy Wayland Kiosk Session configuration
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/wayland-sessions"
if [ -f "$CHROOT_CONFIG_DIR/wayland/liquidnodes.desktop" ]; then
    cp -f "$CHROOT_CONFIG_DIR/wayland/liquidnodes.desktop" "$BUILD_DIR/config/includes.chroot/usr/share/wayland-sessions/"
fi
if [ -f "$CHROOT_CONFIG_DIR/cage-session/axisos-kiosk.sh" ]; then
    cp -f "$CHROOT_CONFIG_DIR/cage-session/axisos-kiosk.sh" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
    chmod +x "$BUILD_DIR/config/includes.chroot/usr/local/bin/axisos-kiosk.sh"
fi

# 10. Install Chroot Setup Hooks
mkdir -p "$BUILD_DIR/config/hooks/live"
cat << 'EOF' > "$BUILD_DIR/config/hooks/live/0050-setup-axisos.hook.chroot"
#!/bin/sh
set -e

# Create AxisOS user if not exists
if ! id -u axis >/dev/null 2>&1; then
    useradd -m -s /bin/bash -u 1000 -U -G sudo,video,render,input,audio,plugdev,users,disk axis || true
    echo "axis:axis" | chpasswd
fi

# Ensure required hardware groups exist and axis user is member
for grp in video render input audio plugdev users disk seat _seatd tty; do
    groupadd -f "$grp" 2>/dev/null || true
    usermod -aG "$grp" axis 2>/dev/null || true
done

# Passwordless PAM autologin configuration for direct Wayland session
cat << 'PAM_EOF' > /etc/pam.d/axisos-autologin
#%PAM-1.0
auth        sufficient  pam_permit.so
account     sufficient  pam_permit.so
password    sufficient  pam_permit.so
session     required    pam_loginuid.so
session     required    pam_limits.so
session     required    pam_systemd.so
session     optional    pam_env.so readenv=1
session     optional    pam_env.so readenv=1 envfile=/etc/default/locale
PAM_EOF

# Passwordless sudo for axis user
mkdir -p /etc/sudoers.d
echo "axis ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/axis
chmod 0440 /etc/sudoers.d/axis

# Ensure /home/axis skeleton and permissions
mkdir -p /home/axis/.config/gtk-3.0 /home/axis/Desktop /home/axis/Downloads
if [ -f /etc/gtk-3.0/settings.ini ]; then
    cp -f /etc/gtk-3.0/settings.ini /home/axis/.config/gtk-3.0/settings.ini
fi
chown -R 1000:1000 /home/axis 2>/dev/null || true

# Systemd target and service configuration
systemctl enable axisos.service 2>/dev/null || true
systemctl enable seatd.service 2>/dev/null || true
systemctl disable lightdm.service 2>/dev/null || true
systemctl mask lightdm.service 2>/dev/null || true
systemctl set-default graphical.target 2>/dev/null || true

# Ensure all binaries in /usr/local/bin are executable
chmod 755 /usr/local/bin/* 2>/dev/null || true

# Clean up any runtime/temporary directories before squashing
rm -rf /run/* /run/user /tmp/* /var/tmp/* /var/log/* 2>/dev/null || true
mkdir -p /run/lock
chmod 1777 /tmp /var/tmp 2>/dev/null || true
chmod 755 /run 2>/dev/null || true
EOF
chmod +x "$BUILD_DIR/config/hooks/live/0050-setup-axisos.hook.chroot"

# 11. Run live-build config & build
if command -v lb >/dev/null 2>&1; then
    lb config \
        --distribution bookworm \
        --architecture amd64 \
        --archive-areas "main contrib non-free non-free-firmware" \
        --linux-flavours amd64 \
        --binary-images iso-hybrid \
        --bootloaders "syslinux,grub-efi" \
        --bootappend-live "boot=live components username=axis user-fullname=AxisOS hostname=axisos quiet splash" \
        --iso-application "AxisOS Pure C Linux" \
        --iso-publisher "AxisOS Team" \
        --iso-volume "AxisOS_Live" \
        --chroot-squashfs-compression-type zstd \
        --chroot-squashfs-compression-level 6 \
        --memtest none \
        --verbose

    lb build 2>&1 | tee "$BUILD_DIR/build.log"
    cp -f "$BUILD_DIR"/live-image-amd64.hybrid.iso "$WORKSPACE_DIR/axisos-live-amd64.iso" 2>/dev/null || true
    cp -f "$BUILD_DIR"/*.iso "$WORKSPACE_DIR/axisos-live-amd64.iso" 2>/dev/null || true
fi

if [ -f "$WORKSPACE_DIR/axisos-live-amd64.iso" ]; then
    echo "SUCCESS: AxisOS Live ISO generated successfully!"
    echo "Location: $WORKSPACE_DIR/axisos-live-amd64.iso"
    ls -lh "$WORKSPACE_DIR/axisos-live-amd64.iso"
fi

echo "=================================================="
echo "   AxisOS Chroot & Build Pipeline Completed!      "
echo "=================================================="
