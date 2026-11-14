<div style="font-family: 'IBM Plex Sans', 'IBM Plex Mono', 'Liberation Mono', Menlo, Consolas, monospace; line-height: 1.6;">

# AxisOS Development & Build Guide

Welcome to the development guide for **AxisOS**. This document provides detailed instructions on setting up your local environment, developing the native Wayland compositor and Pure C desktop shell, compiling native C11 GTK 3 applications, and generating bootable hybrid UEFI/BIOS ISOs.

---

## Table of Contents
1. [Prerequisites & System Requirements](#prerequisites--system-requirements)
2. [Setting Up the Development Environment](#setting-up-the-development-environment)
3. [Building the Native Wayland Compositor](#building-the-native-wayland-compositor)
4. [Developing the Pure C Desktop Shell](#developing-the-pure-c-desktop-shell)
5. [Compiling Native C Applications](#compiling-native-c-applications)
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
| **GCC** | `>= 12.x` | Compiling native pure C compositor, shell, and applications |
| **Make** | `>= 4.x` | Native build scripts |
| **wayland-scanner** | `>= 1.20` | Dynamic Wayland protocol generation |
| **wlroots** | `>= 0.16` | Wayland display server library |
| **GTK 3 & Layer-Shell** | `>= 3.24` | Pure C glassmorphic desktop shell runtime |
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
   sudo apt-get update && sudo apt-get install -y \
       build-essential \
       gcc \
       pkg-config \
       libgtk-3-dev \
       libgtk-layer-shell-dev \
       libvterm-dev \
       libudev-dev \
       libpam0g-dev \
       libwlroots-dev \
       libwayland-dev \
       wayland-protocols \
       libinput-dev \
       libxkbcommon-dev \
       libjson-c-dev \
       libcurl4-openssl-dev \
       libarchive-dev \
       libssl-dev \
       xorriso \
       qemu-system-x86
   ```

---

## Building the Native Wayland Compositor

Compile the wlroots-based Wayland compositor:

```bash
cd compositor
make clean
make
```

The resulting binary `axis-compositor` is installed to `/usr/local/bin/axis-compositor`.

---

## Developing the Pure C Desktop Shell

The desktop shell is located in `shell-c/` and built in pure C11 using GTK 3, Cairo, and `gtk-layer-shell-0`:

```bash
cd shell-c
make clean
make
```

The resulting binary `axisos-shell` (symlinked as `LiquidNodesOS`) is installed to `/usr/local/bin/axis-shell`.

---

## Compiling Native C Applications

To compile all 12 native applications, package manager, and shell components:

```bash
./os-build/scripts/compile-native-apps.sh
```

---

## Testing in QEMU Virtual Machine

Test the system in QEMU:

```bash
./os-build/scripts/run-qemu.c
```

---

## Building the Bootable Linux ISO

Generate the bootable ISO:

```bash
./os-build/scripts/build-iso.sh
```

---

</div>
