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
    lb clean --binary || true
    rm -f "$BUILD_DIR/.build/binary"* "$BUILD_DIR/.build/chroot_includes"* "$BUILD_DIR/.build/chroot_hooks"* 2>/dev/null || true
    if [ -x "$BUILD_DIR/chroot/usr/bin/plasmashell" ] && [ -x "$BUILD_DIR/chroot/usr/bin/sddm" ] && [ -x "$BUILD_DIR/chroot/usr/bin/Xwayland" ]; then
        touch "$BUILD_DIR/.build/chroot_package-lists.install" \
              "$BUILD_DIR/.build/chroot_package-lists.live" \
              "$BUILD_DIR/.build/chroot_install-packages.install" \
              "$BUILD_DIR/.build/chroot_install-packages.live" 2>/dev/null || true
    else
        rm -f "$BUILD_DIR/.build/chroot_package-lists"* "$BUILD_DIR/.build/chroot_install-packages"* 2>/dev/null || true
    fi
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
        # Never stage legacy axis-installer or axis-settings desktop entries (use Calamares / System Settings)
        if [ "$app_name" != "axis-installer" ] && [ "$app_name" != "axis-settings" ]; then
            if [ -f "$app_dir/$app_name.desktop" ]; then
                cp -f "$app_dir/$app_name.desktop" "$BUILD_DIR/config/includes.chroot/usr/share/applications/"
            fi
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
if [ -f "$WORKSPACE_DIR/os-build/scripts/install-axisos.sh" ]; then
    cp -f "$WORKSPACE_DIR/os-build/scripts/install-axisos.sh" "$BUILD_DIR/config/includes.chroot/usr/local/bin/install-axisos"
    chmod 0755 "$BUILD_DIR/config/includes.chroot/usr/local/bin/install-axisos"
    ln -sf /usr/local/bin/install-axisos "$BUILD_DIR/config/includes.chroot/usr/local/bin/install-debian" || true
fi

# 5b. Copy AxisOS System Helper Daemons & Tools
for helper in axis-game-boost axis-game-runner axis-switch-session axis-update-notifier axis-autotimezone gpu-autodetect axis-biometrics axis-fastboot-patch axis-captive-portal axis-rewind; do
    if [ -x "$WORKSPACE_DIR/os-build/scripts/$helper" ]; then
        cp -f "$WORKSPACE_DIR/os-build/scripts/$helper" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
        ln -sf "$helper" "$BUILD_DIR/config/includes.chroot/usr/local/bin/$helper.sh" || true
    fi
done

# Copy GPU Setup and Game Mode session scripts
if [ -f "$WORKSPACE_DIR/os-build/scripts/axis-gpu-setup.sh" ]; then
    cp -f "$WORKSPACE_DIR/os-build/scripts/axis-gpu-setup.sh" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
    chmod 0755 "$BUILD_DIR/config/includes.chroot/usr/local/bin/axis-gpu-setup.sh"
fi
if [ -f "$WORKSPACE_DIR/os-build/scripts/axisos-gamemode-session.sh" ]; then
    cp -f "$WORKSPACE_DIR/os-build/scripts/axisos-gamemode-session.sh" "$BUILD_DIR/config/includes.chroot/usr/local/bin/"
    chmod 0755 "$BUILD_DIR/config/includes.chroot/usr/local/bin/axisos-gamemode-session.sh"
