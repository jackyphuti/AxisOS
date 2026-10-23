#!/usr/bin/env python3
import os
import subprocess

chroot = "/build/chroot"

print("=== 1. Fixing /etc/network/interfaces (loopback only, NetworkManager handles interfaces) ===")
with open(f"{chroot}/etc/network/interfaces", "w") as f:
    f.write("""# AxisOS Network Interfaces
auto lo
iface lo inet loopback
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

def chroot_run(cmd):
    return subprocess.run(["chroot", chroot, "bash", "-c", cmd], check=False)

print("=== 3. Disabling blocking network-wait services ===")
chroot_run("systemctl disable NetworkManager-wait-online.service || true")
chroot_run("systemctl mask NetworkManager-wait-online.service || true")
chroot_run("systemctl disable ifupdown-wait-online.service || true")

print("=== 4. Enabling seatd and axisos-session.service ===")
chroot_run("systemctl enable seatd.service || true")
chroot_run("systemctl enable axisos-session.service || true")
chroot_run("chown -R 1000:1000 /home/axis || true")

print("Network and session configuration applied cleanly!")
