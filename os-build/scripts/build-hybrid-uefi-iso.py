#!/usr/bin/env python3
import os
import sys
import shutil
import subprocess

build_dir = "/build"
chroot_dir = f"{build_dir}/chroot"
binary_dir = f"{build_dir}/binary"

workspace_dir = os.environ.get("WORKSPACE_DIR")
if not workspace_dir:
    for candidate in [
        "/mnt/c/Users/jacky/Documents/GitHub/AxisOS",
        "/mnt/c/Users/jacky/documents/github/AxisOS",
        os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")),
    ]:
        if os.path.exists(candidate):
            workspace_dir = candidate
            break
if not workspace_dir:
    workspace_dir = "/build"
os.environ["WORKSPACE_DIR"] = workspace_dir

print("==================================================")
print("  AxisOS Dual-Boot UEFI + Legacy BIOS ISO Engine  ")
print("==================================================")
print(f"Workspace Dir: {workspace_dir}")
print(f"Chroot Dir:    {chroot_dir}")
print(f"Binary Dir:    {binary_dir}")

# Step 0: Sync latest workspace assets into chroot
print("--> 0. Synchronizing latest workspace assets into chroot")
shell_dist_src = os.path.join(workspace_dir, "shell", "dist")
shell_dist_dst = os.path.join(chroot_dir, "opt", "axisos-shell", "dist")
if os.path.exists(shell_dist_src):
    print("    Syncing shell/dist -> /opt/axisos-shell/dist")
    if os.path.exists(shell_dist_dst):
        shutil.rmtree(shell_dist_dst)
    shutil.copytree(shell_dist_src, shell_dist_dst)

pkg_axis_src = os.path.join(workspace_dir, "pkg-mgr", "bin", "axis")
if os.path.exists(pkg_axis_src):
    for dst in [os.path.join(chroot_dir, "usr", "bin", "axis"), os.path.join(chroot_dir, "usr", "local", "bin", "axis")]:
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(pkg_axis_src, dst)
        os.chmod(dst, 0o755)

# Deploy compiled C Atomic Update Engine (/usr/bin/axis-update) and boot confirmation service
updater_dir = os.path.join(workspace_dir, "os-build", "updater")
updater_bin_src = os.path.join(updater_dir, "axis-update")
if not os.path.exists(updater_bin_src) and os.path.exists(os.path.join(updater_dir, "Makefile")):
    try:
        subprocess.run(["make", "-C", updater_dir], check=False)
    except Exception:
        pass
if os.path.exists(updater_bin_src):
    dst_upd = os.path.join(chroot_dir, "usr", "bin", "axis-update")
    os.makedirs(os.path.dirname(dst_upd), exist_ok=True)
    shutil.copyfile(updater_bin_src, dst_upd)
    os.chmod(dst_upd, 0o755)

mark_good_src = os.path.join(workspace_dir, "os-build", "updater", "axis-mark-good.service")
if os.path.exists(mark_good_src):
    dst_mg = os.path.join(chroot_dir, "etc", "systemd", "system", "axis-mark-good.service")
    os.makedirs(os.path.dirname(dst_mg), exist_ok=True)
    shutil.copyfile(mark_good_src, dst_mg)
    wants_dir = os.path.join(chroot_dir, "etc", "systemd", "system", "multi-user.target.wants")
    os.makedirs(wants_dir, exist_ok=True)
    mg_link = os.path.join(wants_dir, "axis-mark-good.service")
    if not os.path.lexists(mg_link):
        os.symlink("/etc/systemd/system/axis-mark-good.service", mg_link)

axis_conf_src = os.path.join(workspace_dir, "pkg-mgr", "configs", "axis.conf")
if os.path.exists(axis_conf_src):
    dst = os.path.join(chroot_dir, "etc", "axis", "axis.conf")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(axis_conf_src, dst)

repo_index_src = os.path.join(workspace_dir, "pkg-mgr", "examples", "repo-index.json")
if os.path.exists(repo_index_src):
    for dst in [os.path.join(chroot_dir, "etc", "axis", "repo-index.json"), os.path.join(chroot_dir, "var", "lib", "axis", "repo-index.json")]:
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(repo_index_src, dst)

daemon_src = os.path.join(workspace_dir, "os-build", "configs", "cage-session", "axisos-daemon.cjs")
if os.path.exists(daemon_src):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "axisos-daemon.cjs")
    shutil.copyfile(daemon_src, dst)
    os.chmod(dst, 0o755)

pam_greeter_src = os.path.join(workspace_dir, "data", "pam", "axis-greeter")
if os.path.exists(pam_greeter_src):
    dst = os.path.join(chroot_dir, "etc", "pam.d", "axis-greeter")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(pam_greeter_src, dst)
    os.chmod(dst, 0o644)

def copy_text_lf(src, dst, mode=None):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(src, "rb") as f:
        content = f.read().replace(b"\r\n", b"\n").replace(b"\r", b"\n")
    with open(dst, "wb") as f:
        f.write(content)
    if mode is not None:
        os.chmod(dst, mode)

def copy_binary_or_text(src, dst, mode=None):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(src, "rb") as f:
        magic = f.read(4)
    if magic == b"\x7fELF":
        shutil.copyfile(src, dst)
    else:
        with open(src, "rb") as f:
            content = f.read().replace(b"\r\n", b"\n").replace(b"\r", b"\n")
        with open(dst, "wb") as f:
            f.write(content)
    if mode is not None:
        os.chmod(dst, mode)

def write_text_lf(dst, text, mode=None):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "w", newline="\n", encoding="utf-8") as f:
        f.write(text.replace("\r\n", "\n").replace("\r", "\n"))
    if mode is not None:
        os.chmod(dst, mode)

kiosk_src = os.path.join(workspace_dir, "os-build", "configs", "cage-session", "axisos-kiosk.sh")
if os.path.exists(kiosk_src):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "axisos-kiosk.sh")
    copy_text_lf(kiosk_src, dst, 0o755)

installer_src = os.path.join(workspace_dir, "os-build", "scripts", "axisos-install.sh")
if os.path.exists(installer_src):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "axisos-installer.sh")
    copy_text_lf(installer_src, dst, 0o755)

gpu_detect_src = os.path.join(workspace_dir, "os-build", "scripts", "gpu-autodetect.sh")
if os.path.exists(gpu_detect_src):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "gpu-autodetect.sh")
    copy_text_lf(gpu_detect_src, dst, 0o755)

gpu_svc_src = os.path.join(workspace_dir, "os-build", "configs", "systemd", "gpu-autodetect.service")
if os.path.exists(gpu_svc_src):
    dst = os.path.join(chroot_dir, "etc", "systemd", "system", "gpu-autodetect.service")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(gpu_svc_src, dst)
    wants_dir = os.path.join(chroot_dir, "etc", "systemd", "system", "multi-user.target.wants")
    os.makedirs(wants_dir, exist_ok=True)
    gpu_link = os.path.join(wants_dir, "gpu-autodetect.service")
    if not os.path.lexists(gpu_link):
        os.symlink("/etc/systemd/system/gpu-autodetect.service", gpu_link)

game_boost_src = os.path.join(workspace_dir, "os-build", "scripts", "axis-game-boost.sh")
if os.path.exists(game_boost_src):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "axis-game-boost")
    copy_text_lf(game_boost_src, dst, 0o755)

game_boost_svc_src = os.path.join(workspace_dir, "os-build", "configs", "systemd", "axis-game-boost.service")
if os.path.exists(game_boost_svc_src):
    dst = os.path.join(chroot_dir, "etc", "systemd", "system", "axis-game-boost.service")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(game_boost_svc_src, dst)

