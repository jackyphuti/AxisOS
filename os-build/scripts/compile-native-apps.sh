#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
APPS_DIR="${1:-$REPO_ROOT/os-build/apps}"
CC="${CC:-gcc}"
CFLAGS="-std=gnu11 -Wall -Wextra -O2"

echo "======================================================================"
echo "  AxisOS 2.0 Pure C (-std=gnu11) Desktop & Operating System Suite    "
echo "  Strict Language Policy: 100% Pure C / Zero C++ / Zero Qt / Zero Py "
echo "======================================================================"

# 1. axis-terminal (GTK 3 + libvterm)
echo "--> [1/12] Compiling axis-terminal..."
$CC $CFLAGS "$APPS_DIR/axis-terminal/main.c" $(pkg-config --cflags --libs gtk+-3.0 vterm) -lutil -o "$APPS_DIR/axis-terminal/axis-terminal"
chmod 755 "$APPS_DIR/axis-terminal/axis-terminal"

# 2. axis-installer (GTK 3 + GIO)
echo "--> [2/12] Compiling axis-installer..."
$CC $CFLAGS "$APPS_DIR/axis-installer/main.c" $(pkg-config --cflags --libs gtk+-3.0 gio-2.0) -o "$APPS_DIR/axis-installer/axis-installer"
chmod 755 "$APPS_DIR/axis-installer/axis-installer"

# 3. axis-settings & axisos-settings-panel (GTK 3 + GIO)
echo "--> [3/12] Compiling axis-settings..."
$CC $CFLAGS "$APPS_DIR/axis-settings/main.c" $(pkg-config --cflags --libs gtk+-3.0 gio-2.0) -o "$APPS_DIR/axis-settings/axisos-settings-panel"
chmod 755 "$APPS_DIR/axis-settings/axisos-settings-panel"
cat << 'EOF' > "$APPS_DIR/axis-settings/axis-settings"
#!/bin/sh
if [ "$1" = "about" ] || [ "$1" = "--about" ] || [ "$1" = "-a" ]; then
    exec /usr/local/bin/axisos-settings-panel about
elif [ "$1" = "updates" ] || [ "$1" = "--updates" ] || [ "$1" = "-u" ]; then
    exec /usr/local/bin/axisos-settings-panel updates
fi
exec /usr/local/bin/axisos-settings-panel "$@"
EOF
chmod 755 "$APPS_DIR/axis-settings/axis-settings"

# 4. axis-task-manager (GTK 3 + Cairo + Pango)
echo "--> [4/12] Compiling axis-task-manager..."
$CC $CFLAGS "$APPS_DIR/axis-task-manager/main.c" $(pkg-config --cflags --libs gtk+-3.0 pango pangocairo cairo) -lm -o "$APPS_DIR/axis-task-manager/axis-task-manager"
chmod 755 "$APPS_DIR/axis-task-manager/axis-task-manager"

# 5. axis-oobe (GTK 3)
echo "--> [5/12] Compiling axis-oobe..."
$CC $CFLAGS "$APPS_DIR/axis-oobe/main.c" $(pkg-config --cflags --libs gtk+-3.0) -o "$APPS_DIR/axis-oobe/axis-oobe"
chmod 755 "$APPS_DIR/axis-oobe/axis-oobe"

# 6. nobara-welcome (GTK 3)
echo "--> [6/12] Compiling nobara-welcome..."
$CC $CFLAGS "$APPS_DIR/nobara-welcome/main.c" $(pkg-config --cflags --libs gtk+-3.0) -o "$APPS_DIR/nobara-welcome/nobara-welcome"
chmod 755 "$APPS_DIR/nobara-welcome/nobara-welcome"

# 7. axis-package-installer (GTK 3)
echo "--> [7/12] Compiling axis-package-installer..."
$CC $CFLAGS "$APPS_DIR/axis-package-installer/main.c" $(pkg-config --cflags --libs gtk+-3.0) -o "$APPS_DIR/axis-package-installer/axis-package-installer"
chmod 755 "$APPS_DIR/axis-package-installer/axis-package-installer"

# 8. axis-scheduler-manager (GTK 3)
echo "--> [8/12] Compiling axis-scheduler-manager..."
$CC $CFLAGS "$APPS_DIR/axis-scheduler-manager/main.c" $(pkg-config --cflags --libs gtk+-3.0) -o "$APPS_DIR/axis-scheduler-manager/axis-scheduler-manager"
chmod 755 "$APPS_DIR/axis-scheduler-manager/axis-scheduler-manager"

# 9. axis-welcome (GTK 3)
echo "--> [9/12] Compiling axis-welcome..."
$CC $CFLAGS "$APPS_DIR/axis-welcome/main.c" $(pkg-config --cflags --libs gtk+-3.0) -o "$APPS_DIR/axis-welcome/axis-welcome"
chmod 755 "$APPS_DIR/axis-welcome/axis-welcome"

