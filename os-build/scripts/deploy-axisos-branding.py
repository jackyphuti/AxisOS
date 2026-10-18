#!/usr/bin/env python3
"""
AxisOS OS Identity, Logo, and Branding Deployer
Deploys official AxisOS vector triangle logo, os-release branding,
boot splash wallpapers, icon themes, desktop entries, and Plymouth/GRUB branding across chroot.
"""

import os
import sys
import shutil
import subprocess

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "..", ".."))
BRANDING_DIR = os.path.join(REPO_ROOT, "data", "branding")

FALLBACK_ICON_SVG = """<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 900 900" width="96" height="96">
  <title>AxisOS icon</title>
  <defs><filter id="glow" x="-40%" y="-40%" width="180%" height="180%">
      <feGaussianBlur stdDeviation="38"/>
    </filter></defs>
  <polygon points="450,170 770,713 130,713" fill="#3A9CC9" filter="url(#glow)" opacity="0.95"/>
  <polygon points="450,170 770,713 130,713" fill="#FDFDFD"/>
</svg>
"""

FALLBACK_SYMBOLIC_SVG = """<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
  <polygon points="24,8 41,38 7,38" fill="#F8FAFC"/>
</svg>
"""

OS_RELEASE_CONTENT = """NAME="AxisOS"
PRETTY_NAME="AxisOS 2.0 (Liquid Nodes Edition)"
VERSION="2.0 (Liquid Nodes)"
VERSION_ID="2.0"
VERSION_CODENAME=bookworm
ID=axisos
ID_LIKE=debian
HOME_URL="https://github.com/jackyphuti/AxisOS"
SUPPORT_URL="https://github.com/jackyphuti/AxisOS/issues"
BUG_REPORT_URL="https://github.com/jackyphuti/AxisOS/issues"
LOGO=axisos
DOCUMENTATION_URL="https://github.com/jackyphuti/AxisOS"
"""

LSB_RELEASE_CONTENT = """DISTRIB_ID=AxisOS
DISTRIB_RELEASE=2.0
DISTRIB_CODENAME=bookworm
DISTRIB_DESCRIPTION="AxisOS 2.0 (Liquid Nodes Edition)"
"""

ISSUE_CONTENT = "AxisOS 2.0 (Liquid Nodes Edition) \\n \\l\n\n"
ISSUE_NET_CONTENT = "AxisOS 2.0 (Liquid Nodes Edition)\n"
DEBIAN_VERSION_CONTENT = "12 (AxisOS 2.0)\n"

def write_file(path, content, mode=0o644):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    try:
        os.chmod(path, mode)
    except Exception:
        pass

