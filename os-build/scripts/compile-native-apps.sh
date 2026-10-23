#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
APPS_DIR="${1:-$REPO_ROOT/os-build/apps}"
JOBS="$(nproc 2>/dev/null || echo 2)"

echo "=== AxisOS 2.1 Native C++20 / Qt 6 & GTK3 Desktop Application Suite Builder ==="

# Helper to build a CMake C++/Qt6 application directory
build_qt_app() {
    local app_name="$1"
    local app_dir="$APPS_DIR/$app_name"
    if [ ! -f "$app_dir/CMakeLists.txt" ]; then
        return 0
    fi
    echo "--> Building C++20 / Qt 6 application: $app_name..."
    mkdir -p "$app_dir/build"
    (
        cd "$app_dir/build"
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j"$JOBS"
        if [ -f "$app_name" ]; then
            cp -f "$app_name" "$app_dir/$app_name"
            chmod 755 "$app_dir/$app_name"
        fi
    ) || echo "[WARN] Qt6 build for $app_name failed or skipped."
}

# 1. Build C++20 / Qt 6 QML Applications
if command -v cmake >/dev/null 2>&1; then
    for app in axis-terminal axis-package-installer axis-scheduler-manager axis-welcome; do
        build_qt_app "$app"
    done
fi

# Fallback for axis-terminal if Qt6 build didn't run or failed
if [ ! -f "$APPS_DIR/axis-terminal/axis-terminal" ] && [ -f "$APPS_DIR/axis-terminal/axis-terminal.py" ]; then
    cp -f "$APPS_DIR/axis-terminal/axis-terminal.py" "$APPS_DIR/axis-terminal/axis-terminal"
    chmod 755 "$APPS_DIR/axis-terminal/axis-terminal"
fi

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

# 6. Fallback launchers for package installer, scheduler manager, welcome, oobe, nobara-welcome
for py_app in axis-package-installer axis-scheduler-manager axis-welcome axis-oobe nobara-welcome; do
    if [ ! -f "$APPS_DIR/$py_app/$py_app" ] && [ -f "$APPS_DIR/$py_app/$py_app.py" ]; then
        echo "--> Configuring fallback launcher for $py_app..."
        cp -f "$APPS_DIR/$py_app/$py_app.py" "$APPS_DIR/$py_app/$py_app"
        chmod 755 "$APPS_DIR/$py_app/$py_app"
    fi
done

# 7. Compile axis-update (Atomic Btrfs/A-B System Update Engine)
if [ -d "$REPO_ROOT/os-build/updater" ] && command -v gcc >/dev/null 2>&1; then
    echo "--> Compiling axis-update atomic updater..."
    make -C "$REPO_ROOT/os-build/updater" clean all || true
fi

# 8. Compile Native C wlroots Fallback Compositor (axis-compositor)
if [ -d "$REPO_ROOT/compositor" ] && command -v gcc >/dev/null 2>&1 && pkg-config --exists wlroots 2>/dev/null; then
    echo "--> Compiling native wlroots fallback compositor (axis-compositor)..."
    make -C "$REPO_ROOT/compositor" clean all || true
fi

# 9. Compile Primary C++20 / Qt 6 LiquidNodesOS Wayland Compositor
if [ -d "$REPO_ROOT/compositor-qt" ] && command -v cmake >/dev/null 2>&1; then
    echo "--> Building Qt6 LiquidNodesOS Wayland Compositor..."
    mkdir -p "$REPO_ROOT/compositor-qt/build"
    (
        cd "$REPO_ROOT/compositor-qt/build"
        cmake .. -DCMAKE_BUILD_TYPE=Release
        make -j"$JOBS"
        if [ -f "LiquidNodesOS" ]; then
            cp -f LiquidNodesOS "$REPO_ROOT/compositor-qt/LiquidNodesOS"
            chmod 755 "$REPO_ROOT/compositor-qt/LiquidNodesOS"
        fi
    ) || echo "[WARN] LiquidNodesOS build skipped or failed, using pre-existing binary."
fi

# 10. Install to /usr/local/bin and /usr/bin if running as root
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
    if [ -f "$REPO_ROOT/compositor-qt/LiquidNodesOS" ]; then
        cp -f "$REPO_ROOT/compositor-qt/LiquidNodesOS" "/usr/local/bin/LiquidNodesOS"
        chmod 755 "/usr/local/bin/LiquidNodesOS"
        ln -sfn "/usr/local/bin/LiquidNodesOS" "/usr/bin/LiquidNodesOS"
        mkdir -p /usr/share/axis-compositor-qt
        cp -rf "$REPO_ROOT/compositor-qt/"*.qml /usr/share/axis-compositor-qt/ 2>/dev/null || true
    fi
    if [ -f "$REPO_ROOT/compositor/axis-compositor" ]; then
        cp -f "$REPO_ROOT/compositor/axis-compositor" "/usr/local/bin/axis-compositor"
        chmod 755 "/usr/local/bin/axis-compositor"
        ln -sfn "/usr/local/bin/axis-compositor" "/usr/bin/axis-compositor"
    fi
fi

echo "=== All AxisOS Desktop Applications Configured Successfully! ==="
