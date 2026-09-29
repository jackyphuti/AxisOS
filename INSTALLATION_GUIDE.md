# AxisOS Installation & Hardware Deployment Guide

Welcome to the official deployment and installation guide for **AxisOS Linux 1.0 (Sonoma Edition)**.

AxisOS is a next-generation Linux operating system designed for speed, beauty, and security. It features a modern Wayland-native desktop shell (Horizon Shell), the native `axis` package manager, and an automated system daemon.

---

## 1. System Requirements

| Component | Minimum Requirements | Recommended Specifications |
| :--- | :--- | :--- |
| **Processor** | 64-bit x86_64 Dual-Core CPU | 64-bit Quad-Core CPU or higher (Intel Core i3/i5/i7/i9 or AMD Ryzen) |
| **RAM (Memory)** | 2 GB RAM | 4 GB to 8 GB RAM |
| **Storage** | 20 GB free disk space | 64 GB+ SSD or dedicated Hard Drive |
| **Graphics** | Standard UEFI Framebuffer or Intel HD 4000+ | Intel UHD/Iris Xe, AMD Radeon, or NVIDIA |
| **Firmware** | Modern UEFI (Secure Boot Compatible) or Legacy BIOS | UEFI with Microsoft UEFI CA / Secure Boot |
| **USB Media** | 2 GB Flash Drive | 8 GB+ USB 3.0 Flash Drive |

---

## 2. Creating a Bootable USB Drive