# Deploy Signature AxisOS Utilities (Axis Rewind, NeuralPulse, USB Quarantine Vault, Gaze Shield, OTA Sysupgrade)
for util_name, script_name in [
    ("axis-rewind", "axis-rewind.sh"),
    ("axis-neural-pulse", "axis-neural-pulse.sh"),
    ("axis-usb-vault", "axis-usb-vault.sh"),
    ("axis-gaze-shield", "axis-gaze-shield.sh"),
    ("axis-sysupgrade", "axis-sysupgrade.sh"),
]:
    u_src = os.path.join(workspace_dir, "os-build", "scripts", script_name)
    if os.path.exists(u_src):
        dst = os.path.join(chroot_dir, "usr", "local", "bin", util_name)
        copy_text_lf(u_src, dst, 0o755)

# GNOME Desktop Environment Sessions, GDM3, Dconf & Extensions
gnome_dir = os.path.join(workspace_dir, "os-build", "configs", "gnome")
if os.path.exists(gnome_dir):
    print("    Syncing GNOME desktop configurations, GDM3, and dconf profiles...")
    gdm3_conf = os.path.join(gnome_dir, "gdm3", "daemon.conf")
    if os.path.exists(gdm3_conf):
        dst = os.path.join(chroot_dir, "etc", "gdm3", "daemon.conf")
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        copy_text_lf(gdm3_conf, dst, 0o644)
    dconf_profile = os.path.join(gnome_dir, "dconf", "profile", "user")
    if os.path.exists(dconf_profile):
        dst = os.path.join(chroot_dir, "etc", "dconf", "profile", "user")
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        copy_text_lf(dconf_profile, dst, 0o644)
    dconf_local = os.path.join(gnome_dir, "dconf", "00-axisos-gnome")
    if os.path.exists(dconf_local):
        dst = os.path.join(chroot_dir, "etc", "dconf", "db", "local.d", "00-axisos-gnome")
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        copy_text_lf(dconf_local, dst, 0o644)
    # Remove obsolete lightdm/session files
    for stale in [
        os.path.join(chroot_dir, "etc", "lightdm", "lightdm.conf.d", "01_autologin.conf"),
        os.path.join(chroot_dir, "etc", "lightdm", "lightdm.conf.d", "00-axisos-cinnamon.conf"),
        os.path.join(chroot_dir, "usr", "local", "bin", "axisos-xsession.sh"),
        os.path.join(chroot_dir, "assets"),
    ]:
        if os.path.islink(stale) or os.path.isfile(stale):
            try:
                os.remove(stale)
            except Exception:
                pass
    try:
        subprocess.run(["chroot", chroot_dir, "dconf", "update"], check=False)
        subprocess.run(["chroot", chroot_dir, "systemctl", "set-default", "graphical.target"], check=False)
    except Exception:
        pass

# Deploy Official AxisOS Branding, Logo, and OS Release
branding_script = os.path.join(workspace_dir, "os-build", "scripts", "deploy-axisos-branding.py")
if os.path.exists(branding_script):
    print("    Deploying AxisOS OS-release branding and vector triangle logo...")
    try:
        subprocess.run(["python3", branding_script, chroot_dir], check=True)
    except Exception as e:
        print(f"    [INFO] Branding deployment note: {e}")

# Build/Verify Native C/C++ applications inside chroot
compile_script = os.path.join(workspace_dir, "os-build", "scripts", "compile-in-chroot.py")
if os.path.exists(compile_script):
    print("    Building/Verifying native C/C++ applications...")
    try:
        subprocess.run(["python3", compile_script], check=True)
    except Exception as e:
        print(f"    [INFO] Native compilation note: {e}")

# Configure Debian Terminal Environment & Alternatives
term_cfg_script = os.path.join(workspace_dir, "os-build", "scripts", "configure-terminal-env.py")
if os.path.exists(term_cfg_script):
    print("    Configuring Debian terminal environment, PATH, and alternatives...")
    try:
        subprocess.run(["python3", term_cfg_script], check=True)
    except Exception as e:
        print(f"    [INFO] Terminal configuration note: {e}")

# Deploy HarmonyOS Desktop Theme, Icons, and Apps
harmony_theme_src = os.path.join(workspace_dir, "data", "theme")
if os.path.exists(harmony_theme_src):
    dst = os.path.join(chroot_dir, "usr", "local", "share", "axis", "theme")
    os.makedirs(dst, exist_ok=True)
    for f in os.listdir(harmony_theme_src):
        shutil.copyfile(os.path.join(harmony_theme_src, f), os.path.join(dst, f))

harmony_icons_src = os.path.join(workspace_dir, "data", "icons", "AxisHarmony")
if os.path.exists(harmony_icons_src):
    dst = os.path.join(chroot_dir, "usr", "local", "share", "icons", "AxisHarmony")
    if os.path.exists(dst):
        shutil.rmtree(dst)
    shutil.copytree(harmony_icons_src, dst)
    dst_sys = os.path.join(chroot_dir, "usr", "share", "icons", "AxisHarmony")
    if os.path.exists(dst_sys):
        shutil.rmtree(dst_sys)
    shutil.copytree(harmony_icons_src, dst_sys)

harmony_apps_src = os.path.join(workspace_dir, "src")
if os.path.exists(harmony_apps_src):
    dst = os.path.join(chroot_dir, "usr", "local", "share", "axis")
    os.makedirs(dst, exist_ok=True)
    for f in os.listdir(harmony_apps_src):
        if f.endswith(".py"):
            shutil.copyfile(os.path.join(harmony_apps_src, f), os.path.join(dst, f))
            os.chmod(os.path.join(dst, f), 0o755)

# Deploy Native C++/Qt Apps (Terminal, Package Installer, Scheduler Manager, Welcome, Settings, Task Manager, Installer, USB Daemon)
apps_dir = os.path.join(workspace_dir, "os-build", "apps")
if os.path.exists(apps_dir):
    print("    Syncing native C/C++ apps (Terminal, Package Installer, Scheduler Manager, Welcome, Settings, Task Manager, Installer)...")
    for app_name in [
        "axis-terminal",
        "axis-package-installer",
        "axis-scheduler-manager",
        "axis-welcome",
        "nobara-welcome",
        "axis-software-manager",
        "axis-update-manager",
        "axis-task-manager",
        "axis-settings",
        "axis-installer",
        "axis-usb-daemon"
    ]:
        app_bin = os.path.join(apps_dir, app_name, app_name)
        if not os.path.exists(app_bin):
            app_bin = os.path.join(apps_dir, app_name, f"{app_name}.py")
        app_desktop = os.path.join(apps_dir, app_name, f"{app_name}.desktop")
        # Prefer .py implementation for GTK3 apps so stale Qt6 binaries never break launch
        py_candidate = os.path.join(apps_dir, app_name, f"{app_name}.py")
        if app_name in ["axis-terminal", "axis-welcome", "nobara-welcome", "axis-settings", "axis-package-installer", "axis-scheduler-manager", "axis-oobe"] and os.path.exists(py_candidate):
            app_bin = py_candidate
        if os.path.exists(app_bin):
            for bin_sub in [("usr", "local", "bin"), ("usr", "bin")]:
                dst = os.path.join(chroot_dir, *bin_sub, app_name)
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                copy_binary_or_text(app_bin, dst, 0o755)
        if os.path.exists(app_desktop):
            dst = os.path.join(chroot_dir, "usr", "share", "applications", f"{app_name}.desktop")
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            copy_text_lf(app_desktop, dst, 0o644)
            if app_name in ["axis-installer", "axis-terminal", "axis-welcome", "axis-package-installer"]:
                for desk_dir in [os.path.join(chroot_dir, "home", "axis", "Desktop"), os.path.join(chroot_dir, "etc", "skel", "Desktop")]:
                    os.makedirs(desk_dir, exist_ok=True)
                    copy_text_lf(app_desktop, os.path.join(desk_dir, f"{app_name}.desktop"), 0o755)
            if app_name == "axis-welcome":
                for auto_dir in [os.path.join(chroot_dir, "home", "axis", ".config", "autostart"), os.path.join(chroot_dir, "etc", "skel", ".config", "autostart")]:
                    os.makedirs(auto_dir, exist_ok=True)
                    copy_text_lf(app_desktop, os.path.join(auto_dir, f"{app_name}.desktop"), 0o644)

