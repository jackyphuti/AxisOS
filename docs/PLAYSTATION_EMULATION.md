# PlayStation & High-Performance Emulation on AxisOS

AxisOS provides out-of-the-box hardware-accelerated emulation for Sony PlayStation consoles (PS1, PS2, PS3, PSP, PS Vita, and PS4) as well as PC gaming via Steam, Lutris, Heroic, and RetroArch.

---

## 1. Legal & BIOS Policy

> [!IMPORTANT]
> **No BIOS or Game Images Bundled:**
> In strict accordance with copyright law, AxisOS does **not** include, download, or distribute proprietary console BIOS files or copyrighted game images.
>
> You must dump the BIOS from a physical PlayStation console that you legitimately own and place it into the designated directory.

### Directory Layout

AxisOS automatically pre-configures your emulation hierarchy in `~/Emulation`:

```
~/Emulation/
├── bios/
│   ├── ps1/        <- Place PS1 BIOS (e.g. SCPH1001.BIN, scph5501.bin)
│   ├── ps2/        <- Place PS2 BIOS (e.g. SCPH-70012.bin, ps2-bios.bin)
│   ├── ps3/        <- Place PS3UPDAT.PUP (Sony PlayStation 3 firmware update)
│   ├── psp/        <- Optional font/firmware assets
│   ├── psvita/     <- Place PS Vita firmware PUP
│   └── ps4/        <- Place PS4 recovery/firmware update
└── games/
    ├── ps1/        <- .cue / .bin, .chd, .iso
    ├── ps2/        <- .iso, .chd, .cso, .bin
    ├── ps3/        <- .pkg, .sfb, PS3 folder games
    ├── psp/        <- .iso, .cso, .pbp
    ├── psvita/     <- .vpk, decrypted app folders
    └── ps4/        <- .pkg, decrypted elf/eboot
```

---

## 2. Emulators & Frontends

### Archive Packages (Debian 12 / Backports)
* **Steam**: Native Steam client with Proton GE/Experimental support
* **Lutris**: Game manager for Wine, Proton, and retro emulators
* **Heroic Games Launcher**: Epic Games, GOG, and Amazon Games runner
* **RetroArch**: Multi-system frontend with libretro cores

### Sandboxed Flatpak Emulators (Flathub)
AxisOS includes automated first-run setup and Wayland/Vulkan sandbox permissions for:
* **PCSX2** (`net.pcsx2.PCSX2`) — PlayStation 2
* **RPCS3** (`net.rpcs3.RPCS3`) — PlayStation 3
* **DuckStation** (`org.duckstation.DuckStation`) — PlayStation 1
* **PPSSPP** (`org.ppsspp.PPSSPP`) — PlayStation Portable
* **Vita3K** (`org.vita3k.Vita3K`) — PlayStation Vita
* **shadPS4** (`net.shadps4.shadPS4`) — PlayStation 4

---

## 3. Plug-and-Play Launching

You can launch games in three ways:

1. **Direct Double-Click**: Double-click any `.iso`, `.chd`, `.cso`, `.cue`, `.bin`, `.pkg`, or `.pbp` file from the file manager. The native Pure C runner (`axis-game-runner`) detects the console, verifies BIOS availability, engages CPU game boost, and launches the game fullscreen under Wayland.
2. **Terminal / CLI**:
   ```bash
   axis-game-runner ~/Emulation/games/ps2/GranTurismo4.iso
   # Or explicitly choose the console:
   axis-game-runner ps3 ~/Emulation/games/ps3/DemonSouls.pkg
   ```
3. **Desktop Shell Games Panel**: Open the top bar Game Station icon or Launcher Games tab to browse your Steam, Lutris, RetroArch, and PlayStation libraries in a unified Cairo glassmorphic panel.

---

## 4. Hardware Acceleration & Graphics Setup

AxisOS includes an automated, idempotent driver bootstrap (`axis-gpu-setup.sh`):

```bash
sudo /usr/local/bin/axis-gpu-setup.sh
```

* **AMD GPUs**: Uses Mesa RADV Vulkan drivers with Graphics Pipeline Library (`RADV_PERFTEST=gpl,nggc`) and `firmware-amd-graphics`.
* **Intel GPUs**: Uses Mesa ANV Vulkan drivers with `intel-media-va-driver-non-free`. Modern Intel Arc Alchemist and Battlemage GPUs use the low-overhead `xe` kernel driver.
* **NVIDIA GPUs**: Uses DKMS modules with `nvidia-open-kernel-dkms` for Turing and newer (GTX 16xx, RTX 20/30/40/50 series) or proprietary DKMS for legacy cards. Automatically sets `nvidia-drm.modeset=1 fbdev=1` for seamless Wayland rendering.
* **32-Bit Multiarch**: All 32-bit Vulkan/GL driver components (`mesa-vulkan-drivers:i386`, `libvulkan1:i386`, `libgl1-mesa-dri:i386`) are configured for legacy game compatibility.

---

## 5. Dual Sessions: Desktop Shell vs Game Mode

AxisOS features two distinct Wayland sessions:

1. **LiquidNodes Desktop Shell**: Native Pure C Wayland compositor (`axis-compositor`) and GTK 3 layer-shell with top bar, dock, and task manager.
2. **Game Mode (Console)**: Fullscreen micro-compositor session running `gamescope` directly into Steam Big Picture or Game Station with HDR support and MangoHud overlay.

To switch sessions:
```bash
# Switch to Game Mode:
axis-switch-session gamemode

# Return to Desktop Shell:
axis-switch-session desktop

# Check current session:
axis-switch-session status
```

---

## 6. Controller & Input Support

Supported out of the box with zero configuration:
* **Sony DualSense (PS5) / DualSense Edge**: USB & Bluetooth with rumble/haptics via `hid-playstation`
* **Sony DualShock 4 (PS4)**: USB & Bluetooth via `hid-sony`
* **Sony DualShock 3 (PS3)**: USB & Bluetooth
* **Microsoft Xbox Series X/S / Xbox One**: USB, Wireless Dongle, and Bluetooth via `xpadneo-dkms`
* **Nintendo Switch Pro Controller**: USB & Bluetooth via `hid-nintendo`
* **Universal `/dev/uinput`**: Configured with `TAG+="uaccess"` for non-root SDL2 virtual mapping
