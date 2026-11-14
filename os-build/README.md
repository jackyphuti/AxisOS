<div style="font-family: 'IBM Plex Sans', 'IBM Plex Mono', 'Liberation Mono', Menlo, Consolas, monospace; line-height: 1.6;">

# AxisOS Linux System Architecture & Build System

AxisOS is a modern, responsive Linux-based operating system built strictly in 100% Pure C (-std=gnu11 / -std=c17), featuring a custom glassmorphic shell built with GTK 3, Cairo, and gtk-layer-shell, running directly on top of the Linux kernel and a native wlroots Wayland compositor.

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
        Compositor --> Shell[LiquidNodes Desktop (Pure C / GTK 3 / Layer-Shell)]
        Shell --> Desktop[AxisOS Desktop & Window Management]
        Shell --> Installer[AxisOS System Installer]
        Shell --> Apps[Settings, Terminal, Task Manager, OOBE]
    end
```

---

## Directory Structure

- `shell-c/`: The AxisOS custom Desktop Shell (Pure C11 / GTK 3 / Layer-Shell), Window Manager, and components.
- `compositor/`: Native C wlroots compositor (`axis-compositor`) with dynamic protocol scanner.
- `os-build/configs/`: System configurations (Wayland session, systemd units, Polkit rules, GRUB/ISOLINUX).
- `os-build/apps/`: Native Pure C11 desktop applications (installer, terminal, settings, task manager, software manager, etc.).
- `os-build/scripts/`: Build automation and system utility programs compiled from pure C.
- `pkg-mgr/`: Pure C package manager CLI (`axis`) and repository builder (`axis_repo_builder`).
- `os-build/updater/`: Pure C atomic system updater (`axis-update`).

---

## Build Automation

To compile all native applications and shell components:

```bash
./os-build/scripts/compile-native-apps.sh
```

To assemble a bootable hybrid ISO image:

```bash
./os-build/scripts/build-iso.sh
```

---

</div>
