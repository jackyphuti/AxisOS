#!/usr/bin/env python3
import os
import shutil
import subprocess

REPO_ROOT = os.environ.get("WORKSPACE_DIR") or os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
CHROOT = "/build/chroot"
SRC_APPS = os.path.join(REPO_ROOT, "os-build", "apps")
SCRIPT = os.path.join(REPO_ROOT, "os-build", "scripts", "compile-native-apps.sh")

print("=== Setting up C++20 / Qt 6 compilation inside chroot ===")
mounts = ["/dev", "/dev/pts", "/proc", "/sys"]
for m in mounts:
    target = f"{CHROOT}{m}"
    os.makedirs(target, exist_ok=True)
    subprocess.run(["mount", "--bind", m, target], check=False)

try:
    target_root = os.path.join(CHROOT, "tmp", "axisos-build")
    if os.path.exists(target_root):
        shutil.rmtree(target_root)
    os.makedirs(os.path.join(target_root, "os-build", "scripts"), exist_ok=True)

    # Copy apps, compositor-qt, compositor, and updater into chroot build area
    shutil.copytree(SRC_APPS, os.path.join(target_root, "os-build", "apps"))
    if os.path.exists(os.path.join(REPO_ROOT, "compositor-qt")):
        shutil.copytree(os.path.join(REPO_ROOT, "compositor-qt"), os.path.join(target_root, "compositor-qt"))
    if os.path.exists(os.path.join(REPO_ROOT, "compositor")):
        shutil.copytree(os.path.join(REPO_ROOT, "compositor"), os.path.join(target_root, "compositor"))
    if os.path.exists(os.path.join(REPO_ROOT, "os-build", "updater")):
        shutil.copytree(os.path.join(REPO_ROOT, "os-build", "updater"), os.path.join(target_root, "os-build", "updater"))

    with open(SCRIPT, "r", encoding="utf-8") as f:
        content = f.read().replace("\r\n", "\n")
    compile_sh_chroot = os.path.join(target_root, "os-build", "scripts", "compile-native-apps.sh")
    with open(compile_sh_chroot, "w", encoding="utf-8", newline="\n") as f:
        f.write(content)
    os.chmod(compile_sh_chroot, 0o755)

    print("=== Running CMake & Make inside chroot ===")
    subprocess.run(["chroot", CHROOT, "/bin/bash", "/tmp/axisos-build/os-build/scripts/compile-native-apps.sh", "/tmp/axisos-build/os-build/apps"], check=True)

    print("=== Copying compiled ELF binaries back to workspace ===")
    apps = [
        "axis-terminal",
        "axis-installer",
        "axis-settings",
        "axis-usb-daemon",
        "axis-task-manager",
        "nobara-welcome",
        "axis-oobe",
        "axis-package-installer",
        "axis-scheduler-manager",
        "axis-welcome",
        "axis-software-manager",
        "axis-update-manager",
    ]
    for app in apps:
        built_bin = os.path.join(target_root, "os-build", "apps", app, app)
        repo_bin = os.path.join(SRC_APPS, app, app)
        if os.path.exists(built_bin):
            shutil.copyfile(built_bin, repo_bin)
            os.chmod(repo_bin, 0o755)
            print(f"  [OK] {app} -> {repo_bin}")

    built_comp_qt = os.path.join(target_root, "compositor-qt", "LiquidNodesOS")
    if os.path.exists(built_comp_qt):
        dst_comp_qt = os.path.join(REPO_ROOT, "compositor-qt", "LiquidNodesOS")
        shutil.copyfile(built_comp_qt, dst_comp_qt)
        os.chmod(dst_comp_qt, 0o755)
        print(f"  [OK] LiquidNodesOS -> {dst_comp_qt}")

    built_comp_c = os.path.join(target_root, "compositor", "axis-compositor")
    if os.path.exists(built_comp_c):
        dst_comp_c = os.path.join(REPO_ROOT, "compositor", "axis-compositor")
        shutil.copyfile(built_comp_c, dst_comp_c)
        os.chmod(dst_comp_c, 0o755)
        print(f"  [OK] axis-compositor -> {dst_comp_c}")

finally:
    for m in reversed(mounts):
        subprocess.run(["umount", "-lf", f"{CHROOT}{m}"], check=False)

print("=== All C++20 / Qt 6 applications and compositors compiled! ===")
