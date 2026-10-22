<div style="font-family: 'IBM Plex Sans', 'IBM Plex Mono', 'Liberation Mono', Menlo, Consolas, monospace; line-height: 1.6;">

# AxisOS Installer & Partitioning Troubleshooting Guide

This guide provides troubleshooting steps, diagnostic procedures, and manual disk preparation instructions for resolving installation and partition issues in **AxisOS 2.1**.

---

## Live Session Credentials & Root Access

When booted into the AxisOS Live USB / VM environment:

| Account | Username | Password | Sudo Privileges |
| :--- | :--- | :--- | :--- |
| **Live User** | `axis` | `axis` | Passwordless (`NOPASSWD: ALL`) |
| **Root** | `root` | `axis` | Full administrative root |

> [!TIP]
> Because `axis` has passwordless sudo configured, you can open **Axis Terminal** and enter a root shell immediately with:
> ```bash
> sudo -i
> ```
> If any GUI prompt or service asks for authentication, enter **`axis`**.

---

## Step 1: Checking the Installer Error Log

The installer automatically logs all partitioning, formatting, and installation steps to `/tmp/axisos-installer.log`.

To check the exact error that caused the installation to stop:

```bash
cat /tmp/axisos-installer.log
```

Or view recent errors with:
```bash
grep -i "error" /tmp/axisos-installer.log
```

### Common Root Causes:
1. **Device or Resource Busy (`EBUSY`)**: The file manager or system automounter automatically mounted a partition on the target drive in the background.
2. **Active Swap Space**: An existing Linux swap partition on the target drive was automatically detected and activated by the live kernel.
3. **Refusing to Overwrite Installation Medium**: The installer automatically safeguards against wiping the USB flash drive you are currently booted from. You must select an internal hard drive or SSD.
4. **Hardware Storage Controller in Intel RST / RAID Mode**: Linux cannot access NVMe or SATA drives if the motherboard UEFI is configured in Intel RST (VMD) RAID mode instead of AHCI.

---

## Step 2: Preparing and Wiping the Target Drive Clean

If an existing partition table (Windows BitLocker, Linux LVM, dirty partition table, or corrupt GPT backup header) is blocking the installer, wiping the disk signatures on the terminal resolves the issue.

### 1. Identify the Target Block Device

List all available storage devices:
```bash
lsblk -o NAME,SIZE,TYPE,FSTYPE,MODEL,MOUNTPOINTS
```

> [!CAUTION]
> Identify your **internal** drive (e.g. `/dev/nvme0n1` for NVMe SSD, `/dev/sda` for SATA SSD/HDD).  
> **DO NOT select your Live USB drive.** (Look at the `SIZE` and `MOUNTPOINTS` column—the live USB usually has `/run/live/medium` or `/cdrom`).

---

### 2. Deactivate Swap & Unmount Partitions

Make sure the target drive is not in use by any process or mount point:

```bash
# Disable all active swap
sudo swapoff -a

# Stop automatic disk mounting
sudo systemctl stop udisks2.service

# Unmount all partitions on the target drive (e.g. /dev/sda or /dev/nvme0n1)
sudo umount -f /dev/sdX* 2>/dev/null || true
```
*(Replace `/dev/sdX` with your target drive path, e.g. `/dev/sda` or `/dev/nvme0n1`)*

---

### 3. Clear All Device Locks, LVM, and Multipath

If the drive was previously used with LVM, software RAID, or device-mapper:

```bash
sudo dmsetup remove_all -f 2>/dev/null || true
sudo vgchange -an 2>/dev/null || true
```

---

### 4. Wipe Partition Tables and Signatures

Run `wipefs` and `sgdisk` to remove all old partition headers, signatures, and GPT tables:

#### For SATA SSD / HDD (`/dev/sda`, `/dev/sdb`, etc.):
```bash
# 1. Wipe all filesystem and partition signatures
sudo wipefs --all --force /dev/sda

# 2. Zap GPT and MBR partition tables
sudo sgdisk --zap-all /dev/sda

# 3. Overwrite the first 50 MB with zeros to clear any stubborn headers
sudo dd if=/dev/zero of=/dev/sda bs=1M count=50 status=progress conv=fsync

# 4. Notify kernel of partition changes
sudo partprobe /dev/sda
```

#### For NVMe SSD (`/dev/nvme0n1`):
```bash
# 1. Wipe all filesystem and partition signatures
sudo wipefs --all --force /dev/nvme0n1

# 2. Zap GPT and MBR partition tables
sudo sgdisk --zap-all /dev/nvme0n1

# 3. Overwrite the first 50 MB with zeros
sudo dd if=/dev/zero of=/dev/nvme0n1 bs=1M count=50 status=progress conv=fsync

# 4. Notify kernel of partition changes
sudo partprobe /dev/nvme0n1
```

---

## Step 3: BIOS / UEFI Firmware Settings Check

If `lsblk` does not show your internal hard drive or NVMe SSD at all:

1. **Storage Controller Mode (SATA / NVMe)**:
   - Reboot into BIOS/UEFI setup (press <kbd>F2</kbd>, <kbd>F12</kbd>, <kbd>Del</kbd>, or <kbd>Esc</kbd> at boot).
   - Find **SATA Operation** / **Storage Configuration** / **VMD Controller**.
   - Change from **RAID On / Intel RST / VMD** to **AHCI / NVMe**.
   - *Note*: Linux cannot communicate with drives hidden behind proprietary Intel RST RAID controllers.

2. **Secure Boot Settings**:
   - AxisOS uses signed bootloaders. If your BIOS has a strict 3rd-party vendor certificate policy, enable **"Allow Microsoft 3rd Party UEFI CA"** in the Secure Boot tab, or temporarily set Secure Boot to **Disabled**.

3. **Disable Windows Fast Startup**:
   - If dual-booting or replacing Windows, boot into Windows once and turn off **Fast Startup** in Power Options. Windows Fast Startup hibernates the NTFS filesystem and leaves drive controllers in a locked state.

---

## Step 4: Re-launching the Installer

Once the disk is wiped clean and unlocked:

### Option A: Launch Graphical Installer (GUI)
Run in terminal:
```bash
axis-installer
```
*(Or double-click the **Install AxisOS 2.1** desktop icon).*

1. Select your target drive from the dropdown.
2. Select your filesystem type (**Btrfs** with snapshot protection or **EXT4**).
3. Set your username and password.
4. Click **Install AxisOS Now**.

---

### Option B: Direct Terminal / CLI Installer
If you prefer installing entirely from the command line or need detailed real-time console feedback, run the installation script directly:

```bash
sudo -n /usr/local/bin/axisos-install.sh \
  --disk /dev/sdX \
  --username axis \
  --password password \
  --fullname "AxisOS User" \
  --hostname axis-pc \
  --fs btrfs
```
*(Replace `/dev/sdX` with `/dev/sda` or `/dev/nvme0n1`, and set your preferred username and password).*

---

## Post-Installation Note

Once AxisOS finishes installing to disk and you reboot into the installed system:
- **`axis-installer` is automatically hidden** from the application menu and desktop.
- This is intentional to safeguard your installed operating system from accidentally re-partitioning or wiping itself.
- For post-install management and updates, use **Settings** or run:
  ```bash
  axis-sysupgrade
  ```

</div>