fi

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
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/bios/psvita" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/bios/ps4" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/games/ps1" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/games/ps2" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/games/ps3" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/games/psp" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/games/psvita" \
         "$BUILD_DIR/config/includes.chroot/etc/skel/Emulation/games/ps4"

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
# Deploy SDDM display manager configuration
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/sddm.conf.d"
if [ -d "$WORKSPACE_DIR/os-build/configs/sddm" ]; then
    cp -rf "$WORKSPACE_DIR"/os-build/configs/sddm/*.conf "$BUILD_DIR/config/includes.chroot/etc/sddm.conf.d/" 2>/dev/null || true
    if [ -f "$WORKSPACE_DIR/os-build/configs/sddm/sddm.conf" ]; then
        cp -f "$WORKSPACE_DIR/os-build/configs/sddm/sddm.conf" "$BUILD_DIR/config/includes.chroot/etc/sddm.conf"
    fi
    mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/sddm/themes/breeze"
    if [ -f "$WORKSPACE_DIR/os-build/configs/sddm/theme.conf.user" ]; then
        cp -f "$WORKSPACE_DIR/os-build/configs/sddm/theme.conf.user" "$BUILD_DIR/config/includes.chroot/etc/sddm.conf.d/theme.conf.user"
        cp -f "$WORKSPACE_DIR/os-build/configs/sddm/theme.conf.user" "$BUILD_DIR/config/includes.chroot/usr/share/sddm/themes/breeze/theme.conf.user"
    fi
fi

# Deploy KDE Plasma One UI Dark configuration & color schemes
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/color-schemes"
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/xdg"
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/skel/.config"
if [ -d "$WORKSPACE_DIR/os-build/configs/kde" ]; then
    if [ -f "$WORKSPACE_DIR/os-build/configs/kde/AxisOSOneUIDark.colors" ]; then
        cp -f "$WORKSPACE_DIR/os-build/configs/kde/AxisOSOneUIDark.colors" "$BUILD_DIR/config/includes.chroot/usr/share/color-schemes/"
    fi
    for kde_cfg in kdeglobals plasmarc kwinrc kcminputrc kscreenlockerrc baloofilerc powermanagementprofilesrc; do
        if [ -f "$WORKSPACE_DIR/os-build/configs/kde/$kde_cfg" ]; then
            cp -f "$WORKSPACE_DIR/os-build/configs/kde/$kde_cfg" "$BUILD_DIR/config/includes.chroot/etc/xdg/"
            cp -f "$WORKSPACE_DIR/os-build/configs/kde/$kde_cfg" "$BUILD_DIR/config/includes.chroot/etc/skel/.config/"
        fi
    done
fi
rm -f "$BUILD_DIR/config/includes.chroot/etc/xdg/plasma-org.kde.plasma.desktop-appletsrc" \
      "$BUILD_DIR/config/includes.chroot/etc/skel/.config/plasma-org.kde.plasma.desktop-appletsrc" 2>/dev/null || true

# Plasma wallpaper package "AxisOS" (used via plasmarc defaultWallpaperTheme=AxisOS)
WP_SRC="$WORKSPACE_DIR/data/wallpapers/08-axisos-obsidian-wave.jpg"
WP_PKG="$BUILD_DIR/config/includes.chroot/usr/share/wallpapers/AxisOS"
if [ -f "$WP_SRC" ]; then
    mkdir -p "$WP_PKG/contents/images"
    for res in 1920x1080 2560x1440 3840x2160 1366x768 1280x720; do
        cp -f "$WP_SRC" "$WP_PKG/contents/images/$res.jpg"
    done
    cat << 'WPEOF' > "$WP_PKG/metadata.desktop"
[Desktop Entry]
Name=AxisOS Obsidian Wave
X-KDE-PluginInfo-Name=AxisOS
X-KDE-PluginInfo-Author=AxisOS Team
X-KDE-PluginInfo-License=CC-BY-SA-4.0
WPEOF
fi

# Deploy AxisOS system & application icons
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/icons/hicolor/scalable/apps"
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/icons/hicolor/scalable/places"
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/pixmaps"
mkdir -p "$BUILD_DIR/config/includes.chroot/usr/share/axisos/branding"
if [ -f "$WORKSPACE_DIR/data/branding/axisos-icon.svg" ]; then
    cp -f "$WORKSPACE_DIR/data/branding/axisos-icon.svg" "$BUILD_DIR/config/includes.chroot/usr/share/icons/hicolor/scalable/apps/axisos.svg"
    cp -f "$WORKSPACE_DIR/data/branding/axisos-icon.svg" "$BUILD_DIR/config/includes.chroot/usr/share/icons/hicolor/scalable/places/start-here.svg"
    cp -f "$WORKSPACE_DIR/data/branding/axisos-icon.svg" "$BUILD_DIR/config/includes.chroot/usr/share/icons/hicolor/scalable/places/start-here-kde.svg"
    cp -f "$WORKSPACE_DIR/data/branding/axisos-icon.svg" "$BUILD_DIR/config/includes.chroot/usr/share/pixmaps/axisos.svg"
    cp -f "$WORKSPACE_DIR/data/branding/axisos-icon.svg" "$BUILD_DIR/config/includes.chroot/usr/share/pixmaps/klogo.svg" || true
    cp -f "$WORKSPACE_DIR/data/branding/axisos-icon.svg" "$BUILD_DIR/config/includes.chroot/usr/share/pixmaps/distributor-logo.svg" || true
fi
if [ -d "$WORKSPACE_DIR/data/branding" ]; then
    cp -rf "$WORKSPACE_DIR/data/branding"/* "$BUILD_DIR/config/includes.chroot/usr/share/axisos/branding/" 2>/dev/null || true
fi

# Pre-stage Live Desktop shortcuts in /etc/skel/Desktop (Calamares Installer, System Settings, Terminal, Welcome)
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/skel/Desktop"
if [ -f "$WORKSPACE_DIR/os-build/configs/desktop/install-axisos.desktop" ]; then
    cp -f "$WORKSPACE_DIR/os-build/configs/desktop/install-axisos.desktop" "$BUILD_DIR/config/includes.chroot/etc/skel/Desktop/"
    cp -f "$WORKSPACE_DIR/os-build/configs/desktop/install-axisos.desktop" "$BUILD_DIR/config/includes.chroot/usr/share/applications/"
    cp -f "$WORKSPACE_DIR/os-build/configs/desktop/install-debian.desktop" "$BUILD_DIR/config/includes.chroot/usr/share/applications/install-debian.desktop"
    cp -f "$WORKSPACE_DIR/os-build/configs/desktop/calamares.desktop" "$BUILD_DIR/config/includes.chroot/usr/share/applications/calamares.desktop"
fi
if [ -f "/usr/share/applications/systemsettings.desktop" ]; then
    cp -f "/usr/share/applications/systemsettings.desktop" "$BUILD_DIR/config/includes.chroot/etc/skel/Desktop/" 2>/dev/null || true
fi
for d_app in axis-terminal axis-welcome; do
    if [ -f "$BUILD_DIR/config/includes.chroot/usr/share/applications/$d_app.desktop" ]; then
        cp -f "$BUILD_DIR/config/includes.chroot/usr/share/applications/$d_app.desktop" "$BUILD_DIR/config/includes.chroot/etc/skel/Desktop/"
    fi
done
chmod +x "$BUILD_DIR/config/includes.chroot/etc/skel/Desktop"/*.desktop 2>/dev/null || true
rm -f "$BUILD_DIR/config/includes.chroot/etc/skel/Desktop/calamares.desktop" \
      "$BUILD_DIR/config/includes.chroot/etc/skel/Desktop/install-debian.desktop" \
      "$BUILD_DIR/config/includes.chroot/etc/skel/Desktop/axis-installer.desktop" \
      "$BUILD_DIR/config/includes.chroot/etc/skel/Desktop/axis-settings.desktop" 2>/dev/null || true

if [ -f "$WORKSPACE_DIR/os-build/configs/systemd/axisos-gamemode.service" ]; then
    cp -f "$WORKSPACE_DIR/os-build/configs/systemd/axisos-gamemode.service" "$BUILD_DIR/config/includes.chroot/etc/systemd/system/"
fi

# Deploy Sysctl Gaming Performance Drop-in
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/sysctl.d"
if [ -f "$WORKSPACE_DIR/os-build/configs/sysctl/99-axisos-gaming.conf" ]; then
    cp -f "$WORKSPACE_DIR/os-build/configs/sysctl/99-axisos-gaming.conf" "$BUILD_DIR/config/includes.chroot/etc/sysctl.d/"
fi

# Deploy Gaming Kernel Configuration Fragment
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/axisos/kernel"
if [ -f "$WORKSPACE_DIR/os-build/configs/kernel/gaming-kernel.config" ]; then
    cp -f "$WORKSPACE_DIR/os-build/configs/kernel/gaming-kernel.config" "$BUILD_DIR/config/includes.chroot/etc/axisos/kernel/"
fi

# Deploy GTK 3 & GTK 4 Global Dark Theme settings
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/gtk-3.0"
mkdir -p "$BUILD_DIR/config/includes.chroot/etc/gtk-4.0"
if [ -f "$WORKSPACE_DIR/os-build/configs/gtk-3.0/settings.ini" ]; then
    cp -f "$WORKSPACE_DIR/os-build/configs/gtk-3.0/settings.ini" "$BUILD_DIR/config/includes.chroot/etc/gtk-3.0/settings.ini"
    cp -f "$WORKSPACE_DIR/os-build/configs/gtk-3.0/settings.ini" "$BUILD_DIR/config/includes.chroot/etc/gtk-4.0/settings.ini"
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
if [ -f "$CHROOT_CONFIG_DIR/wayland/axisos-gamemode.desktop" ]; then
    cp -f "$CHROOT_CONFIG_DIR/wayland/axisos-gamemode.desktop" "$BUILD_DIR/config/includes.chroot/usr/share/wayland-sessions/"
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

# Repair any non-root owned system files caused by host staging
find /etc /usr /var /opt /srv /root -xdev -user 1000 -exec chown root:root {} + 2>/dev/null || true
chown root:root / /etc /usr /var /var/lib /tmp /var/tmp /run 2>/dev/null || true
chown root:shadow /etc/shadow /etc/gshadow 2>/dev/null || true
chmod 0640 /etc/shadow /etc/gshadow 2>/dev/null || true
chown root:root /etc/passwd /etc/group /etc/sudoers /etc/crontab 2>/dev/null || true
chmod 0440 /etc/sudoers 2>/dev/null || true
chmod 0644 /etc/passwd /etc/group /etc/crontab 2>/dev/null || true

# Restore setuid bits on critical system binaries
for suid_bin in /bin/su /bin/mount /bin/umount /usr/bin/passwd /usr/bin/chsh /usr/bin/chfn /usr/bin/gpasswd /usr/bin/newgrp /usr/bin/sudo /usr/bin/pkexec /usr/lib/polkit-1/polkit-agent-helper-1 /usr/lib/xorg/Xorg.wrap; do
    if [ -f "$suid_bin" ]; then
        chown root:root "$suid_bin" 2>/dev/null || true
        chmod 4755 "$suid_bin" 2>/dev/null || true
    fi
done
if [ -f /usr/lib/dbus-1.0/dbus-daemon-launch-helper ]; then
    chown root:messagebus /usr/lib/dbus-1.0/dbus-daemon-launch-helper 2>/dev/null || true
    chmod 4754 /usr/lib/dbus-1.0/dbus-daemon-launch-helper 2>/dev/null || true
fi
if [ -d /var/lib/sddm ]; then
    chown -R sddm:sddm /var/lib/sddm 2>/dev/null || true
fi

# Passwordless sudo for axis user
mkdir -p /etc/sudoers.d
echo "axis ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/axis
chown root:root /etc/sudoers.d /etc/sudoers.d/axis 2>/dev/null || true
chmod 0440 /etc/sudoers.d/axis

# Remove duplicate GNOME tray applets and heavy file indexer autostart
rm -f /etc/xdg/autostart/nm-applet.desktop \
      /etc/xdg/autostart/blueman.desktop \
      /etc/xdg/autostart/polkit-gnome-authentication-agent-1.desktop \
      /etc/xdg/autostart/baloo_file.desktop 2>/dev/null || true

# Remove any incomplete desktop-appletsrc so Plasma layout.js builds the full panel & containment
rm -f /etc/xdg/plasma-org.kde.plasma.desktop-appletsrc \
      /etc/skel/.config/plasma-org.kde.plasma.desktop-appletsrc \
      /home/axis/.config/plasma-org.kde.plasma.desktop-appletsrc 2>/dev/null || true

# Configure Plasma default desktop layout script & look-and-feel defaults with AxisOS branding
if [ -f /usr/share/plasma/shells/org.kde.plasma.desktop/contents/layout.js ]; then
    cat << 'LAYOUT_EOF' > /usr/share/plasma/shells/org.kde.plasma.desktop/contents/layout.js
var panel = new Panel;
var panelScreen = panel.screen;
panel.height = 2 * Math.floor(gridUnit * 2.5 / 2);

var kickoff = panel.addWidget("org.kde.plasma.kickoff");
kickoff.currentConfigGroup = ["Shortcuts"];
kickoff.writeConfig("global", "Alt+F1");
kickoff.currentConfigGroup = ["General"];
kickoff.writeConfig("icon", "axisos");

panel.addWidget("org.kde.plasma.pager");
panel.addWidget("org.kde.plasma.icontasks");
panel.addWidget("org.kde.plasma.marginsseparator");
panel.addWidget("org.kde.plasma.systemtray");
panel.addWidget("org.kde.plasma.digitalclock");
panel.addWidget("org.kde.plasma.showdesktop");

var desktopsArray = desktopsForActivity(currentActivity());
for (var j = 0; j < desktopsArray.length; j++) {
    desktopsArray[j].wallpaperPlugin = 'org.kde.image';
    desktopsArray[j].currentConfigGroup = ['Wallpaper', 'org.kde.image', 'General'];
    desktopsArray[j].writeConfig('Image', 'file:///usr/share/backgrounds/axisos/08-axisos-obsidian-wave.jpg');
    desktopsArray[j].writeConfig('FillMode', '2');
}
LAYOUT_EOF
fi

if [ -f /usr/share/plasma/layout-templates/org.kde.plasma.desktop.defaultPanel/contents/layout.js ]; then
    sed -i '/addWidget("org.kde.plasma.kickoff")/a kickoff.currentConfigGroup = ["General"]; kickoff.writeConfig("icon", "axisos");' /usr/share/plasma/layout-templates/org.kde.plasma.desktop.defaultPanel/contents/layout.js 2>/dev/null || true
fi

for lnf_def in /usr/share/plasma/look-and-feel/org.kde.breezedark.desktop/contents/defaults /usr/share/plasma/look-and-feel/org.kde.breeze.desktop/contents/defaults; do
    if [ -f "$lnf_def" ]; then
        sed -i 's/^Image=.*/Image=AxisOS/' "$lnf_def" 2>/dev/null || true
        sed -i 's/^ColorScheme=.*/ColorScheme=AxisOSOneUIDark/' "$lnf_def" 2>/dev/null || true
        sed -i 's/^Theme=breeze.*/Theme=Papirus-Dark/' "$lnf_def" 2>/dev/null || true
    fi
