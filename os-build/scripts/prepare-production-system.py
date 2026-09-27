#!/usr/bin/env python3
import os
import shutil
import subprocess

chroot = "/build/chroot"
workspace = "/mnt/c/Users/jacky/documents/github/AxisOS"
binary = "/build/binary"

print("==================================================")
print("  AxisOS Production System & Real Hardware Setup  ")
print("==================================================")

# 1. Mount bind mounts for chroot
print("--> 1. Mounting bind mounts for chroot")
mounts = ["/dev", "/dev/pts", "/proc", "/sys"]
for m in mounts:
    target = f"{chroot}{m}"
    os.makedirs(target, exist_ok=True)
    subprocess.run(["mount", "--bind", m, target], check=False)

def chroot_exec(cmd):
    return subprocess.run(["chroot", chroot, "bash", "-c", cmd], check=True)

try:
    # 2. Install hardware firmware for real machines
    print("--> 2. Installing real machine hardware firmware (Intel, AMD, Realtek WiFi & GPU)")
    chroot_exec("apt-get update")
    chroot_exec("DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends firmware-misc-nonfree firmware-realtek firmware-iwlwifi xterm")
    chroot_exec("apt-get clean")
    chroot_exec("rm -rf /var/cache/apt/archives/*.deb /var/cache/apt/archives/partial/*")

    # 3. Create root assets symlink so any relative/absolute path resolves
    print("--> 3. Configuring shell asset links")
    if not os.path.exists(f"{chroot}/assets"):
        subprocess.run(["ln", "-sf", "/opt/axisos-shell/dist/assets", f"{chroot}/assets"], check=False)

    # 4. Deploy updated axisos-daemon.cjs
    print("--> 4. Deploying axisos-daemon.cjs")
    src_daemon = f"{workspace}/os-build/configs/cage-session/axisos-daemon.cjs"
    shutil.copyfile(src_daemon, f"{chroot}/usr/local/bin/axisos-daemon.cjs")
    os.chmod(f"{chroot}/usr/local/bin/axisos-daemon.cjs", 0o755)

    os.makedirs(f"{chroot}/opt/axisos-shell", exist_ok=True)
    shutil.copyfile(src_daemon, f"{chroot}/opt/axisos-shell/server.cjs")
    os.chmod(f"{chroot}/opt/axisos-shell/server.cjs", 0o755)

    # 5. Deploy production axisos-kiosk.sh
    print("--> 5. Deploying production axisos-kiosk.sh")
    kiosk_content = """#!/usr/bin/env bash
# ==============================================================================
# AxisOS Production Kiosk Launcher
# Compatible with both Real Hardware (Intel/AMD/Nvidia) and Virtual Machines (QEMU/KVM)
# ==============================================================================

# Setup XDG runtime directory with required 0700 permissions
UID_NUM=$(id -u)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/${UID_NUM}}"
mkdir -p "$XDG_RUNTIME_DIR" 2>/dev/null || true
chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true

# Display and compositor environment
export XDG_SESSION_TYPE=wayland
export XDG_CURRENT_DESKTOP=AxisOS
export WAYLAND_DISPLAY=wayland-0
export OZONE_PLATFORM=wayland
export MOZ_ENABLE_WAYLAND=1
export GDK_BACKEND=wayland
export QT_QPA_PLATFORM=wayland
export WLR_LIBINPUT_NO_DEVICES=1
export WLR_RENDERER_ALLOW_SOFTWARE=1
export LIBSEAT_BACKEND=seatd

# Create user directories
mkdir -p /home/axis/.config/chromium 2>/dev/null || true
mkdir -p /home/axis/.cache 2>/dev/null || true

# Start system daemon if not already responding
if ! curl -s -f http://127.0.0.1:3000/api/system-info >/dev/null 2>&1; then
    /usr/bin/node /usr/local/bin/axisos-daemon.cjs >> /home/axis/.daemon.log 2>&1 &
fi

# Wait for local daemon to be healthy
URL="http://127.0.0.1:3000"
for i in {1..60}; do
    if curl -s -f http://127.0.0.1:3000/api/system-info >/dev/null 2>&1; then
        break
    fi
    sleep 0.2
done

CHROME_BIN=$(command -v chromium || command -v chromium-browser || true)

if [ -n "$CHROME_BIN" ]; then
    # Launch Cage with Chromium in kiosk mode
    exec cage -s -d -- "$CHROME_BIN" \\
        --kiosk \\
        --ozone-platform=wayland \\
        --enable-features=UseOzonePlatform,WaylandWindowDecorations \\
        --no-sandbox \\
        --disable-dev-shm-usage \\
        --disable-gpu-sandbox \\
        --in-process-gpu \\
        --ignore-gpu-blocklist \\
        --enable-gpu-rasterization \\
        --enable-zero-copy \\
        --allow-file-access-from-files \\
        --disable-features=Translate,OptimizationHints,MediaRouter \\
        --noerrdialogs \\
        --disable-infobars \\
        --disable-session-crashed-bubble \\
        --no-first-run \\
        --no-default-browser-check \\
        --check-for-update-interval=31536000 \\
        --password-store=basic \\
        --user-data-dir=/home/axis/.config/chromium \\
        --window-position=0,0 \\
        --window-size=1280,800 \\
        --start-maximized \\
        --app="$URL" >> /home/axis/.kiosk.log 2>&1
else
    echo "Error: Chromium not found in PATH." >&2
    exec cage -s -- xterm >> /home/axis/.kiosk.log 2>&1
fi
"""
    with open(f"{chroot}/usr/local/bin/axisos-kiosk.sh", "w") as f:
        f.write(kiosk_content)
    os.chmod(f"{chroot}/usr/local/bin/axisos-kiosk.sh", 0o755)

    with open(f"{workspace}/os-build/configs/cage-session/axisos-kiosk.sh", "w") as f:
        f.write(kiosk_content)

    # 6. Deploy production axisos.service
    print("--> 6. Deploying production axisos.service")
    svc_content = """[Unit]
Description=AxisOS Desktop Shell Session
After=systemd-user-sessions.service network.target sound.target axisos-daemon.service seatd.service
Wants=network.target sound.target axisos-daemon.service seatd.service
Conflicts=getty@tty1.service

[Service]
User=axis
Group=axis
PAMName=login
Type=simple
StandardInput=null
StandardOutput=journal
StandardError=journal
Environment=XDG_SESSION_TYPE=wayland
Environment=XDG_CURRENT_DESKTOP=AxisOS
Environment=WAYLAND_DISPLAY=wayland-0
Environment=XDG_RUNTIME_DIR=/run/user/1000
ExecStart=/usr/local/bin/axisos-kiosk.sh
Restart=always
RestartSec=2

[Install]
WantedBy=graphical.target
"""
    with open(f"{chroot}/etc/systemd/system/axisos.service", "w") as f:
        f.write(svc_content)
    os.chmod(f"{chroot}/etc/systemd/system/axisos.service", 0o644)

    with open(f"{workspace}/os-build/configs/cage-session/axisos.service", "w") as f:
        f.write(svc_content)

    # 7. Configure Systemd targets and enable services
    print("--> 7. Configuring service links")
    # Link axisos.service into graphical.target.wants
    os.makedirs(f"{chroot}/etc/systemd/system/graphical.target.wants", exist_ok=True)
    axisos_want = f"{chroot}/etc/systemd/system/graphical.target.wants/axisos.service"
    if os.path.islink(axisos_want) or os.path.exists(axisos_want):
        os.remove(axisos_want)
    os.symlink("/etc/systemd/system/axisos.service", axisos_want)

    # Link axisos-daemon.service into multi-user.target.wants
    os.makedirs(f"{chroot}/etc/systemd/system/multi-user.target.wants", exist_ok=True)
    daemon_want = f"{chroot}/etc/systemd/system/multi-user.target.wants/axisos-daemon.service"
    if not os.path.lexists(daemon_want):
        os.symlink("/etc/systemd/system/axisos-daemon.service", daemon_want)

    # Enable seatd
    chroot_exec("systemctl enable seatd || true")

    # Set default target to graphical.target
    def_target = f"{chroot}/etc/systemd/system/default.target"
    if os.path.islink(def_target) or os.path.exists(def_target):
        os.remove(def_target)
    os.symlink("/lib/systemd/system/graphical.target", def_target)

    # User groups
    for grp in ["video", "render", "input", "audio", "seat", "tty", "netdev", "sudo"]:
        chroot_exec(f"groupadd -f {grp} || true")
        chroot_exec(f"usermod -aG {grp} axis || true")

    chroot_exec("chown -R 1000:1000 /home/axis")

finally:
    print("--> 8. Unmounting bind mounts")
    for m in reversed(mounts):
        target = f"{chroot}{m}"
        subprocess.run(["umount", "-lf", target], check=False)

print("System and hardware configuration successfully updated!")
