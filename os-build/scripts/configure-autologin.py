#!/usr/bin/env python3
import os
import subprocess

chroot = "/build/chroot"

# 1. Getty autologin drop-in
getty_dir = f"{chroot}/etc/systemd/system/getty@tty1.service.d"
os.makedirs(getty_dir, exist_ok=True)
with open(f"{getty_dir}/autologin.conf", "w") as f:
    f.write("""[Service]
ExecStart=
ExecStart=-/sbin/agetty --autologin axis --noclear %I $TERM
Type=idle
""")

# 2. Systemd graphical target & service activation
sys_dir = f"{chroot}/etc/systemd/system"
os.makedirs(f"{sys_dir}/graphical.target.wants", exist_ok=True)
os.makedirs(f"{sys_dir}/multi-user.target.wants", exist_ok=True)

try:
    os.remove(f"{sys_dir}/default.target")
except FileNotFoundError:
    pass
os.symlink("/lib/systemd/system/graphical.target", f"{sys_dir}/default.target")

axisos_svc = f"{sys_dir}/graphical.target.wants/axisos.service"
if not os.path.lexists(axisos_svc):
    os.symlink("/etc/systemd/system/axisos.service", axisos_svc)

daemon_svc = f"{sys_dir}/multi-user.target.wants/axisos-daemon.service"
if not os.path.lexists(daemon_svc):
    os.symlink("/etc/systemd/system/axisos-daemon.service", daemon_svc)

# 3. .profile autostart fallback for axis user
profile_path = f"{chroot}/home/axis/.profile"
hook = """
# Auto-start AxisOS Wayland desktop on tty1 login
if [ -z "$WAYLAND_DISPLAY" ] && [ -z "$DISPLAY" ] && [ "$(tty 2>/dev/null)" = "/dev/tty1" ]; then
    exec /usr/local/bin/axisos-kiosk.sh
fi
"""
if os.path.exists(profile_path):
    with open(profile_path, "r") as f:
        existing = f.read()
    if "axisos-kiosk.sh" not in existing:
        with open(profile_path, "a") as f:
            f.write(hook)
else:
    with open(profile_path, "w") as f:
        f.write(hook)

# Fix permissions
subprocess.run(["chroot", chroot, "chown", "-R", "1000:1000", "/home/axis"])

# 4. Allow blank password in PAM
pam_path = f"{chroot}/etc/pam.d/common-auth"
if os.path.exists(pam_path):
    with open(pam_path, "r") as f:
        pam = f.read()
    pam = pam.replace("nullok_secure", "nullok")
    with open(pam_path, "w") as f:
        f.write(pam)

print("Direct boot and autologin configuration successfully applied to chroot.")
