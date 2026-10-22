# AxisOS Linux System Architecture & Build System

AxisOS is a modern, responsive Linux-based operating system featuring a custom glassmorphic shell built with Qt 6 and QML, running directly on top of the Linux kernel and a native wlroots Wayland compositor.

---

## System Architecture

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
        Seat --> Compositor[Wayland Compositor (axis-compositor)]
    end

    subgraph AxisOS Shell & User Experience
        Compositor --> Shell[LiquidNodes Desktop (Qt 6 / QML)]
        Shell --> Desktop[AxisOS Desktop & Window Management]
        Shell --> Installer[AxisOS System Installer]
        Shell --> Apps[Settings, Terminal, Task Manager, OOBE]
    end
```

---

## Directory Structure

- `compositor-qt/`: The AxisOS custom Desktop Shell (QML/Qt6), Window Manager, and components.
- `compositor/`: Native C wlroots compositor (`axis-compositor`) with dynamic protocol scanner.
- `os-build/configs/`: System configurations (Wayland session, systemd units, Polkit rules, GRUB/ISOLINUX).
- `os-build/apps/`: Native C++ desktop applications (installer, terminal, settings, task manager).
- `os-build/scripts/`: Build automation scripts:
  - `build-hybrid-uefi-iso.py`: Assembles a bootable dual-boot UEFI + BIOS hybrid live ISO.
  - `compile-native-apps.sh`: Compiles C/C++ native applications.
  - `run-qemu.sh`: Launches QEMU virtual machine for rapid OS testing.

---

## How to Test & Develop

### 1. Compile Native Applications
```bash
./os-build/scripts/compile-native-apps.sh
```

### 2. Test in QEMU VM
```bash
./os-build/scripts/run-qemu.sh
```

### 3. Build Full Bootable Live ISO
```bash
sudo python3 ./os-build/scripts/build-hybrid-uefi-iso.py
```
The resulting ISO can be written to a USB drive with `dd`, BalenaEtcher, Ventoy, or Rufus, or booted in VirtualBox/VMware/QEMU.