# Deploy Native Qt 6 Wayland Compositor & Display Manager (LiquidNodesOS)
liquid_src = os.path.join(workspace_dir, "compositor-qt", "LiquidNodesOS")
if not os.path.exists(liquid_src):
    liquid_src = os.path.join(workspace_dir, "compositor-qt", "build", "LiquidNodesOS")
if not os.path.exists(liquid_src):
    chroot_liquid = os.path.join(chroot_dir, "usr", "local", "bin", "LiquidNodesOS")
    if os.path.exists(chroot_liquid):
        liquid_src = chroot_liquid
if os.path.exists(liquid_src):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "LiquidNodesOS")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if liquid_src != dst:
        shutil.copyfile(liquid_src, dst)
    os.chmod(dst, 0o755)
    print("    Deployed Native Qt 6 LiquidNodesOS Compositor -> /usr/local/bin/LiquidNodesOS")

# Deploy Browser Performance and Dark Theme Configs
browser_configs = os.path.join(workspace_dir, "os-build", "configs", "browser")
if os.path.exists(browser_configs):
    print("    Syncing browser GPU acceleration & Liquid Nodes configs...")
    chrom_src = os.path.join(browser_configs, "chromium", "99-axisos-performance")
    if os.path.exists(chrom_src):
        dst = os.path.join(chroot_dir, "etc", "chromium.d", "99-axisos-performance")
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        copy_text_lf(chrom_src, dst, 0o644)
    ff_pref = os.path.join(browser_configs, "firefox", "syspref.js")
    if os.path.exists(ff_pref):
        for ffd in ["firefox", "firefox-esr"]:
            dst = os.path.join(chroot_dir, "etc", ffd, "syspref.js")
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            copy_text_lf(ff_pref, dst, 0o644)
    ff_chrome = os.path.join(browser_configs, "firefox", "userChrome.css")
    if os.path.exists(ff_chrome):
        for skel in [os.path.join(chroot_dir, "etc", "skel", ".mozilla", "firefox", "axisos.default", "chrome"),
                     os.path.join(chroot_dir, "home", "axis", ".mozilla", "firefox", "axisos.default", "chrome")]:
            os.makedirs(skel, exist_ok=True)
            copy_text_lf(ff_chrome, os.path.join(skel, "userChrome.css"), 0o644)

# Deploy Wayland Desktop Configs (Waybar, Wofi, Mako, Foot, Backgrounds)
wayland_configs = os.path.join(workspace_dir, "os-build", "configs", "wayland")
if os.path.exists(wayland_configs):
    print("    Syncing Nobara Wayland configurations (Waybar, Wofi, Mako, Foot)...")
    for cfg in ["waybar", "wofi", "mako", "foot"]:
        src_dir = os.path.join(wayland_configs, cfg)
        if os.path.exists(src_dir):
            dst_dir = os.path.join(chroot_dir, "etc", "xdg", cfg)
            os.makedirs(dst_dir, exist_ok=True)
            for item in os.listdir(src_dir):
                s_file = os.path.join(src_dir, item)
                d_file = os.path.join(dst_dir, item)
                if os.path.isfile(s_file):
                    copy_text_lf(s_file, d_file)
    # Deploy official AxisOS wallpaper collection & GNOME background properties
    wallpapers_src_dir = os.path.join(workspace_dir, "data", "wallpapers")
    wallpapers_dst_dir = os.path.join(chroot_dir, "usr", "share", "backgrounds", "axisos")
    os.makedirs(wallpapers_dst_dir, exist_ok=True)
    if os.path.exists(wallpapers_src_dir):
        for wp in os.listdir(wallpapers_src_dir):
            if wp.endswith(".jpg") or wp.endswith(".png"):
                copy_binary_or_text(os.path.join(wallpapers_src_dir, wp), os.path.join(wallpapers_dst_dir, wp), 0o644)
        print("    Deployed official AxisOS 20-wallpaper collection -> /usr/share/backgrounds/axisos/")

    dst_bg_dir = os.path.join(chroot_dir, "usr", "share", "backgrounds")
    os.makedirs(dst_bg_dir, exist_ok=True)
    primary_wp = os.path.join(wallpapers_dst_dir, "01-axisos-signature-core.jpg")
    if os.path.exists(primary_wp):
        shutil.copyfile(primary_wp, os.path.join(dst_bg_dir, "nobara-gaming.png"))
        shutil.copyfile(primary_wp, os.path.join(dst_bg_dir, "axis-wallpaper.png"))
    elif os.path.exists(bg_src):
        shutil.copyfile(bg_src, os.path.join(dst_bg_dir, "nobara-gaming.png"))
        shutil.copyfile(bg_src, os.path.join(dst_bg_dir, "axis-wallpaper.png"))

    gnome_bg_props = os.path.join(chroot_dir, "usr", "share", "gnome-background-properties")
    os.makedirs(gnome_bg_props, exist_ok=True)
    xml_src = os.path.join(workspace_dir, "os-build", "configs", "gnome", "axisos-wallpapers.xml")
    if os.path.exists(xml_src):
        copy_text_lf(xml_src, os.path.join(gnome_bg_props, "axisos-wallpapers.xml"), 0o644)

    # Disable non-AxisOS wallpaper XMLs from GNOME Settings
    stock_disabled = os.path.join(gnome_bg_props, "stock-disabled")
    os.makedirs(stock_disabled, exist_ok=True)
    for f in os.listdir(gnome_bg_props):
        if f.endswith(".xml") and f != "axisos-wallpapers.xml":
            try:
                shutil.move(os.path.join(gnome_bg_props, f), os.path.join(stock_disabled, f))
            except Exception:
                pass
    print("    Configured GNOME background properties and isolated only AxisOS wallpapers for Settings")

# Deploy Nobara-Axis Wayland Session
nobara_sess_dst = os.path.join(chroot_dir, "usr", "share", "wayland-sessions", "nobara-axis.desktop")
write_text_lf(nobara_sess_dst, """[Desktop Entry]
Name=AxisOS Nobara Edition
Comment=Native wlroots Wayland Compositor Session
Exec=/usr/local/bin/axisos-kiosk.sh
Type=Application
DesktopNames=Nobara-Axis
""")

