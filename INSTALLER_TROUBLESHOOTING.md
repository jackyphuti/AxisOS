# AxisOS Installer & Partitioning Troubleshooting Guide

This guide provides direct troubleshooting steps for diagnosing and resolving installation or disk partitioning issues in **AxisOS**.

---

## Live Session Credentials

| Account | Username | Password | Sudo Privileges |
| :--- | :--- | :--- | :--- |
| **Live User** | `axis` | `axis` | Passwordless (`sudo`) |
| **Root** | `root` | `axis` | Full administrative root |

---

## 1. Checking the Installer Log

The installer logs all operations to `/tmp/axisos-installer.log`. To check what caused an error:

```bash
cat /tmp/axisos-installer.log
```

Or filter for errors:

```bash
grep -i "error" /tmp/axisos-installer.log
```

---

## 2. Common Causes & Quick Solutions

### Cause A: Target Drive is In Use (`EBUSY` / Device Busy)
If a partition on the target drive was automatically mounted or swap was enabled:

```bash
# 1. Turn off all swap partitions
sudo swapoff -a

# 2. Stop automatic automounters
sudo systemctl stop udisks2.service 2>/dev/null || true

# 3. Unmount target partitions (replace /dev/sdX with target, e.g., /dev/sda or /dev/nvme0n1)
sudo umount -f /dev/sdX* 2>/dev/null || true
```

### Cause B: Corrupt Old Partition Tables or BitLocker Headers
If an old partition table or BitLocker signature is preventing clean partitioning:

```bash
# Identify your target internal drive
lsblk -o NAME,SIZE,TYPE,MODEL,MOUNTPOINTS

# Wipe signatures (CAUTION: Ensure you specify your target internal disk, NOT your USB)
sudo wipefs -a /dev/sdX
sudo sgdisk --zap-all /dev/sdX
sudo partprobe /dev/sdX
```

### Cause C: Storage Controller Set to Intel RST / RAID
Linux cannot detect NVMe or SATA drives if the motherboard UEFI is configured in Intel RST (VMD) RAID mode.
* **Fix**: Enter BIOS Setup (<kbd>F1</kbd>, <kbd>F2</kbd>, or <kbd>Del</kbd>) -> Storage Configuration -> Change mode from **RST/RAID** to **AHCI / NVMe**.

### Cause D: Secure Boot Violation on Enterprise Laptops
* **Fix**: In BIOS Setup, go to **Security** -> **Secure Boot** -> enable **"Allow Microsoft 3rd Party UEFI CA"**, or temporarily disable Secure Boot during installation.

---

## 3. Rerunning the Installer

After wiping or preparing the drive, relaunch the installer from the terminal:

```bash
sudo axis-installer
```
