#!/bin/bash
# AxisOS Remote APT Repository Builder
# Compiles and packages native apps and scripts into a Debian package and repository.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

echo "=== AxisOS Remote APT Repository Builder ==="

VERSION="2.1.0-$(date +%Y%m%d%H%M)"
DEB_DIR="/tmp/deb-build/axisos-desktop-suite_${VERSION}_amd64"
rm -rf /tmp/deb-build public_apt
mkdir -p "${DEB_DIR}/DEBIAN"
mkdir -p "${DEB_DIR}/usr/local/bin"
mkdir -p "${DEB_DIR}/usr/bin"
mkdir -p "${DEB_DIR}/usr/share/applications"
mkdir -p "${DEB_DIR}/etc/apt/sources.list.d"
mkdir -p "${DEB_DIR}/etc/apt/apt.conf.d"
chmod 755 /tmp/deb-build "${DEB_DIR}" "${DEB_DIR}/DEBIAN"

# Generate control file with dynamic versioning
cat << EOF > "${DEB_DIR}/DEBIAN/control"
Package: axisos-desktop-suite
Version: ${VERSION}
Section: x11
Priority: optional
Architecture: amd64
Depends: libgtk-3-0, libcairo2, libpango-1.0-0, libpangocairo-1.0-0, fonts-dejavu-core, fonts-cantarell, fonts-liberation, fonts-inter, libqt6core6, libqt6gui6, libqt6qml6, libqt6quick6, libqt6dbus6, libqt6waylandcompositor6, qml6-module-qtwayland-compositor, qml6-module-qtquick, qml6-module-qtquick-controls, qml6-module-qtquick-layouts, qml6-module-qtquick-window, qml6-module-qtquick-templates, qt6-wayland, libpam0g
Replaces: axisos-desktop-suite (<< ${VERSION})
Breaks: axisos-desktop-suite (<< ${VERSION})
Maintainer: Jacky Mpoka <jackympoka22@gmail.com>
Description: AxisOS LiquidNodes Desktop Environment, HUD & System Suite
 AxisOS Native LiquidNodes Wayland Compositor & Glassmorphic Desktop,
 featuring HUD Task Manager, Settings Panels, Terminal, Rewind CoW Engine,
 and Remote OTA Updates.
EOF
chmod 644 "${DEB_DIR}/DEBIAN/control"

# Generate post-installation hook
cat << 'EOF' > "${DEB_DIR}/DEBIAN/postinst"
#!/bin/sh
set -e
if command -v systemctl >/dev/null 2>&1; then
    systemctl --global enable axis-update-notifier.timer 2>/dev/null || true
    systemctl daemon-reload 2>/dev/null || true
fi
# Rebuild font cache so newly installed fonts and fallbacks are indexed
if command -v fc-cache >/dev/null 2>&1; then
    fc-cache -f 2>/dev/null || true
fi
if command -v glib-compile-schemas >/dev/null 2>&1; then
    glib-compile-schemas /usr/share/glib-2.0/schemas 2>/dev/null || true
fi
exit 0
EOF
chmod 755 "${DEB_DIR}/DEBIAN/postinst"

# Copy binaries and desktop files
for app in axis-task-manager axis-terminal axis-installer axis-settings axis-usb-daemon axis-welcome axis-package-installer axis-scheduler-manager nobara-welcome; do
    if [ -f "os-build/apps/$app/$app" ]; then
        cp -f "os-build/apps/$app/$app" "${DEB_DIR}/usr/local/bin/$app"
        chmod 755 "${DEB_DIR}/usr/local/bin/$app"
    fi
    if [ -f "os-build/apps/$app/$app.desktop" ]; then
        cp -f "os-build/apps/$app/$app.desktop" "${DEB_DIR}/usr/share/applications/$app.desktop"
        chmod 644 "${DEB_DIR}/usr/share/applications/$app.desktop"
    fi
done

if [ -f "os-build/apps/axis-settings/axisos-settings-panel" ]; then
    cp -f "os-build/apps/axis-settings/axisos-settings-panel" "${DEB_DIR}/usr/local/bin/axisos-settings-panel"
    chmod 755 "${DEB_DIR}/usr/local/bin/axisos-settings-panel"
fi

for s in axis-rewind.sh axis-sysupgrade.sh axis-update-notifier.sh axis-game-boost.sh axis-neural-pulse.sh axis-usb-vault.sh axis-gaze-shield.sh axis-disk-info.sh axis-biometrics.sh axis-switch-edition.sh axis-domain-join.sh; do
    if [ -f "os-build/scripts/$s" ]; then
        out_name="$(echo "$s" | sed 's/\.sh$//')"
        cp -f "os-build/scripts/$s" "${DEB_DIR}/usr/local/bin/$out_name"
        chmod 755 "${DEB_DIR}/usr/local/bin/$out_name"
    fi
done

if [ -f "os-build/configs/apt/sources.list.d/axisos.list" ]; then
    cp -f os-build/configs/apt/sources.list.d/axisos.list "${DEB_DIR}/etc/apt/sources.list.d/axisos.list"
