# AxisOS Native wlroots Wayland Compositor (Nobara Edition)

`axis-compositor` is a native, GPU-accelerated Wayland display server and window manager built directly with `wlroots` and `wlr_scene`.

It completely eliminates browser-based/kiosk shells in favor of a native, lightweight, event-driven desktop architecture designed for Linux gaming and responsiveness.

---

## Architecture & Initialization Sequence

The compositor follows the official wlroots 6-step initialization pipeline:

1. **Wayland Display Initialization**:
   - `wl_display_create()` sets up the core Wayland display server and event loop.
2. **Backend Hardware Abstraction**:
   - `wlr_backend_autocreate(server.wl_display)` abstracts DRM/KMS outputs and `libinput` peripherals.
3. **Renderer & Allocator Setup**:
   - `wlr_renderer_autocreate(server.backend)` handles OpenGL/Vulkan GPU acceleration.
   - `wlr_allocator_autocreate(server.backend, server.renderer)` manages hardware graphics buffers.
   - `wlr_compositor_create(server.wl_display, server.renderer)` initializes surface memory management.
4. **Shell & Scene Graph Setup**:
   - `wlr_xdg_shell_create(server.wl_display)` provides XDG application window management.
   - `wlr_layer_shell_v1_create(server.wl_display)` provides layer shell support for status bars (`waybar`), wallpapers (`swaybg`), and notifications (`mako`).
   - `wlr_scene_create()` and `wlr_scene_attach_output_layout()` manages surface hierarchy and damaged region tracking.
5. **Hardware Listeners & Input Routing**:
   - Listeners for `new_output` (DRM modesetting, frame commit rate).
   - Listeners for `new_input` (keyboards with XKB layouts and cursor pointers with `wlr_cursor`).
   - Interactive window move (`Alt + Left Click`) and resize (`Alt + Right Click`).
6. **Server Start & Event Loop Execution**:
   - Spawns background session utilities (`waybar`, `swaybg`, `mako`, `nobara-welcome`).
   - `wlr_backend_start(server.backend)` and `wl_display_run(server.wl_display)`.

---

## Default Keyboard Shortcuts

| Shortcut | Action |
| :--- | :--- |
| <kbd>Super</kbd> + <kbd>Return</kbd> / <kbd>Super</kbd> + <kbd>T</kbd> | Launch Foot Terminal |
| <kbd>Super</kbd> + <kbd>D</kbd> / <kbd>Super</kbd> + <kbd>Space</kbd> | Launch Wofi Application Menu |
| <kbd>Super</kbd> + <kbd>H</kbd> | Open Nobara Hardware Driver & Codec Manager |
| <kbd>Super</kbd> + <kbd>G</kbd> | Launch Steam |
| <kbd>Super</kbd> + <kbd>B</kbd> / <kbd>Super</kbd> + <kbd>W</kbd> | Launch Web Browser (Chromium / Firefox) |
| <kbd>Super</kbd> + <kbd>E</kbd> | Launch File Manager |
| <kbd>Alt</kbd> + <kbd>Tab</kbd> | Cycle Window Focus |
| <kbd>Super</kbd> + <kbd>Q</kbd> / <kbd>Alt</kbd> + <kbd>F4</kbd> | Close Focused Window |
| <kbd>Super</kbd> + <kbd>F</kbd> | Toggle Fullscreen |
| <kbd>Super</kbd> + <kbd>M</kbd> | Toggle Window Maximize |
| <kbd>Super</kbd> + <kbd>Shift</kbd> + <kbd>E</kbd> | Exit Wayland Session |
| <kbd>Volume Up/Down/Mute</kbd> | ALSA / PipeWire Volume Control |
| <kbd>Brightness Up/Down</kbd> | Backlight Adjustment |

---

## Building from Source

```bash
make
sudo make install
```
