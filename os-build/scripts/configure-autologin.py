#!/usr/bin/env python3
import os
import subprocess

chroot = "/build/chroot"

sys_dir = f"{chroot}/etc/systemd/system"
os.makedirs(f"{sys_dir}/graphical.target.wants", exist_ok=True)

try:
    os.remove(f"{sys_dir}/default.target")
except FileNotFoundError:
    pass
os.symlink("/lib/systemd/system/graphical.target", f"{sys_dir}/default.target")

sess_svc = f"{sys_dir}/graphical.target.wants/axisos-session.service"
if not os.path.lexists(sess_svc):
    os.symlink("/etc/systemd/system/axisos-session.service", sess_svc)

# Allow blank password in PAM if needed in live session
pam_path = f"{chroot}/etc/pam.d/common-auth"
if os.path.exists(pam_path):
    with open(pam_path, "r") as f:
        pam = f.read()
    pam = pam.replace("nullok_secure", "nullok")
    with open(pam_path, "w") as f:
        f.write(pam)

subprocess.run(["chroot", chroot, "chown", "-R", "1000:1000", "/home/axis"], check=False)
print("Direct Wayland graphical target and autologin configured cleanly.")