fi
if [ -f "os-build/configs/apt/apt.conf.d/80-axisos-btrfs-snapshot" ]; then
    cp -f os-build/configs/apt/apt.conf.d/80-axisos-btrfs-snapshot "${DEB_DIR}/etc/apt/apt.conf.d/80-axisos-btrfs-snapshot"
fi

# Deploy Background Update Notifier systemd timer and XDG autostart
mkdir -p "${DEB_DIR}/etc/systemd/user/timers.target.wants"
if [ -f "os-build/configs/systemd/user/axis-update-notifier.service" ]; then
    cp -f "os-build/configs/systemd/user/axis-update-notifier.service" "${DEB_DIR}/etc/systemd/user/axis-update-notifier.service"
    chmod 644 "${DEB_DIR}/etc/systemd/user/axis-update-notifier.service"
fi
if [ -f "os-build/configs/systemd/user/axis-update-notifier.timer" ]; then
    cp -f "os-build/configs/systemd/user/axis-update-notifier.timer" "${DEB_DIR}/etc/systemd/user/axis-update-notifier.timer"
    chmod 644 "${DEB_DIR}/etc/systemd/user/axis-update-notifier.timer"
    ln -sf "/etc/systemd/user/axis-update-notifier.timer" "${DEB_DIR}/etc/systemd/user/timers.target.wants/axis-update-notifier.timer"
fi
mkdir -p "${DEB_DIR}/etc/xdg/autostart"
if [ -f "os-build/configs/autostart/axis-update-notifier.desktop" ]; then
    cp -f "os-build/configs/autostart/axis-update-notifier.desktop" "${DEB_DIR}/etc/xdg/autostart/axis-update-notifier.desktop"
    chmod 644 "${DEB_DIR}/etc/xdg/autostart/axis-update-notifier.desktop"
fi
if [ -f "os-build/configs/desktop/axis-sysupgrade.desktop" ]; then
    cp -f "os-build/configs/desktop/axis-sysupgrade.desktop" "${DEB_DIR}/usr/share/applications/axis-sysupgrade.desktop"
    chmod 644 "${DEB_DIR}/usr/share/applications/axis-sysupgrade.desktop"
fi
if [ -f "os-build/configs/desktop/axis-domain-join.desktop" ]; then
    cp -f "os-build/configs/desktop/axis-domain-join.desktop" "${DEB_DIR}/usr/share/applications/axis-domain-join.desktop"
    chmod 644 "${DEB_DIR}/usr/share/applications/axis-domain-join.desktop"
fi

