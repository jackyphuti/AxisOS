# AxisOS 1.0 "Horizon" (Sonoma Edition) — Official Release

Welcome to the updated official release of **AxisOS Linux 1.0 "Horizon"**!

AxisOS is a next-generation Linux operating system designed for speed, glassmorphic beauty, and hardware versatility. It features a native Wayland kiosk compositor (**Cage**), an asynchronous **System Management Daemon** (`axisos-daemon`), everyday desktop apps, and a complete **graphical installation experience**.

---

## 🌟 What's New in this Build

1. **100% Graphical Installer UI (No Terminal Commands Required)**:
   - When you boot the Live USB, a clean **Welcome Dialog** appears immediately with one-click options to **Install AxisOS** or **Try Live Demo**.
   - Visual disk selector displaying your hard drives and SSDs (e.g. *Toshiba 930 GB*, *Samsung NVMe*) with visual icons, capacity, and Windows/BitLocker detection badges.
   - Protection against accidental overwriting of the Live USB installer flash drive.
   - Real-time animated progress bar across 6 phases with live log streaming.
   - One-click **Restart Computer Now** button upon completion.

2. **Microsoft UEFI CA Signed Bootloader (`shimx64.efi.signed`)**:
   - Both the Live USB and the installed hard drive now use the official Microsoft-signed Shim loader as `BOOTX64.EFI`.
   - Boots cleanly on modern UEFI systems without "Selected boot image did not authenticate" or "Security Boot Violation" errors.

3. **Guaranteed Lenovo / HP / Dell Boot Menu Visibility**:
   - The installer now automatically installs the UEFI bootloader into both `/EFI/AxisOS/` AND `/EFI/BOOT/BOOTX64.EFI` (the universal hardware fallback).
   - Automatically registers dual entries into motherboard NVRAM with `efibootmgr` (`AxisOS` and `AxisOS (UEFI Fallback)`).
   - Resolves the missing boot option issue on Lenovo laptops and desktops.

---

## 📦 Release Asset Verification

| Asset | Size | SHA-256 Checksum |
| :--- | :--- | :--- |
| **`axisos-live-amd64.iso`** | 882 MB | `7F82D544A28A15FBF4F08DF43D4266E4E13CC21AB2336DBDD5D9B414273CEE92` |

---

## 📖 Step-by-Step Installation Guide

For the full detailed walkthrough (writing with Rufus/BalenaEtcher, BIOS keys, and dual-booting), see the [**AxisOS Installation Guide**](https://github.com/jackyphuti/AxisOS/blob/main/INSTALLATION_GUIDE.md).