done

# Configure Calamares installer branding with AxisOS logos & One UI dark theme
CAL_DIR="/etc/calamares/branding/debian"
mkdir -p "$CAL_DIR"
if [ -f /usr/share/axisos/branding/axisos-logo.png ]; then
    cp -f /usr/share/axisos/branding/axisos-logo.png "$CAL_DIR/axisos-logo.png"
    cp -f /usr/share/axisos/branding/axisos-logo.png "$CAL_DIR/debian-logo.png"
fi
if [ -f /usr/share/axisos/branding/axisos-icon-512.png ]; then
    cp -f /usr/share/axisos/branding/axisos-icon-512.png "$CAL_DIR/axisos-icon.png"
fi
if [ -f /usr/share/axisos/branding/axisos-boot-1920x1080.png ]; then
    cp -f /usr/share/axisos/branding/axisos-boot-1920x1080.png "$CAL_DIR/welcome.png"
    cp -f /usr/share/axisos/branding/axisos-boot-1920x1080.png "$CAL_DIR/slide1.png"
fi

cat << 'CAL_DESC_EOF' > "$CAL_DIR/branding.desc"
---
componentName:   debian
welcomeStyleCalamares: true
welcomeExpandingLogo: true
windowExpanding: normal
windowSize: 850px,560px
windowPlacement: center

