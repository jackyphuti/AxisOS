#!/usr/bin/env bash
# ==============================================================================
# AxisOS Remote APT Repository Builder (Pure C11 Ecosystem)
# Compiles, packages, and indexes pure C native apps and shell into Debian repository
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

echo "======================================================================"
echo "  AxisOS Remote APT Repository Builder (Pure C11 Ecosystem)           "
echo "======================================================================"

VERSION="2.1.0-$(date +%Y%m%d%H%M)"
DEB_DIR="/tmp/deb-build/axisos-desktop-suite_${VERSION}_amd64"

rm -rf /tmp/deb-build public_apt
mkdir -p "${DEB_DIR}/DEBIAN"
mkdir -p "${DEB_DIR}/usr/local/bin"
mkdir -p "${DEB_DIR}/usr/bin"
mkdir -p "${DEB_DIR}/usr/share/applications"
mkdir -p "${DEB_DIR}/usr/share/themes"
mkdir -p "${DEB_DIR}/usr/share/backgrounds/axisos"
mkdir -p "${DEB_DIR}/etc/apt/sources.list.d"
mkdir -p "${DEB_DIR}/etc/apt/apt.conf.d"
chmod 755 /tmp/deb-build "${DEB_DIR}" "${DEB_DIR}/DEBIAN"

# Generate control file with Pure C runtime dependencies
cat << EOF > "${DEB_DIR}/DEBIAN/control"
Package: axisos-desktop-suite
Version: ${VERSION}
Section: x11
Priority: optional
Architecture: amd64
Depends: libgtk-3-0, libcairo2, libpango-1.0-0, libpangocairo-1.0-0, fonts-dejavu-core, fonts-cantarell, fonts-liberation, fonts-inter, libpam0g, libvterm0, libvte-2.91-0, libarchive13, libjson-c5, libudev1
Replaces: axisos-desktop-suite (<< ${VERSION})
Breaks: axisos-desktop-suite (<< ${VERSION})
Maintainer: Jacky Mpoka <jackympoka22@gmail.com>
Description: AxisOS LiquidNodes Pure C Desktop Environment, HUD & System Suite
 AxisOS Native LiquidNodes Wayland Desktop Environment, featuring HUD Task Manager,
 Settings Panels, Terminal, Rewind CoW Engine, and Pure C System Daemons.
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
if command -v fc-cache >/dev/null 2>&1; then
    fc-cache -f 2>/dev/null || true
fi
if command -v glib-compile-schemas >/dev/null 2>&1; then
    glib-compile-schemas /usr/share/glib-2.0/schemas 2>/dev/null || true
fi
exit 0
EOF
chmod 755 "${DEB_DIR}/DEBIAN/postinst"

# Deploy compiled native desktop applications
APPS=(
    axis-task-manager
    axis-terminal
    axis-installer
    axis-settings
    axis-usb-daemon
    axis-welcome
    axis-package-installer
    axis-scheduler-manager
    nobara-welcome
    axis-software-manager
    axis-update-manager
    axis-calc
    axis-color-picker
    axis-fetch
    axis-oobe
)

for app in "${APPS[@]}"; do
    if [ -f "os-build/apps/$app/$app" ]; then
        cp -f "os-build/apps/$app/$app" "${DEB_DIR}/usr/local/bin/$app"
        chmod 755 "${DEB_DIR}/usr/local/bin/$app"
        ln -sf "/usr/local/bin/$app" "${DEB_DIR}/usr/bin/$app"
    fi
    if [ -f "os-build/apps/$app/$app.desktop" ]; then
        cp -f "os-build/apps/$app/$app.desktop" "${DEB_DIR}/usr/share/applications/$app.desktop"
        chmod 644 "${DEB_DIR}/usr/share/applications/$app.desktop"
    fi
done

# Hardware Manager symlink
ln -sf /usr/local/bin/nobara-welcome "${DEB_DIR}/usr/local/bin/axis-hardware-manager"
ln -sf /usr/local/bin/nobara-welcome "${DEB_DIR}/usr/bin/axis-hardware-manager"

if [ -f "os-build/apps/axis-settings/axisos-settings-panel" ]; then
    cp -f "os-build/apps/axis-settings/axisos-settings-panel" "${DEB_DIR}/usr/local/bin/axisos-settings-panel"
    chmod 755 "${DEB_DIR}/usr/local/bin/axisos-settings-panel"
    ln -sf /usr/local/bin/axisos-settings-panel "${DEB_DIR}/usr/bin/axisos-settings-panel"
fi

# Deploy compiled helper daemons & tools
HELPERS=(
    axis-disk-info
    axis-game-boost
    axis-update-notifier
    axis-autotimezone
    gpu-autodetect
    axis-biometrics
    axis-fastboot-patch
    axis-captive-portal
    axis-rewind
)

for h in "${HELPERS[@]}"; do
    if [ -f "os-build/scripts/$h" ]; then
        cp -f "os-build/scripts/$h" "${DEB_DIR}/usr/local/bin/$h"
        chmod 755 "${DEB_DIR}/usr/local/bin/$h"
        ln -sf "/usr/local/bin/$h" "${DEB_DIR}/usr/bin/$h"
    fi
done

