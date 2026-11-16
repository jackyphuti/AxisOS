# Contributing to AxisOS

Thank you for your interest in contributing to **AxisOS**!

AxisOS is an independent, hobby and educational Linux desktop operating system built on Debian 12 (Bookworm), native Wayland (`wlroots`), and a custom desktop environment written strictly in **100% Pure C (C11)**.

---

## Core Guidelines & Language Policy

> [!IMPORTANT]
> **Strict Pure C Policy (For Now):** AxisOS currently keeps all runtime and system components in **pure C11** (`-std=gnu11`) with zero C++, Qt/QML, or Python. While we may consider introducing **Rust** in the future for certain system modules or utilities, **for now we are keeping everything strictly in pure C**. Automated CI checks will reject PRs containing `.cpp`, `.hpp`, `.qml`, or `.py` files.

---

## Quick Start & Build Setup

### 1. Install Build Dependencies (Debian / Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential \
  gcc \
  pkg-config \
  libgtk-3-dev \
  libgtk-layer-shell-dev \
  libvterm-dev \
  libarchive-dev \
  libjson-c-dev \
  libcurl4-openssl-dev \
  libssl-dev \
  libpam0g-dev \
  libudev-dev \
  libwayland-dev \
  libxkbcommon-dev \
  libwlroots-dev \
  libdrm-dev
```

### 2. Compile All Applications & Shell

```bash
bash os-build/scripts/compile-native-apps.sh
```

### 3. Verify Strict Pure C Linkage

```bash
bash scripts/verify-pure-c.sh
```

---

## Repository Structure

Understanding where code lives:

| Directory | Description | Technologies |
| :--- | :--- | :--- |
| `compositor/` | Native Wayland compositor (`axis-compositor`) | C11, wlroots, libdrm, wayland-server |
| `shell-c/` | Native desktop shell (`axisos-shell`, panel, dock, launcher) | C11, GTK 3, gtk-layer-shell, Cairo |
| `os-build/apps/` | Built-in desktop apps (Terminal, Settings, Task Manager, Installer) | C11, GTK 3, libvterm |
| `os-build/scripts/` | System helper daemons, time-sync, game-boost, build scripts | C11, POSIX, Bash |
| `os-build/configs/` | Session scripts, systemd units, package lists | systemd, PAM, seatd |
| `data/` | Theme CSS tokens, wallpapers, icons, desktop files | CSS, SVG, FreeDesktop specs |
| `pkg-mgr/` | Pure C package manager runtime and repository tooling | C11, libcurl, libjson-c, libarchive |
| `tests/` | Hardware probing and UI architecture verification suites | C11 |

---

## Contribution Workflow

1. **Fork the Repository** on GitHub.
2. **Clone your fork**:
   ```bash
   git clone https://github.com/<your-username>/AxisOS.git
   cd AxisOS
   ```
3. **Create a topic branch**:
   ```bash
   git checkout -b feat/my-new-feature
   # or
   git checkout -b fix/issue-description
   ```
4. **Make your changes** following our pure C coding guidelines.
5. **Verify locally**:
   ```bash
   bash os-build/scripts/compile-native-apps.sh
   bash scripts/verify-pure-c.sh
   ```
   Ensure all 22 binaries compile cleanly with zero warnings/errors.
6. **Commit with Conventional Commits**:
   ```bash
   git commit -m "feat(shell): add workspace switch shortcuts"
   # or
   git commit -m "fix(compositor): prevent EGL hang on virtual GPUs"
   ```
   Supported types: `feat`, `fix`, `docs`, `style`, `refactor`, `perf`, `test`, `build`, `ci`, `chore`.
7. **Push to your fork** and open a **Pull Request** targeting `main`.

---

## Coding Standards

* **Language Standard**: ISO C11 (`-std=gnu11`).
* **Compiler Flags**: All code must compile cleanly with `-Wall -Wextra`.
* **Resource Cleanup**: Always free allocated heap memory, close file descriptors and sockets, and unref GObject/GTK widgets when destroyed.
* **Non-blocking Wayland**: Avoid long blocking loops in the compositor and shell threads. Use GLib/Wayland event loop callbacks and timers (`g_timeout_add`, `wl_event_loop`).
* **Safe Process Spawning**: Spawn child processes asynchronously via `g_spawn_async` or `fork`/`exec` rather than synchronous blocking calls.

---

## Reporting Issues

If you find a bug or have a suggestion:
1. Check existing [GitHub Issues](https://github.com/jackyphuti/AxisOS/issues) first.
2. Open a new issue including:
   * Clear description of expected vs. actual behavior.
   * Steps to reproduce.
   * Environment details (bare metal vs. VM hypervisor, GPU model, screen resolution).
   * Terminal output or `/home/axis/.compositor.log` snippets if applicable.