# Deploy AxisOS Wallpaper Collection & GNOME Properties
mkdir -p "${DEB_DIR}/usr/share/backgrounds/axisos"
if [ -d "data/wallpapers" ]; then
    cp -f data/wallpapers/*.jpg "${DEB_DIR}/usr/share/backgrounds/axisos/" 2>/dev/null || true
    chmod 644 "${DEB_DIR}/usr/share/backgrounds/axisos/"*.jpg 2>/dev/null || true
fi
mkdir -p "${DEB_DIR}/usr/share/gnome-background-properties"
if [ -f "os-build/configs/gnome/axisos-wallpapers.xml" ]; then
    cp -f "os-build/configs/gnome/axisos-wallpapers.xml" "${DEB_DIR}/usr/share/gnome-background-properties/axisos-wallpapers.xml"
    chmod 644 "${DEB_DIR}/usr/share/gnome-background-properties/axisos-wallpapers.xml"
fi
if [ -f "data/wallpapers/01-axisos-signature-core.png" ]; then
    cp -f "data/wallpapers/01-axisos-signature-core.png" "${DEB_DIR}/usr/share/backgrounds/axis-wallpaper.png"
    chmod 644 "${DEB_DIR}/usr/share/backgrounds/axis-wallpaper.png"
elif [ -f "data/branding/axis-wallpaper.png" ]; then
    cp -f "data/branding/axis-wallpaper.png" "${DEB_DIR}/usr/share/backgrounds/axis-wallpaper.png"
    chmod 644 "${DEB_DIR}/usr/share/backgrounds/axis-wallpaper.png"
elif [ -f "data/theme/axis-wallpaper.png" ]; then
    cp -f "data/theme/axis-wallpaper.png" "${DEB_DIR}/usr/share/backgrounds/axis-wallpaper.png"
    chmod 644 "${DEB_DIR}/usr/share/backgrounds/axis-wallpaper.png"
fi

# Deploy AxisOS LiquidNodes Qt6 Wayland Compositor & Shell
if [ -f "compositor-qt/LiquidNodesOS" ]; then
    echo "--> Packaging LiquidNodesOS compositor binary..."
    cp -f "compositor-qt/LiquidNodesOS" "${DEB_DIR}/usr/local/bin/LiquidNodesOS"
    chmod 755 "${DEB_DIR}/usr/local/bin/LiquidNodesOS"
    ln -sfn "/usr/local/bin/LiquidNodesOS" "${DEB_DIR}/usr/bin/LiquidNodesOS"
    ln -sfn "/usr/local/bin/LiquidNodesOS" "${DEB_DIR}/usr/local/bin/axis-compositor"
    ln -sfn "/usr/local/bin/LiquidNodesOS" "${DEB_DIR}/usr/bin/axis-compositor"
fi

mkdir -p "${DEB_DIR}/usr/share/axis-compositor-qt"
if [ -d "compositor-qt" ]; then
    cp -rf compositor-qt/*.qml "${DEB_DIR}/usr/share/axis-compositor-qt/" 2>/dev/null || true
    chmod 644 "${DEB_DIR}/usr/share/axis-compositor-qt/"* 2>/dev/null || true
fi

mkdir -p "${DEB_DIR}/usr/share/wayland-sessions"
cat << 'EOF' > "${DEB_DIR}/usr/share/wayland-sessions/axisos-liquidnodes.desktop"
[Desktop Entry]
Name=AxisOS LiquidNodes
Comment=AxisOS Native Qt 6 Wayland Compositor & Glassmorphic Desktop
Exec=/usr/local/bin/LiquidNodesOS
Type=Application
DesktopNames=AxisOS:LiquidNodes
EOF
chmod 644 "${DEB_DIR}/usr/share/wayland-sessions/axisos-liquidnodes.desktop"
ln -sfn "/usr/share/wayland-sessions/axisos-liquidnodes.desktop" "${DEB_DIR}/usr/share/wayland-sessions/axisos.desktop"

mkdir -p "${DEB_DIR}/etc/systemd/system"
cat << 'EOF' > "${DEB_DIR}/etc/systemd/system/axisos-session.service"
[Unit]
Description=AxisOS LiquidNodes Desktop Environment
After=systemd-user-sessions.service plymouth-quit-wait.service
Conflicts=getty@tty1.service
After=getty@tty1.service

[Service]
Type=simple
User=axis
WorkingDirectory=/home/axis
Environment=XDG_SESSION_TYPE=wayland
Environment=XDG_CURRENT_DESKTOP=AxisOS:LiquidNodes
Environment=WAYLAND_DISPLAY=wayland-0
Environment=QT_QPA_PLATFORM=wayland;xcb;eglfs
Environment=QSG_RHI_PREFER_SOFTWARE_RENDERER=0
ExecStart=/usr/local/bin/LiquidNodesOS
Restart=on-failure
RestartSec=1
StandardInput=tty
StandardOutput=journal
TTYPath=/dev/tty1
TTYReset=yes
TTYVHangup=yes

[Install]
WantedBy=graphical.target
EOF
chmod 644 "${DEB_DIR}/etc/systemd/system/axisos-session.service"

mkdir -p public_apt/apt
dpkg-deb --build "${DEB_DIR}" "public_apt/apt/axisos-desktop-suite_${VERSION}_amd64.deb"

cd public_apt/apt

if command -v dpkg-scanpackages >/dev/null 2>&1; then
    dpkg-scanpackages . /dev/null > Packages
else
    > Packages
    for deb in *.deb; do
        [ -f "$deb" ] || continue
        dpkg-deb -I "$deb" control >> Packages
        echo "Filename: ./$deb" >> Packages
        echo "Size: $(stat -c %s "$deb")" >> Packages
        echo "SHA256: $(sha256sum "$deb" | awk '{print $1}')" >> Packages
        echo "" >> Packages
    done
fi
gzip -k -f Packages

# Generate translation indices to prevent APT 404 lookup warnings
touch Translation-en
gzip -k -f Translation-en

cat << EOF > Release
Origin: AxisOS
Label: AxisOS Official Repository
Suite: stable
Codename: axis
Architectures: amd64
Components: main
Description: AxisOS Remote OTA Updates Repository
Date: $(date -Ru)
MD5Sum:
 $(md5sum Packages | awk '{print $1}') $(stat -c %s Packages) Packages
 $(md5sum Packages.gz | awk '{print $1}') $(stat -c %s Packages.gz) Packages.gz
 $(md5sum Translation-en | awk '{print $1}') $(stat -c %s Translation-en) Translation-en
 $(md5sum Translation-en.gz | awk '{print $1}') $(stat -c %s Translation-en.gz) Translation-en.gz
SHA1:
 $(sha1sum Packages | awk '{print $1}') $(stat -c %s Packages) Packages
 $(sha1sum Packages.gz | awk '{print $1}') $(stat -c %s Packages.gz) Packages.gz
 $(sha1sum Translation-en | awk '{print $1}') $(stat -c %s Translation-en) Translation-en
 $(sha1sum Translation-en.gz | awk '{print $1}') $(stat -c %s Translation-en.gz) Translation-en.gz
SHA256:
 $(sha256sum Packages | awk '{print $1}') $(stat -c %s Packages) Packages
 $(sha256sum Packages.gz | awk '{print $1}') $(stat -c %s Packages.gz) Packages.gz
 $(sha256sum Translation-en | awk '{print $1}') $(stat -c %s Translation-en) Translation-en
 $(sha256sum Translation-en.gz | awk '{print $1}') $(stat -c %s Translation-en.gz) Translation-en.gz
EOF

echo "=== APT Repository Artifacts Generated Successfully ==="
