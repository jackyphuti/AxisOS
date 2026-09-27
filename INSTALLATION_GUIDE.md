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
| **Firmware** | UEFI (Recommended) or Legacy BIOS | UEFI with Secure Boot Disabled |
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

Before booting from your USB drive, adjust your PC's BIOS settings:

### 1. Disable Secure Boot
AxisOS uses custom-built live bootloaders. You must temporarily disable **Secure Boot**:
1. Power on the computer and tap the BIOS setup key repeatedly:
   - **Lenovo**: Tap <kbd>F2</kbd> (or press the tiny **Novo button** on the side with a pin).
   - **Dell**: Tap <kbd>F2</kbd> or <kbd>F12</kbd>.
   - **HP**: Tap <kbd>ESC</kbd> then <kbd>F10</kbd>.
   - **ASUS / Acer / MSI**: Tap <kbd>Del</kbd> or <kbd>F2</kbd>.
2. Go to the **Security** or **Authentication** tab.
3. Set **Secure Boot** to **[Disabled]**.
4. Press <kbd>F10</kbd> to save and restart.

### 2. Open the One-Time Boot Menu
As the computer restarts, tap your boot menu key:
- **Lenovo / Dell**: <kbd>F12</kbd>
- **HP**: <kbd>F9</kbd>
- **ASUS**: <kbd>F8</kbd>
- **Acer / MSI**: <kbd>F12</kbd> or <kbd>F11</kbd>

Select your **USB Drive** from the menu.

---

## 4. Booting into the Live Environment

When booting from the USB drive:

### Option 1: Standard Boot (Default)
If your computer boots straight into the GRUB boot menu or `grub>` prompt:
```grub
search --set=root --file /live/vmlinuz
linux /live/vmlinuz boot=live components username=axis quiet splash
initrd /live/initrd.img
boot
```
The computer will boot into RAM and display the **AxisOS Horizon desktop**.

### Option 2: Safe Graphics Mode (`nomodeset`)
If your laptop screen stays black or fails GPU mode-setting:
```grub
search --set=root --file /live/vmlinuz
linux /live/vmlinuz boot=live components nomodeset username=axis
initrd /live/initrd.img
boot
```
`nomodeset` forces the universal UEFI display buffer, ensuring full compatibility on all screens.

---

## 5. Installing AxisOS Permanently

You can install AxisOS onto an internal SSD/HDD or an external secondary hard drive.

### Step 1: Identify Your Target Drive
In the terminal, run:
```bash
lsblk -o NAME,SIZE,TYPE,MODEL
```
Find your target installation disk (e.g. `/dev/sda`, `/dev/sdb`, or `/dev/nvme0n1`).

> [!CAUTION]
> Double-check the drive size and model! Installing will wipe the target drive. Do not select your main Windows drive unless you intend to replace it.

### Step 2: Run the Installer

#### Graphical Method (Desktop App):
1. In the AxisOS desktop, open the **Install AxisOS** app.
2. Select your target drive from the dropdown.
3. Enter your username, full name, and password.
4. Click **Begin Installation**.

#### Command Line Method (Terminal):
```bash
sudo /usr/local/bin/axisos-installer.sh --disk /dev/sdX --username axis --password password --autologin true
```
*(Replace `/dev/sdX` with your target drive, e.g. `/dev/sda`)*

The automated engine will:
- Partition the target drive with **GPT**.
- Create a 512 MB FAT32 **EFI System Partition**.
- Format the root partition with **Btrfs** or **ext4**.
- Copy the complete system root and user accounts.
- Install the **GRUB 2 UEFI** bootloader and universal fallback `/EFI/BOOT/BOOTX64.EFI`.

---

## 6. Post-Installation Boot Setup

Once installation reaches **100% Complete**:
1. Remove your USB installer drive.
2. Restart the computer (`sudo reboot`).
3. Press <kbd>F12</kbd> (or <kbd>F9</kbd> / <kbd>F8</kbd>) to select **AxisOS** from the boot menu.

### If the drive is not listed in the Lenovo / HP boot menu:
Modern UEFI motherboards (especially Lenovo) require the universal fallback executable:
```bash
sudo mount /dev/sdX1 /mnt
sudo mkdir -p /mnt/EFI/BOOT
sudo cp /mnt/EFI/AxisOS/grubx64.efi /mnt/EFI/BOOT/BOOTX64.EFI
sudo umount /mnt
sudo reboot
```

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
