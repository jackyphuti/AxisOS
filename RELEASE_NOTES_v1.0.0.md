# AxisOS v1.0.0 — Official First Release (AxisOS Obsidian KDE Plasma & Pure C Edition)

Welcome to the official **v1.0.0 First Release** of **AxisOS Linux**, an ultra-high performance operating system engineered for PC gaming, software development, and everyday desktop computing.

### 🌟 Highlights & Features

- **Signature AxisOS Obsidian Desktop Experience (KDE Plasma 5.27)**:
  - Custom **AxisOS Obsidian** Global Theme (`org.axisos.obsidian.desktop`) & frosted glass Plasma Desktop Theme (`AxisOS-Obsidian`) with translucent deep obsidian surfaces (`#0B0F1A`) and glowing electric cyan (`#38BDF8`) 1px glass rims.
  - **Dual-Zone Desktop Layout**:
    - **Floating Top Command HUD Bar**: AxisOS launcher, global app menu, workspace switcher pills, centered date & time pill, live CPU & RAM telemetry gauges, media controls, system tray, quick search, and power controls.
    - **Centered Floating Glass Capsule Dock**: Auto-tucks cleanly behind maximized windows (`windowscover`) with pinned AxisOS applications and Trash.
  - **KWin & Breeze Polish**: Frosted blur (`BlurStrength=12`), Magic Lamp & Scale animations, Top-Left Hot Corner Overview, centered borderless-separator titlebars with deep obsidian ambient shadows, floating centered Spotlight KRunner (`Alt+Space`), instant splash-free login, and `Ctrl+Shift+Esc` bound directly to the Windows 11-style Task Manager.
  - High-resolution wallpaper: *08-axisos-obsidian-wave*.

- **100% Offline System Installation (Calamares)**:
  - Branded **Calamares** live installer with full offline support for **both UEFI (with EFI fallback) and Legacy BIOS**, LUKS full-disk encryption (`cryptsetup-initramfs` + `keyutils`), Btrfs/ext4 partitioning, and dual-boot `os-prober`.
  - Pre-installed `squashfs-tools` (`unsquashfs`), `grub2-common` (`grub-install`, `update-grub`), `grub-efi-amd64-bin`, and `grub-pc-bin` for reliable offline installation without requiring external package downloads.
  - Automated post-install cleanup removing live autologin overrides, live-only packages, and installer shortcuts.

- **Windows 11 Redesigned Task Manager (`axis-task-manager`)**:
  - 100% Pure C11 GTK 3 & Cairo architecture (`Ctrl+Shift+Esc`).
  - Live search bar, elevated *Run new task* dialog, *End task*, *Efficiency mode*, collapsible Mica navigation rail, heat-mapped resource columns, and 60-second real-time rolling Cairo performance graphs.

- **Hardware & Gaming Readiness**:
  - **Axis Hardware & Codec Suite** (`nobara-welcome` / `axis-hardware-manager`) with One UI Dark styling, automatic GPU detection (NVIDIA, AMD, Intel), Vulkan runtime verification, and multimedia codecs.
  - Gaming & developer optimizations: low-latency KWin compositing, GameMode, MangoHud, Gamescope, Wine/Lutris, ZRAM, and GCC/CMake/Ninja/GDB/Git toolchain.

### 💿 Verification

- **ISO Checksum (SHA-256)**:
  `928619721d93488c9dcdd422f43979242517602264351c4d5cee4a29b266a786  axisos-live-amd64.iso`
