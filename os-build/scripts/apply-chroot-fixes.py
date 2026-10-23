#!/usr/bin/env python3
import os
import shutil
import subprocess

chroot = "/build/chroot"
workspace = os.environ.get("WORKSPACE_DIR") or os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

mounts = ["/dev", "/dev/pts", "/proc", "/sys"]
for m in mounts:
    target = f"{chroot}{m}"
    os.makedirs(target, exist_ok=True)
    subprocess.run(["mount", "--bind", m, target], check=False)

def chroot_exec(cmd):
    return subprocess.run(["chroot", chroot, "bash", "-c", cmd], check=True)

try:
    src_kiosk = f"{workspace}/os-build/configs/cage-session/axisos-kiosk.sh"
    shutil.copyfile(src_kiosk, f"{chroot}/usr/local/bin/axisos-kiosk.sh")
    os.chmod(f"{chroot}/usr/local/bin/axisos-kiosk.sh", 0o755)

    src_svc = f"{workspace}/os-build/configs/cage-session/axisos.service"
    shutil.copyfile(src_svc, f"{chroot}/etc/systemd/system/axisos-session.service")

    for grp in ["video", "render", "input", "audio", "seat", "tty", "netdev", "sudo"]:
        chroot_exec(f"groupadd -f {grp} || true")
        chroot_exec(f"usermod -aG {grp} axis || true")

    os.makedirs(f"{chroot}/run/user/1000", exist_ok=True)
    os.chmod(f"{chroot}/run/user/1000", 0o700)
    chroot_exec("chown -R 1000:1000 /home/axis /run/user/1000 || true")

    chroot_exec("systemctl enable seatd.service || true")
    chroot_exec("systemctl enable axisos-session.service || true")
finally:
    for m in reversed(mounts):
        subprocess.run(["umount", "-lf", f"{chroot}{m}"], check=False)

print("Chroot session fixes applied cleanly!")
