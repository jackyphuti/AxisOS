# AxisOS Installation & Hardware Deployment Guide

Welcome to the installation guide for **AxisOS**.

AxisOS is a lightweight, pure C Linux desktop operating system powered by Debian 12 (Bookworm), a native `wlroots` Wayland compositor, and the LiquidNodes desktop shell.

---

## 1. System Requirements

| Component | Minimum | Recommended |
| :--- | :--- | :--- |
| **Processor** | 64-bit x86_64 Dual-Core CPU | 64-bit Quad-Core CPU or higher |
| **Memory (RAM)** | 2 GB (4 GB for Live Desktop) | 8 GB+ RAM |
| **Storage** | 20 GB free disk space | 40 GB+ SSD |
| **Graphics** | UEFI Framebuffer or Intel HD Graphics | Intel UHD/Iris, AMD Radeon, NVIDIA |
| **Boot Mode** | UEFI or Legacy BIOS | UEFI with Secure Boot |
| **USB Media** | 4 GB Flash Drive | 8 GB+ USB 3.0 Flash Drive |

---

## 2. Creating a Bootable USB Drive

Download `axisos-live-amd64.iso` from the [GitHub Releases](https://github.com/jackyphuti/AxisOS/releases) page.

### Method A: Using BalenaEtcher (Windows, macOS, Linux)
1. Download and run [BalenaEtcher](https://etcher.balena.io/).
2. Click **Flash from file** and select the AxisOS ISO.
3. Select your target USB drive.
4. Click **Flash!**.

### Method B: Using Rufus (Windows)
1. Download and run [Rufus](https://rufus.ie/).
2. Select your USB drive under **Device**.
3. Click **SELECT** and choose `axisos-live-amd64.iso`.
4. Partition scheme: **GPT** (Target system: **UEFI**).
5. Click **START** (select **DD Image mode** if prompted).

### Method C: Linux Terminal (`dd`)
```bash
sudo dd if=axisos-live-amd64.iso of=/dev/sdX bs=4M status=progress oflag=sync
```
*(Replace `/dev/sdX` with your target USB drive, e.g., `/dev/sdb`)*

---

## 3. Booting the Live USB

1. Insert the USB drive and power on the computer.
2. Tap your motherboard's boot menu key repeatedly:
   - **Lenovo**: <kbd>F12</kbd> or <kbd>Fn</kbd> + <kbd>F12</kbd>
   - **Dell**: <kbd>F12</kbd>
   - **HP**: <kbd>ESC</kbd> then <kbd>F9</kbd>
   - **ASUS**: <kbd>F8</kbd> or <kbd>Esc</kbd>
   - **Acer / MSI**: <kbd>F12</kbd> or <kbd>F11</kbd>
3. Select your USB flash drive from the boot options.
4. From the GRUB menu, select **AxisOS Live Desktop** (or **Safe Graphics** if your screen remains black).

---

## 4. Live Session Credentials

* **User**: `axis`
* **Password**: `axis`
* **Sudo**: Passwordless (`sudo command` or enter `axis` if prompted by GUI dialogs).

---

## 5. Installing to Disk

1. In the live desktop, launch **Install AxisOS** from the welcome screen or desktop icon.
2. Select your language, timezone, and keyboard layout.
3. Select your target drive (the installer automatically protects and excludes your live USB drive).
4. Enter your username, full name, and password.
5. Review the partitioning plan and click **Install**.
6. When the installation finishes, unplug the USB drive and reboot into your new system.

---

## 6. Quick Troubleshooting

* **Black screen on boot**: Select **Safe Graphics Mode** from the initial GRUB menu.
* **Installer fails on drive partitioning**: See the [Installer Troubleshooting Guide](INSTALLER_TROUBLESHOOTING.md) to unmount or wipe old partition signatures (`wipefs`).
