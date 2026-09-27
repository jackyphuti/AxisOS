# AxisOS — Next-Generation Linux Operating System

<div align="center">
  <h3>▲ AxisOS 1.0 "Horizon"</h3>
  <p>A modern, fluid, glassmorphic Linux operating system combining the power of the Linux kernel with a customizable Wayland desktop skin.</p>
</div>

---

## 🌟 Overview

**AxisOS** is designed from the ground up to provide a fresh, futuristic user experience on top of a rock-solid Linux base. Instead of traditional desktop bloat, AxisOS pairs the **Linux Kernel** with a lightweight Wayland compositor (**Cage**) and a hardware-accelerated **Modern Desktop Shell**.

### 🧩 Core Components Included in this Release:

1. **AxisOS System Installer Wizard**:
   - Welcome & system feature highlights.
   - Language and keyboard layout selector.
   - Storage drive selection with automatic GPT, ESP, and Btrfs partitioning.
   - User account and hostname configuration.
   - Live installation simulation with real-time kernel & package deployment progress.
   - Completion screen with "Restart Now" or "Continue Testing".

2. **AxisOS Desktop Shell ("Skin")**:
   - **Interactive Window Manager**: Draggable, resizable, minimizable, and maximizable windows with z-index stacking.
   - **Floating Glass Dock**: Application launcher, running app indicators, and quick-access shortcuts.
   - **Top Status Bar**: Live clock, battery meter, volume, Wi-Fi status, and active window pill.
   - **Quick Settings Control Center**: Wi-Fi toggle, Bluetooth, Dark/Light mode switch, Night Light slider, Volume, and Brightness controls.
   - **Application Drawer / Start Menu**: Search bar, categorized app view (System, Utilities, Accessories), and power options.
   - **Power Modal**: Quick access to Power Off, Restart, and Lock.

3. **Built-in System Applications**:
   - **Settings App**: Change wallpapers dynamically, customize accent colors (Cyan, Blue, Purple, Emerald, Amber, Rose), adjust display resolutions, manage audio via PipeWire, and view system specifications.
   - **Terminal Emulator**: Interactive command shell with `axis-fetch` (ASCII art banner), `uname -a`, `cat /etc/os-release`, `free -h`, `ls`, `help`, and command history navigation.
   - **Files App**: Directory navigation (Home, Documents, Downloads, Pictures, Root `/`), folder drill-down, and storage capacity indicator.
   - **System Monitor**: Real-time animated CPU usage sparkline graph, RAM gauge, and running process table.
   - **Text Editor**: Notepad with syntax indicator, character/line counters, and file save state.
   - **About AxisOS**: Kernel version, architecture, and update checks.

---

## 🏗️ Architecture

```mermaid
flowchart TD
    subgraph Hardware & Kernel
        HW[Hardware / CPU / GPU / Storage] --> GRUB[GRUB 2 EFI Bootloader]
        GRUB --> Kernel[Linux Kernel 6.12]
        Kernel --> Rootfs[Btrfs / SquashFS Root Filesystem]
    end

    subgraph Linux Core Userspace
        Rootfs --> Systemd[systemd Init & Services]
        Systemd --> Udev[systemd-udevd]
        Systemd --> PipeWire[PipeWire Audio]
        Systemd --> NM[NetworkManager]
    end

    subgraph Wayland Display Layer
        Systemd --> Cage[Wayland Compositor (Cage / Weston)]
    end

    subgraph AxisOS Shell
        Cage --> Shell[AxisOS Kiosk Runtime]
        Shell --> Desktop[Desktop & Window Manager]
        Desktop --> Installer[AxisOS Installer]
        Desktop --> Apps[Settings, Terminal, Files, Monitor]
    end
```

---

## 🚀 Running AxisOS Shell

### Quick Start (Dev Server / Browser)
To run and test the UI in your browser:
```bash
cd shell
npm run dev
```
Open `http://localhost:3000` to interact with the full desktop and installer.

### Native Desktop Window (Electron)
To launch AxisOS as a standalone native desktop window right on your screen:
```bash
cd shell
npm start
```

---

## 💿 Building the Bootable Linux ISO

AxisOS includes an automated build pipeline based on Debian Live:

```bash
# 1. Build the production shell bundle
./os-build/scripts/build-shell.sh

# 2. Build the bootable hybrid ISO
./os-build/scripts/build-iso.sh

# 3. Test in QEMU virtual machine
./os-build/scripts/run-qemu.sh
```

---

## 🛠️ Technology Stack

- **Kernel**: Linux Kernel 6.x (x86_64)
- **Base Distro**: Debian Stable Core / Btrfs subvolumes
- **Display Server**: Native Wayland (Cage compositor)
- **Audio Server**: PipeWire / WirePlumber
- **Desktop Shell**: React 19, TypeScript, Tailwind CSS, Lucide Icons, Vite, Electron

---

## 📄 License
GPL-3.0 License.
