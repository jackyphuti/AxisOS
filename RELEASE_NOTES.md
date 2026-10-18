# AxisOS 2.1 — Live Hybrid ISO (Community Testing & Feedback Edition)

Welcome to the latest community testing release of **AxisOS 2.1**! This bootable live ISO includes the unified desktop environment, genuine hardware telemetry HUD, automatic remote OTA updates with atomic rollback snapshots, and full hybrid boot support.

We invite the community to test this release across diverse hardware, laptops, and virtual environments and provide feedback!

---

### What's New & Included

- **Zero-Dummy Real-Hardware Task Manager (`axis-task-manager`)**:
  - Windows 11-style collapsible navigation rail with neon cyan (`#00F2FE`) & obsidian (`#0B0D11`) aesthetics.
  - Dynamically sized multi-core load matrix from `/proc/stat`.
  - Process I/O read/write rates from `/proc/[pid]/io` and GPU engine usage via `/dev/dri` and `/dev/nvidia*`.
  - Clean `N/A` fallback for thermal readings when virtualized (zero synthetic numbers).
  - Real user session accounting grouped by UID from `/proc/[pid]/status`.

- **Unified Settings Integration**:
  - Dedicated standalone settings app merged directly into the default **GNOME Settings** (`gnome-control-center`).
  - AxisOS custom panels (System Health, Gaming Mode, Biometrics, and Signature Features) accessible directly via desktop actions and application search.

- **Continuous Remote OTA Updates (No Reinstall Required)**:
  - Built-in APT sources pointing to the official AxisOS hosted repository.
  - Automated background update notifier (`axis-update-notifier`) checks for newer versions and alerts the desktop with an interactive *"Update Now"* button.
  - One-click or CLI upgrades via `axis-sysupgrade` with automatic pre-transaction Btrfs rollback snapshot checkpoints.

- **Hybrid Boot & Hardware Support**:
  - Dual UEFI Secure Boot (Shim + GRUB) & Legacy BIOS (isolinux hybrid MBR).
  - Preloaded drivers for Intel, AMD, and NVIDIA graphics.

---

### Testing & Installation Instructions

1. **Flashing to USB**:
   - Write the ISO using [BalenaEtcher](https://etcher.balena.io/), [Rufus](https://rufus.ie/) (in *DD Image mode*), or standard Linux CLI:
     ```bash
     sudo dd if=axisos-v2.1-live-amd64.iso of=/dev/sdX bs=4M status=progress conv=fsync
     ```
2. **Virtual Machine Testing**:
   - **RAM**: Minimum 4 GB (8 GB recommended).
   - **CPUs**: 2+ cores.
   - **Firmware**: UEFI (OVMF) enabled with 3D graphics acceleration.

---

### Verification Checksums

- **File**: `axisos-v2.1-live-amd64.iso`
- **Size**: `2,056,257,536` bytes (1.91 GB)
- **SHA-256 Checksum**:
  ```
  37c429c20d8d57a67052e9bc7934ff81638342db751cc65eb03ec8cd8013cb07
  ```

---

### Feedback & Issue Reporting

Please test and let us know your findings:
- Wi-Fi and Bluetooth detection
- Graphics card performance and resolution
- Audio and thermal sensor detection
- Overall desktop fluidity

Open issues and share feedback at [GitHub Issues](https://github.com/jackyphuti/AxisOS/issues) or [Discussions](https://github.com/jackyphuti/AxisOS/discussions)!
