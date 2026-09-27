#!/usr/bin/env python3
import os
import shutil
import subprocess

chroot = "/build/chroot"
workspace = "/mnt/c/Users/jacky/documents/github/AxisOS"

print("=== 1. Setting up bind mounts for chroot ===")
mounts = ["/dev", "/dev/pts", "/proc", "/sys"]
for m in mounts:
    target = f"{chroot}{m}"
    os.makedirs(target, exist_ok=True)
    subprocess.run(["mount", "--bind", m, target], check=False)

def chroot_exec(cmd):
    return subprocess.run(["chroot", chroot, "bash", "-c", cmd], check=True)

try:
    print("=== 2. Updating package list and installing chromium & network-manager ===")
    chroot_exec("apt-get update")
    chroot_exec("DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends chromium network-manager polkitd xterm")

    print("=== 3. Deploying updated scripts and services ===")
    # Copy axisos-daemon.cjs
    src_daemon = f"{workspace}/os-build/configs/cage-session/axisos-daemon.cjs"
    shutil.copyfile(src_daemon, f"{chroot}/usr/local/bin/axisos-daemon.cjs")
    os.chmod(f"{chroot}/usr/local/bin/axisos-daemon.cjs", 0o755)
    
    os.makedirs(f"{chroot}/opt/axisos-shell", exist_ok=True)
    shutil.copyfile(src_daemon, f"{chroot}/opt/axisos-shell/server.cjs")
    os.chmod(f"{chroot}/opt/axisos-shell/server.cjs", 0o755)

    # Copy axisos-kiosk.sh
    src_kiosk = f"{workspace}/os-build/configs/cage-session/axisos-kiosk.sh"
    shutil.copyfile(src_kiosk, f"{chroot}/usr/local/bin/axisos-kiosk.sh")
    os.chmod(f"{chroot}/usr/local/bin/axisos-kiosk.sh", 0o755)

    # Copy systemd services
    src_svc = f"{workspace}/os-build/configs/cage-session/axisos.service"
    shutil.copyfile(src_svc, f"{chroot}/etc/systemd/system/axisos.service")

    src_dsvc = f"{workspace}/os-build/configs/cage-session/axisos-daemon.service"
    shutil.copyfile(src_dsvc, f"{chroot}/etc/systemd/system/axisos-daemon.service")

    print("=== 4. Configuring user groups and seat permissions ===")
    for grp in ["video", "render", "input", "audio", "seat", "tty", "netdev", "sudo"]:
        chroot_exec(f"groupadd -f {grp} || true")
        chroot_exec(f"usermod -aG {grp} axis || true")

    # Set up runtime directories and ownership
    os.makedirs(f"{chroot}/home/axis/.config/chromium", exist_ok=True)
    os.makedirs(f"{chroot}/home/axis/.cache", exist_ok=True)
    os.makedirs(f"{chroot}/run/user/1000", exist_ok=True)
    os.chmod(f"{chroot}/run/user/1000", 0o700)
    chroot_exec("chown -R 1000:1000 /home/axis /run/user/1000")

    print("=== 5. Enabling systemd services ===")
    chroot_exec("systemctl enable seatd || true")
    chroot_exec("systemctl enable axisos-daemon || true")
    chroot_exec("systemctl enable axisos || true")

    print("=== 6. Verifying Chromium binary ===")
    chroot_exec("which chromium")

finally:
    print("=== Unmounting bind mounts ===")
    for m in reversed(mounts):
        target = f"{chroot}{m}"
        subprocess.run(["umount", "-lf", target], check=False)

print("Chroot fixes applied successfully!")
