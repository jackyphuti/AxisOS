# AxisOS Installation & Hardware Deployment Guide

Welcome to the official deployment and installation guide for **AxisOS 2.0 (Beta Edition)**.

AxisOS is a modern, high-performance Linux operating system powered by Debian 12 (Bookworm), XanMod low-latency kernel, native Wayland / X11 desktop environments, and a responsive C++ / Qt 6 application suite. It features the **GameBoost Extreme Performance Engine**, automated Btrfs/EXT4 partition orchestration, and a seamless graphical setup experience.

---

## 1. System Requirements

| Component | Minimum Requirements | Recommended Specifications |
| :--- | :--- | :--- |
| **Processor** | 64-bit x86_64 Dual-Core CPU | 64-bit Quad-Core CPU or higher (Intel Core i3/i5/i7/i9 or AMD Ryzen) |
| **RAM (Memory)** | 2 GB RAM (4 GB for Live Desktop) | 8 GB to 16 GB+ RAM |
| **Storage** | 25 GB free disk space | 64 GB+ NVMe SSD or high-speed SATA SSD |
| **Graphics** | UEFI Framebuffer or Intel HD 4000+ | Intel UHD/Iris Xe/Arc, AMD Radeon RX, or NVIDIA GeForce |
| **Firmware** | Modern UEFI (Secure Boot Compatible) or Legacy BIOS | UEFI with Microsoft UEFI CA / Secure Boot enabled |
| **USB Media** | 4 GB Flash Drive | 8 GB+ USB 3.0 / 3.1 Flash Drive |

---

## 2. Creating a Bootable USB Drive