# Deploy Qt 6 LiquidNodes Wayland Compositor (LiquidNodesOS)
liquid_src = os.path.join(chroot_dir, "usr", "local", "bin", "LiquidNodesOS")
if os.path.exists(liquid_src):
    print("    Detected Qt6 LiquidNodes Compositor -> /usr/local/bin/LiquidNodesOS")
    qml_dir = os.path.join(workspace_dir, "compositor-qt")
    if os.path.exists(qml_dir):
        dst_qml_dir = os.path.join(chroot_dir, "usr", "share", "axis-compositor-qt")
        os.makedirs(dst_qml_dir, exist_ok=True)
        for item in os.listdir(qml_dir):
            if item.endswith(".qml") or item.endswith(".js"):
                copy_text_lf(os.path.join(qml_dir, item), os.path.join(dst_qml_dir, item))
        print("    Synced Qt 6 QML desktop files -> /usr/share/axis-compositor-qt/")
    liquid_sess_dst = os.path.join(chroot_dir, "usr", "share", "wayland-sessions", "liquidnodes.desktop")
    write_text_lf(liquid_sess_dst, """[Desktop Entry]
Name=AxisOS LiquidNodes (Qt 6 QML)
Comment=Glassmorphic LiquidNodes Wayland Desktop Environment
Exec=/usr/local/bin/LiquidNodesOS
Type=Application
DesktopNames=LiquidNodes;AxisOS
""")

# Deploy Emoji Block Pin & Vector Symbol Fontconfig
block_emojis_src = os.path.join(workspace_dir, "os-build", "configs", "apt", "preferences.d", "block-emojis")
if os.path.exists(block_emojis_src):
    dst_pref = os.path.join(chroot_dir, "etc", "apt", "preferences.d", "block-emojis")
    os.makedirs(os.path.dirname(dst_pref), exist_ok=True)
    copy_text_lf(block_emojis_src, dst_pref, 0o644)
    print("    Deployed APT preferences block-emojis")

font_local_src = os.path.join(workspace_dir, "os-build", "configs", "fonts", "local.conf")
if os.path.exists(font_local_src):
    dst_font = os.path.join(chroot_dir, "etc", "fonts", "local.conf")
    os.makedirs(os.path.dirname(dst_font), exist_ok=True)
    copy_text_lf(font_local_src, dst_font, 0o644)
    print("    Deployed Fontconfig symbol fonts (FontAwesome / Papirus)")

# Deploy prime-run helper
primerun_dst = os.path.join(chroot_dir, "usr", "local", "bin", "prime-run")
write_text_lf(primerun_dst, """#!/usr/bin/env bash
# AxisOS NVIDIA PRIME Render Offload Runner
export __NV_PRIME_RENDER_OFFLOAD=1
export __GLX_VENDOR_LIBRARY_NAME=nvidia
export __VK_LAYER_NV_optimus=NVIDIA_only
exec "$@"
""", 0o755)

# Deploy Steam pre-installed launcher & menu entry
steam_desktop_dst = os.path.join(chroot_dir, "usr", "share", "applications", "steam.desktop")
write_text_lf(steam_desktop_dst, """[Desktop Entry]
Name=Steam
Comment=Application for managing and playing games on Steam
Exec=/usr/local/bin/steam %U
Icon=steam
Terminal=false
Type=Application
Categories=Network;FileTransfer;Game;
MimeType=x-scheme-handler/steam;x-scheme-handler/steamlink;
Actions=Store;Community;Library;Servers;Screenshots;News;Settings;BigPicture;Friends;
""")
for desk_dir in [os.path.join(chroot_dir, "home", "axis", "Desktop"), os.path.join(chroot_dir, "etc", "skel", "Desktop")]:
    os.makedirs(desk_dir, exist_ok=True)
    copy_text_lf(steam_desktop_dst, os.path.join(desk_dir, "steam.desktop"), 0o755)

# Deploy Visual Studio Code desktop entry & shortcuts
code_desktop_src = os.path.join(workspace_dir, "os-build", "configs", "wayland", "code.desktop")
if os.path.exists(code_desktop_src):
    code_desktop_dst = os.path.join(chroot_dir, "usr", "share", "applications", "code.desktop")
    copy_text_lf(code_desktop_src, code_desktop_dst, 0o644)
    for desk_dir in [os.path.join(chroot_dir, "home", "axis", "Desktop"), os.path.join(chroot_dir, "etc", "skel", "Desktop")]:
        os.makedirs(desk_dir, exist_ok=True)
        copy_text_lf(code_desktop_src, os.path.join(desk_dir, "code.desktop"), 0o755)
    print("    Deployed Visual Studio Code desktop entry -> /usr/share/applications/code.desktop")

# Deploy Visual Studio Code Unified Launcher (/usr/local/bin/code)
vscode_wrapper_dst = os.path.join(chroot_dir, "usr", "local", "bin", "code")
write_text_lf(vscode_wrapper_dst, """#!/usr/bin/env bash
# AxisOS Visual Studio Code Smart Launcher & Auto-Installer
if [ -x "/usr/share/code/code" ]; then
    exec /usr/share/code/code --no-sandbox "$@"
elif [ -x "/usr/bin/code" ] && [ "$(readlink -f /usr/bin/code)" != "/usr/local/bin/code" ]; then
    exec /usr/bin/code --no-sandbox "$@"
elif command -v codium >/dev/null 2>&1; then
    exec codium --no-sandbox "$@"
elif command -v flatpak >/dev/null 2>&1 && flatpak info com.visualstudio.code >/dev/null 2>&1; then
    exec flatpak run com.visualstudio.code "$@"
else
    exec /usr/local/bin/axis-terminal -e "echo 'Installing Visual Studio Code...'; sudo apt-get update && sudo apt-get install -y code || sudo flatpak install -y flathub com.visualstudio.code"
fi
""", 0o755)

steam_wrapper_dst = os.path.join(chroot_dir, "usr", "local", "bin", "steam")
write_text_lf(steam_wrapper_dst, """#!/usr/bin/env bash
# AxisOS Steam Unified Launcher with Proton & GameMode
if [ -x "/usr/games/steam" ]; then
    exec /usr/games/steam "$@"
elif command -v steam >/dev/null 2>&1 && [ "$(which steam)" != "/usr/local/bin/steam" ]; then
    exec "$(which steam)" "$@"
elif command -v steam-installer >/dev/null 2>&1; then
    exec steam-installer "$@"
elif command -v flatpak >/dev/null 2>&1 && flatpak info com.valvesoftware.Steam >/dev/null 2>&1; then
    exec flatpak run com.valvesoftware.Steam "$@"
else
    echo "Starting Steam setup..."
    apt-get update -qq && apt-get install -y steam-installer || true
    if [ -x "/usr/games/steam" ]; then
        exec /usr/games/steam "$@"
    fi
fi
""", 0o755)

# Deploy Android & HarmonyOS universal udev rules
udev_rules_dst = os.path.join(chroot_dir, "etc", "udev", "rules.d", "51-android.rules")
write_text_lf(udev_rules_dst, """# Universal Android & HarmonyOS USB rules
SUBSYSTEM=="usb", ATTR{idVendor}=="[0-9a-fA-F]*", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="04e8", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="12d1", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="2717", MODE="0666", GROUP="plugdev"
""")

# Deploy Gaming & High-Performance Sysctl configs (Proton/Steam max_map_count)
gaming_sysctl_dst = os.path.join(chroot_dir, "etc", "sysctl.d", "99-axisos-gaming.conf")
write_text_lf(gaming_sysctl_dst, """# AxisOS Gaming & Development Performance Tuning
vm.max_map_count = 2147483642
fs.file-max = 2097152
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192
vm.swappiness = 10
""")

# Deploy Out-of-Box Experience (OOBE) First-Boot Setup Wizard
oobe_bin = os.path.join(workspace_dir, "os-build", "apps", "axis-oobe", "axis-oobe")
if not os.path.exists(oobe_bin):
    oobe_bin = os.path.join(workspace_dir, "os-build", "apps", "axis-oobe", "axis-oobe.py")
