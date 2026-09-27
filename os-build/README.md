# AxisOS Linux System Architecture & Build System

AxisOS is a modern, responsive Linux-based operating system featuring a custom glassmorphic shell built with modern web technologies, running directly on top of the Linux kernel and Wayland compositor.

---

## 🏗️ System Architecture

```mermaid
flowchart TD
    subgraph Hardware & Boot
        HW[Computer / Motherboard] --> UEFI[UEFI / BIOS Bootloader]
        UEFI --> GRUB[GRUB 2 Bootloader]
        GRUB --> Kernel[Linux Kernel 6.x]
        Kernel --> Initramfs[Initramfs / Live Boot]
    end

    subgraph Linux Base Userspace (Debian Core)
        Initramfs --> Systemd[systemd Init System]
        Systemd --> Udev[systemd-udevd (Hardware Devices)]
        Systemd --> Net[NetworkManager (Wi-Fi & Ethernet)]
        Systemd --> Audio[PipeWire / WirePlumber (Sound)]
        Systemd --> Session[axisos-session.service]
    end

    subgraph Display & Graphics Layer
        Session --> Seat[seatd / logind]
        Seat --> Compositor[Wayland Compositor (Cage / Weston)]
    end

    subgraph AxisOS Shell & User Experience
        Compositor --> Shell[AxisOS Modern Shell (Electron / Kiosk)]
        Shell --> Desktop[AxisOS Desktop & Window Manager]
        Shell --> Installer[AxisOS System Installer]
        Shell --> Apps[Settings, Terminal, File Manager, Editor]
    end
```

---

## 📂 Directory Structure

- `shell/`: The AxisOS custom Desktop Shell, Window Manager, and Installer.
- `os-build/configs/`: System configurations (Wayland kiosk, systemd units, live-build package lists).
- `os-build/scripts/`: Build automation scripts:
  - `build-shell.sh`: Compiles the UI shell into an optimized bundle.
  - `build-iso.sh`: Generates a bootable hybrid ISO (`axisos-amd64.iso`) using `live-build`.
  - `run-qemu.sh`: Launches QEMU virtual machine to test AxisOS without needing a physical USB/machine.

---

## 🚀 How to Test & Develop

### 1. Test Shell Locally (Instant Preview)
```bash
cd shell
npm run dev
```
Open the browser or launch the desktop preview window.

### 2. Test in QEMU VM
```bash
./os-build/scripts/run-qemu.sh
```

### 3. Build Full Bootable Live ISO
```bash
sudo ./os-build/scripts/build-iso.sh
```
The resulting ISO can be burned to a USB drive with `dd` or Rufus, or booted in VirtualBox/VMware/QEMU.
