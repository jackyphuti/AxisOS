#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
APPS_DIR="${1:-$REPO_ROOT/os-build/apps}"

echo "=== AxisOS Native C/C++ Applications Compiler ==="
echo "Building all native OS apps for x86_64..."

# 1. Compile axis-terminal (Native Qt 6 / C++20 / libvterm or GTK fallback)
echo "--> Compiling axis-terminal..."
if [ -f "$APPS_DIR/axis-terminal/CMakeLists.txt" ]; then
    echo "    Using CMake for Qt 6 / libvterm terminal build..."
    cmake -B "$APPS_DIR/axis-terminal/build" -S "$APPS_DIR/axis-terminal" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$APPS_DIR/axis-terminal/build" -j$(nproc 2>/dev/null || echo 4)
    cp -f "$APPS_DIR/axis-terminal/build/axis-terminal" "$APPS_DIR/axis-terminal/axis-terminal"
else
    g++ -O2 -std=c++17 \
        "$APPS_DIR/axis-terminal/main.cpp" \
        $(pkg-config --cflags --libs gtk+-3.0 vte-2.91) \
        -o "$APPS_DIR/axis-terminal/axis-terminal"
fi
chmod 755 "$APPS_DIR/axis-terminal/axis-terminal"

# 2. Compile axis-installer (C++ with GTK 3, Windows 10 Wizard Style)
echo "--> Compiling axis-installer..."
g++ -O2 -std=c++17 \
    "$APPS_DIR/axis-installer/main.cpp" \
    $(pkg-config --cflags --libs gtk+-3.0) \
    -o "$APPS_DIR/axis-installer/axis-installer"
chmod 755 "$APPS_DIR/axis-installer/axis-installer"

# 3. Compile axis-settings (C++ with GTK 3, Hardware Controls & Desktop Tuning)
echo "--> Compiling axis-settings..."
g++ -O2 -std=c++17 \
    "$APPS_DIR/axis-settings/main.cpp" \
    $(pkg-config --cflags --libs gtk+-3.0) \
    -o "$APPS_DIR/axis-settings/axis-settings"
chmod 755 "$APPS_DIR/axis-settings/axis-settings"

# 4. Compile axis-usb-daemon (C with libudev)
echo "--> Compiling axis-usb-daemon..."
gcc -O2 \
    "$APPS_DIR/axis-usb-daemon/main.c" \
    $(pkg-config --cflags --libs libudev) \
    -o "$APPS_DIR/axis-usb-daemon/axis-usb-daemon"
chmod 755 "$APPS_DIR/axis-usb-daemon/axis-usb-daemon"

# 5. Compile axis-task-manager (C++ with GTK 3)
echo "--> Compiling axis-task-manager..."
g++ -O2 -std=c++17 \
    "$APPS_DIR/axis-task-manager/main.cpp" \
    $(pkg-config --cflags --libs gtk+-3.0) \
    -o "$APPS_DIR/axis-task-manager/axis-task-manager"
chmod 755 "$APPS_DIR/axis-task-manager/axis-task-manager"

# 6. Compile axis-package-installer (Qt 6 / C++20)
if [ -f "$APPS_DIR/axis-package-installer/CMakeLists.txt" ] && which cmake >/dev/null 2>&1; then
    echo "--> Compiling axis-package-installer..."
    cmake -B "$APPS_DIR/axis-package-installer/build" -S "$APPS_DIR/axis-package-installer" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$APPS_DIR/axis-package-installer/build" -j$(nproc 2>/dev/null || echo 4)
    cp -f "$APPS_DIR/axis-package-installer/build/axis-package-installer" "$APPS_DIR/axis-package-installer/axis-package-installer"
    chmod 755 "$APPS_DIR/axis-package-installer/axis-package-installer"
fi