oobe_desktop = os.path.join(workspace_dir, "os-build", "apps", "axis-oobe", "axis-oobe.desktop")
if os.path.exists(oobe_bin):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "axis-oobe")
    copy_binary_or_text(oobe_bin, dst, 0o755)
    print("    Deployed AxisOS OOBE Wizard -> /usr/local/bin/axis-oobe")
if os.path.exists(oobe_desktop):
    dst = os.path.join(chroot_dir, "usr", "share", "applications", "axis-oobe.desktop")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    copy_text_lf(oobe_desktop, dst, 0o644)

# Deploy Real-Time USB Subsystem Daemon Service
usb_service = os.path.join(workspace_dir, "os-build", "configs", "systemd", "axis-usb-daemon.service")
if os.path.exists(usb_service):
    dst = os.path.join(chroot_dir, "etc", "systemd", "system", "axis-usb-daemon.service")
    copy_text_lf(usb_service, dst, 0o644)
    wants_dir = os.path.join(chroot_dir, "etc", "systemd", "system", "multi-user.target.wants")
    os.makedirs(wants_dir, exist_ok=True)
    symlink_dst = os.path.join(wants_dir, "axis-usb-daemon.service")
    if not os.path.exists(symlink_dst):
        try:
            os.symlink("/etc/systemd/system/axis-usb-daemon.service", symlink_dst)
        except Exception:
            pass
    print("    Deployed and enabled axis-usb-daemon.service")

# Deploy Automated NVIDIA Driver Installer & Service
nvidia_script = os.path.join(workspace_dir, "os-build", "scripts", "nvidia-auto-install.sh")
nvidia_service = os.path.join(workspace_dir, "os-build", "configs", "systemd", "nvidia-first-boot.service")
if os.path.exists(nvidia_script):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "nvidia-auto-install.sh")
    copy_text_lf(nvidia_script, dst, 0o755)
    print("    Deployed NVIDIA Auto-Install Script -> /usr/local/bin/nvidia-auto-install.sh")
if os.path.exists(nvidia_service):
    dst = os.path.join(chroot_dir, "etc", "systemd", "system", "nvidia-first-boot.service")
    copy_text_lf(nvidia_service, dst, 0o644)
    wants_dir = os.path.join(chroot_dir, "etc", "systemd", "system", "multi-user.target.wants")
    os.makedirs(wants_dir, exist_ok=True)
    symlink_dst = os.path.join(wants_dir, "nvidia-first-boot.service")
    if not os.path.exists(symlink_dst):
        try:
            os.symlink("/etc/systemd/system/nvidia-first-boot.service", symlink_dst)
        except Exception:
            shutil.copyfile(dst, symlink_dst)
    print("    Enabled NVIDIA First-Boot Service -> multi-user.target.wants")

# Deploy zRAM memory compression generator config
zram_cfg = os.path.join(workspace_dir, "os-build", "configs", "systemd", "zram-generator.conf")
if os.path.exists(zram_cfg):
    dst = os.path.join(chroot_dir, "etc", "systemd", "zram-generator.conf")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    copy_text_lf(zram_cfg, dst, 0o644)
    print("    Deployed zRAM generator config -> /etc/systemd/zram-generator.conf")

# Deploy Gamepad and SSD I/O scheduler udev rules
udev_configs = os.path.join(workspace_dir, "os-build", "configs", "udev")
if os.path.exists(udev_configs):
    dst_udev = os.path.join(chroot_dir, "etc", "udev", "rules.d")
    os.makedirs(dst_udev, exist_ok=True)
    for rule in os.listdir(udev_configs):
        if rule.endswith(".rules"):
            copy_text_lf(os.path.join(udev_configs, rule), os.path.join(dst_udev, rule), 0o644)
    print("    Deployed Gamepad & I/O scheduler udev rules")

# Deploy Bluetooth AutoEnable config
bt_cfg = os.path.join(workspace_dir, "os-build", "configs", "bluetooth", "main.conf")
if os.path.exists(bt_cfg):
    dst = os.path.join(chroot_dir, "etc", "bluetooth", "main.conf")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    copy_text_lf(bt_cfg, dst, 0o644)
    print("    Deployed Bluetooth AutoEnable config -> /etc/bluetooth/main.conf")

# Deploy XanMod Kernel Apt Repository & Keyring
xanmod_key = os.path.join(workspace_dir, "os-build", "configs", "apt", "keyrings", "xanmod-archive-keyring.gpg")
xanmod_list = os.path.join(workspace_dir, "os-build", "configs", "apt", "sources.list.d", "xanmod-release.list")
if os.path.exists(xanmod_key):
    dst_key = os.path.join(chroot_dir, "etc", "apt", "keyrings", "xanmod-archive-keyring.gpg")
    os.makedirs(os.path.dirname(dst_key), exist_ok=True)
    shutil.copyfile(xanmod_key, dst_key)
if os.path.exists(xanmod_list):
    dst_list = os.path.join(chroot_dir, "etc", "apt", "sources.list.d", "xanmod-release.list")
    os.makedirs(os.path.dirname(dst_list), exist_ok=True)
    copy_text_lf(xanmod_list, dst_list, 0o644)
    print("    Deployed XanMod Kernel Apt Repository")

# Deploy Official AxisOS OTA Apt Repository & Snapshot Hook
axis_list = os.path.join(workspace_dir, "os-build", "configs", "apt", "sources.list.d", "axisos.list")
if os.path.exists(axis_list):
    dst_list = os.path.join(chroot_dir, "etc", "apt", "sources.list.d", "axisos.list")
    os.makedirs(os.path.dirname(dst_list), exist_ok=True)
    copy_text_lf(axis_list, dst_list, 0o644)
    print("    Deployed AxisOS OTA Apt Repository -> /etc/apt/sources.list.d/axisos.list")

snap_hook = os.path.join(workspace_dir, "os-build", "configs", "apt", "apt.conf.d", "80-axisos-btrfs-snapshot")
if os.path.exists(snap_hook):
    dst_hook = os.path.join(chroot_dir, "etc", "apt", "apt.conf.d", "80-axisos-btrfs-snapshot")
    os.makedirs(os.path.dirname(dst_hook), exist_ok=True)
    copy_text_lf(snap_hook, dst_hook, 0o644)
    print("    Deployed AxisOS Btrfs Snapshot Pre-Upgrade Hook -> /etc/apt/apt.conf.d/80-axisos-btrfs-snapshot")

# Deploy Session Launcher
kiosk_sh = os.path.join(workspace_dir, "os-build", "configs", "cage-session", "axisos-kiosk.sh")
if os.path.exists(kiosk_sh):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "axisos-kiosk.sh")
    copy_text_lf(kiosk_sh, dst, 0o755)
    print("    Deployed Session Launcher -> /usr/local/bin/axisos-kiosk.sh")

# Deploy Installer Scripts
installer_sh = os.path.join(workspace_dir, "os-build", "scripts", "axisos-install.sh")
if os.path.exists(installer_sh):
    copy_text_lf(installer_sh, os.path.join(chroot_dir, "usr", "local", "bin", "axisos-install.sh"), 0o755)
    copy_text_lf(installer_sh, os.path.join(chroot_dir, "usr", "local", "bin", "axisos-installer.sh"), 0o755)
    print("    Deployed Installer Scripts with OOBE flag")

# Deploy Disk Inspector Script
disk_info_sh = os.path.join(workspace_dir, "os-build", "scripts", "axis-disk-info.sh")
if os.path.exists(disk_info_sh):
    copy_text_lf(disk_info_sh, os.path.join(chroot_dir, "usr", "local", "bin", "axis-disk-info.sh"), 0o755)
    print("    Deployed AxisOS Disk Information Script -> /usr/local/bin/axis-disk-info.sh")