def deploy_branding(target_root="/build/chroot"):
    print(f"--> Deploying AxisOS branding to target: {target_root}")
    
    # Load user's official SVG files if present
    icon_svg_path = os.path.join(BRANDING_DIR, "axisos-icon.svg")
    logo_svg_path = os.path.join(BRANDING_DIR, "axisos-logo.svg")
    boot_png_path = os.path.join(BRANDING_DIR, "axisos-boot-1920x1080.png")
    icon_512_path = os.path.join(BRANDING_DIR, "axisos-icon-512.png")
    logo_png_path = os.path.join(BRANDING_DIR, "axisos-logo.png")

    if os.path.exists(icon_svg_path):
        with open(icon_svg_path, "r", encoding="utf-8") as f:
            axis_icon_svg = f.read()
    else:
        axis_icon_svg = FALLBACK_ICON_SVG

    # Enforce standard scalable icon dimensions (96x96) so GNOME Settings About panel and desktop icons render at perfect proportions
    import re
    axis_icon_svg = re.sub(r'width="\d+"', 'width="96"', axis_icon_svg)
    axis_icon_svg = re.sub(r'height="\d+"', 'height="96"', axis_icon_svg)

    if os.path.exists(logo_svg_path):
        with open(logo_svg_path, "r", encoding="utf-8") as f:
            axis_logo_svg = f.read()
    else:
        axis_logo_svg = axis_icon_svg

    # 1. OS Release and ID files
    write_file(os.path.join(target_root, "etc", "os-release"), OS_RELEASE_CONTENT)
    write_file(os.path.join(target_root, "usr", "lib", "os-release"), OS_RELEASE_CONTENT)
    write_file(os.path.join(target_root, "etc", "lsb-release"), LSB_RELEASE_CONTENT)
    write_file(os.path.join(target_root, "etc", "issue"), ISSUE_CONTENT)
    write_file(os.path.join(target_root, "etc", "issue.net"), ISSUE_NET_CONTENT)
    write_file(os.path.join(target_root, "etc", "debian_version"), DEBIAN_VERSION_CONTENT)
    print("    [OK] Deployed /etc/os-release and system identification files")

    # 2. Deploy Official Wallpapers & Boot Splash
    wallpapers_src_dir = os.path.join(REPO_ROOT, "data", "wallpapers")
    wallpapers_dst_dir = os.path.join(target_root, "usr", "share", "backgrounds", "axisos")
    os.makedirs(wallpapers_dst_dir, exist_ok=True)

    if os.path.exists(wallpapers_src_dir):
        for wp in os.listdir(wallpapers_src_dir):
            if wp.endswith(".jpg") or wp.endswith(".png"):
                src_f = os.path.join(wallpapers_src_dir, wp)
                dst_f = os.path.join(wallpapers_dst_dir, wp)
                shutil.copyfile(src_f, dst_f)
                try:
                    os.chmod(dst_f, 0o644)
                except Exception:
                    pass
        print("    [OK] Deployed official AxisOS 20-wallpaper collection to /usr/share/backgrounds/axisos/")

    # Deploy primary wallpaper to standard targets
    primary_wp = os.path.join(wallpapers_dst_dir, "01-axisos-signature-core.jpg")
    if not os.path.exists(primary_wp) and os.path.exists(boot_png_path):
        primary_wp = boot_png_path

    if os.path.exists(primary_wp):
        wallpaper_targets = [
            os.path.join(target_root, "usr", "share", "backgrounds", "axis-wallpaper.png"),
            os.path.join(target_root, "usr", "share", "backgrounds", "nobara-gaming.png"),
            os.path.join(target_root, "usr", "share", "images", "desktop-base", "desktop-background.png"),
            os.path.join(target_root, "usr", "share", "images", "desktop-base", "login-background.png"),
            os.path.join(target_root, "usr", "share", "images", "desktop-base", "desktop-grub.png"),
            os.path.join(target_root, "usr", "share", "desktop-base", "emerald-theme", "grub", "grub-16x9.png"),
            os.path.join(target_root, "usr", "share", "desktop-base", "emerald-theme", "grub", "grub-4x3.png"),
            os.path.join(target_root, "usr", "share", "plymouth", "themes", "emerald", "glow.png"),
            os.path.join(target_root, "usr", "share", "plymouth", "themes", "emerald", "logo+emerald.png"),
        ]
        for wp in wallpaper_targets:
            os.makedirs(os.path.dirname(wp), exist_ok=True)
            shutil.copyfile(primary_wp, wp)
        print("    [OK] Deployed official AxisOS boot splash & primary wallpaper")

    # Deploy GNOME Settings wallpaper properties XML and remove Debian wallpapers
    gnome_bg_props = os.path.join(target_root, "usr", "share", "gnome-background-properties")
    os.makedirs(gnome_bg_props, exist_ok=True)
    xml_src = os.path.join(REPO_ROOT, "os-build", "configs", "gnome", "axisos-wallpapers.xml")
    if os.path.exists(xml_src):
        shutil.copyfile(xml_src, os.path.join(gnome_bg_props, "axisos-wallpapers.xml"))
        try:
            os.chmod(os.path.join(gnome_bg_props, "axisos-wallpapers.xml"), 0o644)
        except Exception:
            pass

    # Disable all non-AxisOS wallpaper entries from GNOME Settings
    stock_disabled = os.path.join(gnome_bg_props, "stock-disabled")
    os.makedirs(stock_disabled, exist_ok=True)
    for f in os.listdir(gnome_bg_props):
        if f.endswith(".xml") and f != "axisos-wallpapers.xml":
            try:
                shutil.move(os.path.join(gnome_bg_props, f), os.path.join(stock_disabled, f))
            except Exception:
                pass
    print("    [OK] Configured GNOME background properties and isolated only AxisOS wallpapers for Settings")

    # 3. Deploy Vector SVG Logos & Icons
    svg_icon_targets = [
        os.path.join(target_root, "usr", "share", "pixmaps", "axisos.svg"),
        os.path.join(target_root, "usr", "share", "pixmaps", "distributor-logo.svg"),
        os.path.join(target_root, "usr", "share", "pixmaps", "debian-logo.svg"),
        os.path.join(target_root, "usr", "share", "pixmaps", "distributor-logo-debian.svg"),
        os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "gnome-logo-text.svg"),
        os.path.join(target_root, "usr", "share", "icons", "gnome-logo-text-dark.svg"),
        # Vendor logo loaded directly by GNOME Control Center (About panel)
        os.path.join(target_root, "usr", "share", "icons", "vendor", "scalable", "emblems", "emblem-vendor.svg"),
        os.path.join(target_root, "usr", "share", "icons", "vendor", "scalable", "emblems", "emblem-vendor-white.svg"),
        os.path.join(target_root, "usr", "share", "icons", "desktop-base", "scalable", "emblems", "emblem-debian.svg"),
        os.path.join(target_root, "usr", "share", "icons", "desktop-base", "scalable", "emblems", "emblem-debian-white.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "emblems", "emblem-debian.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "emblems", "emblem-debian-white.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "apps", "axisos.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "apps", "distributor-logo-axisos.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "apps", "distributor-logo-debian.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "apps", "debian-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "places", "distributor-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "places", "start-here.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus", "scalable", "apps", "axisos.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus", "scalable", "apps", "distributor-logo-debian.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus", "scalable", "apps", "debian-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "apps", "axisos.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "apps", "distributor-logo-debian.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "apps", "debian-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus", "scalable", "places", "distributor-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "places", "distributor-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus", "scalable", "places", "start-here.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "places", "start-here.svg"),
        os.path.join(target_root, "usr", "share", "icons", "AxisHarmony", "scalable", "apps", "axisos.svg"),
        os.path.join(target_root, "usr", "share", "icons", "AxisHarmony", "scalable", "places", "distributor-logo.svg"),
    ]

    # Add all Papirus and Papirus-Dark size variants
    for p_size in ["16x16", "22x22", "24x24", "32x32", "48x48", "64x64"]:
        for theme in ["Papirus", "Papirus-Dark"]:
            svg_icon_targets.extend([
                os.path.join(target_root, "usr", "share", "icons", theme, p_size, "apps", "distributor-logo-debian.svg"),
                os.path.join(target_root, "usr", "share", "icons", theme, p_size, "apps", "debian-logo.svg"),
                os.path.join(target_root, "usr", "share", "icons", theme, p_size, "apps", "distributor-logo.svg"),
                os.path.join(target_root, "usr", "share", "icons", theme, p_size, "apps", "axisos.svg"),
                os.path.join(target_root, "usr", "share", "icons", theme, p_size, "places", "distributor-logo.svg"),
            ])

    for p in svg_icon_targets:
        write_file(p, axis_icon_svg)

    svg_logo_targets = [
        os.path.join(target_root, "usr", "share", "pixmaps", "axisos-logo.svg"),
        os.path.join(target_root, "usr", "share", "pixmaps", "system-logo.svg"),
        os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text.svg"),
        os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-version.svg"),
        os.path.join(target_root, "usr", "share", "desktop-base", "emerald-theme", "login", "background.svg"),
        os.path.join(target_root, "usr", "share", "desktop-base", "emerald-theme", "login", "background-nologo.svg"),
        os.path.join(target_root, "usr", "share", "plymouth", "themes", "emerald", "logo+emerald.svg"),
        os.path.join(target_root, "usr", "share", "plymouth", "themes", "emerald", "Emerald_plymouth.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "apps", "axisos-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "apps", "axisos-logo.svg"),
    ]
    for p in svg_logo_targets:
        write_file(p, axis_logo_svg)

    symbolic_targets = [
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "apps", "axisos-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "emblems", "emblem-debian-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "vendor", "scalable", "emblems", "emblem-vendor-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "desktop-base", "scalable", "emblems", "emblem-debian-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus", "scalable", "apps", "axisos-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "apps", "axisos-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "places", "distributor-logo-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "places", "distributor-logo-debian-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "places", "start-here-symbolic.svg"),
    ]
    for p in symbolic_targets:
        write_file(p, FALLBACK_SYMBOLIC_SVG)
    print("    [OK] Deployed official vector SVG triangle icons & system logos")

    # 4. Render Multi-Resolution Bitmaps
    temp_svg = "/tmp/axis_icon_render.svg"
    with open(temp_svg, "w", encoding="utf-8") as f:
        f.write(axis_icon_svg)

    sizes = [16, 22, 24, 32, 36, 48, 64, 128, 256, 512]
    for sz in sizes:
        png_targets = [
            os.path.join(target_root, "usr", "share", "icons", "hicolor", f"{sz}x{sz}", "apps", "axisos.png"),
            os.path.join(target_root, "usr", "share", "icons", "hicolor", f"{sz}x{sz}", "apps", "distributor-logo-debian.png"),
            os.path.join(target_root, "usr", "share", "icons", "hicolor", f"{sz}x{sz}", "apps", "debian-logo.png"),
            os.path.join(target_root, "usr", "share", "icons", "hicolor", f"{sz}x{sz}", "emblems", "emblem-debian.png"),
            os.path.join(target_root, "usr", "share", "icons", "hicolor", f"{sz}x{sz}", "emblems", "emblem-debian-white.png"),
            os.path.join(target_root, "usr", "share", "icons", "hicolor", f"{sz}x{sz}", "emblems", "emblem-debian-symbolic.png"),
            os.path.join(target_root, "usr", "share", "icons", "desktop-base", f"{sz}x{sz}", "emblems", "emblem-debian.png"),
            os.path.join(target_root, "usr", "share", "icons", "desktop-base", f"{sz}x{sz}", "emblems", "emblem-debian-white.png"),
            os.path.join(target_root, "usr", "share", "icons", "desktop-base", f"{sz}x{sz}", "emblems", "emblem-debian-symbolic.png"),
            os.path.join(target_root, "usr", "share", "icons", "Papirus", f"{sz}x{sz}", "apps", "axisos.png"),
            os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", f"{sz}x{sz}", "apps", "axisos.png"),
        ]
        if sz == 256 or sz == 64:
            png_targets.append(os.path.join(target_root, "usr", "share", "pixmaps", "axisos.png"))
            png_targets.append(os.path.join(target_root, "usr", "share", "pixmaps", "debian-logo.png"))
            png_targets.append(os.path.join(target_root, "usr", "share", "pixmaps", "debian-security.png"))
        if sz == 64:
            png_targets.append(os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-64.png"))
        if sz == 128:
            png_targets.append(os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-128.png"))
            png_targets.append(os.path.join(target_root, "usr", "share", "plymouth", "themes", "emerald", "debian.png"))
        if sz == 256:
            png_targets.append(os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-256.png"))

        rendered_png = f"/tmp/axis_icon_{sz}.png"
        try:
            cmd = ["rsvg-convert", "-w", str(sz), "-h", str(sz), "-f", "png", "-o", rendered_png, temp_svg]
            res = subprocess.run(cmd, check=False)
            if res.returncode == 0 and os.path.exists(rendered_png):
                for dst in png_targets:
                    os.makedirs(os.path.dirname(dst), exist_ok=True)
                    shutil.copyfile(rendered_png, dst)
        except Exception:
            pass

    # Copy 512 high-res and full logo PNG if present
    if os.path.exists(icon_512_path):
        for dst in [
            os.path.join(target_root, "usr", "share", "icons", "hicolor", "512x512", "apps", "axisos.png"),
            os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "512x512", "apps", "axisos.png"),
        ]:
            os.makedirs(os.path.dirname(dst), exist_ok=True)
            shutil.copyfile(icon_512_path, dst)

    if os.path.exists(logo_png_path):
        # Full resolution backup
        full_dst = os.path.join(target_root, "usr", "share", "pixmaps", "axisos-logo-full.png")
        os.makedirs(os.path.dirname(full_dst), exist_ok=True)
        shutil.copyfile(logo_png_path, full_dst)

        # Scale down logos using PIL for crisp, proper UI sizing in GNOME Control Center and GTK dialogs
        try:
            from PIL import Image
            orig_img = Image.open(logo_png_path)

            def save_scaled(dst_path, max_w, max_h):
                os.makedirs(os.path.dirname(dst_path), exist_ok=True)
                img = orig_img.copy()
                img.thumbnail((max_w, max_h), Image.Resampling.LANCZOS)
                img.save(dst_path, "PNG")

            # GNOME Settings & System Logo (clean badge size)
            save_scaled(os.path.join(target_root, "usr", "share", "pixmaps", "axisos-logo.png"), 160, 120)
            save_scaled(os.path.join(target_root, "usr", "share", "pixmaps", "system-logo.png"), 160, 120)

            # Debian desktop-base variants
            save_scaled(os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-64.png"), 64, 48)
            save_scaled(os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-version-64.png"), 64, 48)
            save_scaled(os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-128.png"), 128, 96)
            save_scaled(os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-version-128.png"), 128, 96)
            save_scaled(os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-256.png"), 256, 192)
            save_scaled(os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-version-256.png"), 256, 192)
        except Exception as e:
            print(f"    [WARN] PIL scaling fallback: {e}")
            for dst in [
                os.path.join(target_root, "usr", "share", "pixmaps", "axisos-logo.png"),
                os.path.join(target_root, "usr", "share", "pixmaps", "system-logo.png"),
                os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-64.png"),
                os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-128.png"),
                os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-256.png"),
                os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-version-64.png"),
                os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-version-128.png"),
                os.path.join(target_root, "usr", "share", "desktop-base", "debian-logos", "logo-text-version-256.png"),
            ]:
                os.makedirs(os.path.dirname(dst), exist_ok=True)
                shutil.copyfile(logo_png_path, dst)
    print("    [OK] Generated multi-resolution PNG icons and system logos")

    # 5. Update icon caches
    for theme in ["hicolor", "Papirus", "Papirus-Dark", "vendor", "desktop-base"]:
        tpath = os.path.join(target_root, "usr", "share", "icons", theme)
        if os.path.exists(tpath):
            try:
                subprocess.run(["gtk-update-icon-cache", "-q", "-t", "-f", tpath], check=False)
            except Exception:
                pass

    # 6. Ensure axis-installer.desktop uses the triangle logo and is pinned
    installer_desktop = os.path.join(target_root, "usr", "share", "applications", "axis-installer.desktop")
    installer_content = """[Desktop Entry]
Name=Install AxisOS 2.0
GenericName=Live System Installer
Comment=Install AxisOS permanently to your drive
Exec=/usr/local/bin/axis-installer
Icon=axisos
Terminal=false
Type=Application
Categories=System;PackageManager;Settings;GTK;
Keywords=Install;Setup;Drive;Partition;Live;Installer;
StartupNotify=true
"""
    write_file(installer_desktop, installer_content)
    
    # Copy to user Desktop
    for desk_dir in [os.path.join(target_root, "home", "axis", "Desktop"), os.path.join(target_root, "etc", "skel", "Desktop")]:
        os.makedirs(desk_dir, exist_ok=True)
        write_file(os.path.join(desk_dir, "axis-installer.desktop"), installer_content, 0o755)

    print("    [OK] Configured axis-installer.desktop with official AxisOS triangle icon")

    # 7. Configure GTK 4 stylesheet to constrain system logo in GNOME Settings About panel
    gtk4_css = """/* AxisOS GTK4 Desktop Theme Fixes */
picture#system_logo,
#system_logo,
.about-logo {
    max-width: 96px;
    max-height: 96px;
    min-width: 96px;
    min-height: 96px;
    margin-top: 12px;
    margin-bottom: 12px;
}
"""
    for css_path in [
        os.path.join(target_root, "etc", "gtk-4.0", "gtk.css"),
        os.path.join(target_root, "home", "axis", ".config", "gtk-4.0", "gtk.css"),
        os.path.join(target_root, "etc", "skel", ".config", "gtk-4.0", "gtk.css"),
    ]:
        write_file(css_path, gtk4_css)
    print("    [OK] Deployed GTK4 About-panel logo size constraints")

if __name__ == "__main__":
    chroot_path = sys.argv[1] if len(sys.argv) > 1 else "/build/chroot"
    deploy_branding(chroot_path)
