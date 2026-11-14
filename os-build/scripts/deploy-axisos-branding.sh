#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="${WORKSPACE_DIR:-$(cd "${SCRIPT_DIR}/../.." && pwd)}"
CHROOT="/build/chroot"
BRANDING_DIR="${REPO_ROOT}/data/branding"
WALLPAPERS_DIR="${REPO_ROOT}/data/wallpapers"

echo "=================================================="
echo "  Deploying AxisOS Pure C Branding & Identity     "
echo "=================================================="

# 1. os-release and lsb-release
mkdir -p "${CHROOT}/etc" "${CHROOT}/usr/lib"
cat << 'EOF' > "${CHROOT}/etc/os-release"
NAME="AxisOS"
PRETTY_NAME="AxisOS 2.0 (Pure C Edition)"
VERSION="2.0 (Pure C)"
VERSION_ID="2.0"
VERSION_CODENAME=bookworm
ID=axisos
ID_LIKE=debian
HOME_URL="https://github.com/jackyphuti/AxisOS"
SUPPORT_URL="https://github.com/jackyphuti/AxisOS/issues"
BUG_REPORT_URL="https://github.com/jackyphuti/AxisOS/issues"
LOGO=axisos
DOCUMENTATION_URL="https://github.com/jackyphuti/AxisOS"
EOF
cp -f "${CHROOT}/etc/os-release" "${CHROOT}/usr/lib/os-release"

cat << 'EOF' > "${CHROOT}/etc/lsb-release"
DISTRIB_ID=AxisOS
DISTRIB_RELEASE=2.0
DISTRIB_CODENAME=bookworm
DISTRIB_DESCRIPTION="AxisOS 2.0 (Pure C Edition)"
EOF

echo "AxisOS 2.0 Pure C \n \l" > "${CHROOT}/etc/issue"
echo "AxisOS 2.0 Pure C %h" > "${CHROOT}/etc/issue.net"

# 2. Icons & Pixmaps
mkdir -p "${CHROOT}/usr/share/pixmaps"
mkdir -p "${CHROOT}/usr/share/icons/hicolor/scalable/apps"
mkdir -p "${CHROOT}/usr/share/icons/hicolor/scalable/places"

if [ -f "${BRANDING_DIR}/axisos-logo.svg" ]; then
    cp -f "${BRANDING_DIR}/axisos-logo.svg" "${CHROOT}/usr/share/pixmaps/axisos.svg"
    cp -f "${BRANDING_DIR}/axisos-logo.svg" "${CHROOT}/usr/share/icons/hicolor/scalable/apps/axisos.svg"
    cp -f "${BRANDING_DIR}/axisos-logo.svg" "${CHROOT}/usr/share/icons/hicolor/scalable/places/distributor-logo-axisos.svg"
fi

# 3. Wallpapers
mkdir -p "${CHROOT}/usr/share/backgrounds/axisos"
if [ -d "$WALLPAPERS_DIR" ]; then
    cp -rf "${WALLPAPERS_DIR}/"* "${CHROOT}/usr/share/backgrounds/axisos/" 2>/dev/null || true
fi

# 4. Themes
mkdir -p "${CHROOT}/usr/share/axisos/theme"
if [ -d "${REPO_ROOT}/data/theme" ]; then
    cp -rf "${REPO_ROOT}/data/theme/"* "${CHROOT}/usr/share/axisos/theme/" 2>/dev/null || true
fi

echo "[SUCCESS] AxisOS OS branding and identity deployed."