# Deploy Biometric Authentication Manager (Fingerprint & Iris)
bio_sh = os.path.join(workspace_dir, "os-build", "scripts", "axis-biometrics.sh")
if os.path.exists(bio_sh):
    copy_text_lf(bio_sh, os.path.join(chroot_dir, "usr", "local", "bin", "axis-biometrics"), 0o755)
    print("    Deployed AxisOS Biometric Manager -> /usr/local/bin/axis-biometrics")

# Deploy Game Boost Script & Service
game_boost_sh = os.path.join(workspace_dir, "os-build", "scripts", "axis-game-boost.sh")
if os.path.exists(game_boost_sh):
    copy_text_lf(game_boost_sh, os.path.join(chroot_dir, "usr", "local", "bin", "axis-game-boost"), 0o755)
    print("    Deployed AxisOS Game Boost Script -> /usr/local/bin/axis-game-boost")

game_boost_svc = os.path.join(workspace_dir, "os-build", "configs", "systemd", "axis-game-boost.service")
if os.path.exists(game_boost_svc):
    game_boost_svc_dst = os.path.join(chroot_dir, "etc", "systemd", "system", "axis-game-boost.service")
    os.makedirs(os.path.dirname(game_boost_svc_dst), exist_ok=True)
    shutil.copyfile(game_boost_svc, game_boost_svc_dst)
    print("    Deployed AxisOS Game Boost Service -> /etc/systemd/system/axis-game-boost.service")

# Deploy Signature AxisOS Utilities (Axis Rewind, NeuralPulse, USB Quarantine Vault, Gaze Shield, OTA Sysupgrade)
for util_name, script_name in [
    ("axis-rewind", "axis-rewind.sh"),
    ("axis-neural-pulse", "axis-neural-pulse.sh"),
    ("axis-usb-vault", "axis-usb-vault.sh"),
    ("axis-gaze-shield", "axis-gaze-shield.sh"),
    ("axis-sysupgrade", "axis-sysupgrade.sh"),
    ("axis-update-notifier", "axis-update-notifier.sh"),
]:
    u_src = os.path.join(workspace_dir, "os-build", "scripts", script_name)
    if os.path.exists(u_src):
        copy_text_lf(u_src, os.path.join(chroot_dir, "usr", "local", "bin", util_name), 0o755)
        print(f"    Deployed AxisOS {util_name} -> /usr/local/bin/{util_name}")

# Deploy Remote OTA Update Notifier systemd timer, XDG autostart, and menu launcher
notifier_svc = os.path.join(workspace_dir, "os-build", "configs", "systemd", "user", "axis-update-notifier.service")
notifier_tmr = os.path.join(workspace_dir, "os-build", "configs", "systemd", "user", "axis-update-notifier.timer")
user_systemd_dir = os.path.join(chroot_dir, "etc", "systemd", "user")
user_timers_dir = os.path.join(user_systemd_dir, "timers.target.wants")
os.makedirs(user_timers_dir, exist_ok=True)
if os.path.exists(notifier_svc):
    copy_text_lf(notifier_svc, os.path.join(user_systemd_dir, "axis-update-notifier.service"), 0o644)
if os.path.exists(notifier_tmr):
    dst_tmr = os.path.join(user_systemd_dir, "axis-update-notifier.timer")
    copy_text_lf(notifier_tmr, dst_tmr, 0o644)
    symlink_tmr = os.path.join(user_timers_dir, "axis-update-notifier.timer")
    if not os.path.exists(symlink_tmr):
        try:
            os.symlink("/etc/systemd/user/axis-update-notifier.timer", symlink_tmr)
        except Exception:
            shutil.copyfile(dst_tmr, symlink_tmr)

autostart_src = os.path.join(workspace_dir, "os-build", "configs", "autostart", "axis-update-notifier.desktop")
if os.path.exists(autostart_src):
    autostart_dst_dir = os.path.join(chroot_dir, "etc", "xdg", "autostart")
    os.makedirs(autostart_dst_dir, exist_ok=True)
    copy_text_lf(autostart_src, os.path.join(autostart_dst_dir, "axis-update-notifier.desktop"), 0o644)

apps_menu_dir = os.path.join(chroot_dir, "usr", "share", "applications")
sysup_desktop_src = os.path.join(workspace_dir, "os-build", "configs", "desktop", "axis-sysupgrade.desktop")
if os.path.exists(sysup_desktop_src):
    copy_text_lf(sysup_desktop_src, os.path.join(apps_menu_dir, "axis-sysupgrade.desktop"), 0o644)
print("    Deployed Background Remote OTA Update Notifier and System Update desktop entry")


# Hide duplicate UXTerm and XTerm menu entries so only AxisOS Terminal appears
for dup_d in [
    "debian-xterm.desktop",
    "debian-uxterm.desktop",
    "xterm.desktop",
    "uxterm.desktop",
]:
    dup_p = os.path.join(apps_menu_dir, dup_d)
    if os.path.exists(dup_p):
        try:
            with open(dup_p, "r", encoding="utf-8", errors="ignore") as f:
                d_content = f.read()
            if "NoDisplay=true" not in d_content:
                d_content = d_content.rstrip() + "\nNoDisplay=true\n"
                write_text_lf(dup_p, d_content, 0o644)
        except Exception:
            pass

# Deploy AxisOS-branded org.gnome.Settings.desktop override (adds Desktop Actions for AxisOS panels)
gnome_settings_override = os.path.join(workspace_dir, "os-build", "configs", "gnome", "org.gnome.Settings.desktop")
if os.path.exists(gnome_settings_override):
    dst_settings = os.path.join(apps_menu_dir, "org.gnome.Settings.desktop")
    copy_text_lf(gnome_settings_override, dst_settings, 0o644)
    print("    Deployed org.gnome.Settings.desktop override with AxisOS panel Actions")

# Deploy GNOME Settings panel search integration files
for p_desk in [
    "axis-health-panel.desktop",
    "axis-gaming-panel.desktop",
    "axis-biometrics-panel.desktop",
    "axis-signature-panel.desktop",
]:
    p_src = os.path.join(workspace_dir, "os-build", "configs", "gnome", p_desk)
    if os.path.exists(p_src):
        copy_text_lf(p_src, os.path.join(apps_menu_dir, p_desk), 0o644)
        print(f"    Deployed GNOME Settings panel search entry: {p_desk}")

# Deploy axisos-settings-panel helper (replaces standalone axis-settings)
panel_src = os.path.join(apps_dir, "axis-settings", "axis-settings.py")
if os.path.exists(panel_src):
    panel_dst = os.path.join(chroot_dir, "usr", "local", "bin", "axisos-settings-panel")
    copy_binary_or_text(panel_src, panel_dst, 0o755)
    print("    Deployed /usr/local/bin/axisos-settings-panel")

