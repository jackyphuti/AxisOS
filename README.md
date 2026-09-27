# AxisOS — Production Linux Operating System

<div align="center">
  <h3>▲ AxisOS 1.0 "Horizon"</h3>
  <p>A modern, fluid, glassmorphic Linux operating system combining the Linux kernel, Cage Wayland compositor, and a hardware-accelerated desktop shell.</p>
</div>

---

## 🌟 Overview

**AxisOS** is a fully functional, bootable Linux operating system with a native Wayland kiosk compositor (**Cage**), an asynchronous **System Management Daemon** (`axisos-daemon`), and a modern desktop environment with a real **Operating System Installer**.

### 🧩 Core Production Systems:

1. **AxisOS Real System Installation Engine (`axisos-install.sh`)**:
   - **Real Block Device Detection**: Probes NVMe, SATA, and VirtIO disks via `lsblk` / sysfs.
   - **Automated GPT Partitioning**: 512MB EFI System Partition (ESP FAT32), 4GB Linux Swap, and remaining capacity for Linux Root.
   - **Btrfs Subvolumes with zstd Compression**: Sets up `@` (root), `@home` (user profiles), `@snapshots` (system rollback), and `@var_log`.
   - **Live Root Deployment**: Replicates base userspace to target drive via `rsync` with attribute preservation.
   - **Fstab & Identity Configuration**: Automatically writes `/etc/fstab` with persistent UUIDs, sets hostname, locale, and keyboard map.
   - **User Provisioning**: Creates user account with sudo privileges, hardware groups (video, audio, input, seat), and autologin session.
   - **GRUB 2 EFI / BIOS Bootloader**: Installs EFI boot binaries into ESP (`/boot/efi`) and executes `update-grub`.

2. **AxisOS System Management Daemon (`axisos-daemon.cjs`)**:
   - Runs as a systemd service (`axisos-daemon.service`) listening on `http://127.0.0.1:3000`.
   - High-performance, zero-external-dependency Node.js HTTP server.
   - Provides REST endpoints:
     - `/api/system-info`: Live hardware stats (kernel, CPU cores, RAM, GPU, disks, uptime).
     - `/api/disks`: Probes and filters block storage devices.
     - `/api/terminal-exec`: Real Linux bash command execution.
     - `/api/fs-read`, `/api/fs-write`, `/api/fs-mkdir`, `/api/fs-delete`: Filesystem operations.
     - `/api/installer/start` & `/api/installer/status`: Background OS installation with live log streaming.
     - `/api/power`: Real `poweroff` and `reboot` integration via `systemd`.

3. **Wayland Display Layer & Desktop Shell**:
   - Runs on bare metal DRM/KMS using the **Cage** Wayland kiosk compositor.
   - Kiosk launcher `axisos-kiosk.sh` starts Chromium or Electron with native Wayland Ozone flags.
   - Interactive Window Manager with dragging, resizing, minimizing, and maximizing.
   - Top status bar, Spotlight Search (`Ctrl+Space`), Control Center quick settings, and animated floating glass dock.
   - **Everyday Desktop Apps**:
     - 🛒 **Axis Store**: Discover, install, update, and remove Linux APT & Debian packages with live log streaming.
     - 🧭 **Safari Web Browser**: Multi-tab browsing, speed dial bookmarks, smart address bar, and DuckDuckGo search.
     - 🎵 **Music Player**: Built-in Web Audio synthesis tracks, custom MP3/FLAC import, scrub bar, and frequency spectrum visualizer.
     - 🖼️ **Photos**: Media gallery, category albums, photo zoom/rotate, and 1-click desktop wallpaper integration.
     - 📝 **Notes**: Categorized note taking (Personal, Work, Ideas), live note search, pin/unpin, and persistence.
     - 📸 **Photo Booth / Camera**: Real WebRTC video feed, live filters (Sepia, Noir, Cyberpunk), 3s self-timer, and snapshots.
     - ⏰ **Clock**: World clock across international time zones, alarms with synthesized audio chimes, precision stopwatch with lap history, and radial countdown timer.
     - ⛅ **Weather**: Dynamic atmospheric conditions, 24-hour hourly forecast cards, 7-day extended forecasts, and global city search.
     - 🖥️ **Terminal**: Real Linux shell command execution with bash integration.
     - 📁 **Finder / Files**: Directory navigation, file/folder creation, deletion, and file metadata viewer.
     - 📊 **Activity Monitor**: Real-time CPU, RAM, and storage graphs with live process management.
     - 🧮 **Calculator**: Standard and scientific calculations with keyboard shortcuts.
     - 📝 **Text Editor**: Multi-tab text and code editor with disk file saving.
     - ⚙️ **System Settings**: Themes, appearance, display resolution, network, audio, and automated updates.
     - 💿 **Install AxisOS**: Live-to-disk installation wizard with real GPT partitioning and Btrfs subvolumes.
     - ℹ️ **About This AxisPC**: Real hardware overview, processor, memory, and kernel details.

---

## 🏗️ Architecture

```mermaid
flowchart TD
    subgraph Hardware & Boot
        HW[Hardware / CPU / GPU / Storage] --> GRUB[GRUB 2 EFI / BIOS Bootloader]
        GRUB --> Kernel[Linux Kernel 6.12]
        Kernel --> Live[Debian Live Boot & Initramfs]
    end

    subgraph Linux Core Userspace
        Live --> Systemd[systemd Init System]
        Systemd --> Udev[systemd-udevd]
        Systemd --> PipeWire[PipeWire / WirePlumber Audio]
        Systemd --> NM[NetworkManager]
        Systemd --> Daemon[axisos-daemon.service (Port 3000)]
    end

    subgraph Wayland Display Layer
        Systemd --> Cage[Cage Wayland Compositor]
        Cage --> Kiosk[axisos-kiosk.sh]
    end

    subgraph AxisOS Desktop Shell
        Kiosk --> Shell[AxisOS Desktop Shell]
        Shell --> Installer[Real OS Installer Engine]
        Shell --> Apps[Terminal, Files, Editor, Monitor]
        Installer --> TargetDisk[Partition GPT + Btrfs + Install GRUB to SSD]
    end
```

---

## 🚀 Running AxisOS Locally

### 1. Build Desktop Shell
```bash
cd shell
npm install
npm run build
```

### 2. Run Production System Daemon
```bash
cd shell
npm run serve
```
Open `http://localhost:3000` to interact with the full live operating system interface.

### 3. Native Electron Window
```bash
cd shell
npm start
```

---

## 💿 Building the Bootable Linux ISO

AxisOS builds a bootable hybrid ISO (`axisos-live-amd64.iso`) compatible with both UEFI and Legacy BIOS machines:

### On Linux / Docker / Podman:
```bash
# Build the bootable hybrid ISO
./os-build/scripts/build-iso.sh
```

### On Windows / PowerShell:
```powershell
.\os-build\scripts\build-iso.ps1
```

---

## 🖥️ Testing in QEMU Virtual Machine

Test the full live boot and installer onto a virtual hard drive:
```bash
./os-build/scripts/run-qemu.sh
```
This automatically allocates a 20GB virtual hard disk (`axisos-disk.qcow2`), boots the ISO, allows you to run the real installer to partition and install the OS, and then boots directly from the installed disk!

---

## 📄 License
GPL-3.0 License.
