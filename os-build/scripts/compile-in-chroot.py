#!/usr/bin/env python3
import os
import shutil
import subprocess

REPO_ROOT = os.environ.get("WORKSPACE_DIR") or os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
CHROOT = "/build/chroot"
SRC_APPS = os.path.join(REPO_ROOT, "os-build", "apps")
SCRIPT = os.path.join(REPO_ROOT, "os-build", "scripts", "compile-native-apps.sh")

print("=== Setting up compilation inside chroot ===")
target_apps = os.path.join(CHROOT, "tmp", "apps")
os.makedirs(target_apps, exist_ok=True)

# Copy all app sources to chroot /tmp/apps
for item in os.listdir(SRC_APPS):
    s = os.path.join(SRC_APPS, item)
    d = os.path.join(target_apps, item)
    if os.path.isdir(s):
        if os.path.exists(d):
            shutil.rmtree(d)
        shutil.copytree(s, d)

# Copy compile script to chroot /tmp/compile.sh with LF
with open(SCRIPT, "r", encoding="utf-8") as f:
    content = f.read().replace("\r\n", "\n")
compile_sh_chroot = os.path.join(CHROOT, "tmp", "compile.sh")
with open(compile_sh_chroot, "w", encoding="utf-8", newline="\n") as f:
    f.write(content)
os.chmod(compile_sh_chroot, 0o755)

print("=== Running compilation inside chroot ===")
cmd = ["chroot", CHROOT, "/bin/bash", "/tmp/compile.sh", "/tmp/apps"]
res = subprocess.run(cmd, check=True)

print("=== Copying compiled binaries back to repository ===")
apps = ["axis-terminal", "axis-installer", "axis-settings", "axis-usb-daemon", "axis-task-manager", "nobara-welcome", "axis-oobe", "axis-package-installer", "axis-scheduler-manager", "axis-welcome"]
for app in apps:
    built_bin = os.path.join(target_apps, app, app)
    repo_bin = os.path.join(SRC_APPS, app, app)
    if os.path.exists(built_bin):
        shutil.copyfile(built_bin, repo_bin)
        os.chmod(repo_bin, 0o755)
        print(f"  [OK] {app} -> {repo_bin}")
    else:
        print(f"  [WARN] {app} not found at {built_bin}")
    if app == "axis-settings":
        panel_bin = os.path.join(target_apps, app, "axisos-settings-panel")
        repo_panel = os.path.join(SRC_APPS, app, "axisos-settings-panel")
        if os.path.exists(panel_bin):
            shutil.copyfile(panel_bin, repo_panel)
            os.chmod(repo_panel, 0o755)
            print(f"  [OK] axisos-settings-panel -> {repo_panel}")

print("=== All C/C++ applications successfully compiled! ===")