# Deploy Pure C Desktop Shell
if [ -f "shell-c/axisos-shell" ]; then
    cp -f "shell-c/axisos-shell" "${DEB_DIR}/usr/local/bin/axisos-shell"
    chmod 755 "${DEB_DIR}/usr/local/bin/axisos-shell"
    ln -sf /usr/local/bin/axisos-shell "${DEB_DIR}/usr/local/bin/axis-shell"
    ln -sf /usr/local/bin/axisos-shell "${DEB_DIR}/usr/local/bin/LiquidNodesOS"
    ln -sf /usr/local/bin/axisos-shell "${DEB_DIR}/usr/bin/axisos-shell"
    ln -sf /usr/local/bin/axisos-shell "${DEB_DIR}/usr/bin/axis-shell"
fi

# Deploy Pure C Compositor (if compiled)
if [ -f "compositor/axis-compositor" ]; then
    cp -f "compositor/axis-compositor" "${DEB_DIR}/usr/local/bin/axis-compositor"
    chmod 755 "${DEB_DIR}/usr/local/bin/axis-compositor"
    ln -sf /usr/local/bin/axis-compositor "${DEB_DIR}/usr/bin/axis-compositor"
fi

# Deploy Package Manager & Updater
if [ -f "pkg-mgr/bin/axis" ]; then
    cp -f "pkg-mgr/bin/axis" "${DEB_DIR}/usr/local/bin/axis"
    chmod 755 "${DEB_DIR}/usr/local/bin/axis"
    ln -sf /usr/local/bin/axis "${DEB_DIR}/usr/bin/axis"
fi
if [ -f "os-build/updater/axis-update" ]; then
    cp -f "os-build/updater/axis-update" "${DEB_DIR}/usr/local/bin/axis-update"
    chmod 755 "${DEB_DIR}/usr/local/bin/axis-update"
    ln -sf /usr/local/bin/axis-update "${DEB_DIR}/usr/bin/axis-update"
fi

# Deploy Wallpapers & Theme
if [ -d "data/wallpapers" ]; then
    cp -rf data/wallpapers/* "${DEB_DIR}/usr/share/backgrounds/axisos/" 2>/dev/null || true
fi
if [ -f "data/wallpapers/01-axisos-signature-core.png" ]; then
    cp -f "data/wallpapers/01-axisos-signature-core.png" "${DEB_DIR}/usr/share/backgrounds/axis-wallpaper.png"
fi
if [ -d "data/theme" ]; then
    mkdir -p "${DEB_DIR}/usr/share/themes/Axis-Dark"
    cp -rf data/theme/* "${DEB_DIR}/usr/share/themes/Axis-Dark/" 2>/dev/null || true
fi

# Deploy systemd user services & timers
mkdir -p "${DEB_DIR}/etc/systemd/user"
if [ -f "os-build/configs/systemd/user/axis-update-notifier.service" ]; then
    cp -f "os-build/configs/systemd/user/axis-update-notifier.service" "${DEB_DIR}/etc/systemd/user/"
fi
if [ -f "os-build/configs/systemd/user/axis-update-notifier.timer" ]; then
    cp -f "os-build/configs/systemd/user/axis-update-notifier.timer" "${DEB_DIR}/etc/systemd/user/"
fi

# Build Debian Package
mkdir -p public_apt/pool/main public_apt/apt
DEB_NAME="axisos-desktop-suite_${VERSION}_amd64.deb"
dpkg-deb --build "${DEB_DIR}" "public_apt/pool/main/${DEB_NAME}"
cp -f "public_apt/pool/main/${DEB_NAME}" "public_apt/apt/${DEB_NAME}"

# Generate Repository Structure (Debian dists format)
mkdir -p public_apt/dists/bookworm/main/binary-amd64
cd public_apt

dpkg-scanpackages pool/main /dev/null > dists/bookworm/main/binary-amd64/Packages 2>/dev/null || true
gzip -9c dists/bookworm/main/binary-amd64/Packages > dists/bookworm/main/binary-amd64/Packages.gz

# Also generate flat repo structure inside public_apt/apt for backward compatibility
cd apt
if command -v dpkg-scanpackages >/dev/null 2>&1; then
    dpkg-scanpackages . /dev/null > Packages 2>/dev/null || true
else
    > Packages
fi
gzip -9c Packages > Packages.gz

# Generate Release metadata
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
SHA1:
 $(sha1sum Packages | awk '{print $1}') $(stat -c %s Packages) Packages
 $(sha1sum Packages.gz | awk '{print $1}') $(stat -c %s Packages.gz) Packages.gz
SHA256:
 $(sha256sum Packages | awk '{print $1}') $(stat -c %s Packages) Packages
 $(sha256sum Packages.gz | awk '{print $1}') $(stat -c %s Packages.gz) Packages.gz
EOF

cd ..

# Bookworm Release metadata
cat << EOF > dists/bookworm/Release
Origin: AxisOS
Label: AxisOS Bookworm Repository
Suite: bookworm
Codename: bookworm
Architectures: amd64
Components: main
Description: AxisOS Debian 12 Bookworm Repository
Date: $(date -Ru)
EOF

echo "======================================================================"
echo "  APT Repository Artifacts Generated Successfully under public_apt/   "
echo "======================================================================"
