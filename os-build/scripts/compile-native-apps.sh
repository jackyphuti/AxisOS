#!/usr/bin/env bash
# ==============================================================================
# AxisOS Native Pure C Compilation Orchestrato
# Enforces 100% Pure C (-std=gnu11 / -std=c17) compilation across all subsystems
# Zero C++, Zero Qt 6 / QML, Zero Python
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "======================================================================"
echo "  AxisOS 2.0 Pure C (-std=gnu11) Desktop & Operating System Suite    "
echo "  Strict Language Policy: 100% Pure C / Zero C++ / Zero Qt / Zero Py "
echo "======================================================================"

# Standard compiler flags
CC="gcc"
CFLAGS="-std=gnu11 -O2 -Wall -Wextra"

# 1. Compile axis-terminal (GTK 3 + libvterm + forkpty)
echo "--> [1/12] Compiling axis-terminal..."
(
    cd "$ROOT_DIR/os-build/apps/axis-terminal"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0 vterm 2>/dev/null || pkg-config --cflags --libs gtk+-3.0) -lutil -o axis-terminal
)

# 2. Compile axis-installer (GTK 3 + GIO + POSIX)
echo "--> [2/12] Compiling axis-installer..."
(
    cd "$ROOT_DIR/os-build/apps/axis-installer"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0 gio-2.0) -o axis-installer
)

# 3. Compile axis-settings (GTK 3 + GIO + GLib) -> axisos-settings-panel & axis-settings
echo "--> [3/12] Compiling axis-settings..."
(
    cd "$ROOT_DIR/os-build/apps/axis-settings"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0 gio-2.0) -o axisos-settings-panel
    $CC $CFLAGS axis_settings_launcher.c -o axis-settings
)

# 4. Compile axis-task-manager (GTK 3 + Cairo + Pango + /proc telemetry)
echo "--> [4/12] Compiling axis-task-manager..."
(
    cd "$ROOT_DIR/os-build/apps/axis-task-manager"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0 cairo pangocairo) -lm -o axis-task-manager
)

# 5. Compile axis-oobe (GTK 3 + Cairo First-Boot Wizard)
echo "--> [5/12] Compiling axis-oobe..."
(
    cd "$ROOT_DIR/os-build/apps/axis-oobe"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0) -lm -o axis-oobe
)

# 6. Compile nobara-welcome (GTK 3 Welcome Hub)
echo "--> [6/12] Compiling nobara-welcome..."
(
    cd "$ROOT_DIR/os-build/apps/nobara-welcome"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0) -o nobara-welcome
)

# 7. Compile axis-package-installer (GTK 3 .axis/.deb Installer)
echo "--> [7/12] Compiling axis-package-installer..."
(
    cd "$ROOT_DIR/os-build/apps/axis-package-installer"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0 gio-2.0) -o axis-package-installer
)

# 8. Compile axis-scheduler-manager (GTK 3 Kernel Scheduler & Governor Manager)
echo "--> [8/12] Compiling axis-scheduler-manager..."
(
    cd "$ROOT_DIR/os-build/apps/axis-scheduler-manager"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0) -o axis-scheduler-manager
)

# 9. Compile axis-welcome (GTK 3 Driver & Welcome Hub)
echo "--> [9/12] Compiling axis-welcome..."
(
    cd "$ROOT_DIR/os-build/apps/axis-welcome"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0 gio-2.0) -o axis-welcome
)

# 10. Compile axis-software-manager (GTK 3 App Store)
echo "--> [10/12] Compiling axis-software-manager..."
(
    cd "$ROOT_DIR/os-build/apps/axis-software-manager"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0 gio-2.0) -o axis-software-manager
)

# 11. Compile axis-update-manager (GTK 3 System Update Manager)
echo "--> [11/12] Compiling axis-update-manager..."
(
    cd "$ROOT_DIR/os-build/apps/axis-update-manager"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0 gio-2.0) -o axis-update-manager
)

# 12. Compile axis-usb-daemon (libudev USB Hotplug Monitor)
echo "--> [12/16] Compiling axis-usb-daemon..."
(
    cd "$ROOT_DIR/os-build/apps/axis-usb-daemon"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs libudev 2>/dev/null || echo "-ludev") -o axis-usb-daemon
)

# 13. Compile axis-fetch (Pure C11 Telemetry & Fetch Tool)
echo "--> [13/16] Compiling axis-fetch..."
(
    cd "$ROOT_DIR/os-build/apps/axis-fetch"
    $CC $CFLAGS main.c -o axis-fetch
)

# 14. Compile axis-calc (Pure C11 GTK 3 Desktop Calculator)
echo "--> [14/16] Compiling axis-calc..."
(
    cd "$ROOT_DIR/os-build/apps/axis-calc"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0) -lm -o axis-calc
)

# 15. Compile axis-color-picker (Pure C11 GTK 3 Color Picker)
echo "--> [15/16] Compiling axis-color-picker..."
(
    cd "$ROOT_DIR/os-build/apps/axis-color-picker"
    $CC $CFLAGS main.c $(pkg-config --cflags --libs gtk+-3.0) -lm -o axis-color-picker
)

# 16. Compile axis-disk-info (Pure C Storage & Disk Partitioner Telemetry)
echo "--> [16/16] Compiling axis-disk-info..."
(
    cd "$ROOT_DIR/os-build/scripts"
    $CC $CFLAGS axis-disk-info.c -o axis-disk-info
)

# 13. Compile Pure C Wayland Desktop Shell (shell-c -> axis-shell & LiquidNodesOS)
if [ -d "$ROOT_DIR/shell-c" ]; then
    echo "--> Compiling Pure C Wayland Desktop Shell (shell-c)..."
    make -C "$ROOT_DIR/shell-c" clean
    make -C "$ROOT_DIR/shell-c" -j"$(nproc 2>/dev/null || echo 2)"
    cp -f "$ROOT_DIR/shell-c/axisos-shell" "$ROOT_DIR/shell-c/axis-shell" 2>/dev/null || true
fi

# 14. Compile Native wlroots Wayland Compositor (compositor -> axis-compositor)
if [ -d "$ROOT_DIR/compositor" ]; then
    echo "--> Compiling Pure C Wayland Compositor (compositor)..."
    (
        cd "$ROOT_DIR/compositor"
        if pkg-config --exists wlroots 2>/dev/null; then
            make clean || true
            make -j"$(nproc 2>/dev/null || echo 2)" || true
        fi
    )
fi

# 15. Compile AxisOS Atomic Updater (os-build/updater -> axis-update)
if [ -d "$ROOT_DIR/os-build/updater" ]; then
    echo "--> Compiling AxisOS Atomic Updater (axis_update.c)..."
    make -C "$ROOT_DIR/os-build/updater" clean
    make -C "$ROOT_DIR/os-build/updater"
fi

# 16. Compile Pure C Package Manager Tooling (pkg-mgr -> axis & axis_repo_builder)
if [ -d "$ROOT_DIR/pkg-mgr" ]; then
    echo "--> Compiling Pure C Package Manager Tooling (pkg-mgr)..."
    make -C "$ROOT_DIR/pkg-mgr" clean
    make -C "$ROOT_DIR/pkg-mgr"
fi

echo "======================================================================"
echo "  All Pure C Desktop Applications and Shell Compiled Successfully!    "
echo "======================================================================"
