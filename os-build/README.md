# AxisOS System Architecture & Build System

AxisOS is a lightweight Linux operating system built in pure C11 (`-std=gnu11`) on top of Debian 12 (Bookworm) and a native `wlroots` Wayland compositor.

---

## Directory Overview

* `apps/`: Native pure C applications (`axis-terminal`, `axis-settings`, `axis-task-manager`, `axis-installer`, etc.).
* `scripts/`: System daemons, helper utilities, and build automation scripts compiled from pure C.
* `configs/`: Session launchers (`axisos-kiosk.sh`), systemd services, PAM rules, and package manifests.
* `updater/`: Pure C atomic system updater (`axis-update`).

---

## Build Commands

### 1. Compile All Native Apps & Daemons:
```bash
bash scripts/compile-native-apps.sh
```

### 2. Build Bootable Hybrid ISO:
```bash
sudo bash scripts/build-iso.sh
```

The resulting hybrid image can be flashed to USB or tested in QEMU/VMware/VirtualBox.
