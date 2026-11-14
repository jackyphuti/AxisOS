#!/usr/bin/env bash
set -e

REPO_ROOT="${WORKSPACE_DIR:-$(cd "$(dirname "$0")/../.." && pwd)}"
CHROOT="/build/chroot"
SRC_APPS="${REPO_ROOT}/os-build/apps"
SCRIPT="${REPO_ROOT}/os-build/scripts/compile-native-apps.sh"

echo "=== Setting up Pure C (-std=gnu11) compilation inside chroot ==="

MOUNTS=("/dev" "/dev/pts" "/proc" "/sys")
for m in "${MOUNTS[@]}"; do
    mkdir -p "${CHROOT}${m}"
    mount --bind "$m" "${CHROOT}${m}" 2>/dev/null || true
done

cleanup() {
    for (( i=${#MOUNTS[@]}-1; i>=0; i-- )); do
        umount -lf "${CHROOT}${MOUNTS[i]}" 2>/dev/null || true
    done
}
trap cleanup EXIT

TARGET_ROOT="${CHROOT}/tmp/axisos-build"
rm -rf "$TARGET_ROOT"
mkdir -p "${TARGET_ROOT}/os-build/scripts"

# Copy apps, shell-c, compositor, updater, pkg-mgr into chroot
cp -r "$SRC_APPS" "${TARGET_ROOT}/os-build/apps"

if [ -d "${REPO_ROOT}/shell-c" ]; then
    cp -r "${REPO_ROOT}/shell-c" "${TARGET_ROOT}/shell-c"
fi
if [ -d "${REPO_ROOT}/compositor" ]; then
    cp -r "${REPO_ROOT}/compositor" "${TARGET_ROOT}/compositor"
fi
if [ -d "${REPO_ROOT}/os-build/updater" ]; then
    cp -r "${REPO_ROOT}/os-build/updater" "${TARGET_ROOT}/os-build/updater"
fi
if [ -d "${REPO_ROOT}/pkg-mgr" ]; then
    cp -r "${REPO_ROOT}/pkg-mgr" "${TARGET_ROOT}/pkg-mgr"
fi

cp -f "$SCRIPT" "${TARGET_ROOT}/os-build/scripts/compile-native-apps.sh"
chmod 755 "${TARGET_ROOT}/os-build/scripts/compile-native-apps.sh"

echo "=== Running GCC Pure C build inside chroot ===")
chroot "$CHROOT" /bin/bash /tmp/axisos-build/os-build/scripts/compile-native-apps.sh /tmp/axisos-build/os-build/apps

echo "=== Copying compiled ELF binaries back to workspace ==="
APPS=(
    "axis-terminal"
    "axis-installer"
    "axis-settings"
    "axis-usb-daemon"
    "axis-task-manager"
    "nobara-welcome"
    "axis-oobe"
    "axis-package-installer"
    "axis-scheduler-manager"
    "axis-welcome"
    "axis-software-manager"
    "axis-update-manager"
)

for app in "${APPS[@]}"; do
    built_bin="${TARGET_ROOT}/os-build/apps/${app}/${app}"
    repo_bin="${SRC_APPS}/${app}/${app}"
    if [ -f "$built_bin" ]; then
        cp -f "$built_bin" "$repo_bin"
        chmod 755 "$repo_bin"
        echo "  [OK] ${app} -> ${repo_bin}"
    fi
done

# Copy shell-c binary
if [ -f "${TARGET_ROOT}/shell-c/axisos-shell" ]; then
    mkdir -p "${REPO_ROOT}/shell-c"
    cp -f "${TARGET_ROOT}/shell-c/axisos-shell" "${REPO_ROOT}/shell-c/axisos-shell"
    ln -sf axisos-shell "${REPO_ROOT}/shell-c/LiquidNodesOS"
    echo "  [OK] axisos-shell -> ${REPO_ROOT}/shell-c/axisos-shell"
fi

# Copy compositor binary
if [ -f "${TARGET_ROOT}/compositor/axis-compositor" ]; then
    mkdir -p "${REPO_ROOT}/compositor"
    cp -f "${TARGET_ROOT}/compositor/axis-compositor" "${REPO_ROOT}/compositor/axis-compositor"
    echo "  [OK] axis-compositor -> ${REPO_ROOT}/compositor/axis-compositor"
fi

echo "=== All Pure C applications, compositor, and shell compiled! ==="
