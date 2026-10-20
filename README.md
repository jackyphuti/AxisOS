<div align="center">

  <img src="data/branding/axisos-logo.svg" alt="AxisOS" width="420" />

  <p><b>A modern, high-performance Linux operating system powered by Debian 12 (Bookworm), native Wayland, and a bespoke Qt 6 LiquidNodes glassmorphic desktop environment.</b></p>

  <p>
    <a href="https://github.com/jackyphuti/AxisOS/releases"><img src="https://img.shields.io/github/v/release/jackyphuti/AxisOS?style=flat-square&color=3A9CC9" alt="Release"></a>
    <img src="https://img.shields.io/badge/Base-Debian%2012%20(Bookworm)-D70A53?style=flat-square" alt="Debian 12">
    <img src="https://img.shields.io/badge/Architecture-x86__64-informational?style=flat-square" alt="Arch">
    <img src="https://img.shields.io/badge/Compositor-LiquidNodes%20Wayland-00F2FE?style=flat-square" alt="LiquidNodes Wayland">
    <img src="https://img.shields.io/badge/UI%20Stack-Qt%206%20%2F%20C%2B%2B%20%2F%20QML-8B5CF6?style=flat-square" alt="Qt 6 QML">
    <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL--3.0-green?style=flat-square" alt="License"></a>
  </p>

  <p>
    <a href="./INSTALLATION_GUIDE.md"><b>📖 Installation Guide</b></a> •
    <a href="./INSTALLER_TROUBLESHOOTING.md"><b>🔧 Installer Troubleshooting</b></a> •
    <a href="./DEVELOPMENT.md"><b>🛠️ Developer Guide</b></a> •
    <a href="https://github.com/jackyphuti/AxisOS/releases"><b>⬇️ Download ISO</b></a> •
    <a href="https://github.com/jackyphuti/AxisOS/issues"><b>🐛 Report Issue</b></a>
  </p>

</div>

---

## ⚡ About AxisOS

**AxisOS** is an independent Linux distribution engineered for speed, low latency, and aesthetic precision. It completely eliminates third-party desktop shells (GNOME/KDE/XFCE) in favor of **LiquidNodesOS** — an in-house native Wayland compositor and glassmorphic graphical environment crafted in C++20 and Qt 6.

Powered by the Debian 12 Bookworm core and the XanMod performance kernel, AxisOS brings an organic glassmorphic interface, spring physics, per-app Btrfs rewind checkpoints, and focus-driven kernel boosting.

---

## 🖥️ System Specifications

| Component | Specification |
| :--- | :--- |
| **Base System** | Debian 12 (Bookworm) `amd64` |
| **Linux Kernel** | XanMod Realtime / Performance Linux Kernel (`linux-image-xanmod-x64v3`) |
| **Display Server & Compositor** | Native Qt 6 Wayland Compositor (`LiquidNodesOS`) with XDG-Shell & hardware acceleration |
| **Desktop Shell** | Glassmorphic LiquidNodes UI (Unified Top Bar, Floating Prism Dock, App Launchpad, Biometric PAM Lock Screen) |
| **Audio Server** | PipeWire + WirePlumber pro-audio stack |
| **Graphics & Compute** | Mesa RADV (AMD ACO), Intel Arc ANV, NVIDIA PRIME offload, Vulkan, VA-API hardware decoding |
| **Default Shell** | GNU Bash with modern interactive completions |
| **File Systems** | Btrfs (with zstd compression & automated subvolumes) and EXT4 |
| **Boot Mode** | UEFI (`x86_64-efi`) with fallback to Legacy BIOS (`isolinux`) |

---

## 🚀 Native C++ / Qt 6 Application Suite

AxisOS features zero Electron or web-wrapper overhead. All core utilities run natively on top of the Wayland compositor:

- **LiquidNodes Compositor (`LiquidNodesOS`)**: Bespoke Wayland display server featuring organic jelly window physics, live Bezier window piping, and integrated biometric login.
- **Settings (`axis-settings`)**: Dedicated AxisOS Settings control center featuring system hardware specs, OTA software updates, and the OS Edition switcher (Home & Gaming, Enterprise, Work & School).
- **GameBoost (`axis-game-boost`)**: Dedicated hardware acceleration daemon and CLI utility optimizing CPU governors, GPU clocks, memory maps, and `gamemoded`.
- **Terminal (`axis-terminal`)**: Ultra-low-latency PTY terminal emulator powered by `libvterm` and OpenGL rendering.
- **Installer (`axis-installer`)**: High-reliability graphical system installer supporting automated Btrfs/EXT4 disk partitioning, user creation, dynamic device node polling, and GRUB EFI setup.
- **Software Center (`axis-software-manager`)**: Native package manager front-end for APT and Flatpak.
- **Welcome Center (`axis-welcome`)**: Out-of-the-box system setup, driver verification, and onboarding.
- **HUD Task Manager (`axis-task-manager`)**: Process inspector and hardware resource monitor querying Linux `/proc` and `sysfs`.

---

## 🔗 Quick Links & Documentation

- **[Installation Guide](INSTALLATION_GUIDE.md)** — Step-by-step flashing, BIOS/UEFI setup, and drive partitioning guide.
- **[Installer Troubleshooting Guide](INSTALLER_TROUBLESHOOTING.md)** — Live session credentials (`axis` / `axis`), partition wiping (`wipefs`/`sgdisk`), and installer error diagnosis.
- **[Developer & Build Guide](DEVELOPMENT.md)** — Compiling native C++ applications, chroot setup, and generating bootable ISOs.
- **[Releases & ISO Downloads](https://github.com/jackyphuti/AxisOS/releases)** — Download the latest bootable live ISO (`axisos-v2.1-live-amd64.iso`).
- **[Issue Tracker](https://github.com/jackyphuti/AxisOS/issues)** — Bug reports, feature requests, and system discussions.

---

## 📄 License

AxisOS is free and open-source software licensed under the **GNU General Public License v3.0 (GPL-3.0)**. See the [LICENSE](LICENSE) file for details.
