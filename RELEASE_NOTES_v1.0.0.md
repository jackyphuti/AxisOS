# AxisOS v1.0.0 — Official First Release (KDE Plasma & Pure C Edition)

Welcome to the official **v1.0.0 First Release** of **AxisOS Linux**, an ultra-high performance operating system engineered for PC gaming, software development, and everyday desktop computing.

### 🌟 Highlights & Features

- **Desktop Experience**:
  - Powered by **KDE Plasma Wayland** with SDDM display manager and auto-login for live sessions.
  - Bespoke **One UI Dark** theme design tokens (`#141826` base, `#1F2231` cards, `#7D95FF` accent).
  - AxisOS system branding across Kickoff application launcher, desktop, and Papirus-Dark icons.
  - High-resolution wallpaper: *08-axisos-obsidian-wave*.

- **System Installation**:
  - Branded **Calamares** live installer with automated disk partitioning, LUKS encryption, Btrfs subvolume layouts, and UEFI/BIOS support.
  - Fully integrated `squashfs-tools` / `unsquashfs` live unpack engine.

- **Windows 11 Redesigned Task Manager (`axis-task-manager`)**:
  - 100% Pure C11 GTK 3 & Cairo architecture.
  - Live search bar with instant process filtering.
  - Command bar with *Run new task* (with administrative elevation support), *End task*, and *Efficiency mode (🍃)*.
  - Collapsible Mica-style navigation rail (54px compact icon rail <-> 200px labeled rail).
  - Real-time heat-mapped resource columns (CPU, Memory, Disk, Network).
  - 60-Second Real-Time Rolling Cairo Performance graphs with hardware telemetry (CPU, RAM, NVMe/SATA, Ethernet/Wi-Fi).
  - Dedicated views for Startup apps, Services, Users, App history, Details, and Settings.

- **Hardware & Gaming Readiness**:
  - **Axis Hardware & Codec Suite** (`nobara-welcome` / `axis-hardware-manager`) with One UI Dark styling, automatic GPU detection (NVIDIA, AMD, Intel), Vulkan runtime verification, and multimedia codecs.
  - System performance optimizations for low-latency kernel scheduling, elevated open-file limits, ZRAM zstd compressed memory swap, and sysctl gaming profiles.

### 💿 Verification

- **ISO Checksum (SHA-256)**:
  `cf79151c5082c2cabc15a9f71ee4972a1a677e64ffc00bbbdbfe2b9f425d27ea  axisos-live-amd64.iso`