# Deploy Captive Portal & Auto-Timezone services
captive_sh = os.path.join(workspace_dir, "os-build", "scripts", "axis-captive-portal.sh")
if os.path.exists(captive_sh):
    copy_text_lf(captive_sh, os.path.join(chroot_dir, "usr", "local", "bin", "axis-captive-portal.sh"), 0o755)
    dispatcher_dst = os.path.join(chroot_dir, "etc", "NetworkManager", "dispatcher.d", "99-captive-portal.sh")
    dispatcher_src = os.path.join(workspace_dir, "os-build", "configs", "networkmanager", "dispatcher.d", "99-captive-portal.sh")
    if os.path.exists(dispatcher_src):
        os.makedirs(os.path.dirname(dispatcher_dst), exist_ok=True)
        copy_text_lf(dispatcher_src, dispatcher_dst, 0o755)
    
    nm_conn_src = os.path.join(workspace_dir, "os-build", "configs", "networkmanager", "conf.d", "20-connectivity.conf")
    if os.path.exists(nm_conn_src):
        nm_conn_dst = os.path.join(chroot_dir, "etc", "NetworkManager", "conf.d", "20-connectivity.conf")
        os.makedirs(os.path.dirname(nm_conn_dst), exist_ok=True)
        copy_text_lf(nm_conn_src, nm_conn_dst, 0o644)

    wifi_desktop_src = os.path.join(workspace_dir, "os-build", "configs", "networkmanager", "axis-wifi-login.desktop")
    if os.path.exists(wifi_desktop_src):
        wifi_desktop_dst = os.path.join(chroot_dir, "usr", "share", "applications", "axis-wifi-login.desktop")
        copy_text_lf(wifi_desktop_src, wifi_desktop_dst, 0o644)
        for desk_dir in [os.path.join(chroot_dir, "home", "axis", "Desktop"), os.path.join(chroot_dir, "etc", "skel", "Desktop")]:
            os.makedirs(desk_dir, exist_ok=True)
            copy_text_lf(wifi_desktop_src, os.path.join(desk_dir, "axis-wifi-login.desktop"), 0o755)

    chrom_desktop_src = os.path.join(chroot_dir, "usr", "share", "applications", "chromium.desktop")
    if os.path.exists(chrom_desktop_src):
        for desk_dir in [os.path.join(chroot_dir, "home", "axis", "Desktop"), os.path.join(chroot_dir, "etc", "skel", "Desktop")]:
            os.makedirs(desk_dir, exist_ok=True)
            copy_text_lf(chrom_desktop_src, os.path.join(desk_dir, "chromium.desktop"), 0o755)

    print("    Deployed Wi-Fi Captive Portal Redirect Handler, NetworkManager config, and Chromium desktop shortcuts")

tz_sh = os.path.join(workspace_dir, "os-build", "scripts", "axis-autotimezone.sh")
if os.path.exists(tz_sh):
    copy_text_lf(tz_sh, os.path.join(chroot_dir, "usr", "local", "bin", "axis-autotimezone.sh"), 0o755)
    tz_svc = os.path.join(workspace_dir, "os-build", "configs", "systemd", "axis-autotimezone.service")
    if os.path.exists(tz_svc):
        dst_svc = os.path.join(chroot_dir, "etc", "systemd", "system", "axis-autotimezone.service")
        copy_text_lf(tz_svc, dst_svc, 0o644)
        multi_user = os.path.join(chroot_dir, "etc", "systemd", "system", "multi-user.target.wants")
        os.makedirs(multi_user, exist_ok=True)
        symlink_svc = os.path.join(multi_user, "axis-autotimezone.service")
        if not os.path.exists(symlink_svc):
            try:
                os.symlink("/etc/systemd/system/axis-autotimezone.service", symlink_svc)
            except Exception:
                shutil.copyfile(dst_svc, symlink_svc)
    print("    Deployed Automatic Geolocation & Timezone Service")


# Step 1: Generate filesystem.squashfs
print("--> 1. Building filesystem.squashfs with parallel XZ compression")
squashfs_target = f"{binary_dir}/live/filesystem.squashfs"
os.makedirs(f"{binary_dir}/live", exist_ok=True)
if os.environ.get("REUSE_SQUASHFS") == "1" and os.path.exists(squashfs_target) and os.path.getsize(squashfs_target) > 500 * 1024 * 1024:
    print(f"    [OPTIMIZATION] Reusing verified squashfs ({os.path.getsize(squashfs_target)} bytes) -> skipping recompression")
else:
    if os.path.exists(squashfs_target):
        os.remove(squashfs_target)
    subprocess.run([
        "mksquashfs", chroot_dir, squashfs_target,
        "-comp", "xz",
        "-b", "1048576",
        "-Xbcj", "x86",
        "-noappend",
        "-e",
        "boot/vmlinuz*",
        "boot/initrd.img*",
        "var/cache/apt/archives/*",
        "var/lib/apt/lists/*",
        "tmp/*",
        "var/tmp/*",
        "var/log/*",
        "root/.cache/*"
    ], check=True)

# Step 2: Ensure kernel and initrd are in binary/live
print("--> 2. Syncing kernel and initramfs to /live")
vmlinuz_candidates = sorted(os.listdir(f"{chroot_dir}/boot"))
for item in vmlinuz_candidates:
    if item.startswith("vmlinuz-"):
        shutil.copyfile(f"{chroot_dir}/boot/{item}", f"{binary_dir}/live/vmlinuz")
        print(f"    Kernel: {item} -> {binary_dir}/live/vmlinuz")
        break

for item in vmlinuz_candidates:
    if item.startswith("initrd.img-"):
        shutil.copyfile(f"{chroot_dir}/boot/{item}", f"{binary_dir}/live/initrd.img")
        print(f"    Initrd: {item} -> {binary_dir}/live/initrd.img")
        break

# Step 3: Configure ISOLINUX for Legacy BIOS
print("--> 3. Configuring ISOLINUX (Legacy BIOS)")
os.makedirs(f"{binary_dir}/isolinux", exist_ok=True)
for c32 in ["ldlinux.c32", "libcom32.c32", "libutil.c32", "vesamenu.c32"]:
    src = f"/usr/lib/syslinux/modules/bios/{c32}"
    if os.path.exists(src):
        shutil.copyfile(src, f"{binary_dir}/isolinux/{c32}")

if os.path.exists("/usr/lib/ISOLINUX/isolinux.bin"):
    shutil.copyfile("/usr/lib/ISOLINUX/isolinux.bin", f"{binary_dir}/isolinux/isolinux.bin")

write_text_lf(f"{binary_dir}/isolinux/isolinux.cfg", """UI vesamenu.c32
PROMPT 0
TIMEOUT 50
DEFAULT live

MENU TITLE AxisOS 2.1 (GNOME Desktop Edition)
MENU COLOR border       30;44   #40ffffff #a0000000 std
MENU COLOR title        1;36;44 #902563eb #a0000000 std
MENU COLOR sel          7;37;40 #e0ffffff #20ffffff all
MENU COLOR unsel        37;44   #50ffffff #a0000000 std
MENU COLOR help         37;40   #c0ffffff #a0000000 std

LABEL live
  MENU LABEL ^1. Try AxisOS 2.1 (GNOME Desktop)
  MENU DEFAULT
  KERNEL /live/vmlinuz
  APPEND initrd=/live/initrd.img boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev,gamemode,games,docker live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3 amdgpu.freesync_video=1

LABEL install
  MENU LABEL ^2. Install AxisOS 2.1 (Direct Setup Wizard)
  KERNEL /live/vmlinuz
  APPEND initrd=/live/initrd.img boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev,gamemode,games,docker live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3 axisos.autoinstall=1 amdgpu.freesync_video=1

LABEL failsafe
  MENU LABEL ^3. AxisOS 2.1 (Safe Graphics Mode)
  KERNEL /live/vmlinuz
  APPEND initrd=/live/initrd.img boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev live-config.locales=en_US.UTF-8 live-config.timezone=UTC nomodeset quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3
""")

