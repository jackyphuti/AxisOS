# AxisOS Native wlroots Wayland Compositor

`axis-compositor` is a native, GPU-accelerated Wayland display server and window manager built directly with `wlroots` and `wlr_scene` in pure C11.

---

## Architecture Overview

The compositor implements:
* **Output Management**: Multi-output modesetting and frame scheduling via DRM/KMS.
* **Input Routing**: Keyboard handling with XKB layouts and cursor pointer events via `libinput`.
* **Window Management**: `xdg-shell` toplevels and popups with interactive move (<kbd>Alt</kbd> + Left Click) and resize (<kbd>Alt</kbd> + Right Click).
* **Layer Shell Support**: Anchored panels, notifications, and wallpapers via `wlr-layer-shell-v1`.
* **Virtual GPU Autodetection**: Automatic fallback to `pixman` software rasterization on hypervisors (VMware, VirtualBox, QEMU) to prevent EGL command buffer deadlocks.

---

## Keyboard Shortcuts

| Shortcut | Action |
| :--- | :--- |
| <kbd>Super</kbd> (tap) | Toggle Application Launcher |
| <kbd>Super</kbd> + <kbd>Return</kbd> / <kbd>Super</kbd> + <kbd>T</kbd> | Launch Terminal |
| <kbd>Super</kbd> + <kbd>D</kbd> / <kbd>Super</kbd> + <kbd>Space</kbd> | Launch App Launcher |
| <kbd>Super</kbd> + <kbd>B</kbd> / <kbd>Super</kbd> + <kbd>W</kbd> | Launch Browser |
| <kbd>Super</kbd> + <kbd>E</kbd> | Launch File Manager |
| <kbd>Super</kbd> + <kbd>Q</kbd> / <kbd>Alt</kbd> + <kbd>F4</kbd> | Close Window |
| <kbd>Super</kbd> + <kbd>F</kbd> | Toggle Fullscreen |
| <kbd>Super</kbd> + <kbd>M</kbd> | Toggle Maximize |
| <kbd>Alt</kbd> + <kbd>Tab</kbd> | Cycle Window Focus |
| <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>E</kbd> | Exit Session |

---

## Building from Source

```bash
make clean
make
sudo make install
```
