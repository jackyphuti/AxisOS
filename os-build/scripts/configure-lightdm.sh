#!/usr/bin/env bash
set -e

CHROOT="/build/chroot"

echo "=================================================="
echo "  Configuring LightDM Autologin & Session Setup   "
echo "=================================================="

# 1. LightDM autologin drop-in
echo "--> 1. Configuring LightDM 01_autologin.conf"
mkdir -p "${CHROOT}/etc/lightdm/lightdm.conf.d"
cat << 'EOF' > "${CHROOT}/etc/lightdm/lightdm.conf.d/01_autologin.conf"
[Seat:*]
autologin-user=axis
autologin-user-timeout=0
user-session=axisos
EOF

# 2. XSession Desktop Entry
echo "--> 2. Creating AxisOS XSession (/usr/share/xsessions/axisos.desktop)"
mkdir -p "${CHROOT}/usr/share/xsessions"
cat << 'EOF' > "${CHROOT}/usr/share/xsessions/axisos.desktop"
[Desktop Entry]
Name=AxisOS
Comment=AxisOS Desktop Shell Session
Exec=/usr/local/bin/axisos-session
Type=Application
EOF

# 2b. Touchpad Tapping & Natural Scrolling
echo "--> 2b. Configuring Touchpad Tapping (/etc/X11/xorg.conf.d/40-libinput-touchpad.conf)"
mkdir -p "${CHROOT}/etc/X11/xorg.conf.d"
cat << 'EOF' > "${CHROOT}/etc/X11/xorg.conf.d/40-libinput-touchpad.conf"
Section "InputClass"
    Identifier "libinput touchpad catchall"
    MatchIsTouchpad "on"
    MatchDevicePath "/dev/input/event*"
    Driver "libinput"
    Option "Tapping" "on"
    Option "NaturalScrolling" "true"
EndSection
EOF

# 3. User groups
echo "--> 3. Adding user axis to autologin, video, render, audio groups"
chroot "$CHROOT" groupadd -f autologin 2>/dev/null || true
chroot "$CHROOT" gpasswd -a axis autologin 2>/dev/null || true
for grp in video render audio input seat sudo netdev; do
    chroot "$CHROOT" usermod -aG "$grp" axis 2>/dev/null || true
done

# 4. Clean up old tty1 conflicting drop-ins
echo "--> 4. Cleaning up old tty1 service conflicts"
rm -rf "${CHROOT}/etc/systemd/system/getty@tty1.service.d"
rm -f "${CHROOT}/etc/systemd/system/graphical.target.wants/axisos.service"

echo "LightDM session and user configuration finished."