strings:
    productName:         AxisOS Linux
    shortProductName:    AxisOS
    version:             2026.1 (Rolling)
    shortVersion:        2026.1
    versionedName:       AxisOS 2026.1
    shortVersionedName:  AxisOS 2026
    bootloaderEntryName: AxisOS
    productUrl:          https://github.com/AxisOS-Linux
    supportUrl:          https://github.com/AxisOS-Linux
    knownIssuesUrl:      https://github.com/AxisOS-Linux
    releaseNotesUrl:     https://github.com/AxisOS-Linux
    donateUrl:           https://github.com/AxisOS-Linux

images:
    productLogo:         "axisos-logo.png"
    productIcon:         "axisos-icon.png"
    productWelcome:      "welcome.png"

slideshow:               "show.qml"

style:
   sidebarBackground:    "#141826"
   sidebarText:          "#FFFFFF"
   sidebarTextSelect:    "#7D95FF"
   sidebarTextCurrent:   "#7D95FF"

slideshowAPI: 2
CAL_DESC_EOF

if [ -f "$CAL_DIR/show.qml" ]; then
    sed -i 's/Welcome to Debian GNU\/Linux/Welcome to AxisOS Linux/g; s/Debian/AxisOS/g' "$CAL_DIR/show.qml" 2>/dev/null || true
