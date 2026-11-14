#!/usr/bin/env bash
set -e

CHROOT="/build/chroot"
SYS_DIR="${CHROOT}/etc/systemd/system"

mkdir -p "${SYS_DIR}/graphical.target.wants"

rm -f "${SYS_DIR}/default.target"
ln -sf /lib/systemd/system/graphical.target "${SYS_DIR}/default.target"

SESS_SVC="${SYS_DIR}/graphical.target.wants/axisos-session.service"
if [ ! -e "$SESS_SVC" ]; then
    ln -sf /etc/systemd/system/axisos-session.service "$SESS_SVC"
fi

PAM_PATH="${CHROOT}/etc/pam.d/common-auth"
if [ -f "$PAM_PATH" ]; then
    sed -i 's/nullok_secure/nullok/g' "$PAM_PATH"
fi

chroot "$CHROOT" chown -R 1000:1000 /home/axis 2>/dev/null || true

echo "Direct Wayland graphical target and autologin configured cleanly."
