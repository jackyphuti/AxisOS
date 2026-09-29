# AxisOS 1.0 "Horizon" (Sonoma Edition) — Official Release

Welcome to the updated official release of **AxisOS Linux 1.0 "Horizon"**!

AxisOS is a next-generation Linux operating system designed for speed, glassmorphic beauty, and hardware versatility. It features **LightDM** autologin, a lightweight **Openbox/X11** display engine, an asynchronous **System Management Daemon** (`axisos-daemon`), everyday desktop apps, and a complete **Windows-style graphical installation experience**.

---

## 🌟 What's New in this Build

1. **Fully Responsive Mouse, Touchpad & Keyboard (Xorg Input Driver Stack)**:
   - Installed `xserver-xorg-input-libinput`, `xserver-xorg-input-synaptics`, `xserver-xorg-input-evdev`, and `xserver-xorg-input-all`.
   - Added automatic touchpad tapping and natural scrolling configuration (`/etc/X11/xorg.conf.d/40-libinput-touchpad.conf`).
   - Configured global standard arrow pointer cursor (`left_ptr`).
   - Resolves unresponsive mouse cursor, touchpad clicks, and keyboard inputs on laptop and desktop hardware.

2. **LightDM Display Manager Auto-Login (Bypasses tty1 / MOTD / Console)**:
   - Configured **LightDM** with zero-timeout passwordless autologin (`/etc/lightdm/lightdm.conf.d/01_autologin.conf`) directly into an X11 Openbox kiosk session.
   - Completely eliminates the `Debian GNU/Linux 12 debian tty1 ... ABSOLUTELY NO WARRANTY` text console screen and blinking prompt.
   - 100% stable across all Intel, AMD, and NVIDIA laptop and desktop hardware.

2. **Aggressively Silenced Kernel Boot (Zero Hardware Noise)**:
   - Configured kernel command line parameters:
     `quiet splash loglevel=0 systemd.show_status=false rd.udev.log_level=3 vt.global_cursor_default=0`
   - Suppresses all harmless motherboard ACPI errors, SGX disabled warnings, and X.509 certificate notices during boot.
   - Provides a calm, clean, zero-touch boot sequence straight into the graphical environment.

3. **Windows-Style Installation Experience (No Commands Required)**:
   - **Language, Location & Keyboard Preferences**: Just like Windows Setup, select your preferred *Language to install*, *Time and currency format (Location / Timezone)*, and *Keyboard input method*.
   - **"Where do you want to install AxisOS?" Table**: Displays all connected storage devices (Drive 0: Toshiba 930 GB HDD, NVMe SSDs, etc.) with capacity, drive type, and Windows/BitLocker detection flags.
   - **Visual Partition Layout Preview**: Automatically sets up the 512 MB UEFI Boot ESP, 4.0 GB swap, and Btrfs root filesystem with transparent `zstd` compression.
   - **Windows-Style Checklist Progress**: Live progress checklist for Copying files, Getting files ready, Installing drivers, Installing Secure Boot certificates, and Registering BIOS boot entries.
   - **One-Click Reboot**: Click **Restart Computer Now** upon completion.

4. **Boot from USB to Test Before Installing**:
   - Test AxisOS completely in RAM without touching your internal hard drive or Windows partitions.
   - The Welcome Window provides:
     - **Try Live Demo**: Explore the Horizon desktop, test Wi-Fi, sound, Safari browser, and apps.
     - **Install AxisOS**: Launches the Windows-style Setup Wizard whenever you are ready.

5. **Microsoft UEFI CA Signed Bootloader (`shimx64.efi.signed`)**:
   - Both the Live USB and the target drive use the official Microsoft-signed Shim loader as `BOOTX64.EFI`.
   - Compatible with modern UEFI Secure Boot without security policy violations.

6. **Guaranteed Lenovo / HP / Dell Boot Menu Visibility**:
   - Installs to both `/EFI/AxisOS/` and `/EFI/BOOT/BOOTX64.EFI` (the universal hardware fallback).
   - Automatically registers dual entries into motherboard NVRAM (`AxisOS` and `AxisOS (UEFI Fallback)`).

7. **Direct Internal Drive Booting & Installer Suppression**:
   - Deploys `/etc/axisos-installed` marker on target drives so the system permanently identifies as an installed OS, disabling live mode, welcome modals, and the installer wizard.
   - Automatically configures UEFI `BootNext` and #1 `BootOrder` priority via `efibootmgr` so your PC boots straight into AxisOS from the drive upon restart.
   - Configures LightDM autologin specifically for the newly created user account, completely bypassing console tty prompts and previous kiosk loops.
   - Adds a clear prompt on the final setup screen reminding you to remove your USB flash drive before clicking **Restart Computer Now**.

---

## 📦 Release Asset Verification

| Asset | Size | SHA-256 Checksum |
| :--- | :--- | :--- |
| **`axisos-live-amd64.iso`** | 884 MB | `CD483CC189B73A3F41157EBCEFD089D23D976AB5B78F7AB07F225827412E8243` |

---

## 📖 Step-by-Step Installation Guide

For the full detailed walkthrough (writing with Rufus/BalenaEtcher, BIOS keys, and dual-booting), see the [**AxisOS Installation Guide**](https://github.com/jackyphuti/AxisOS/blob/main/INSTALLATION_GUIDE.md).