fi

# Override Debian Calamares desktop launcher script to create single "Install AxisOS" launcher
cat << 'CAL_ICON_EOF' > /usr/bin/add-calamares-desktop-icon
#!/bin/sh
DESKTOP=$(xdg-user-dir DESKTOP 2>/dev/null || echo "$HOME/Desktop")
mkdir -p "$DESKTOP"
rm -f "$DESKTOP/install-debian.desktop" "$DESKTOP/calamares.desktop" "$DESKTOP/axis-installer.desktop" "$DESKTOP/axis-settings.desktop"
if [ -f /usr/share/applications/install-axisos.desktop ]; then
    cp -f /usr/share/applications/install-axisos.desktop "$DESKTOP/install-axisos.desktop"
elif [ -f /usr/share/applications/install-debian.desktop ]; then
    cp -f /usr/share/applications/install-debian.desktop "$DESKTOP/install-axisos.desktop"
fi
chmod +x "$DESKTOP/install-axisos.desktop"
gio set --type=string "$DESKTOP/install-axisos.desktop" metadata::trusted true 2>/dev/null || true
CAL_ICON_EOF
chmod 0755 /usr/bin/add-calamares-desktop-icon 2>/dev/null || true

# Override install-debian.desktop and calamares.desktop with Install AxisOS and hide redundant entries
if [ -f /usr/share/applications/install-axisos.desktop ]; then
    cp -f /usr/share/applications/install-axisos.desktop /usr/share/applications/install-debian.desktop 2>/dev/null || true
    echo "NoDisplay=true" >> /usr/share/applications/install-debian.desktop 2>/dev/null || true
    cp -f /usr/share/applications/install-axisos.desktop /usr/share/applications/calamares.desktop 2>/dev/null || true
    echo "NoDisplay=true" >> /usr/share/applications/calamares.desktop 2>/dev/null || true
fi

# Replace Debian installer icons with AxisOS branding
if [ -f /usr/share/axisos/branding/axisos-icon-512.png ]; then
    cp -f /usr/share/axisos/branding/axisos-icon-512.png /usr/share/pixmaps/install-debian.png 2>/dev/null || true
    cp -f /usr/share/axisos/branding/axisos-icon-512.png /usr/share/pixmaps/calamares.png 2>/dev/null || true
fi
if [ -f /usr/share/icons/hicolor/scalable/apps/axisos.svg ]; then
    cp -f /usr/share/icons/hicolor/scalable/apps/axisos.svg /usr/share/pixmaps/install-debian.svg 2>/dev/null || true
    cp -f /usr/share/icons/hicolor/scalable/apps/axisos.svg /usr/share/pixmaps/calamares.svg 2>/dev/null || true
