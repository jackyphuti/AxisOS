#!/usr/bin/env python3
import os
import shutil
import subprocess

chroot = "/build/chroot"
workspace = os.environ.get("WORKSPACE_DIR") or os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

print("==================================================")
print("  AxisOS Production System & Real Hardware Setup  ")
print("==================================================")

mounts = ["/dev", "/dev/pts", "/proc", "/sys"]
for m in mounts:
    target = f"{chroot}{m}"
    os.makedirs(target, exist_ok=True)
    subprocess.run(["mount", "--bind", m, target], check=False)

def chroot_exec(cmd):
    return subprocess.run(["chroot", chroot, "bash", "-c", cmd], check=True)

try:
    print("--> 1. Installing hardware firmware (Intel, AMD, Realtek WiFi & GPU)")
    chroot_exec("apt-get update")
    chroot_exec("DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends firmware-misc-nonfree firmware-realtek firmware-iwlwifi")
    chroot_exec("apt-get clean")

    print("--> 2. Deploying native Wayland session launcher (axisos-kiosk.sh) & service")
    src_kiosk = f"{workspace}/os-build/configs/cage-session/axisos-kiosk.sh"
    shutil.copyfile(src_kiosk, f"{chroot}/usr/local/bin/axisos-kiosk.sh")
    os.chmod(f"{chroot}/usr/local/bin/axisos-kiosk.sh", 0o755)

    src_svc = f"{workspace}/os-build/configs/cage-session/axisos.service"
    shutil.copyfile(src_svc, f"{chroot}/etc/systemd/system/axisos-session.service")
    os.chmod(f"{chroot}/etc/systemd/system/axisos-session.service", 0o644)

    os.makedirs(f"{chroot}/etc/systemd/system/graphical.target.wants", exist_ok=True)
    sess_want = f"{chroot}/etc/systemd/system/graphical.target.wants/axisos-session.service"
    if not os.path.lexists(sess_want):
        os.symlink("/etc/systemd/system/axisos-session.service", sess_want)

    chroot_exec("systemctl enable seatd || true")

    def_target = f"{chroot}/etc/systemd/system/default.target"
    if os.path.islink(def_target) or os.path.exists(def_target):
        os.remove(def_target)
    os.symlink("/lib/systemd/system/graphical.target", def_target)

    for grp in ["video", "render", "input", "audio", "seat", "tty", "netdev", "sudo", "gamemode", "games", "docker"]:
        chroot_exec(f"groupadd -f {grp} || true")
        chroot_exec(f"usermod -aG {grp} axis || true")

    chroot_exec("chown -R 1000:1000 /home/axis || true")

finally:
    for m in reversed(mounts):
        subprocess.run(["umount", "-lf", f"{chroot}{m}"], check=False)

print("Production system configuration updated cleanly!")