Download `axisos-v2.0-live-amd64.iso` and `SHA256SUMS.txt` from the [GitHub Releases](https://github.com/jackyphuti/AxisOS/releases) page.

### Integrity Verification (Recommended):
- **Windows (PowerShell)**:
  ```powershell
  (Get-FileHash axisos-v2.0-live-amd64.iso -Algorithm SHA256).Hash
  ```
  Expected: `C0132769C5C6DAFF13D31E7855CCD448EA6DFD20F54921A3381907244C82C9F7`
- **Linux / macOS**:
  ```bash
  sha256sum -c SHA256SUMS.txt
  ```

### Method A: Using Rufus (Windows — Recommended)
1. Download and run [Rufus](https://rufus.ie/).
2. Insert your USB flash drive (at least 4 GB).
3. Under **Device**, select your USB flash drive.
4. Under **Boot selection**, click **SELECT** and choose `axisos-v2.0-live-amd64.iso`.
5. Under **Partition scheme**:
   - Choose **GPT** (Target system: **UEFI (non CSM)**) for modern computers.
   - Choose **MBR** (Target system: **BIOS or UEFI**) for older legacy hardware.
6. Click **START**. If prompted between *ISO Image mode* or *DD Image mode*, select **ISO Image mode** (or **DD Image mode** for exact byte duplication).

### Method B: Using BalenaEtcher (Windows, macOS, Linux)
1. Download and launch [BalenaEtcher](https://etcher.balena.io/).
2. Click **Flash from file** and select `axisos-v2.0-live-amd64.iso`.
3. Click **Select target** and pick your USB drive.
4. Click **Flash!**.

### Method C: Using Ventoy (Multi-ISO USB)
1. Install [Ventoy](https://www.ventoy.net/) onto your USB drive.
2. Copy `axisos-v2.0-live-amd64.iso` directly onto the Ventoy USB drive partition.

### Method D: Linux Terminal (`dd`)
```bash
sudo dd if=axisos-v2.0-live-amd64.iso of=/dev/sdX bs=4M status=progress oflag=sync
```
*(Replace `/dev/sdX` with your target USB drive, e.g. `/dev/sdb`)*

---

## 3. Motherboard & BIOS Setup

AxisOS includes **Microsoft UEFI CA signed binaries (`shimx64.efi.signed`)**, enabling out-of-the-box compatibility with modern UEFI systems.

### Boot Keys by Manufacturer
Power on the computer and tap your boot menu key repeatedly:
- **Lenovo**: Tap <kbd>F12</kbd> (or <kbd>Fn</kbd> + <kbd>F12</kbd>, or press the **Novo button**).
- **Dell**: Tap <kbd>F12</kbd>.
- **HP**: Tap <kbd>ESC</kbd> then <kbd>F9</kbd>.
- **ASUS**: Tap <kbd>F8</kbd> or <kbd>Esc</kbd>.
- **Acer / MSI**: Tap <kbd>F12</kbd> or <kbd>F11</kbd>.

Select your **USB Drive** from the UEFI boot options.

> [!TIP]
> If your motherboard firmware has strict 3rd-party certificate restrictions (common in enterprise Lenovo ThinkPads), go to BIOS Setup (<kbd>F1</kbd> or <kbd>F2</kbd>) -> **Security** -> **Secure Boot** -> enable **"Allow Microsoft 3rd Party UEFI CA"** or temporarily set Secure Boot to **[Disabled]**.

---

## 4. Booting into the Live Environment

When the USB boots, you will arrive at the **AxisOS Live Desktop** without touching any internal disks:

1. **Welcome Center**: A responsive interface presents options:
   - **Install AxisOS**: Launches the interactive graphical installer wizard.
   - **Try Live Demo**: Lets you explore the desktop, apps, browser, terminal, settings, and Wi-Fi while running entirely in RAM.
2. **Safe Graphics Mode**: If your display requires fallback framebuffer rendering, select **Safe Graphics / Failsafe** from the initial GRUB menu.
3. **Wi-Fi Captive Portal**: If connecting to a public or school Wi-Fi network that requires a web browser login, AxisOS automatically prompts and launches the captive portal authentication page.

---

## 5. Installing AxisOS Permanently

AxisOS features an automated graphical installer with verified drive safety checks:

### Step-by-Step Graphical Installation:
1. Boot into the Live Desktop. Click **Install AxisOS** on the welcome dialog or click the **Install AxisOS** desktop icon.
2. **Choose Regional Preferences**:
   - Select your preferred installation language, location/timezone, and keyboard layout with instant keystroke validation.
3. **Choose Target Disk & Safety Checks**:
   - The installer displays all connected storage devices (NVMe SSDs, SATA SSDs, HDDs) with capacity, drive type, and Windows/BitLocker detection flags.
   - The active USB installer drive is automatically excluded and protected against accidental overwrites.
   - You can choose to format and partition the target drive cleanly or inspect existing drive layouts.
4. **Configure User Account**:
   - Enter your **Full Name**, **Username**, and choose a secure **Password**.
   - Optionally toggle **Automatic Login**.
5. **Automated Partitioning & Verification**:
   - The installer automatically creates:
     - 512 MB FAT32 UEFI System Partition (`/boot/efi`, type `EF00`)
     - 4 GB Linux Swap Space
     - Root system partition formatted with **Btrfs** (`zstd` compression) or **EXT4**.
   - The kernel partition table is dynamically settled with `partprobe`, `udevadm`, `partx`, and verified device polling to ensure error-free drive formatting.
6. **Finalizing & Direct Boot**:
   - Once installation completes, **unplug your USB installation flash drive**.
   - Click **Restart Computer Now**.
   - The system boots straight into your new AxisOS installation. The installer desktop shortcut, dock launcher, and background scripts are automatically deactivated post-installation.

---

## 6. Post-Installation: GameBoost & System Settings

Once installed into your drive, take advantage of built-in hardware acceleration:

### ⚡ Enabling GameBoost Extreme Performance:
1. Open **Settings** from the dock or application menu.
2. Navigate to **⚡ Gaming Boost & Performance**.
3. Toggle the switch to **ON (Extreme Boost)**.
   - Unlocks maximum CPU turbo frequencies and pins governor to `performance`.
   - Engages peak GPU wattage, NVIDIA PowerMizer Level 1, and AMD Radeon high-performance DPM.
   - Applies low-latency kernel tuning (`vm.split_lock_mitigate=0`, `vm.swappiness=10`, `vm.max_map_count=2147483642`).
   - Automatically starts the `gamemoded` scheduler.
4. Toggle back to **OFF (Balanced Standard)** anytime to return to power-saving governors.

---

## 7. Troubleshooting FAQ

- **Q: "Selected boot device failed" / "Security Boot Violation"**:
  - *Fix*: Enable "Allow Microsoft 3rd Party UEFI CA" in BIOS Secure Boot options, or temporarily disable Secure Boot.
- **Q: Screen is blank / black on boot**:
  - *Fix*: Select option **3. AxisOS 2.0 (Safe Graphics)** from the GRUB boot menu.
- **Q: Can I test AxisOS without wiping my Windows drive?**:
  - *Answer*: Yes! Boot in Live Mode from the USB. Everything runs entirely in RAM, leaving internal hard drives and partitions 100% untouched.