fi

# Remove legacy axis-installer and axis-settings from application menus
rm -f /usr/share/applications/axis-installer.desktop /usr/share/applications/axis-settings.desktop 2>/dev/null || true

# Brand all icon themes: replace KDE and Plasma logos with AxisOS logo
for theme_dir in /usr/share/icons/breeze /usr/share/icons/breeze-dark /usr/share/icons/Papirus /usr/share/icons/Papirus-Dark /usr/share/icons/Papirus-Light /usr/share/icons/hicolor /usr/share/icons/ePapirus /usr/share/icons/Adwaita; do
    if [ -d "$theme_dir" ]; then
        find "$theme_dir" -type f \( \
            -name "start-here-kde*.svg" -o \
            -name "start-here*.svg" -o \
            -name "kde.svg" -o \
            -name "kde-*.svg" -o \
            -name "klogo*.svg" -o \
            -name "plasma.svg" -o \
            -name "plasma-*.svg" -o \
            -name "distributor-logo*.svg" -o \
            -name "about_kde.svg" \
        \) -exec cp -f /usr/share/icons/hicolor/scalable/apps/axisos.svg {} + 2>/dev/null || true
    fi
done

# Replace Plasma splash screen images with AxisOS branding
SPLASH_IMAGES="/usr/share/plasma/look-and-feel/org.kde.breeze.desktop/contents/splash/images"
if [ -d "$SPLASH_IMAGES" ] && [ -f /usr/share/axisos/branding/axisos-logo.svg ]; then
    gzip -c /usr/share/axisos/branding/axisos-logo.svg > "$SPLASH_IMAGES/kde.svgz" 2>/dev/null || true
    gzip -c /usr/share/axisos/branding/axisos-logo.svg > "$SPLASH_IMAGES/plasma.svgz" 2>/dev/null || true
fi

# Replace SDDM default logo with AxisOS logo
if [ -d /usr/share/sddm/themes/breeze ] && [ -f /usr/share/axisos/branding/axisos-logo.svg ]; then
    cp -f /usr/share/axisos/branding/axisos-logo.svg /usr/share/sddm/themes/breeze/default-logo.svg 2>/dev/null || true
fi

# System Information Branding (/etc/os-release & /usr/lib/os-release)
cat << 'OS_REL_EOF' > /etc/os-release
PRETTY_NAME="AxisOS Linux (One UI Dark)"
NAME="AxisOS"
ID=axisos
ID_LIKE=debian
VERSION="2026.1 (Rolling)"
VERSION_ID="2026.1"
VERSION_CODENAME=rolling
HOME_URL="https://github.com/AxisOS-Linux"
SUPPORT_URL="https://github.com/AxisOS-Linux"
BUG_REPORT_URL="https://github.com/AxisOS-Linux"
LOGO=axisos
OS_REL_EOF
cp -f /etc/os-release /usr/lib/os-release 2>/dev/null || true
echo "AxisOS Linux \n \l" > /etc/issue
echo "AxisOS Linux \n \l" > /etc/issue.net

# ZRAM high-performance swap configuration
if [ -f /etc/default/zramswap ]; then
    sed -i 's/^#*ALGO=.*/ALGO=zstd/' /etc/default/zramswap 2>/dev/null || true
    sed -i 's/^#*PERCENT=.*/PERCENT=50/' /etc/default/zramswap 2>/dev/null || true
    sed -i 's/^#*PRIORITY=.*/PRIORITY=100/' /etc/default/zramswap 2>/dev/null || true
fi

# Development & Gaming System Resource Limits
mkdir -p /etc/security/limits.d
cat << 'LIMITS_EOF' > /etc/security/limits.d/99-axisos-gaming.conf
* soft nofile 1048576
* hard nofile 1048576
* soft memlock unlimited
* hard memlock unlimited
* soft nice -20
* hard nice -20
LIMITS_EOF

# Patch live-config 0085-sddm so it preserves AxisOS theme & autologin settings at boot
if [ -f /lib/live/config/0085-sddm ]; then
    cat << 'SDDM_LIVE_EOF' > /lib/live/config/0085-sddm
#!/bin/sh
. /lib/live/config.sh

LIVE_USERNAME="axis"
for _PARAMETER in ${LIVE_CONFIG_CMDLINE}; do
    case "${_PARAMETER}" in
        live-config.noautologin|noautologin|live-config.nox11autologin|nox11autologin)
            exit 0
            ;;
        live-config.username=*|username=*)
            LIVE_USERNAME="${_PARAMETER#*username=}"
            ;;
    esac
done

mkdir -p /etc/sddm.conf.d
cat > /etc/sddm.conf << SDDM_CONF_EOF
[Autologin]
User=${LIVE_USERNAME}
Session=plasma.desktop
Relogin=false

