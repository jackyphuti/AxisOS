# AxisOS 1.0 "Horizon" (Sonoma Edition) — Official Release

Welcome to the updated official release of **AxisOS Linux 1.0 "Horizon"**!

AxisOS is a next-generation Linux operating system designed for speed, glassmorphic beauty, and hardware versatility. It features a native Wayland kiosk compositor (**Cage**), an asynchronous **System Management Daemon** (`axisos-daemon`), everyday desktop apps, and a complete **Windows-style graphical installation experience**.

---

## 🌟 What's New in this Build

1. **Windows-Style Installation Experience (No Commands Required)**:
   - **Language, Location & Keyboard Setup**: Just like Windows Setup, select your preferred *Language to install*, *Time and currency format (Location / Timezone)*, and *Keyboard input method*.
   - **"Where do you want to install AxisOS?" Table**: Displays all connected storage devices (Drive 0, Drive 1, Toshiba HDD, NVMe SSD) with drive type, capacity, and Windows/BitLocker detection flags.
   - **Automatic Partition Layout**: Visually partitions the drive with 512 MB UEFI Boot ESP, 4.0 GB swap, and Btrfs root subvolumes with transparent `zstd` compression.
   - **Windows-Style Checklist Progress**: Watch live progress through Copying files, Getting files ready, Installing drivers, Installing Secure Boot certificates, and Registering BIOS boot entries.
   - **One-Click Reboot**: Click **Restart Computer Now** upon completion.

2. **Fixed Screen Blinking / Flickering**:
   - Resolved the systemd service conflict between `axisos.service` and `getty@tty1.service` that was causing rapid display restart loops.
   - Removed unstable `--in-process-gpu` flag from Chromium, preventing Wayland Ozone GPU process crashes.
   - Added graceful crash handling and fallback modes.

3. **Clean, Silent Boot (SGX / ACPI / X.509 Warnings Suppressed)**:
   - Added kernel parameters `loglevel=3 vt.global_cursor_default=0 systemd.show_status=false` to suppress harmless motherboard BIOS ACPI bugs, SGX disabled warnings, and X.509 certificate notices.
   - Provides a clean, calm boot screen directly into the desktop or setup wizard.

4. **Boot from USB to Test Before Installing**:
   - You can test AxisOS completely in RAM without modifying your internal hard drive or Windows partitions.
   - The Welcome Window provides:
     - **Try Live Demo**: Explore the Horizon desktop, test Wi-Fi, audio, Safari browser, and apps.
     - **Install AxisOS**: Launches the Windows-style Setup Wizard whenever you're ready.

5. **Microsoft UEFI CA Signed Bootloader (`shimx64.efi.signed`)**:
   - Both the Live USB and the target drive use the official Microsoft-signed Shim loader as `BOOTX64.EFI`.
   - Compatible with modern UEFI Secure Boot without security policy violations.

6. **Guaranteed Lenovo / HP / Dell Boot Menu Visibility**:
   - Installs to both `/EFI/AxisOS/` and `/EFI/BOOT/BOOTX64.EFI` (the universal hardware fallback).
   - Automatically registers dual entries into motherboard NVRAM (`AxisOS` and `AxisOS (UEFI Fallback)`).

---

## 📦 Release Asset Verification

| Asset | Size | SHA-256 Checksum |
| :--- | :--- | :--- |
| **`axisos-live-amd64.iso`** | 882 MB | `86F8F2155A6A6287B0AACB895D6190AD427746F384A07D41BA0A3E16C2DBEAC1` |

---

## 📖 Step-by-Step Installation Guide

For the full detailed walkthrough (writing with Rufus/BalenaEtcher, BIOS keys, and dual-booting), see the [**AxisOS Installation Guide**](https://github.com/jackyphuti/AxisOS/blob/main/INSTALLATION_GUIDE.md).