# 7. Compile axis-scheduler-manager (Qt 6 / C++20)
if [ -f "$APPS_DIR/axis-scheduler-manager/CMakeLists.txt" ] && which cmake >/dev/null 2>&1; then
    echo "--> Compiling axis-scheduler-manager..."
    cmake -B "$APPS_DIR/axis-scheduler-manager/build" -S "$APPS_DIR/axis-scheduler-manager" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$APPS_DIR/axis-scheduler-manager/build" -j$(nproc 2>/dev/null || echo 4)
    cp -f "$APPS_DIR/axis-scheduler-manager/build/axis-scheduler-manager" "$APPS_DIR/axis-scheduler-manager/axis-scheduler-manager"
    chmod 755 "$APPS_DIR/axis-scheduler-manager/axis-scheduler-manager"
fi

# 8. Compile axis-welcome (Qt 6 / C++20)
if [ -f "$APPS_DIR/axis-welcome/CMakeLists.txt" ] && which cmake >/dev/null 2>&1; then
    echo "--> Compiling axis-welcome..."
    cmake -B "$APPS_DIR/axis-welcome/build" -S "$APPS_DIR/axis-welcome" -DCMAKE_BUILD_TYPE=Release
    cmake --build "$APPS_DIR/axis-welcome/build" -j$(nproc 2>/dev/null || echo 4)
    cp -f "$APPS_DIR/axis-welcome/build/axis-welcome" "$APPS_DIR/axis-welcome/axis-welcome"
    chmod 755 "$APPS_DIR/axis-welcome/axis-welcome"
fi

# 8b. Compile axis-update (Atomic Btrfs/A-B System Update Engine)
if [ -d "$REPO_ROOT/os-build/updater" ]; then
    echo "--> Compiling axis-update atomic updater..."
    make -C "$REPO_ROOT/os-build/updater" clean all
fi

# 9. Compile axis-oobe (C++ with GTK 3)
echo "--> Compiling axis-oobe..."
g++ -O2 -std=c++17 \
    "$APPS_DIR/axis-oobe/main.cpp" \
    $(pkg-config --cflags --libs gtk+-3.0) \
    -o "$APPS_DIR/axis-oobe/axis-oobe"
chmod 755 "$APPS_DIR/axis-oobe/axis-oobe"

# Optional: Install to system /usr/local/bin if running inside root chroot
if [ -d "/usr/local/bin" ] && [ "$EUID" -eq 0 ]; then
    echo "--> Installing compiled binaries to /usr/local/bin..."
    [ -f "$APPS_DIR/axis-terminal/axis-terminal" ] && cp -f "$APPS_DIR/axis-terminal/axis-terminal" /usr/local/bin/
    [ -f "$APPS_DIR/axis-package-installer/axis-package-installer" ] && cp -f "$APPS_DIR/axis-package-installer/axis-package-installer" /usr/local/bin/
    [ -f "$APPS_DIR/axis-scheduler-manager/axis-scheduler-manager" ] && cp -f "$APPS_DIR/axis-scheduler-manager/axis-scheduler-manager" /usr/local/bin/
    [ -f "$APPS_DIR/axis-welcome/axis-welcome" ] && cp -f "$APPS_DIR/axis-welcome/axis-welcome" /usr/local/bin/
    [ -f "$APPS_DIR/axis-installer/axis-installer" ] && cp -f "$APPS_DIR/axis-installer/axis-installer" /usr/local/bin/
    [ -f "$APPS_DIR/axis-settings/axis-settings" ] && cp -f "$APPS_DIR/axis-settings/axis-settings" /usr/local/bin/
    [ -f "$APPS_DIR/axis-usb-daemon/axis-usb-daemon" ] && cp -f "$APPS_DIR/axis-usb-daemon/axis-usb-daemon" /usr/local/bin/
    [ -f "$APPS_DIR/axis-task-manager/axis-task-manager" ] && cp -f "$APPS_DIR/axis-task-manager/axis-task-manager" /usr/local/bin/
    [ -f "$APPS_DIR/axis-oobe/axis-oobe" ] && cp -f "$APPS_DIR/axis-oobe/axis-oobe" /usr/local/bin/
    [ -f "$REPO_ROOT/os-build/updater/axis-update" ] && cp -f "$REPO_ROOT/os-build/updater/axis-update" /usr/bin/axis-update && chmod 755 /usr/bin/axis-update
fi

echo "=== All AxisOS Native C/C++ Applications Built Successfully! ==="