[Theme]
Current=breeze
CursorTheme=breeze_cursors
Font=IBM Plex Sans,10,-1,5,50,0,0,0,0,0

[General]
InputMethod=
Numlock=none
SDDM_CONF_EOF
cp -f /etc/sddm.conf /etc/sddm.conf.d/autologin.conf 2>/dev/null || true
mkdir -p /var/lib/live/config
touch /var/lib/live/config/xinit /var/lib/live/config/sddm
SDDM_LIVE_EOF
    chmod 0755 /lib/live/config/0085-sddm
fi

# Raise KDE Plasma systemd user unit startup timeouts so Live USB & VM boots never thrash
for ufile in /usr/lib/systemd/user/plasma-*.service; do
    if [ -f "$ufile" ]; then
        sed -i 's/TimeoutSec=5sec/TimeoutSec=120sec/g; s/TimeoutSec=40sec/TimeoutSec=180sec/g' "$ufile" 2>/dev/null || true
    fi
done
if [ -f /etc/systemd/user.conf ]; then
    sed -i 's/^#*DefaultTimeoutStartSec=.*/DefaultTimeoutStartSec=120s/' /etc/systemd/user.conf 2>/dev/null || true
fi

# Ensure /home/axis skeleton and permissions
mkdir -p /home/axis/.config /home/axis/Desktop /home/axis/Downloads
if [ -d /etc/skel/.config ]; then
    cp -rf /etc/skel/.config/* /home/axis/.config/ 2>/dev/null || true
fi
if [ -f /usr/share/applications/systemsettings.desktop ]; then
    cp -f /usr/share/applications/systemsettings.desktop /etc/skel/Desktop/ 2>/dev/null || true
fi
if [ -f /usr/share/applications/install-axisos.desktop ]; then
    cp -f /usr/share/applications/install-axisos.desktop /etc/skel/Desktop/ 2>/dev/null || true
fi
rm -f /etc/skel/Desktop/calamares.desktop /etc/skel/Desktop/install-debian.desktop /etc/skel/Desktop/axis-installer.desktop /etc/skel/Desktop/axis-settings.desktop 2>/dev/null || true
if [ -d /etc/skel/Desktop ]; then
    cp -rf /etc/skel/Desktop/* /home/axis/Desktop/ 2>/dev/null || true
    chmod +x /home/axis/Desktop/*.desktop 2>/dev/null || true
fi
rm -f /home/axis/Desktop/calamares.desktop /home/axis/Desktop/install-debian.desktop /home/axis/Desktop/axis-installer.desktop /home/axis/Desktop/axis-settings.desktop 2>/dev/null || true
rm -f /home/axis/.config/plasma-org.kde.plasma.desktop-appletsrc 2>/dev/null || true
chown -R 1000:1000 /home/axis 2>/dev/null || true

# Systemd target and display manager service configuration
systemctl enable sddm.service 2>/dev/null || true
systemctl enable seatd.service 2>/dev/null || true
systemctl disable axisos.service 2>/dev/null || true
systemctl mask axisos.service 2>/dev/null || true
systemctl disable lightdm.service 2>/dev/null || true
systemctl mask lightdm.service 2>/dev/null || true
systemctl set-default graphical.target 2>/dev/null || true

# Ensure all binaries in /usr/local/bin are executable
chmod 755 /usr/local/bin/* 2>/dev/null || true

# Ensure squashfs-tools and installer dependencies exist for Calamares unpackfs
if ! command -v unsquashfs >/dev/null 2>&1; then
    echo "nameserver 1.1.1.1" > /etc/resolv.conf 2>/dev/null || true
    apt-get update 2>/dev/null || true
    apt-get install -y --no-install-recommends squashfs-tools grub-efi-amd64-bin grub-pc-bin 2>/dev/null || true
fi

# Purge docs, man pages, non-English locales, and package caches to keep ISO strictly under 2 GiB
rm -rf /usr/share/doc/* /usr/share/man/* /var/cache/apt/archives/* /var/lib/apt/lists/* 2>/dev/null || true
find /usr/share/locale -maxdepth 1 -mindepth 1 ! -name 'en*' ! -name 'locale.alias' -exec rm -rf {} + 2>/dev/null || true

# Clean up any runtime/temporary directories before squashing
rm -rf /run/* /run/user /tmp/* /var/tmp/* /var/log/* 2>/dev/null || true
mkdir -p /run/lock
chmod 1777 /tmp /var/tmp 2>/dev/null || true
chmod 755 /run 2>/dev/null || true
EOF
chmod +x "$BUILD_DIR/config/hooks/live/0050-setup-axisos.hook.chroot"

# Ensure all staged chroot includes are owned by root:root before live-build copies them
chown -R 0:0 "$BUILD_DIR/config/includes.chroot" 2>/dev/null || true

# Sync includes.chroot directly into chroot if chroot already exists
if [ -d "$BUILD_DIR/chroot/usr/local/bin" ]; then
    echo "--> Syncing staged files directly into existing chroot..."
    if [ -f /etc/resolv.conf ]; then
        cp -f /etc/resolv.conf "$BUILD_DIR/chroot/etc/resolv.conf" 2>/dev/null || true
    else
        echo "nameserver 1.1.1.1" > "$BUILD_DIR/chroot/etc/resolv.conf" 2>/dev/null || true
    fi
    if [ ! -x "$BUILD_DIR/chroot/usr/bin/unsquashfs" ]; then
        echo "--> Installing squashfs-tools inside chroot..."
        chroot "$BUILD_DIR/chroot" apt-get update 2>/dev/null || true
        chroot "$BUILD_DIR/chroot" apt-get install -y --no-install-recommends squashfs-tools grub-efi-amd64-bin grub-pc-bin 2>/dev/null || true
    fi
    cp -rf "$BUILD_DIR/config/includes.chroot/"* "$BUILD_DIR/chroot/" 2>/dev/null || true
    chmod 755 "$BUILD_DIR/chroot/usr/local/bin/"* 2>/dev/null || true
    if [ -f "$BUILD_DIR/config/hooks/live/0050-setup-axisos.hook.chroot" ]; then
        cp -f "$BUILD_DIR/config/hooks/live/0050-setup-axisos.hook.chroot" "$BUILD_DIR/chroot/tmp/setup-axisos.sh"
        chmod +x "$BUILD_DIR/chroot/tmp/setup-axisos.sh"
        chroot "$BUILD_DIR/chroot" /tmp/setup-axisos.sh || true
        rm -f "$BUILD_DIR/chroot/tmp/setup-axisos.sh"
    fi
fi

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
        --chroot-squashfs-compression-type xz \
        --memtest none \
        --verbose

    # Strip any obsolete squashfs compression level and inject 1MB BCJ compression options
    sed -i '/LB_CHROOT_SQUASHFS_COMPRESSION_LEVEL/d' "$BUILD_DIR/config/chroot" 2>/dev/null || true
    sed -i '/LB_CHROOT_SQUASHFS_COMPRESSION_LEVEL/d' config/chroot 2>/dev/null || true
    sed -i '/LB_CHROOT_SQUASHFS_COMPRESSION_LEVEL/d' "$BUILD_DIR/config/binary" 2>/dev/null || true
    sed -i '/LB_CHROOT_SQUASHFS_COMPRESSION_LEVEL/d' config/binary 2>/dev/null || true
    sed -i '/MKSQUASHFS_OPTIONS/d' "$BUILD_DIR/config/binary" 2>/dev/null || true
    sed -i '/MKSQUASHFS_OPTIONS/d' config/binary 2>/dev/null || true
    echo 'MKSQUASHFS_OPTIONS="-b 1048576 -Xbcj x86"' >> "$BUILD_DIR/config/binary" 2>/dev/null || true
    echo 'MKSQUASHFS_OPTIONS="-b 1048576 -Xbcj x86"' >> config/binary 2>/dev/null || true
    export MKSQUASHFS_OPTIONS="-b 1048576 -Xbcj x86"

    # Reset binary stages to ensure fresh squashfs and ISO generation
    rm -f "$BUILD_DIR/.build/binary_"* "$BUILD_DIR/.build/chroot_hooks" "$BUILD_DIR/.build/chroot_includes"* 2>/dev/null || true
    rm -rf "$BUILD_DIR/binary" "$BUILD_DIR"/*.iso* 2>/dev/null || true

    lb build 2>&1 | tee "$BUILD_DIR/build.log"
    cp -f "$BUILD_DIR"/live-image-amd64.hybrid.iso "$WORKSPACE_DIR/axisos-live-amd64.iso" 2>/dev/null || true
    cp -f "$BUILD_DIR"/*.iso "$WORKSPACE_DIR/axisos-live-amd64.iso" 2>/dev/null || true
    chmod 0644 "$WORKSPACE_DIR/axisos-live-amd64.iso" 2>/dev/null || true
    if [ -f "$WORKSPACE_DIR/axisos-live-amd64.iso" ]; then
        (cd "$WORKSPACE_DIR" && sha256sum axisos-live-amd64.iso > axisos-live-amd64.iso.sha256)
    fi
fi

if [ -f "$WORKSPACE_DIR/axisos-live-amd64.iso" ]; then
    echo "SUCCESS: AxisOS Live ISO generated successfully!"
    echo "Location: $WORKSPACE_DIR/axisos-live-amd64.iso"
    ls -lh "$WORKSPACE_DIR/axisos-live-amd64.iso"
fi

echo "=================================================="
echo "   AxisOS Chroot & Build Pipeline Completed!      "
echo "=================================================="
