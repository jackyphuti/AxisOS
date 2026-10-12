#!/usr/bin/env python3
"""
AxisOS OS Identity, Logo, and Branding Deployer
Deploys official AxisOS vector triangle logo, os-release branding,
system issue files, icon themes, and desktop entries across chroot.
"""

import os
import sys
import shutil
import subprocess

AXIS_LOGO_SVG = """<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
  <defs>
    <linearGradient id="axisGrad" x1="6" y1="42" x2="42" y2="6" gradientUnits="userSpaceOnUse">
      <stop offset="0%" stop-color="#00E5FF" />
      <stop offset="50%" stop-color="#7C4DFF" />
      <stop offset="100%" stop-color="#FF007F" />
    </linearGradient>
    <linearGradient id="coreGrad" x1="16" y1="32" x2="32" y2="16" gradientUnits="userSpaceOnUse">
      <stop offset="0%" stop-color="#00E5FF" />
      <stop offset="100%" stop-color="#7C4DFF" />
    </linearGradient>
  </defs>
  <!-- Outer Geometric Prism Ring (Triangle) -->
  <path
    d="M24 5L41 37H7L24 5Z"
    stroke="url(#axisGrad)"
    stroke-width="3.5"
    stroke-linejoin="round"
    stroke-linecap="round"
  />
  <!-- Inner Axis Core Beam (Inverted Prism Triangle) -->
  <path
    d="M24 16L32 32H16L24 16Z"
    fill="url(#coreGrad)"
    fill-opacity="0.9"
  />
  <!-- Central Stabilizer Core -->
  <circle cx="24" cy="23.5" r="2.8" fill="#FFFFFF" />
</svg>
"""

AXIS_LOGO_SYMBOLIC_SVG = """<?xml version="1.0" encoding="UTF-8"?>
<svg width="256" height="256" viewBox="0 0 48 48" fill="none" xmlns="http://www.w3.org/2000/svg">
  <path
    d="M24 5L41 37H7L24 5Z"
    stroke="#F8FAFC"
    stroke-width="3.5"
    stroke-linejoin="round"
    stroke-linecap="round"
  />
  <path
    d="M24 16L32 32H16L24 16Z"
    fill="#F8FAFC"
    fill-opacity="0.85"
  />
  <circle cx="24" cy="23.5" r="2.8" fill="#0A0B0D" />
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
    os.chmod(path, mode)

def deploy_branding(target_root="/build/chroot"):
    print(f"--> Deploying AxisOS branding to target: {target_root}")
    
    # 1. OS Release and ID files
    write_file(os.path.join(target_root, "etc", "os-release"), OS_RELEASE_CONTENT)
    write_file(os.path.join(target_root, "usr", "lib", "os-release"), OS_RELEASE_CONTENT)
    write_file(os.path.join(target_root, "etc", "lsb-release"), LSB_RELEASE_CONTENT)
    write_file(os.path.join(target_root, "etc", "issue"), ISSUE_CONTENT)
    write_file(os.path.join(target_root, "etc", "issue.net"), ISSUE_NET_CONTENT)
    write_file(os.path.join(target_root, "etc", "debian_version"), DEBIAN_VERSION_CONTENT)
    print("    [OK] Deployed /etc/os-release and system identification files")

    # 2. Deploy Vector SVG Logos
    svg_paths = [
        os.path.join(target_root, "usr", "share", "pixmaps", "axisos.svg"),
        os.path.join(target_root, "usr", "share", "pixmaps", "distributor-logo.svg"),
        os.path.join(target_root, "usr", "share", "pixmaps", "debian-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "apps", "axisos.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "apps", "distributor-logo-axisos.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "places", "distributor-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "places", "start-here.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus", "scalable", "apps", "axisos.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "apps", "axisos.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus", "scalable", "places", "distributor-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "places", "distributor-logo.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus", "scalable", "places", "start-here.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "places", "start-here.svg"),
        os.path.join(target_root, "usr", "share", "icons", "AxisHarmony", "scalable", "apps", "axisos.svg"),
        os.path.join(target_root, "usr", "share", "icons", "AxisHarmony", "scalable", "places", "distributor-logo.svg"),
    ]

    for p in svg_paths:
        write_file(p, AXIS_LOGO_SVG)

    symbolic_paths = [
        os.path.join(target_root, "usr", "share", "icons", "hicolor", "scalable", "apps", "axisos-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus", "scalable", "apps", "axisos-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "apps", "axisos-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "places", "distributor-logo-symbolic.svg"),
        os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", "scalable", "places", "start-here-symbolic.svg"),
    ]

    for p in symbolic_paths:
        write_file(p, AXIS_LOGO_SYMBOLIC_SVG)
    print("    [OK] Deployed vector SVG triangle logos across icon themes")

    # 3. Render PNGs at standard resolutions
    temp_svg = "/tmp/axis_logo_tmp.svg"
    with open(temp_svg, "w", encoding="utf-8") as f:
        f.write(AXIS_LOGO_SVG)

    sizes = [16, 24, 32, 48, 64, 128, 256, 512]
    for sz in sizes:
        png_targets = [
            os.path.join(target_root, "usr", "share", "icons", "hicolor", f"{sz}x{sz}", "apps", "axisos.png"),
            os.path.join(target_root, "usr", "share", "icons", "Papirus", f"{sz}x{sz}", "apps", "axisos.png"),
            os.path.join(target_root, "usr", "share", "icons", "Papirus-Dark", f"{sz}x{sz}", "apps", "axisos.png"),
        ]
        if sz == 256 or sz == 64:
            png_targets.append(os.path.join(target_root, "usr", "share", "pixmaps", "axisos.png"))
            png_targets.append(os.path.join(target_root, "usr", "share", "pixmaps", "debian-logo.png"))

        rendered_png = f"/tmp/axis_logo_{sz}.png"
        try:
            cmd = ["rsvg-convert", "-w", str(sz), "-h", str(sz), "-f", "png", "-o", rendered_png, temp_svg]
            res = subprocess.run(cmd, check=False)
            if res.returncode == 0 and os.path.exists(rendered_png):
                for dst in png_targets:
                    os.makedirs(os.path.dirname(dst), exist_ok=True)
                    shutil.copyfile(rendered_png, dst)
        except Exception as e:
            pass
    print("    [OK] Generated multi-resolution PNG icons for apps and control center")

    # 4. Update icon caches if gtk-update-icon-cache is available
    for theme in ["hicolor", "Papirus", "Papirus-Dark"]:
        tpath = os.path.join(target_root, "usr", "share", "icons", theme)
        if os.path.exists(tpath):
            try:
                subprocess.run(["gtk-update-icon-cache", "-q", "-t", "-f", tpath], check=False)
            except Exception:
                pass

    # 5. Ensure axis-installer.desktop uses the triangle logo and is pinned
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

if __name__ == "__main__":
    chroot_path = sys.argv[1] if len(sys.argv) > 1 else "/build/chroot"
    deploy_branding(chroot_path)
