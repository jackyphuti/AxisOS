# AxisOS Development & Build Guide

Welcome to the development guide for **AxisOS**. This document provides detailed instructions on setting up your local environment, developing the native Wayland compositor and Qt 6 desktop shell, compiling native C++ applications, and generating bootable hybrid UEFI/BIOS ISOs.

---

## Table of Contents
1. [Prerequisites & System Requirements](#prerequisites--system-requirements)
2. [Setting Up the Development Environment](#setting-up-the-development-environment)
3. [Building the Native Wayland Compositor](#building-the-native-wayland-compositor)
4. [Developing the Qt 6 Desktop Shell](#developing-the-qt-6-desktop-shell)
5. [Compiling Native C++ Applications](#compiling-native-c-applications)
6. [Testing in QEMU Virtual Machine](#testing-in-qemu-virtual-machine)
7. [Building the Bootable Linux ISO](#building-the-bootable-linux-iso)
8. [Troubleshooting & FAQs](#troubleshooting--faqs)
9. [Related Resources](#related-resources)

---

## Prerequisites & System Requirements

### Host Operating Systems Supported:
- **Linux** (Debian 12+, Ubuntu 22.04+, Fedora 38+, Arch Linux)
- **Windows 11 / 10** via **WSL 2** (Ubuntu or Debian recommended)

### Required Toolchains:
| Tool | Minimum Version | Purpose |
| :--- | :--- | :--- |
| **GCC / G++** | `>= 12.x` | Compiling native C compositor and C++ applications |
| **Make** | `>= 4.x` | Native build scripts |
| **wayland-scanner** | `>= 1.20` | Dynamic Wayland protocol generation |
| **wlroots** | `>= 0.16` | Wayland display server library |
| **Qt 6 / QML** | `>= 6.4` | Glassmorphic desktop shell runtime |
| **Python** | `>= 3.10` | ISO builder engine and tooling |
| **xorriso** | `>= 1.5` | Hybrid ISO image assembly |
| **QEMU** | `>= 7.x` | Local virtual machine hardware simulation |

---

## Setting Up the Development Environment

1. **Clone the repository**:
   ```bash
   git clone https://github.com/jackyphuti/AxisOS.git
   cd AxisOS
   ```

2. **Install Build Dependencies (Debian / Ubuntu / WSL)**:
   ```bash
   sudo apt-get update
   sudo apt-get install -y \
       build-essential cmake pkg-config \
       libwlroots-dev libwayland-dev wayland-protocols \
       libxkbcommon-dev libinput-dev libdrm-dev libpixman-1-dev \
       qt6-base-dev qt6-declarative-dev qml6-module-qtquick \
       libgtk-3-dev libvte-2.91-dev \
       xorriso squashfs-tools dosfstools mtools isolinux syslinux-utils \
       qemu-system-x86 ovmf
   ```

---

## Building the Native Wayland Compositor

The native compositor (`compositor/`) is built on `wlroots` and `wlr_scene`. Wayland protocol bindings (`xdg-shell`, `wlr-layer-shell-unstable-v1`) are compiled dynamically via `wayland-scanner`:

```bash
cd compositor
make clean all
```

This generates the native executable:
```
compositor/axis-compositor
```

To install the compositor locally:
```bash
sudo make install
```

---

## Developing the Qt 6 Desktop Shell

The **LiquidNodes** desktop shell (`compositor-qt/`) provides the visual desktop experience (dock, top bar, application launcher, and lock screen):

- **Main entry point**: `compositor-qt/Main.qml`
- **Shell layout components**:
  - `LiquidTopBar.qml`: Status indicators, clock, network, and audio controls.
  - `LiquidDock.qml`: Floating application dock with launch animations.
  - `LiquidAppLauncher.qml`: Fullscreen application grid with search.
  - `QuickSettingsPanel.qml`: Hardware toggles (Wi-Fi, Bluetooth, Dark Mode, Performance).
  - `Theme.qml`: Centralized One UI design tokens (palette, corner radii, typography).

To run the shell in standalone preview mode with `qml6`:
```bash
qml6 compositor-qt/Main.qml
```

---

## Compiling Native C++ Applications

Core AxisOS applications are written in native C++ for minimal memory overhead and zero runtime lag:

- **Installer (`os-build/apps/axis-installer/`)**: GTK 3 / C++17 graphical disk partitioning and installation wizard.
- **Terminal (`os-build/apps/axis-terminal/`)**: Low-latency terminal emulator built with VTE and GTK 3.
- **Settings (`os-build/apps/axis-settings/`)**: Unified control center for hardware, display, and OS editions.
- **Task Manager (`os-build/apps/axis-task-manager/`)**: Real-time hardware telemetry HUD reading directly from `/proc` and `/sys`.

To compile all applications:
```bash
./os-build/scripts/compile-native-apps.sh
```

---

## Testing in QEMU Virtual Machine

To test AxisOS on virtual hardware without formatting physical media:

### On Linux / WSL 2:
```bash
./os-build/scripts/run-qemu.sh
```

### What QEMU Simulates:
1. Allocates a virtual hard drive (`axisos-disk.qcow2`).
2. Boots the hybrid ISO with UEFI firmware (OVMF) or Legacy BIOS.
3. Launches the native Wayland desktop session with software rendering fallback.
4. Allows end-to-end testing of the **Install AxisOS** installer (GPT partitioning, Btrfs/EXT4 formatting, GRUB EFI setup).
5. Verifies direct booting from the installed drive after reboot.

---

## Building the Bootable Linux ISO

The hybrid UEFI + Legacy BIOS ISO engine is driven by `os-build/scripts/build-hybrid-uefi-iso.py`:

```bash
sudo python3 os-build/scripts/build-hybrid-uefi-iso.py
```

### Build Pipeline:
1. Synchronizes workspace assets into the target chroot.
2. Compiles native desktop applications and the Wayland compositor.
3. Generates high-compression Squashfs filesystem (`live/filesystem.squashfs`).
4. Configures ISOLINUX (Legacy BIOS) and Microsoft-signed Shim + GRUB (UEFI Secure Boot).
5. Assembles the hybrid ISO using `xorriso` and applies `isohybrid` partition flags.

### Output:
```
axisos-v2.1-live-amd64.iso
```
The ISO can be written to a USB drive via Rufus, BalenaEtcher, Ventoy, or `dd`.

---

## Troubleshooting & FAQs

### 1. Black Screen in Virtual Machines
If running in QEMU, VirtualBox, or VMware without 3D acceleration, ensure software rendering environment variables are present:
```bash
export WLR_RENDERER_ALLOW_SOFTWARE=1
export WLR_NO_HARDWARE_CURSORS=1
export QT_QPA_PLATFORM=wayland
```

### 2. Missing Wayland Protocol Headers
Protocol headers are generated automatically during the build:
```bash
cd compositor
make clean all
```
Ensure `wayland-scanner` is installed via `libwayland-bin` or `wayland-protocols`.

### 3. Non-Interactive Sudo Execution
The live installer runs non-interactively. Ensure `/etc/sudoers.d/axis` contains:
```
axis ALL=(ALL:ALL) NOPASSWD: ALL
Defaults secure_path="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
Defaults !use_pty
Defaults !requiretty
```

---

## Related Resources
- [Installation & Deployment Guide](INSTALLATION_GUIDE.md)
- [Installer Troubleshooting Guide](INSTALLER_TROUBLESHOOTING.md)
- [Contributing Guidelines](CONTRIBUTING.md)
