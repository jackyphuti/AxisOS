# AxisOS v1.0.0 — Official First Release (AxisOS Obsidian KDE Plasma & Pure C Edition)

Welcome to the official **v1.0.0 First Release** of **AxisOS Linux**, an ultra-high performance operating system engineered for PC gaming, software development, and everyday desktop computing — install once and stay up to date automatically.

### 🌟 Highlights & Features

- **Signature AxisOS Obsidian Desktop Experience (KDE Plasma 5.27)**:
  - Custom **AxisOS Obsidian** Global Theme (`org.axisos.obsidian.desktop`) & frosted glass Plasma Desktop Theme (`AxisOS-Obsidian`) with deep obsidian surfaces (`#0B0F1A`) and electric cyan (`#38BDF8`) glass rims.
  - **Dual-Zone Desktop Layout**: Floating Top Command HUD Bar (AxisOS launcher, global app menu, workspace pills, centered date/time pill, live CPU & RAM gauges, media controls, tray, search, power) + Centered Floating Glass Capsule Dock (auto-tucks cleanly behind maximized windows).
  - Clean SDDM login defaulted directly to **Plasma (X11)** with zero conflicting sessions, frosted blur, Magic Lamp & Scale animations, Top-Left Hot Corner Overview, floating Spotlight KRunner (`Alt+Space`), instant splash-free login, and `Ctrl+Shift+Esc` bound to Task Manager.

- **100% Offline Calamares System Installer**:
  - Full offline support for **UEFI (with EFI fallback) and Legacy BIOS**, LUKS full-disk encryption (`cryptsetup-initramfs` + `keyutils`), Btrfs/ext4 partitioning, dual-boot `os-prober`, and `squashfs-tools` (`unsquashfs`).
  - Automated post-install cleanup removing live autologin overrides, live-only packages, and installer shortcuts.

- **Install-Once Bi-Monthly OTA Update System (`axisos-update`)**:
  - Automatic systemd timer (`axisos-ota.timer`) updates AxisOS themes, Pure C applications, gaming tuning, and Debian security patches every 2 months without ever needing to rebuild or reflash an ISO.
  - Trigger updates manually anytime via `sudo axisos-update`.

- **Universal Package Installer & App Store (Flatpak / Flathub + APT)**:
  - Built-in **Flathub (Flatpak)** & **Debian APT** support out of the box.
  - 40+ curated apps (Firefox, Brave, Chrome, Discord, Telegram, WhatsApp, Zoom, Steam, Heroic, Lutris, Bottles, ProtonUp-Qt, RetroArch, VS Code, OBS Studio, VLC, Spotify, GIMP, Blender, LibreOffice, ONLYOFFICE, Bitwarden) plus **live search across all of Flathub & APT**, one-click **Install / Launch / Remove**, and local `.deb` / `.flatpakref` / `.flatpak` file installation.

- **Windows 11 Redesigned Task Manager (`axis-task-manager`)**:
  - 100% Pure C11 GTK 3 & Cairo with authentic Windows 11 Dark Fluent styling (verified zero GTK 3 CSS errors), **3D-shaded realistic Cairo vector icons**, real-time process filtering, heat-mapped CPU/Memory cells, 60-second rolling performance graphs, and live update-speed / Always-on-Top settings.

- **AxisOS Terminal & System Fetch (`axis-terminal` & `axis-fetch`)**:
  - Automatically greets every new terminal session with the glowing **AxisOS Delta Triangle Logo** and live hardware/OS telemetry (OS, Host, Kernel, Uptime, Packages, Desktop, Session, CPU, GPU, Memory, Disk, Local IP).

### 💿 Verification

- **ISO Size**: 1.9 GB (`2,003,435,520` bytes)
- **ISO Checksum (SHA-256)**:
  `2f8df3be03bd7fc145f0e38af12b1fa2dd084ba0393117e91c71320698b290ec  axisos-live-amd64.iso`
