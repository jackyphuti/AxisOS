# AxisOS 2.0 (Beta Preview — Live USB & Performance Edition)

Welcome to **AxisOS 2.0 Beta**! This release introduces major performance, stability, and system management upgrades engineered for gamers, developers, and power users. AxisOS combines the stability of Debian 12 (Bookworm) with high-performance kernel tuning, an obsidian glassmorphic desktop environment, native C++ / Qt 6 applications, and the all-new **GameBoost Extreme Performance Engine**.

---

## ⚡ Key Highlights & What's New in AxisOS 2.0 Beta

### 1. 🚀 GameBoost Extreme Performance & Hardware Overclocking Engine
- **One-Click Performance Mode in Settings**: Switch between **Balanced Standard Mode** and **Extreme Overclock Mode** via the new *⚡ Gaming Boost & Performance* panel integrated into Settings (`gnome-control-center` / `axisos-settings-panel`).
- **CPU Overclocking & Unconstrained Governors**: Pins CPU scaling governor to `performance`, sets Energy Performance Preference (EPP) to maximum throughput, unthrottles Intel P-state and AMD Core Performance Boost, and fixes minimum core clocks to maximum turbo frequencies.
- **GPU Overclocking & Power Unleashed**:
  - **NVIDIA**: Sets PowerMizer to Maximum Performance (Level 1) and raises board power limits to maximum factory wattage via `nvidia-smi`.
  - **AMD Radeon**: Engages high performance DPM force performance level and profile mode 1 (3D Full Speed) in `sysfs`.
  - **Intel Arc / Iris Xe**: Elevates GT boost frequencies to peak allowable MHz.
- **Kernel & Latency Optimization**: Sets `vm.split_lock_mitigate=0` (eliminates stutter in Windows games under Proton/Wine), tunes `vm.swappiness=10`, raises `vm.max_map_count=2147483642` for heavy game memory maps, and integrates Feral Interactive `gamemoded`.
- **CLI & Service Integration**: Managed by `/usr/local/bin/axis-game-boost` and the `axis-game-boost.service` systemd daemon.

### 2. 🛡️ Hardened Disk Partitioning & Real Device Node Detection
- **Eliminated Partition Missing Errors**: Completely fixes previous `/dev/sda3 or /dev/sda1 not found` installer errors.
- **Atomic Disk Partitioning**: Replaced error-prone manual name concatenation with atomic `sfdisk` GPT partitioning, backed by `sgdisk` and `parted` safety fallbacks.
- **Pre-Wipe Drive Sanitation**: Safely unmounts active mounts with `findmnt` and `lsblk`, executes verified `wipefs -a -f`, and performs GPT zap with `sgdisk --zap-all`.
- **Dynamic Partition Node Polling**: Implements a high-precision 30-second polling loop reading real partition nodes via `lsblk -rno NAME,TYPE` with automatic self-healing retries using `partx -a` and `blockdev --rereadpt`.
- **Universal Storage Device Support**: Full support for SATA/SCSI (`/dev/sda`), NVMe (`/dev/nvme0n1pX`), and eMMC (`/dev/mmcblk0pX`).

### 3. 🧹 Complete Post-Installation Installer Cleanup
- **Automatic Installer Removal on Target Drive**: After installation completes, the installer is permanently hidden from the installed OS.
- **Desktop & Dock Cleanup**: Desktop shortcuts and dock pinned launchers (`axis-installer.desktop`) are purged from `/etc/skel`, user desktops, and `/etc/dconf/db/local.d/00-axisos-gnome`.
- **Installed State Marker (`/etc/axisos-installed`)**: Signals system applications that the OS is permanently installed.
- **Welcome Center Adaptive UI**: In live mode, the Welcome Center offers installation options; on an installed system, it automatically transforms into **✓ System Status**, displaying uptime, memory utilization, and hardware specs.

### 4. 🎨 Compact & Refined Settings Branding
- **OS Logo Resizing**: The operating system logo in Settings has been redesigned from an oversized 120x80 banner into a sleek, crisp 48x48 icon badge placed beside the system title and edition tag.
- **Theme Consistency**: Obsidian black (`#0A0B0D`), titanium slate (`#14161A`, `#202329`), and crisp white typography with zero visual clipping across all display resolutions.

### 5. 🌐 Captive Portal & Web-Based Wi-Fi Login Support
- **Pre-Installed Web Login Compatibility**: Built-in support for public, hotel, and school Wi-Fi networks requiring captive portal login authentication.
- **Automatic Portal Detection**: NetworkManager captive portal detection automatically opens the authentication window when connected to restricted networks.

### 6. 🌍 Expanded Internationalization & Localization
- **Installer Regional Selection**: Full selection for system display languages, locations/timezones, and international keyboard layouts (US, UK, European, Asian, and Latin layouts) with instant live test input.

---

## 🖥️ System Requirements

| Component | Minimum Requirements | Recommended Specifications |
| :--- | :--- | :--- |
| **Processor** | 64-bit x86_64 Dual-Core CPU | 64-bit Quad-Core CPU or higher (Intel Core i3/i5/i7/i9 or AMD Ryzen) |
| **RAM (Memory)** | 2 GB RAM (4 GB for Live Desktop) | 8 GB to 16 GB+ RAM |
| **Storage** | 25 GB free disk space | 64 GB+ NVMe SSD or high-speed SATA SSD |
| **Graphics** | UEFI Framebuffer / Intel HD Graphics 4000+ | Intel UHD/Iris Xe/Arc, AMD Radeon RX, or NVIDIA GeForce |
| **Firmware** | Modern UEFI (Secure Boot Compatible) or Legacy BIOS | UEFI with Microsoft UEFI CA enabled |
| **USB Media** | 4 GB Flash Drive | 8 GB+ USB 3.0 / 3.1 Flash Drive |

---

## 📦 Release Asset Verification

| Asset | Size | SHA-256 Checksum |
| :--- | :--- | :--- |
| **`axisos-v2.0-live-amd64.iso`** | 1.90 GiB (2,043,674,624 bytes) | `04b5b9ad99d5ae63e7c4c07e9a560e77eba9fb8d26efbc4ed68db7886a47d297` |
| **`SHA256SUMS.txt`** | 99 bytes | *(Checksum validation manifest)* |

### Verifying Download Integrity:
- **Windows (PowerShell)**:
  ```powershell
  (Get-FileHash axisos-v2.0-live-amd64.iso -Algorithm SHA256).Hash
  ```
- **Linux / macOS (Terminal)**:
  ```bash
  sha256sum -c SHA256SUMS.txt
  ```

---

## 🚀 How to Try and Install AxisOS

1. **Download the ISO**: Download `axisos-v2.0-live-amd64.iso` and `SHA256SUMS.txt`.
2. **Flash to USB**:
   - **Windows**: Use [Rufus](https://rufus.ie/) (select GPT / UEFI, ISO or DD mode).
   - **macOS / Linux / Windows**: Use [balenaEtcher](https://etcher.balena.io/) or [Ventoy](https://www.ventoy.net/).
3. **Boot Your Computer**: Insert the USB flash drive and restart your PC while tapping your boot menu key (<kbd>F12</kbd>, <kbd>F11</kbd>, <kbd>F8</kbd>, <kbd>F9</kbd>, or <kbd>Esc</kbd>).
4. **Try Live Session**: Select **`1. Try AxisOS 2.0 (Live Desktop)`** in GRUB. The OS runs entirely in RAM, leaving internal drives untouched.
5. **Install AxisOS**: Launch **Install AxisOS** from the desktop or Welcome Center whenever you are ready to deploy to disk.
