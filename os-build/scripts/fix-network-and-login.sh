#!/usr/bin/env bash
set -e

CHROOT="/build/chroot"

echo "=== 1. Fixing /etc/network/interfaces ==="
cat << 'EOF' > "${CHROOT}/etc/network/interfaces"
# AxisOS Network Interfaces
auto lo
iface lo inet loopback
EOF

echo "=== 2. Configuring NetworkManager ==="
mkdir -p "${CHROOT}/etc/NetworkManager"
cat << 'EOF' > "${CHROOT}/etc/NetworkManager/NetworkManager.conf"
[main]
plugins=ifupdown,keyfile

[ifupdown]
managed=true

[device]
wifi.scan-rand-mac-address=no
EOF

echo "=== 3. Disabling blocking network-wait services ==="
chroot "$CHROOT" systemctl disable NetworkManager-wait-online.service 2>/dev/null || true
chroot "$CHROOT" systemctl mask NetworkManager-wait-online.service 2>/dev/null || true
chroot "$CHROOT" systemctl disable ifupdown-wait-online.service 2>/dev/null || true

echo "=== 4. Enabling seatd and axisos-session.service ===")
chroot "$CHROOT" systemctl enable seatd.service 2>/dev/null || true
chroot "$CHROOT" systemctl enable axisos-session.service 2>/dev/null || true
chroot "$CHROOT" chown -R 1000:1000 /home/axis 2>/dev/null || true

echo "Network and session configuration applied cleanly!"
