#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
APPS_DIR="${1:-$REPO_ROOT/os-build/apps}"

echo "=== AxisOS Native C/C++ Applications Compiler ==="
echo "Building all native OS apps for x86_64..."

# 1. Compile axis-terminal (C++ with GTK 3 & VTE 2.91)
echo "--> Compiling axis-terminal..."
g++ -O2 -std=c++17 \
    "$APPS_DIR/axis-terminal/main.cpp" \
    $(pkg-config --cflags --libs gtk+-3.0 vte-2.91) \
    -o "$APPS_DIR/axis-terminal/axis-terminal"
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

# 6. Compile nobara-welcome (C++ with GTK 3)
echo "--> Compiling nobara-welcome..."
g++ -O2 -std=c++17 \
    "$APPS_DIR/nobara-welcome/main.cpp" \
    $(pkg-config --cflags --libs gtk+-3.0) \
    -o "$APPS_DIR/nobara-welcome/nobara-welcome"
chmod 755 "$APPS_DIR/nobara-welcome/nobara-welcome"

# 7. Compile axis-oobe (C++ with GTK 3)
echo "--> Compiling axis-oobe..."
g++ -O2 -std=c++17 \
    "$APPS_DIR/axis-oobe/main.cpp" \
    $(pkg-config --cflags --libs gtk+-3.0) \
    -o "$APPS_DIR/axis-oobe/axis-oobe"
chmod 755 "$APPS_DIR/axis-oobe/axis-oobe"

# Optional: Install to system /usr/local/bin if running inside root chroot
if [ -d "/usr/local/bin" ] && [ "$EUID" -eq 0 ]; then
    echo "--> Installing compiled binaries to /usr/local/bin..."
    cp -f "$APPS_DIR/axis-terminal/axis-terminal" /usr/local/bin/
    cp -f "$APPS_DIR/axis-installer/axis-installer" /usr/local/bin/
    cp -f "$APPS_DIR/axis-settings/axis-settings" /usr/local/bin/
    cp -f "$APPS_DIR/axis-usb-daemon/axis-usb-daemon" /usr/local/bin/
    cp -f "$APPS_DIR/axis-task-manager/axis-task-manager" /usr/local/bin/
    cp -f "$APPS_DIR/nobara-welcome/nobara-welcome" /usr/local/bin/
    cp -f "$APPS_DIR/axis-oobe/axis-oobe" /usr/local/bin/
fi

echo "=== All AxisOS Native C/C++ Applications Built Successfully! ==="
