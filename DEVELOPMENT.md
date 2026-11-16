# AxisOS Development & Build Guide

Welcome to the development guide for **AxisOS**. This guide covers setting up your local environment, building the native Wayland compositor and pure C desktop shell, compiling native applications, and creating bootable ISOs.

---

## 1. Prerequisites & Dependencies

Supported host environments:
- **Debian 12+** / **Ubuntu 22.04+**
- **Windows 10 / 11** via **WSL 2** (Debian or Ubuntu recommended)

Install the required build packages:

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
    build-essential \
    gcc \
    pkg-config \
    libgtk-3-dev \
    libgtk-layer-shell-dev \
    libvterm-dev \
    libarchive-dev \
    libjson-c-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libpam0g-dev \
    libudev-dev \
    libwayland-dev \
    wayland-protocols \
    libinput-dev \
    libxkbcommon-dev \
    libwlroots-dev \
    libdrm-dev \
    xorriso \
    qemu-system-x86
```

---

## 2. Compiling the Suite

### Compile All Applications, Daemons & Shell:
From the repository root:

```bash
bash os-build/scripts/compile-native-apps.sh
```

This compiles:
* `shell-c/` -> `axisos-shell` (symlinked as `axis-shell` and `LiquidNodesOS`)
* `compositor/` -> `axis-compositor` (if `libwlroots-dev` is installed)
* All native applications in `os-build/apps/` (`axis-terminal`, `axis-settings`, `axis-task-manager`, `axis-installer`, etc.)
* All helper daemons in `os-build/scripts/`
* `pkg-mgr/` -> `axis` package manager CLI
* `os-build/updater/` -> `axis-update` atomic updater

### Verify Strict Pure C Linkage:
```bash
bash scripts/verify-pure-c.sh
```

---

## 3. Developing Individual Components

### Wayland Compositor (`compositor/`):
```bash
cd compositor
make clean
make
```

### Desktop Shell (`shell-c/`):
```bash
cd shell-c
make clean
make
```

### Running Test Suites:
```bash
gcc -std=gnu11 -O2 tests/test_hardware_probe.c -o tests/test_hardware_probe && ./tests/test_hardware_probe
gcc -std=gnu11 -O2 tests/verify_harmony_ui.c -o tests/verify_harmony_ui && ./tests/verify_harmony_ui
```

---

## 4. Building the Bootable Live ISO

To assemble a bootable Debian 12 hybrid UEFI/BIOS ISO image:

```bash
sudo bash os-build/scripts/build-iso.sh
```

The output ISO will be saved as `axisos-live-amd64.iso` in the repository root.
