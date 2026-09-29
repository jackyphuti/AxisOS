#!/usr/bin/env python3
import os
import shutil
import subprocess

chroot = "/build/chroot"
workspace = "/mnt/c/Users/jacky/documents/github/AxisOS"

print("==================================================")
print("  Configuring LightDM Autologin & X11 Session    ")
print("==================================================")

# 1. LightDM autologin drop-in
print("--> 1. Configuring LightDM 01_autologin.conf")
os.makedirs(f"{chroot}/etc/lightdm/lightdm.conf.d", exist_ok=True)
with open(f"{chroot}/etc/lightdm/lightdm.conf.d/01_autologin.conf", "w") as f:
    f.write("""[Seat:*]
autologin-user=axis
autologin-user-timeout=0
user-session=axisos
""")

# 2. XSession Desktop Entry
print("--> 2. Creating AxisOS XSession (/usr/share/xsessions/axisos.desktop)")
os.makedirs(f"{chroot}/usr/share/xsessions", exist_ok=True)
with open(f"{chroot}/usr/share/xsessions/axisos.desktop", "w") as f:
    f.write("""[Desktop Entry]
Name=AxisOS
Comment=AxisOS Desktop Shell Session
Exec=/usr/local/bin/axisos-xsession.sh
Type=Application
""")

# 3. AxisOS XSession Launcher Script
print("--> 3. Creating /usr/local/bin/axisos-xsession.sh")
xsession_script = """#!/usr/bin/env bash
# ==============================================================================
# AxisOS Production X11 Desktop Session
# ==============================================================================

UID_NUM=$(id -u)
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/${UID_NUM}}"
mkdir -p "$XDG_RUNTIME_DIR" 2>/dev/null || true
chmod 700 "$XDG_RUNTIME_DIR" 2>/dev/null || true

export XDG_SESSION_TYPE=x11
export XDG_CURRENT_DESKTOP=AxisOS
export DISPLAY="${DISPLAY:-:0}"

# Start AxisOS background system daemon if not responding
if ! curl -s -f http://127.0.0.1:3000/api/system-info >/dev/null 2>&1; then
    /usr/bin/node /usr/local/bin/axisos-daemon.cjs >> /home/axis/.daemon.log 2>&1 &
fi

# Wait for local daemon to be healthy
for i in {1..50}; do
    if curl -s -f http://127.0.0.1:3000/api/system-info >/dev/null 2>&1; then
        break
    fi
    sleep 0.2
done

# Disable screen blanking & DPMS
xset s off 2>/dev/null || true
xset -dpms 2>/dev/null || true
xset s noblank 2>/dev/null || true

# Set clean slate background
xsetroot -solid "#020617" 2>/dev/null || true

# Start Openbox window manager
openbox &

# Launch Chromium in fullscreen kiosk mode loading AxisOS Shell
CHROME_BIN=$(command -v chromium || command -v chromium-browser || true)

exec "$CHROME_BIN" \\
    --kiosk \\
    --no-sandbox \\
    --disable-dev-shm-usage \\
    --disable-gpu-sandbox \\
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
    --start-maximized \\
    --app="http://127.0.0.1:3000" >> /home/axis/.kiosk.log 2>&1
"""

xsession_path = f"{chroot}/usr/local/bin/axisos-xsession.sh"
with open(xsession_path, "w") as f:
    f.write(xsession_script)
os.chmod(xsession_path, 0o755)

# 4. User groups
print("--> 4. Adding user axis to autologin, video, render, audio groups")
subprocess.run(["chroot", chroot, "groupadd", "-f", "autologin"], check=False)
subprocess.run(["chroot", chroot, "gpasswd", "-a", "axis", "autologin"], check=False)
for grp in ["video", "render", "audio", "input", "seat", "sudo", "netdev"]:
    subprocess.run(["chroot", chroot, "usermod", "-aG", grp, "axis"], check=False)

# 5. Clean up old tty1 conflicting drop-ins
print("--> 5. Cleaning up old tty1 service conflicts")
getty_dropin = f"{chroot}/etc/systemd/system/getty@tty1.service.d"
if os.path.exists(getty_dropin):
    shutil.rmtree(getty_dropin)

axisos_svc = f"{chroot}/etc/systemd/system/graphical.target.wants/axisos.service"
if os.path.islink(axisos_svc) or os.path.exists(axisos_svc):
    os.remove(axisos_svc)

# Clean up .profile and .bash_profile hooks
for p in [f"{chroot}/home/axis/.profile", f"{chroot}/home/axis/.bash_profile"]:
    if os.path.exists(p):
        with open(p, "r") as f:
            lines = f.readlines()
        clean_lines = [l for l in lines if "axisos-kiosk.sh" not in l and "axisos-xsession.sh" not in l]
        with open(p, "w") as f:
            f.writelines(clean_lines)

# 6. Enable LightDM and graphical target
print("--> 6. Setting graphical.target and enabling lightdm.service")
subprocess.run(["chroot", chroot, "systemctl", "set-default", "graphical.target"], check=False)
subprocess.run(["chroot", chroot, "systemctl", "enable", "lightdm"], check=False)

# 7. Update /etc/default/grub in chroot
grub_default = f"{chroot}/etc/default/grub"
if os.path.exists(grub_default):
    with open(grub_default, "r") as f:
        content = f.read()
    if 'GRUB_CMDLINE_LINUX_DEFAULT=' in content:
        import re
        content = re.sub(
            r'GRUB_CMDLINE_LINUX_DEFAULT=.*',
            'GRUB_CMDLINE_LINUX_DEFAULT="quiet splash loglevel=0 systemd.show_status=false rd.udev.log_level=3 vt.global_cursor_default=0"',
            content
        )
        with open(grub_default, "w") as f:
            f.write(content)

# Fix ownership
subprocess.run(["chroot", chroot, "chown", "-R", "1000:1000", "/home/axis"])

print("LightDM autologin and X11 session successfully configured!")
