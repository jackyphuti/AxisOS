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

# Generate control file
cat << 'EOF' > "${DEB_DIR}/DEBIAN/control"
Package: axisos-desktop-suite
Version: 2.1.0
Section: x11
Priority: optional
Architecture: amd64
Maintainer: Jacky Mpoka <jackympoka22@gmail.com>
Description: AxisOS Desktop Applications, HUD Task Manager & System Utilities
 AxisOS Native Suite featuring HUD Task Manager, Settings Panels,
 Terminal, Rewind CoW Engine, and Remote OTA Updates with snapshot protection.
EOF
chmod 644 "${DEB_DIR}/DEBIAN/control"

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

for s in axis-rewind.sh axis-sysupgrade.sh axis-game-boost.sh axis-neural-pulse.sh axis-usb-vault.sh axis-gaze-shield.sh axis-disk-info.sh axis-biometrics.sh; do
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
cat << EOF > Release
Origin: AxisOS
Label: AxisOS Official Repository
Suite: stable
Codename: axis
Architectures: amd64
Components: main
Description: AxisOS Remote OTA Updates Repository
Date: $(date -Ru)
EOF

echo "=== APT Repository Artifacts Generated Successfully ==="
