# AxisOS Development & Build Guide 🛠️

Welcome to the comprehensive development guide for **AxisOS**. This document provides detailed, step-by-step instructions on setting up your local environment, developing the desktop shell, testing system daemons, compiling native binaries, and building/testing the full bootable Linux operating system.

---

## 📋 Table of Contents
1. [Prerequisites & System Requirements](#-prerequisites--system-requirements)
2. [Setting Up the Development Environment](#-setting-up-the-development-environment)
3. [Developing the Desktop Shell](#-developing-the-desktop-shell)
   - [Running in Development Mode](#running-in-development-mode)
   - [Building the Production Bundle](#building-the-production-bundle)
   - [Running the Native Electron Wrapper](#running-the-native-electron-wrapper)
4. [Running the System Management Daemon](#-running-the-system-management-daemon)
5. [Compiling Native Systems & Kernel Drivers](#-compiling-native-systems--kernel-drivers)
   - [Atomic Update Engine (`axis update`)](#atomic-update-engine-axis-update)
   - [Linux Kernel Device Driver](#linux-kernel-device-driver)
6. [Testing in QEMU Virtual Machine](#-testing-in-qemu-virtual-machine)
7. [Building the Bootable Linux ISO](#-building-the-bootable-linux-iso)
8. [Troubleshooting & FAQs](#-troubleshooting--faqs)

---

## 💻 Prerequisites & System Requirements

### Host Operating Systems Supported:
- **Linux** (Debian 12+, Ubuntu 22.04+, Fedora 38+, Arch Linux)
- **Windows 11 / 10** via **WSL 2** (Ubuntu or Debian recommended)
- **macOS** (For Desktop Shell development)

### Required Toolchains:
| Tool | Minimum Version | Purpose |
| :--- | :--- | :--- |
| **Node.js** | `>= 20.x` | Desktop shell build system and system daemon |
| **npm** | `>= 10.x` | Package management for the shell |
| **GCC / Clang** | `>= 12.x` | Compiling native C update engine and kernel modules |
| **Make** | `>= 4.x` | Native build scripts |
| **QEMU** | `>= 7.x` | Local virtual machine hardware simulation |
| **live-build** | `>= 20230131` | Debian Linux live filesystem builder (for ISO creation) |

---

## 🚀 Setting Up the Development Environment

1. **Clone the repository**:
   ```bash
   git clone https://github.com/jackyphuti/AxisOS.git
   cd AxisOS
   ```

2. **Install Desktop Shell dependencies**:
   ```bash
   cd shell
   npm install
   ```

---

## 🖥️ Developing the Desktop Shell

The AxisOS desktop shell is built with **React 19**, **TypeScript**, **Vite**, and **Tailwind CSS**, styled with modern glassmorphism.

### Running in Development Mode
To start the Vite hot-reloading development server:
```bash
cd shell
npm run dev
```
By default, the dev server runs on `http://localhost:5173`. Open this URL in any modern browser to inspect UI components, window management, the floating dock, top bar, and applications.

### Building the Production Bundle
To compile TypeScript and bundle static assets with Vite:
```bash
cd shell
npm run build
```
This outputs optimized, minified production assets into `shell/dist/`:
- `index.html`
- `assets/index-*.js`
- `assets/index-*.css`

### Running the Native Electron Wrapper
To test the desktop shell as a native standalone desktop application:
```bash
cd shell
npm start
```

---

## ⚡ Running the System Management Daemon

The **AxisOS System Daemon** (`os-build/configs/cage-session/axisos-daemon.cjs`) is a high-performance, asynchronous Node.js backend providing REST APIs for:
- Live system telemetry (CPU, RAM, GPU, storage, battery, temperatures)
- Real Linux bash command execution (`/api/terminal-exec`)
- Posix filesystem operations, MIME detection, and permissions (`/api/fs-*`)
- Freedesktop.org XDG Trash integration (`/api/fs-trash/*`)
- NetworkManager Wi-Fi and BlueZ Bluetooth scanning
- Real block device detection and mounting (`/api/disks`)
- Atomic OS installer execution (`/api/installer/*`)

### Running the Daemon Locally:
```bash
# From the shell directory:
cd shell
npm run serve

# Or directly using Node.js:
node os-build/configs/cage-session/axisos-daemon.cjs
```
The daemon listens on `http://127.0.0.1:3000`. You can test endpoints via `curl`:
```bash
curl http://127.0.0.1:3000/api/system-info
```

---

## ⚙️ Compiling Native Systems & Kernel Drivers

### Atomic Update Engine (`axis update`)
The native C update engine handles atomic A/B root filesystem staging, kexec near-instant reboots, and bootloader rollbacks:
```bash
cd os-build/updater
gcc -O2 -Wall -Wextra axis_update.c -o axis
```
To install it to your local `/usr/bin/axis`:
```bash
sudo cp axis /usr/bin/axis
sudo ln -sf /usr/bin/axis /usr/bin/axis-update
sudo chmod +x /usr/bin/axis
```

### Linux Kernel Device Driver
AxisOS includes custom Linux platform and character drivers in `os-build/drivers/`:
```bash
cd os-build/drivers
# Build kernel module against active kernel headers
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
```

---

## 🧪 Testing in QEMU Virtual Machine

To test AxisOS on virtual hardware without formatting physical media:

### On Linux / WSL 2:
```bash
./os-build/scripts/run-qemu.sh
```

### On Windows PowerShell:
```powershell
.\os-build\scripts\run-qemu.ps1
```

### What QEMU Simulates:
1. Allocates a 20GB QCOW2 virtual hard drive (`axisos-disk.qcow2`).
2. Boots the hybrid ISO with UEFI firmware (OVMF) or BIOS.
3. Launches the live Wayland desktop session.
4. Allows end-to-end testing of the **Install AxisOS** app (GPT partitioning, Btrfs subvolumes, GRUB install).
5. Tests direct boot from the newly installed disk after rebooting.

---

## 💿 Building the Bootable Linux ISO

> [!NOTE]
> The full production ISO release for v2.0 is scheduled for December. However, developers can build local ISO images for testing anytime.

### On Linux / Debian / Ubuntu / WSL 2:
Make sure `live-build` and `debootstrap` are installed:
```bash
sudo apt-get update
sudo apt-get install -y live-build debootstrap squashfs-tools xorriso
```
Execute the build script:
```bash
sudo ./os-build/scripts/build-iso.sh
```

### Output:
The completed hybrid bootable image is generated at:
```
axisos-live-amd64.iso
```
This image can be flashed to a USB drive via Rufus, BalenaEtcher, Ventoy, or `dd`.

---

## ❓ Troubleshooting & FAQs

### 1. `GLIBC` Version Mismatch
If compiling native binaries on a host with a newer glibc (e.g. Fedora with glibc 2.38+) targeting Debian 12 (glibc 2.36), compile the binary inside the target Debian chroot:
```bash
sudo chroot /path/to/chroot gcc -O2 axis_update.c -o /usr/bin/axis
```

### 2. Port 3000 Already in Use
If starting the daemon fails with `EADDRINUSE`:
```bash
# Check what is using port 3000
lsof -i :3000
# Or kill previous daemon instance:
pkill -f axisos-daemon.cjs
```

### 3. Wayland Compositor (Cage) Display Issues
When running in kiosk mode on real hardware, Cage requires access to `/dev/dri/card*` and `/dev/input/*`. Ensure the session user is a member of the `video`, `render`, and `input` groups:
```bash
sudo usermod -aG video,render,input $USER
```

---

## 🔗 Related Resources
- [Installation & Deployment Guide](INSTALLATION_GUIDE.md)
- [Contributing Guidelines](CONTRIBUTING.md)
- [Axis Package Manager Documentation](pkg-mgr/README.md)
