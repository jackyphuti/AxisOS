#!/usr/bin/env python3
import os
import subprocess

chroot = "/build/chroot"

print("=== 1. Fixing /etc/network/interfaces ===")
with open(f"{chroot}/etc/network/interfaces", "w") as f:
    f.write("""# AxisOS Network Interfaces
auto lo
iface lo inet loopback

allow-hotplug eth0
iface eth0 inet dhcp
""")

print("=== 2. Configuring NetworkManager ===")
nm_conf = f"{chroot}/etc/NetworkManager/NetworkManager.conf"
os.makedirs(os.path.dirname(nm_conf), exist_ok=True)
with open(nm_conf, "w") as f:
    f.write("""[main]
plugins=ifupdown,keyfile

[ifupdown]
managed=true

[device]
wifi.scan-rand-mac-address=no
""")

print("=== 3. Disabling blocking network-wait services ===")
def chroot_run(cmd):
    return subprocess.run(["chroot", chroot, "bash", "-c", cmd], check=False)

chroot_run("systemctl disable NetworkManager-wait-online.service || true")
chroot_run("systemctl mask NetworkManager-wait-online.service || true")
chroot_run("systemctl disable ifupdown-wait-online.service || true")

print("=== 4. Configuring clean TTY1 autologin without service conflicts ===")
# Remove conflicting axisos.service from graphical.target.wants
axisos_want = f"{chroot}/etc/systemd/system/graphical.target.wants/axisos.service"
if os.path.islink(axisos_want) or os.path.exists(axisos_want):
    os.remove(axisos_want)

# Ensure axisos-daemon.service IS enabled in multi-user.target.wants
daemon_want = f"{chroot}/etc/systemd/system/multi-user.target.wants/axisos-daemon.service"
os.makedirs(os.path.dirname(daemon_want), exist_ok=True)
if not os.path.lexists(daemon_want):
    os.symlink("/etc/systemd/system/axisos-daemon.service", daemon_want)

# Ensure getty@tty1 autologin drop-in is active
getty_dropin = f"{chroot}/etc/systemd/system/getty@tty1.service.d/autologin.conf"
os.makedirs(os.path.dirname(getty_dropin), exist_ok=True)
with open(getty_dropin, "w") as f:
    f.write("""[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin axis --noclear %I $TERM
Type=idle
""")

# Configure .profile and .bash_profile for user axis
hook = """
# Auto-start AxisOS Wayland desktop on tty1 login
if [ -z "$WAYLAND_DISPLAY" ] && [ -z "$DISPLAY" ] && [ "$(tty 2>/dev/null)" = "/dev/tty1" ]; then
    exec /usr/local/bin/axisos-kiosk.sh
fi
"""

for p in [f"{chroot}/home/axis/.profile", f"{chroot}/home/axis/.bash_profile"]:
    content = ""
    if os.path.exists(p):
        with open(p, "r") as f:
            content = f.read()
    if "axisos-kiosk.sh" not in content:
        with open(p, "a") as f:
            f.write(hook)

chroot_run("chown -R 1000:1000 /home/axis")

print("=== 5. Enabling seatd service ===")
chroot_run("systemctl enable seatd || true")

print("Network and autologin fixes successfully applied!")