Download `axisos-live-amd64.iso` from the [GitHub Releases](https://github.com/jackyphuti/AxisOS/releases) page.

### Method A: Using Rufus (Windows — Recommended)
1. Download and run [Rufus](https://rufus.ie/).
2. Insert your USB flash drive (at least 2 GB).
3. Under **Device**, select your USB flash drive.
4. Under **Boot selection**, click **SELECT** and choose `axisos-live-amd64.iso`.
5. Under **Partition scheme**:
   - Choose **GPT** (Target system: **UEFI (non CSM)**) for modern computers.
   - Choose **MBR** (Target system: **BIOS or UEFI**) for older hardware.
6. Click **START**. If prompted between *ISO Image mode* or *DD Image mode*, select **ISO Image mode** (or **DD Image mode** for exact byte duplication).

### Method B: Using BalenaEtcher (Windows, macOS, Linux)
1. Download and launch [BalenaEtcher](https://etcher.balena.io/).
2. Click **Flash from file** and select `axisos-live-amd64.iso`.
3. Click **Select target** and pick your USB drive.
4. Click **Flash!**.

### Method C: Using Ventoy (Multi-ISO USB)
1. Install [Ventoy](https://www.ventoy.net/) onto your USB drive.
2. Copy `axisos-live-amd64.iso` directly onto the Ventoy USB drive partition.

### Method D: Linux Terminal (`dd`)
```bash
sudo dd if=axisos-live-amd64.iso of=/dev/sdX bs=4M status=progress oflag=sync
```
*(Replace `/dev/sdX` with your target USB drive, e.g. `/dev/sdb`)*

---

## 3. Motherboard & BIOS Setup

AxisOS includes **Microsoft UEFI CA signed binaries (`shimx64.efi.signed`)**, enabling out-of-the-box compatibility with modern UEFI systems.

### 1. Boot Keys by Manufacturer
Power on the computer and tap your boot menu key repeatedly:
- **Lenovo**: Tap <kbd>F12</kbd> (or <kbd>Fn</kbd> + <kbd>F12</kbd>, or press the **Novo button** on the side).
- **Dell**: Tap <kbd>F12</kbd>.
- **HP**: Tap <kbd>ESC</kbd> then <kbd>F9</kbd>.
- **ASUS**: Tap <kbd>F8</kbd> or <kbd>Esc</kbd>.
- **Acer / MSI**: Tap <kbd>F12</kbd> or <kbd>F11</kbd>.

Select your **USB Drive** from the UEFI boot options.

> [!TIP]
> If your motherboard firmware has strict 3rd-party certificate restrictions (common in enterprise Lenovo ThinkPads), go to BIOS Setup (<kbd>F1</kbd> or <kbd>F2</kbd>) -> **Security** -> **Secure Boot** -> enable **"Allow Microsoft 3rd Party UEFI CA"** or temporarily set Secure Boot to **[Disabled]**.

---

## 4. Booting into the Live Environment

When the USB boots, you will arrive at the **AxisOS Live Desktop** without modifying any disks:

1. **Welcome Window**: A glassmorphic dialog presents two options:
   - **Install AxisOS**: Launches the interactive graphical installer wizard.
   - **Try Live Demo**: Lets you explore the desktop, apps, browser, and settings while running entirely in RAM.
2. **Safe Graphics Mode**: If your display requires fallback framebuffer rendering, select **Safe Graphics / Failsafe** from the initial GRUB menu.

---

## 5. Installing AxisOS Permanently (100% Graphical UI)

You **do not need to use terminal commands** to install AxisOS.

### Step-by-Step Graphical Installation:
1. Boot into the Live Desktop. Click **Install AxisOS** on the welcome dialog or click the **Install AxisOS** desktop icon.
2. **Choose Target Disk**:
   - The installer automatically lists all connected hard drives and SSDs with their model name, size, and disk type (e.g. `Toshiba 930 GB`, `Samsung 980 NVMe`).
   - The active USB installer drive is automatically highlighted and protected against accidental overwrites.
   - Any detected Windows or BitLocker installations are clearly flagged.
3. **Configure User Account**:
   - Enter your **Full Name**, **Username**, and choose a **Password**.
   - Confirm your password to avoid typing errors.
   - Optionally toggle **Automatic Login**.
4. **Partition & File System Layout**:
   - Review the automatic partition preview:
     - 512 MB FAT32 UEFI System Partition (`/boot/efi`)
     - 4 GB Linux Swap Space
     - Root system partition formatted with modern **Btrfs** and `zstd` transparent compression.
5. **Start Installation**:
   - Click **Install AxisOS Now** and confirm the safety prompt.
   - Watch the animated progress bar across 6 distinct phases (Partitioning, Formatting, Deploying Root Filesystem, User Configuration, Secure Boot Deployment, Finalizing).
6. **Direct Boot into Installed Drive**:
   - When the **Installation Complete!** screen appears, **unplug your USB installation flash drive**.
   - Click **Restart Computer Now**.
   - Your computer will boot directly from your internal storage drive (SSD/HDD) into your new AxisOS system.
   - The installation wizard and live demo modals are permanently suppressed on the installed drive, taking you straight into your personal, fully configured desktop session.

---

## 6. Post-Installation UEFI Boot Visibility & Direct Boot Order

After installation, AxisOS automatically configures your motherboard firmware and internal drive:

1. **UEFI NVRAM Priority & BootNext**:
   - AxisOS automatically registers itself as `BootNext` and #1 in `BootOrder` via `efibootmgr`, ensuring the BIOS immediately loads AxisOS from your internal drive upon reboot.
2. **Dual Boot Registration**:
   - Registers primary boot option: `AxisOS` pointing to `\EFI\AxisOS\shimx64.efi`
   - Registers universal fallback option: `AxisOS (UEFI Fallback)` pointing to `\EFI\BOOT\BOOTX64.EFI`
3. **Lenovo / HP Hard Drive Compatibility**:
   - Lenovo and HP motherboards often look exclusively for the fallback path `\EFI\BOOT\BOOTX64.EFI` on the internal drive.
   - AxisOS copies the Microsoft-signed Shim and Debian-signed GRUB bootloader to `\EFI\BOOT\BOOTX64.EFI`, guaranteeing immediate visibility in the F12 / F9 boot menu.

---

## 7. Using the `axis` Package Manager

AxisOS includes its own native package manager client at `/usr/bin/axis`:

| Command | Purpose |
| :--- | :--- |
| `axis update` | Syncs the remote repository index to the local machine. |
| `axis install <pkg>` | Downloads, verifies, and installs a package and dependencies. |
| `axis remove <pkg>` | Completely uninstalls a package and removes its tracked files. |
| `axis list` | Lists all currently installed AxisOS packages. |

---

## 8. Troubleshooting FAQ

- **Q: "Selected boot device failed" / "Security Boot Violation"**:
  - *Fix*: Disable Secure Boot in your BIOS setup (<kbd>F2</kbd> or <kbd>Del</kbd>).
- **Q: Dropped to a `grub>` command prompt**:
  - *Fix*: Type:
    ```grub
    search --set=root --file /live/vmlinuz
    linux /live/vmlinuz boot=live components username=axis
    initrd /live/initrd.img
    boot
    ```
- **Q: Screen is blank / black on boot**:
  - *Fix*: Boot with `nomodeset` added to the `linux` boot line.
- **Q: How do I test without touching my Windows hard drive?**:
  - *Answer*: Boot in Live Mode from the USB. Everything runs in RAM, leaving your internal hard drive 100% untouched.