# 10. axis-software-manager (GTK 3)
echo "--> [10/12] Compiling axis-software-manager..."
$CC $CFLAGS "$APPS_DIR/axis-software-manager/main.c" $(pkg-config --cflags --libs gtk+-3.0) -o "$APPS_DIR/axis-software-manager/axis-software-manager"
chmod 755 "$APPS_DIR/axis-software-manager/axis-software-manager"

# 11. axis-update-manager (GTK 3)
echo "--> [11/12] Compiling axis-update-manager..."
$CC $CFLAGS "$APPS_DIR/axis-update-manager/main.c" $(pkg-config --cflags --libs gtk+-3.0) -o "$APPS_DIR/axis-update-manager/axis-update-manager"
chmod 755 "$APPS_DIR/axis-update-manager/axis-update-manager"

# 12. axis-usb-daemon (libudev)
echo "--> [12/12] Compiling axis-usb-daemon..."
$CC $CFLAGS "$APPS_DIR/axis-usb-daemon/main.c" $(pkg-config --cflags --libs libudev) -o "$APPS_DIR/axis-usb-daemon/axis-usb-daemon"
chmod 755 "$APPS_DIR/axis-usb-daemon/axis-usb-daemon"

# 13. Wayland Desktop Shell (shell-c)
if [ -d "$REPO_ROOT/shell-c" ]; then
    echo "--> Compiling Pure C Wayland Desktop Shell (shell-c)..."
    make -C "$REPO_ROOT/shell-c" clean all
fi

# 14. Atomic Btrfs Updater (os-build/updater)
if [ -d "$REPO_ROOT/os-build/updater" ]; then
    echo "--> Compiling AxisOS Atomic Updater..."
    make -C "$REPO_ROOT/os-build/updater" clean all 2>/dev/null || true
fi

# 15. Package Manager Tooling (pkg-mgr)
if [ -d "$REPO_ROOT/pkg-mgr" ]; then
    echo "--> Compiling Pure C Package Manager Tooling (axis)..."
    make -C "$REPO_ROOT/pkg-mgr" clean all
fi

# 16. Install to /usr/local/bin and /usr/bin if running as root
if [ -d "/usr/local/bin" ] && [ "${EUID:-$(id -u)}" -eq 0 ]; then
    echo "--> Installing applications to /usr/local/bin and /usr/bin..."
    for app in axis-terminal axis-package-installer axis-scheduler-manager axis-welcome axis-installer axis-settings axis-software-manager axis-update-manager axis-usb-daemon axis-task-manager axis-oobe nobara-welcome; do
        if [ -f "$APPS_DIR/$app/$app" ]; then
            cp -f "$APPS_DIR/$app/$app" "/usr/local/bin/$app"
            chmod 755 "/usr/local/bin/$app"
            ln -sfn "/usr/local/bin/$app" "/usr/bin/$app"
        fi
    done
    if [ -f "$APPS_DIR/axis-settings/axisos-settings-panel" ]; then
        cp -f "$APPS_DIR/axis-settings/axisos-settings-panel" "/usr/local/bin/axisos-settings-panel"
        chmod 755 "/usr/local/bin/axisos-settings-panel"
        ln -sfn "/usr/local/bin/axisos-settings-panel" "/usr/bin/axisos-settings-panel"
    fi
    if [ -f "$REPO_ROOT/shell-c/axisos-shell" ]; then
        cp -f "$REPO_ROOT/shell-c/axisos-shell" "/usr/local/bin/axisos-shell"
        chmod 755 "/usr/local/bin/axisos-shell"
        ln -sfn "/usr/local/bin/axisos-shell" "/usr/bin/axisos-shell"
        ln -sfn "/usr/local/bin/axisos-shell" "/usr/local/bin/LiquidNodesOS"
        ln -sfn "/usr/local/bin/axisos-shell" "/usr/bin/LiquidNodesOS"
    fi
    if [ -f "$REPO_ROOT/pkg-mgr/bin/axis" ]; then
        cp -f "$REPO_ROOT/pkg-mgr/bin/axis" "/usr/local/bin/axis"
        chmod 755 "/usr/local/bin/axis"
        ln -sfn "/usr/local/bin/axis" "/usr/bin/axis"
    fi
    if [ -f "$REPO_ROOT/compositor/axis-compositor" ]; then
        cp -f "$REPO_ROOT/compositor/axis-compositor" "/usr/local/bin/axis-compositor"
        chmod 755 "/usr/local/bin/axis-compositor"
        ln -sfn "/usr/local/bin/axis-compositor" "/usr/bin/axis-compositor"
    fi
fi

echo "======================================================================"
echo "  All 12 Pure C Desktop Applications and Shell Compiled Successfully! "
echo "======================================================================"
