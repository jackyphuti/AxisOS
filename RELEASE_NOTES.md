# AxisOS 2.1 Release Notes

Welcome to **AxisOS 2.1**! This release features our native pure C desktop environment, `wlroots` Wayland compositor, hardware telemetry, and remote OTA update tooling.

---

## What's New

* **100% Pure C Architecture**: All desktop shell components, native utilities, compositor bindings, and system daemons run strictly in pure C11 (`-std=gnu11`).
* **LiquidNodes Desktop Shell (`shell-c`)**: Lightweight top panel, floating dock, and fast application launcher powered by GTK 3 and `gtk-layer-shell`.
* **Hardware Telemetry HUD (`axis-task-manager`)**: Real-time multi-core load matrix from `/proc/stat`, memory consumption, and process I/O rates without synthetic numbers.
* **Native Control Center (`axis-settings`)**: Dedicated system settings panel with dark mode toggle live styling, display configuration, and hardware management.
* **Low-Latency Terminal (`axis-terminal`)**: Native PTY terminal emulator powered by `libvterm`, GTK 3, and truecolor support.
* **Remote OTA Updates**: Integrated APT repositories with automatic update checking via `axis-update-notifier` and atomic Btrfs rollback snapshots.
* **Hybrid Boot Compatibility**: Dual UEFI Secure Boot (Shim + GRUB) and Legacy BIOS support for physical hardware and virtual machines.

---

## Live Boot Credentials

* **User**: `axis`
* **Password**: `axis`
* **Sudo**: Passwordless (`sudo`)

---

## Verification Checksum

* **File**: `axisos-live-amd64.iso`
* **Size**: `1,818,001,408` bytes (1.7 GB)
* **SHA-256 Checksum**:
  ```text
  0dd865b2a0dbbfc4e34a4dda9c424c4482292540989eac23bcb47497028c78fd
  ```

---

## Testing & Feedback

Please report bugs, hardware compatibility issues, or feedback on [GitHub Issues](https://github.com/jackyphuti/AxisOS/issues).