# Step 4: Build Microsoft-Signed GRUB UEFI 64-bit Bootloader
print("--> 4. Deploying Microsoft-signed Shim and signed GRUB for Secure Boot trust")
os.makedirs(f"{binary_dir}/EFI/BOOT", exist_ok=True)
os.makedirs(f"{binary_dir}/boot/grub", exist_ok=True)

shim_src = f"{chroot_dir}/usr/lib/shim/shimx64.efi.signed"
grub_src = f"{chroot_dir}/usr/lib/grub/x86_64-efi-signed/grubx64.efi.signed"
mm_src = f"{chroot_dir}/usr/lib/shim/mmx64.efi.signed"

# Fallback to unsigned in chroot if signed not found
if not os.path.exists(shim_src):
    shim_src = f"{chroot_dir}/usr/lib/shim/shimx64.efi"
if not os.path.exists(mm_src):
    mm_src = f"{chroot_dir}/usr/lib/shim/mmx64.efi"

# Deploy Microsoft-signed Shim as universal BOOTX64.EFI
bootx64_path = f"{binary_dir}/EFI/BOOT/BOOTX64.EFI"
grubx64_path = f"{binary_dir}/EFI/BOOT/grubx64.efi"
mmx64_path = f"{binary_dir}/EFI/BOOT/mmx64.efi"

if os.path.exists(shim_src):
    shutil.copyfile(shim_src, bootx64_path)
    print("    [Secure Boot] Deployed Microsoft-signed Shim as /EFI/BOOT/BOOTX64.EFI")

if os.path.exists(grub_src):
    shutil.copyfile(grub_src, grubx64_path)
    print("    [Secure Boot] Deployed Debian-signed GRUB as /EFI/BOOT/grubx64.efi")

if os.path.exists(mm_src):
    shutil.copyfile(mm_src, mmx64_path)

# Copy official AxisOS 1080p boot splash image to GRUB directory
grub_splash_src = os.path.join(workspace_dir, "data", "branding", "axisos-boot-1920x1080.png")
if os.path.exists(grub_splash_src):
    shutil.copyfile(grub_splash_src, f"{binary_dir}/boot/grub/splash.png")

# Early GRUB config in /EFI/BOOT/grub.cfg that finds USB root and chains to /boot/grub/grub.cfg
write_text_lf(f"{binary_dir}/EFI/BOOT/grub.cfg", """search --set=root --file /live/vmlinuz
set prefix=($root)/boot/grub
if [ -f ($root)/boot/grub/grub.cfg ]; then
    configfile ($root)/boot/grub/grub.cfg
fi
""")

# GRUB Config in /boot/grub/grub.cfg
write_text_lf(f"{binary_dir}/boot/grub/grub.cfg", """set default=0
set timeout=5

insmod part_gpt
insmod part_msdos
insmod fat
insmod iso9660
insmod all_video
insmod png
insmod gfxterm

set gfxmode=auto
terminal_output gfxterm

if [ -f /boot/grub/splash.png ]; then
    background_image /boot/grub/splash.png
fi

menuentry "Try AxisOS 2.1 (GNOME Desktop)" --class axisos --class gnu-linux --class gnu {
    linux /live/vmlinuz boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev,gamemode,games,docker live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3 amdgpu.freesync_video=1
    initrd /live/initrd.img
}

menuentry "Install AxisOS 2.1 (Direct Setup Wizard)" --class axisos {
    linux /live/vmlinuz boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev,gamemode,games,docker live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3 axisos.autoinstall=1 amdgpu.freesync_video=1
    initrd /live/initrd.img
}

menuentry "AxisOS 2.1 (Safe Graphics Mode - nomodeset)" --class axisos {
    linux /live/vmlinuz boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev live-config.locales=en_US.UTF-8 live-config.timezone=UTC nomodeset quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3
    initrd /live/initrd.img
}
""")

# Step 5: Create FAT EFI System Partition Image for El Torito UEFI boot
print("--> 5. Creating EFI System Partition image (efi.img)")
efi_img = f"{binary_dir}/boot/grub/efi.img"
if os.path.exists(efi_img):
    os.remove(efi_img)

subprocess.run(["dd", "if=/dev/zero", f"of={efi_img}", "bs=1M", "count=15"], check=True)
subprocess.run(["mkfs.vfat", efi_img], check=True)
subprocess.run(["mmd", "-i", efi_img, "::/EFI"], check=True)
subprocess.run(["mmd", "-i", efi_img, "::/EFI/BOOT"], check=True)
subprocess.run(["mcopy", "-i", efi_img, bootx64_path, "::/EFI/BOOT/BOOTX64.EFI"], check=True)
if os.path.exists(grubx64_path):
    subprocess.run(["mcopy", "-i", efi_img, grubx64_path, "::/EFI/BOOT/grubx64.efi"], check=True)
if os.path.exists(mmx64_path):
    subprocess.run(["mcopy", "-i", efi_img, mmx64_path, "::/EFI/BOOT/mmx64.efi"], check=True)
subprocess.run(["mcopy", "-i", efi_img, f"{binary_dir}/EFI/BOOT/grub.cfg", "::/EFI/BOOT/grub.cfg"], check=True)

# Step 6: Generate SHA256 Checksums
print("--> 6. Generating SHA256 checksums")
subprocess.run(
    ["sh", "-c", f"cd {binary_dir} && find . -type f ! -name SHA256SUMS | sort | xargs sha256sum > SHA256SUMS"],
    check=True
)

# Step 7: Create Dual UEFI + BIOS Hybrid ISO via xorriso
print("--> 7. Assembling Dual-Boot UEFI + Legacy BIOS Hybrid ISO with xorriso")
output_iso = f"{build_dir}/axisos-v2.1-live-amd64.iso"
if os.path.exists(output_iso):
    os.remove(output_iso)

mbr_template = "/usr/lib/ISOLINUX/isohdpfx.bin"

xorriso_cmd = [
    "xorriso", "-as", "mkisofs",
    "-r", "-V", "AXISOS_2_1",
    "-J", "-joliet-long",
    "-b", "isolinux/isolinux.bin",
    "-c", "isolinux/boot.cat",
    "-no-emul-boot", "-boot-load-size", "4", "-boot-info-table",
    "-eltorito-alt-boot",
    "-e", "boot/grub/efi.img",
    "-no-emul-boot",
    "-isohybrid-gpt-basdat",
    "-isohybrid-mbr", mbr_template,
    "-output", output_iso,
    binary_dir
]

subprocess.run(xorriso_cmd, check=True)

# Ensure isohybrid is applied with UEFI support
print("--> 8. Applying isohybrid UEFI partition flags")
subprocess.run(["isohybrid", "--uefi", output_iso], check=False)

# Copy to Windows workspace
dest = f"{workspace_dir}/axisos-v2.1-live-amd64.iso"
dest_compat = f"{workspace_dir}/axisos-v2.0-live-amd64.iso"
print(f"--> 9. Copying ISO to Windows workspace ({dest})")
shutil.copyfile(output_iso, dest)
shutil.copyfile(output_iso, dest_compat)

iso_size_mb = os.path.getsize(dest) / (1024 * 1024)
print("==================================================")
print(f" SUCCESS: AxisOS Universal Hybrid ISO Generated!")
print(f" File: {dest} ({iso_size_mb:.1f} MB)")
print(" Compatible with:")
print("   - Modern UEFI Machines (Secure Boot Off / CSM Off)")
print("   - Legacy BIOS Machines")
print("   - USB Flash Drives (Rufus, Etcher, Ventoy, dd)")
print("   - Virtual Machines (QEMU, VirtualBox, VMware)")
print("==================================================")
