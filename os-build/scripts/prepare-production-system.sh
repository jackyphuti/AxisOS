#!/usr/bin/env bash
set -e

CHROOT="/build/chroot"
WORKSPACE="${WORKSPACE_DIR:-$(cd "$(dirname "$0")/../.." && pwd)}"

echo "=================================================="
echo "  AxisOS Production System & Real Hardware Setup  "
echo "=================================================="

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

echo "--> 1. Installing hardware firmware"
chroot "$CHROOT" apt-get update 2>/dev/null || true
chroot "$CHROOT" env DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends firmware-misc-nonfree firmware-realtek firmware-iwlwifi 2>/dev/null || true
chroot "$CHROOT" apt-get clean 2>/dev/null || true

echo "--> 2. Deploying native Wayland session launcher"
src_kiosk="${WORKSPACE}/os-build/configs/cage-session/axisos-kiosk.sh"
if [ -f "$src_kiosk" ]; then
    cp -f "$src_kiosk" "${CHROOT}/usr/local/bin/axisos-kiosk.sh"
    chmod 755 "${CHROOT}/usr/local/bin/axisos-kiosk.sh"
fi

src_svc="${WORKSPACE}/os-build/configs/cage-session/axisos.service"
if [ -f "$src_svc" ]; then
    cp -f "$src_svc" "${CHROOT}/etc/systemd/system/axisos-session.service"
    chmod 644 "${CHROOT}/etc/systemd/system/axisos-session.service"
fi

mkdir -p "${CHROOT}/etc/systemd/system/graphical.target.wants"
ln -sf /etc/systemd/system/axisos-session.service "${CHROOT}/etc/systemd/system/graphical.target.wants/axisos-session.service"

chroot "$CHROOT" systemctl enable seatd 2>/dev/null || true

rm -f "${CHROOT}/etc/systemd/system/default.target"
ln -sf /lib/systemd/system/graphical.target "${CHROOT}/etc/systemd/system/default.target"

for grp in video render input audio seat tty netdev sudo gamemode games docker; do
    chroot "$CHROOT" groupadd -f "$grp" 2>/dev/null || true
    chroot "$CHROOT" usermod -aG "$grp" axis 2>/dev/null || true
done

chroot "$CHROOT" chown -R 1000:1000 /home/axis 2>/dev/null || true

echo "Production system configuration updated cleanly!"
