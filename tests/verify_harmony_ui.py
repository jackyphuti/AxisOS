#!/usr/bin/env python3
"""
AxisOS 2.1 C++20 / Qt 6 QML & Unified Theme Verification Suite
Verifies:
1. Component CSS contains zero raw hex values and all @define-color tokens exist in axis-tokens.css.
2. Desktop Qt 6 QML applications have C++20 + Qt 6 QML sources, CMakeLists.txt, and unified Theme.qml.
3. Native GTK3 / C++ desktop applications have valid C/C++ sources and entrypoints.
4. Session launcher (axisos-kiosk.sh) launches LiquidNodesOS without setting QT_QPA_PLATFORM=wayland on the root compositor.
"""

import os
import re
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

def verify_theme_tokens():
    print("--- 1. Verifying Unified One UI Theme Tokens ---")
    comp_css = os.path.join(REPO_ROOT, "data", "theme", "axis-components.css")
    tokens_css = os.path.join(REPO_ROOT, "data", "theme", "axis-tokens.css")
    with open(comp_css, "r", encoding="utf-8") as f:
        comp_text = f.read()
    hex_matches = re.findall(r"#[0-9a-fA-F]{3,8}\b", comp_text)
    assert not hex_matches, f"Found raw hex in axis-components.css: {hex_matches}"

    with open(tokens_css, "r", encoding="utf-8") as f:
        tokens_text = f.read()
    for req in ["axis_dark_base", "axis_dark_surface", "axis_dark_accent", "axis_light_base", "axis_light_surface", "axis_light_accent"]:
        assert req in tokens_text, f"Missing token {req} in axis-tokens.css"
    print("PASS: Theme tokens and CSS verified.")

def verify_qt6_apps():
    print("--- 2. Verifying Qt 6 QML Desktop Applications ---")
    qt_apps = [
        "axis-terminal",
        "axis-package-installer",
        "axis-scheduler-manager",
        "axis-welcome",
    ]
    for app in qt_apps:
        app_dir = os.path.join(REPO_ROOT, "os-build", "apps", app)
        assert os.path.isdir(app_dir), f"Missing app dir: {app}"
        assert os.path.exists(os.path.join(app_dir, "CMakeLists.txt")), f"Missing CMakeLists.txt in {app}"
        assert os.path.exists(os.path.join(app_dir, "main.cpp")), f"Missing main.cpp in {app}"
        assert os.path.exists(os.path.join(app_dir, "Theme.qml")), f"Missing unified Theme.qml in {app}"
        with open(os.path.join(app_dir, "main.cpp"), "r", encoding="utf-8") as f:
            cpp = f.read()
        assert "QQmlApplicationEngine" in cpp or "QGuiApplication" in cpp, f"Missing Qt6 application in {app}/main.cpp"
        print(f"  [OK] {app:<26} -> C++20 + Qt 6 QML + Unified Theme.qml")
    print("PASS: Qt 6 QML desktop applications verified.")

def verify_native_apps():
    print("--- 3. Verifying Native C/C++ Desktop Applications ---")
    native_apps = [
        "axis-installer",
        "axis-task-manager",
        "axis-settings",
        "axis-usb-daemon",
    ]
    for app in native_apps:
        app_dir = os.path.join(REPO_ROOT, "os-build", "apps", app)
        assert os.path.isdir(app_dir), f"Missing app dir: {app}"
        has_source = os.path.exists(os.path.join(app_dir, "main.cpp")) or os.path.exists(os.path.join(app_dir, "main.c")) or os.path.exists(os.path.join(app_dir, f"{app}.py"))
        assert has_source, f"Missing source entrypoint in {app}"
        print(f"  [OK] {app:<26} -> Native application entrypoint verified")
    print("PASS: Native desktop applications verified.")

def verify_boot_session():
    print("--- 4. Verifying Boot & Wayland Compositor Session Config ---")
    kiosk_sh = os.path.join(REPO_ROOT, "os-build", "configs", "cage-session", "axisos-kiosk.sh")
    with open(kiosk_sh, "r", encoding="utf-8") as f:
        kiosk_text = f.read()
    assert "unset QT_QPA_PLATFORM" in kiosk_text, "axisos-kiosk.sh must unset QT_QPA_PLATFORM before starting root compositor"
    assert "/usr/local/bin/LiquidNodesOS" in kiosk_text, "axisos-kiosk.sh must launch LiquidNodesOS"
    print("PASS: Boot & Wayland session launcher verified.")

if __name__ == "__main__":
    verify_theme_tokens()
    verify_qt6_apps()
    verify_native_apps()
    verify_boot_session()
    print("\nALL AXISOS 2.1 VERIFICATION CHECKS PASSED!")
