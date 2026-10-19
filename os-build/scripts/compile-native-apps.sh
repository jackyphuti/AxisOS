#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
APPS_DIR="${1:-$REPO_ROOT/os-build/apps}"

echo "=== AxisOS Native & GTK3 Desktop Applications Builder ==="

# 1. Deploy GTK3 Self-Healing axis-terminal (zero Qt6 dependency, works on X11/Wayland/VMs)
echo "--> Configuring axis-terminal..."
if [ -f "$APPS_DIR/axis-terminal/axis-terminal.py" ]; then
    cp -f "$APPS_DIR/axis-terminal/axis-terminal.py" "$APPS_DIR/axis-terminal/axis-terminal"
fi
chmod 755 "$APPS_DIR/axis-terminal/axis-terminal"

# 2. Compile axis-installer (C++ with GTK 3) or fallback to Python GTK3
echo "--> Building axis-installer..."
if command -v g++ >/dev/null 2>&1 && pkg-config --exists gtk+-3.0 2>/dev/null; then
    g++ -O2 -std=c++17 "$APPS_DIR/axis-installer/main.cpp" $(pkg-config --cflags --libs gtk+-3.0) -o "$APPS_DIR/axis-installer/axis-installer"
elif [ -f "$APPS_DIR/axis-installer/axis-installer.py" ]; then
    cp -f "$APPS_DIR/axis-installer/axis-installer.py" "$APPS_DIR/axis-installer/axis-installer"
fi
chmod 755 "$APPS_DIR/axis-installer/axis-installer"

# 3. Deploy unified axisos-settings-panel and axis-settings forwarder
echo "--> Configuring axisos-settings-panel..."
if [ -f "$APPS_DIR/axis-settings/axis-settings.py" ]; then
    cp -f "$APPS_DIR/axis-settings/axis-settings.py" "$APPS_DIR/axis-settings/axisos-settings-panel"
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
fi

# 4. Compile axis-usb-daemon (C with libudev)
echo "--> Building axis-usb-daemon..."
if command -v gcc >/dev/null 2>&1 && pkg-config --exists libudev 2>/dev/null; then
    gcc -O2 "$APPS_DIR/axis-usb-daemon/main.c" $(pkg-config --cflags --libs libudev) -o "$APPS_DIR/axis-usb-daemon/axis-usb-daemon"
fi
chmod 755 "$APPS_DIR/axis-usb-daemon/axis-usb-daemon" 2>/dev/null || true

# 5. Compile axis-task-manager (C++ with GTK 3)
echo "--> Building axis-task-manager..."
if command -v g++ >/dev/null 2>&1 && pkg-config --exists gtk+-3.0 2>/dev/null; then
    g++ -O2 -std=c++17 "$APPS_DIR/axis-task-manager/main.cpp" $(pkg-config --cflags --libs gtk+-3.0) -o "$APPS_DIR/axis-task-manager/axis-task-manager"
    strip "$APPS_DIR/axis-task-manager/axis-task-manager" 2>/dev/null || true
fi
chmod 755 "$APPS_DIR/axis-task-manager/axis-task-manager" 2>/dev/null || true

# 6. Deploy GTK3 axis-package-installer, axis-scheduler-manager, and axis-welcome
for py_app in axis-package-installer axis-scheduler-manager axis-welcome; do
    echo "--> Configuring $py_app..."
    if [ -f "$APPS_DIR/$py_app/$py_app.py" ]; then
        cp -f "$APPS_DIR/$py_app/$py_app.py" "$APPS_DIR/$py_app/$py_app"
    fi
    chmod 755 "$APPS_DIR/$py_app/$py_app"
done

# 7. Compile axis-update (Atomic Btrfs/A-B System Update Engine)
if [ -d "$REPO_ROOT/os-build/updater" ] && command -v gcc >/dev/null 2>&1; then
    echo "--> Compiling axis-update atomic updater..."
    make -C "$REPO_ROOT/os-build/updater" clean all || true
fi

# 8. Install to /usr/local/bin and /usr/bin if running as root
if [ -d "/usr/local/bin" ] && [ "${EUID:-$(id -u)}" -eq 0 ]; then
    echo "--> Installing applications to /usr/local/bin and /usr/bin..."
    for app in axis-terminal axis-package-installer axis-scheduler-manager axis-welcome axis-installer axis-settings axis-software-manager axis-update-manager axis-usb-daemon axis-task-manager axis-oobe; do
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
    ln -sfn /usr/local/bin/axis-terminal /usr/local/bin/gnome-terminal
fi

echo "=== All AxisOS Desktop Applications Configured Successfully! ==="
