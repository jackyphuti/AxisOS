#!/usr/bin/env bash
set -e

CHROOT="/build/chroot"
WORKSPACE="${WORKSPACE_DIR:-$(cd "$(dirname "$0")/../.." && pwd)}"

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

src_kiosk="${WORKSPACE}/os-build/configs/cage-session/axisos-kiosk.sh"
if [ -f "$src_kiosk" ]; then
    cp -f "$src_kiosk" "${CHROOT}/usr/local/bin/axisos-kiosk.sh"
    chmod 755 "${CHROOT}/usr/local/bin/axisos-kiosk.sh"
fi

src_svc="${WORKSPACE}/os-build/configs/cage-session/axisos.service"
if [ -f "$src_svc" ]; then
    cp -f "$src_svc" "${CHROOT}/etc/systemd/system/axisos-session.service"
fi

for grp in video render input audio seat tty netdev sudo gamemode games; do
    chroot "$CHROOT" groupadd -f "$grp" 2>/dev/null || true
    chroot "$CHROOT" usermod -aG "$grp" axis 2>/dev/null || true
done

mkdir -p "${CHROOT}/run/user/1000"
chmod 700 "${CHROOT}/run/user/1000"
chroot "$CHROOT" chown -R 1000:1000 /home/axis /run/user/1000 2>/dev/null || true

chroot "$CHROOT" systemctl enable seatd.service 2>/dev/null || true
chroot "$CHROOT" systemctl enable axisos-session.service 2>/dev/null || true

echo "Chroot session fixes applied cleanly!"
