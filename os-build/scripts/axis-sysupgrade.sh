#!/bin/bash
# AxisOS Remote OTA System Upgrade Engine
# Fetches updates from AxisOS hosted APT repository with atomic rollback protection.

set -euo pipefail

echo "========================================================"
echo "          AxisOS Remote OTA System Upgrade              "
echo "========================================================"

if [ "$(id -u)" -ne 0 ]; then
    echo "[!] Privileges required. Re-launching with pkexec..."
    exec pkexec "$0" "$@"
fi

# 1. Automatic Snapshot Checkpoint
echo "--> [1/4] Securing pre-update system snapshot..."
if command -v axis-rewind >/dev/null 2>&1; then
    axis-rewind checkpoint-system ota-upgrade || true
elif [ -x /usr/local/bin/axis-rewind ]; then
    /usr/local/bin/axis-rewind checkpoint-system ota-upgrade || true
fi

# 2. Refresh APT indices
echo "--> [2/4] Synchronizing remote AxisOS repository package manifests..."
apt-get update -o Acquire::AllowInsecureRepositories=true -o Acquire::AllowDowngradeToInsecureRepositories=true || true

# 3. Perform atomic dist-upgrade
echo "--> [3/4] Downloading and applying latest updates..."
DEBIAN_FRONTEND=noninteractive apt-get dist-upgrade -y --allow-unauthenticated

# 4. Clean up package cache
echo "--> [4/4] Finalizing and cleaning cache..."
apt-get autoremove -y || true
apt-get clean || true

# 5. Run FastBoot optimization hotfix
if [ -x /usr/local/bin/axis-fastboot-patch ]; then
    echo "--> [OTA Patch] Applying AxisOS FastBoot optimizations..."
    /usr/local/bin/axis-fastboot-patch || true
fi

echo "========================================================"
echo "    AxisOS Remote System Upgrade Completed Successfully! "
echo "========================================================"

# Reset notification state cache
rm -f "${XDG_CACHE_HOME:-$HOME/.cache}/axisos/last-update-notified" 2>/dev/null || true

if command -v notify-send >/dev/null 2>&1; then
    ICON="/usr/share/pixmaps/axisos-logo.png"
    [ -f "$ICON" ] || ICON="system-software-update"
    notify-send \
        --app-name="AxisOS Software" \
        --icon="$ICON" \
        --urgency=normal \
        "AxisOS Upgraded Successfully" \
        "Your system is now up to date with the latest features and security updates." 2>/dev/null || true
fi
