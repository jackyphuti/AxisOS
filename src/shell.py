#!/usr/bin/env python3
"""
AxisOS HarmonyOS Desktop Shell
Built with GTK 4 and libadwaita.
Features:
- HarmonyOS Glass Dock with 56px uniform tiles, 4px gap, 20px radius, glass surface,
  linear top highlight, and cursor-distance magnification via Gtk.EventControllerMotion.
- Full-screen frosted launcher with pre-blurred Cairo wallpaper cache,
  surface token overlay (72% dark, 80% light), 250ms fade + scale animation
  respecting gtk-enable-animations, focused search entry, 5-app recent row,
  FlowBox 5-6 columns grid, and 2x2 tile preview folders opening in Gtk.Popover.
- Dynamic ThemeManager integration with live System/Light/Dark switching without restart.
"""

import sys
import os
import math
import time
import shutil
import subprocess
import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
from gi.repository import Gtk, Gdk, Adw, GLib, Gio, Pango

try:
    import cairo
except ImportError:
    cairo = None

sys.path.insert(0, os.path.dirname(__file__))
from theme_manager import ThemeManager, get_saved_wallpaper, relative_luminance, contrast_ratio
from settings import SettingsWindow
from installer import InstallerWindow


# Desktop applications catalog
APPLICATIONS = [
    {
        "id": "terminal",
        "name": "Terminal",
        "icon": "utilities-terminal-symbolic",
        "category": "System",
        "desc": "AxisOS native C/C++ Debian terminal",
        "exec": "axis-terminal"
    },
    {
        "id": "files",
        "name": "Files",
        "icon": "system-file-manager-symbolic",
        "category": "System",
        "desc": "Manage files, folders and drives",
        "exec": "nautilus"
    },
    {
        "id": "browser",
        "name": "Web Browser",
        "icon": "web-browser-symbolic",
        "category": "Internet",
        "desc": "Browse the web with native hardware acceleration",
        "exec": "firefox"
    },
    {
        "id": "software",
        "name": "App Center",
        "icon": "system-software-install-symbolic",
        "category": "System",
        "desc": "Install, update, and manage software packages",
        "exec": "axis-software"
    },
    {
        "id": "settings",
        "name": "Settings",
        "icon": "preferences-system-symbolic",
        "category": "Preferences",
        "desc": "Configure system, appearance, and hardware",
        "exec": "axis-settings"
    },
    {
        "id": "installer",
        "name": "Install AxisOS",
        "icon": "emblem-ok-symbolic",
        "category": "System",
        "desc": "Install AxisOS to local NVMe or SSD storage",
        "exec": "axis-installer"
    },
    {
        "id": "editor",
        "name": "Code & Notes",
        "icon": "text-editor-symbolic",
        "category": "Accessories",
        "desc": "Text editor and development environment",
        "exec": "gnome-text-editor"
    },
    {
        "id": "camera",
        "name": "Camera",
        "icon": "camera-photo-symbolic",
        "category": "Media",
        "desc": "Capture photos and test camera devices",
        "exec": "snapshot"
    },
    {
        "id": "music",
        "name": "Music Player",
        "icon": "media-playback-start-symbolic",
        "category": "Media",
        "desc": "High fidelity audio playback",
        "exec": "gnome-music"
    },
    {
        "id": "games",
        "name": "Steam & Games",
        "icon": "applications-games-symbolic",
        "category": "Games",
        "desc": "Gaming platform with Vulkan and Proton",
        "exec": "steam"
    }
]

FOLDERS = [
    {
        "id": "folder_sys",
        "name": "System Tools",
        "apps": [
            {"id": "disks", "name": "Disks", "icon": "system-file-manager-symbolic", "desc": "Storage drives and partitioning"},
            {"id": "monitor", "name": "System Monitor", "icon": "utilities-terminal-symbolic", "desc": "CPU, RAM, GPU utilization"},
            {"id": "network", "name": "Network", "icon": "web-browser-symbolic", "desc": "Wi-Fi and Ethernet configuration"},
            {"id": "security", "name": "Security & Root", "icon": "preferences-system-symbolic", "desc": "Privileges and authentication"}
        ]
    },
    {
        "id": "folder_media",
        "name": "Media & Creative",
        "apps": [
            {"id": "audio", "name": "Audio Mixer", "icon": "media-playback-start-symbolic", "desc": "PipeWire volume controls"},
            {"id": "photos", "name": "Photo Viewer", "icon": "camera-photo-symbolic", "desc": "Image viewing and albums"},
            {"id": "docs", "name": "Documents", "icon": "text-editor-symbolic", "desc": "Document viewer and reader"},
            {"id": "screen", "name": "Screen Record", "icon": "view-app-grid-symbolic", "desc": "Wayland screen capture"}
        ]
    }
]


class BlurredWallpaperCache:
    """
    Renders and caches pre-blurred Cairo wallpaper surfaces
    with dark (72%) and light (80%) frosted surface overlays.
    """
    def __init__(self):
        self._cache = {}

    def get_surface(self, width, height, is_dark):
        key = (width, height, is_dark)
        if key in self._cache:
            return self._cache[key]

        if not cairo:
            return None

        surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
        cr = cairo.Context(surface)

        # 1. Base gradient matching HarmonyOS design tokens
        pat = cairo.LinearGradient(0, 0, width, height)
        if is_dark:
            # Base #0E1411 to Surface Raised #223029 with teal tint
            pat.add_color_stop_rgb(0.0, 0.055, 0.078, 0.067)
            pat.add_color_stop_rgb(0.5, 0.094, 0.129, 0.114)
            pat.add_color_stop_rgb(1.0, 0.133, 0.188, 0.161)
        else:
            # Base #F4F7F3 to Surface Raised #EAF1E8
            pat.add_color_stop_rgb(0.0, 0.957, 0.969, 0.953)
            pat.add_color_stop_rgb(0.5, 0.918, 0.945, 0.910)
            pat.add_color_stop_rgb(1.0, 0.880, 0.920, 0.870)

        cr.set_source(pat)
        cr.paint()

        # 2. Add subtle soft blurred bokeh shapes
        cr.save()
        if is_dark:
            # Subtle accent (#8BC34A) & teal (#2EC4B6) glowing spheres
            cr.set_source_rgba(0.545, 0.765, 0.290, 0.09)
            cr.arc(width * 0.25, height * 0.35, min(width, height) * 0.45, 0, 2 * math.pi)
            cr.fill()
            cr.set_source_rgba(0.180, 0.769, 0.714, 0.08)
            cr.arc(width * 0.75, height * 0.65, min(width, height) * 0.50, 0, 2 * math.pi)
            cr.fill()
        else:
            # Subtle light forest lime (#4E8A1E) & teal glow
            cr.set_source_rgba(0.306, 0.541, 0.118, 0.06)
            cr.arc(width * 0.30, height * 0.30, min(width, height) * 0.40, 0, 2 * math.pi)
            cr.fill()
            cr.set_source_rgba(0.078, 0.561, 0.525, 0.05)
            cr.arc(width * 0.70, height * 0.70, min(width, height) * 0.45, 0, 2 * math.pi)
            cr.fill()
        cr.restore()

        # 3. Apply frosted surface token overlay
        # Dark scheme: surface token #18211D at 72% opacity
        # Light scheme: surface token #FFFFFF at 80% opacity
        if is_dark:
            cr.set_source_rgba(0.094, 0.129, 0.114, 0.72)
        else:
            cr.set_source_rgba(1.0, 1.0, 1.0, 0.80)
        cr.paint()

        self._cache[key] = surface
        return surface

    def clear(self):
        self._cache.clear()


class HarmonyDesktopShell(Adw.ApplicationWindow):
    def __init__(self, app):
        super().__init__(
            application=app,
            title="AxisOS Desktop",
            default_width=1280,
            default_height=800
        )
        self.theme_mgr = ThemeManager.get_default()
        self.blurred_cache = BlurredWallpaperCache()
        self.recent_apps = ["terminal", "files", "browser", "software", "settings"]

        # Main root layout container
        self.root_overlay = Gtk.Overlay()
        self.set_content(self.root_overlay)

        # 1. Desktop Background Drawing Area
        self.bg_area = Gtk.DrawingArea()
        self.bg_area.set_draw_func(self._draw_desktop_bg)
        self.root_overlay.set_child(self.bg_area)

        # 2. Desktop Interface Layer (Status Bar + Workspace + Bottom Dock)
        self.desktop_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
        self.desktop_box.set_hexpand(True)
        self.desktop_box.set_vexpand(True)
        self.root_overlay.add_overlay(self.desktop_box)

        # Top Status Bar
        self.top_bar = self._build_top_bar()
        self.desktop_box.append(self.top_bar)

        # Empty expandable workspace spacer
        workspace_spacer = Gtk.Box()
        workspace_spacer.set_vexpand(True)
        self.desktop_box.append(workspace_spacer)

        # HarmonyOS Glass Dock at bottom
        self.dock = self._build_glass_dock()
        self.desktop_box.append(self.dock)

        # 3. Full-Screen Frosted Launcher Overlay
        self.launcher_overlay = self._build_launcher_overlay()
        self.launcher_overlay.set_visible(False)
        self.root_overlay.add_overlay(self.launcher_overlay)

        # Animation state for launcher
        self.launcher_anim_id = None
        self.launcher_anim_start = 0.0
        self.launcher_anim_target = 0.0 # 0.0 closed, 1.0 opened
        self.launcher_progress = 0.0

        # Subscribe to theme scheme changes
        self.theme_mgr.connect_scheme_changed(self._on_theme_changed)

        # Keyboard shortcuts (Super / F4 toggles launcher, Esc closes launcher)
        key_controller = Gtk.EventControllerKey()
        key_controller.connect("key-pressed", self._on_key_pressed)
        self.add_controller(key_controller)

    # -------------------------------------------------------------------------
    # Top Status Bar
    # -------------------------------------------------------------------------
    def _build_top_bar(self):
        bar = Gtk.Box(
            orientation=Gtk.Orientation.HORIZONTAL,
            spacing=12,
            margin_start=18,
            margin_end=18,
            margin_top=8,
            margin_bottom=8
        )
        bar.add_css_class("card-surface")
        bar.set_size_request(-1, 38)

        # Left branding
        lbl_brand = Gtk.Label(label="AxisOS")
        lbl_brand.add_css_class("title-4")
        lbl_brand.set_margin_start(14)
        bar.append(lbl_brand)

        badge = Gtk.Label(label="Harmony Edition")
        badge.add_css_class("badge-teal")
        bar.append(badge)

        # Center Clock
        spacer1 = Gtk.Box()
        spacer1.set_hexpand(True)
        bar.append(spacer1)

        self.lbl_clock = Gtk.Label()
        self.lbl_clock.add_css_class("title-3")
        self._update_clock()
        bar.append(self.lbl_clock)
        GLib.timeout_add_seconds(1, self._update_clock)

        spacer2 = Gtk.Box()
        spacer2.set_hexpand(True)
        bar.append(spacer2)

        # Right Quick Toggles
        btn_theme = Gtk.Button(icon_name="weather-clear-symbolic")
        btn_theme.set_tooltip_text("Toggle Dark / Light Theme")
        btn_theme.add_css_class("flat")
        btn_theme.connect("clicked", self._toggle_theme_quick)
        bar.append(btn_theme)

        btn_settings = Gtk.Button(icon_name="preferences-system-symbolic")
        btn_settings.set_tooltip_text("Open Settings")
        btn_settings.add_css_class("flat")
        btn_settings.connect("clicked", lambda *_: self.open_app_by_id("settings"))
        bar.append(btn_settings)

        return bar

    def _update_clock(self):
        current_time = time.strftime("%A, %b %d   %H:%M")
        self.lbl_clock.set_label(current_time)
        return True

    def _toggle_theme_quick(self, *args):
        is_dark = self.theme_mgr.style_manager.get_dark()
        next_scheme = "light" if is_dark else "dark"
        self.theme_mgr.set_user_color_scheme(next_scheme)

    # -------------------------------------------------------------------------
    # Desktop Wallpaper Drawing
    # -------------------------------------------------------------------------
    def _draw_desktop_bg(self, area, cr, width, height):
        if not cairo:
            return
        is_dark = self.theme_mgr.style_manager.get_dark() if self.theme_mgr.style_manager else True

        # Render elegant base gradient
        pat = cairo.LinearGradient(0, 0, width, height)
        if is_dark:
            pat.add_color_stop_rgb(0.0, 0.055, 0.078, 0.067)
            pat.add_color_stop_rgb(0.5, 0.094, 0.129, 0.114)
            pat.add_color_stop_rgb(1.0, 0.133, 0.188, 0.161)
        else:
            pat.add_color_stop_rgb(0.0, 0.957, 0.969, 0.953)
            pat.add_color_stop_rgb(0.5, 0.918, 0.945, 0.910)
            pat.add_color_stop_rgb(1.0, 0.880, 0.920, 0.870)
        cr.set_source(pat)
        cr.paint()

        # Atmospheric accent circles
        cr.save()
        if is_dark:
            cr.set_source_rgba(0.545, 0.765, 0.290, 0.14)
            cr.arc(width * 0.3, height * 0.4, min(width, height) * 0.35, 0, 2 * math.pi)
            cr.fill()
            cr.set_source_rgba(0.180, 0.769, 0.714, 0.10)
            cr.arc(width * 0.7, height * 0.6, min(width, height) * 0.4, 0, 2 * math.pi)
            cr.fill()
        else:
            cr.set_source_rgba(0.306, 0.541, 0.118, 0.08)
            cr.arc(width * 0.3, height * 0.4, min(width, height) * 0.35, 0, 2 * math.pi)
            cr.fill()
            cr.set_source_rgba(0.078, 0.561, 0.525, 0.07)
            cr.arc(width * 0.7, height * 0.6, min(width, height) * 0.4, 0, 2 * math.pi)
            cr.fill()
        cr.restore()

    # -------------------------------------------------------------------------
    # HarmonyOS Glass Dock
    # 56px uniform tiles, 4px gap, 20px glass radius, cursor-distance magnification
    # -------------------------------------------------------------------------
    def _build_glass_dock(self):
        dock_wrapper = Gtk.Box(
            orientation=Gtk.Orientation.HORIZONTAL,
            halign=Gtk.Align.CENTER,
            valign=Gtk.Align.END,
            margin_bottom=18
        )

        self.dock_container = Gtk.Box(
            orientation=Gtk.Orientation.HORIZONTAL,
            spacing=4
        )
        self.dock_container.add_css_class("dock-container")
        dock_wrapper.append(self.dock_container)

        self.dock_buttons = []

        # Items in dock:
        # Launcher Toggle, Files, Terminal, Browser, Software, Settings, Installer
        dock_items = [
            {"id": "launcher_toggle", "icon": "view-app-grid-symbolic", "tooltip": "Launcher (Apps)"},
            {"id": "files", "icon": "system-file-manager-symbolic", "tooltip": "Files"},
            {"id": "terminal", "icon": "utilities-terminal-symbolic", "tooltip": "Terminal"},
            {"id": "browser", "icon": "web-browser-symbolic", "tooltip": "Web Browser"},
            {"id": "software", "icon": "system-software-install-symbolic", "tooltip": "App Center"},
            {"id": "settings", "icon": "preferences-system-symbolic", "tooltip": "Settings"},
            {"id": "installer", "icon": "emblem-ok-symbolic", "tooltip": "Install AxisOS"}
        ]

        for item in dock_items:
            btn = Gtk.Button()
            btn.add_css_class("dock-tile")
            btn.set_size_request(56, 56)
            btn.set_tooltip_text(item["tooltip"])

            img = Gtk.Image.new_from_icon_name(item["icon"])
            img.set_pixel_size(28)
            btn.set_child(img)

            # Click handler
            item_id = item["id"]
            if item_id == "launcher_toggle":
                btn.connect("clicked", lambda *_: self.toggle_launcher())
            else:
                btn.connect("clicked", lambda *_, app_id=item_id: self.open_app_by_id(app_id))

            self.dock_container.append(btn)
            self.dock_buttons.append({"button": btn, "image": img, "id": item_id})

        # Cursor distance magnification via Gtk.EventControllerMotion
        motion_ctrl = Gtk.EventControllerMotion()
        motion_ctrl.connect("motion", self._on_dock_motion)
        motion_ctrl.connect("leave", self._on_dock_leave)
        self.dock_container.add_controller(motion_ctrl)

        return dock_wrapper

    def _on_dock_motion(self, controller, x, y):
        """
        Cursor-distance magnification:
        Calculates distance from cursor x to center of each dock tile.
        Tiles within influence radius smoothly scale up to 1.35x.
        """
        influence_radius = 110.0
        for entry in self.dock_buttons:
            btn = entry["button"]
            img = entry["image"]

            alloc = btn.get_allocation()
            tile_center_x = alloc.x + (alloc.width / 2.0)
            dist = abs(x - tile_center_x)

            if dist < influence_radius:
                factor = math.cos((dist / influence_radius) * (math.pi / 2.0))
                scale = 1.0 + (0.35 * (factor ** 2))
            else:
                scale = 1.0

            # Apply smooth size updates
            icon_size = int(round(28 * scale))
            img.set_pixel_size(icon_size)

            tile_w = int(round(56 * scale))
            btn.set_size_request(tile_w, 56)

    def _on_dock_leave(self, controller):
        """Reset all dock tiles when cursor exits the dock"""
        for entry in self.dock_buttons:
            entry["image"].set_pixel_size(28)
            entry["button"].set_size_request(56, 56)

    # -------------------------------------------------------------------------
    # Full-Screen Frosted Launcher Overlay
    # Pre-blurred Cairo background, focused search, 5-app recent row,
    # FlowBox 5-6 columns, 2x2 tile folders opening in Gtk.Popover,
    # 250ms fade + 0.96 -> 1.0 scale animation.
    # -------------------------------------------------------------------------
    def _build_launcher_overlay(self):
        overlay = Gtk.Overlay()
        overlay.add_css_class("launcher-window")

        # Background Drawing Area for Pre-blurred Cairo Wallpaper Cache
        self.launcher_bg = Gtk.DrawingArea()
        self.launcher_bg.set_draw_func(self._draw_launcher_blurred_bg)
        overlay.set_child(self.launcher_bg)

        # Scrolled container for launcher content
        self.launcher_scrolled = Gtk.ScrolledWindow()
        self.launcher_scrolled.set_hexpand(True)
        self.launcher_scrolled.set_vexpand(True)
        self.launcher_scrolled.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        overlay.add_overlay(self.launcher_scrolled)

        # Content clamp to maintain elegant width
        clamp = Adw.Clamp(maximum_size=880, tightening_threshold=640)
        self.launcher_scrolled.set_child(clamp)

        self.launcher_content_box = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=20,
            margin_top=48,
            margin_bottom=64,
            margin_start=32,
            margin_end=32
        )
        clamp.set_child(self.launcher_content_box)

        # Search Entry with instant live filtering
        search_box = Gtk.Box(halign=Gtk.Align.CENTER)
        search_box.set_size_request(520, -1)
        search_box.add_css_class("launcher-search")

        self.search_entry = Gtk.SearchEntry(placeholder_text="Search applications, files, and settings...")
        self.search_entry.set_hexpand(True)
        self.search_entry.connect("search-changed", self._on_search_changed)
        self.search_entry.connect("stop-search", lambda *_: self.close_launcher())
        search_box.append(self.search_entry)
        self.launcher_content_box.append(search_box)

        # ---------------- Recent Apps Section (5 Apps) ----------------
        self.recent_section = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        lbl_recent = Gtk.Label(label="Recent Applications", halign=Gtk.Align.START)
        lbl_recent.add_css_class("title-4")
        lbl_recent.add_css_class("dim-label")
        self.recent_section.append(lbl_recent)

        self.recent_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=14)
        self.recent_section.append(self.recent_row)
        self.launcher_content_box.append(self.recent_section)
        self._populate_recent_apps()

        # ---------------- Folders Section ----------------
        self.folders_section = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        lbl_folders = Gtk.Label(label="Collections & Folders", halign=Gtk.Align.START)
        lbl_folders.add_css_class("title-4")
        lbl_folders.add_css_class("dim-label")
        self.folders_section.append(lbl_folders)

        folders_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=18)
        for f in FOLDERS:
            folder_widget = self._create_folder_tile(f)
            folders_row.append(folder_widget)
        self.folders_section.append(folders_row)
        self.launcher_content_box.append(self.folders_section)

        # ---------------- Main Applications Grid ----------------
        self.grid_section = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        lbl_all = Gtk.Label(label="All Applications", halign=Gtk.Align.START)
        lbl_all.add_css_class("title-4")
        lbl_all.add_css_class("dim-label")
        self.grid_section.append(lbl_all)

        # FlowBox with 5-6 columns grid
        self.app_flowbox = Gtk.FlowBox(
            selection_mode=Gtk.SelectionMode.NONE,
            min_children_per_line=5,
            max_children_per_line=6,
            column_spacing=16,
            row_spacing=18,
            homogeneous=True
        )
        self.grid_section.append(self.app_flowbox)
        self.launcher_content_box.append(self.grid_section)

        self.flowbox_app_widgets = []
        for app_info in APPLICATIONS:
            tile = self._create_app_tile(app_info)
            self.app_flowbox.append(tile)
            self.flowbox_app_widgets.append({"widget": tile, "info": app_info})

        return overlay

    def _draw_launcher_blurred_bg(self, area, cr, width, height):
        """Draws cached pre-blurred Cairo surface overlaid with surface token"""
        if not cairo:
            return
        is_dark = self.theme_mgr.style_manager.get_dark() if self.theme_mgr.style_manager else True
        surf = self.blurred_cache.get_surface(width, height, is_dark)
        if surf:
            cr.set_source_surface(surf, 0, 0)
            cr.paint()

    def _populate_recent_apps(self):
        """Fills the 5-app Recent row"""
        # Clear previous
        while child := self.recent_row.get_first_child():
            self.recent_row.remove(child)

        count = 0
        for app_id in self.recent_apps:
            if count >= 5:
                break
            app_info = next((a for a in APPLICATIONS if a["id"] == app_id), None)
            if app_info:
                tile = self._create_app_tile(app_info, is_compact=True)
                self.recent_row.append(tile)
                count += 1

    def _create_app_tile(self, app_info, is_compact=False):
        """
        Creates an app tile with 22px radius, faint accent-tinted fill, 1px outline.
        """
        btn = Gtk.Button()
        btn.add_css_class("app-tile")
        btn.set_tooltip_text(app_info.get("desc", app_info["name"]))

        width = 110 if is_compact else 128
        height = 96 if is_compact else 114
        btn.set_size_request(width, height)

        vbox = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=8,
            halign=Gtk.Align.CENTER,
            valign=Gtk.Align.CENTER
        )

        icon_size = 32 if is_compact else 38
        img = Gtk.Image.new_from_icon_name(app_info["icon"])
        img.set_pixel_size(icon_size)
        vbox.append(img)

        lbl = Gtk.Label(label=app_info["name"])
        lbl.add_css_class("title-4")
        lbl.set_ellipsize(Pango.EllipsizeMode.END)
        lbl.set_max_width_chars(12)
        vbox.append(lbl)

        btn.set_child(vbox)
        btn.connect("clicked", lambda *_: self.open_app_by_id(app_info["id"]))
        return btn

    def _create_folder_tile(self, folder_info):
        """
        Creates a 2x2 preview tile folder opening in a Gtk.Popover.
        """
        btn = Gtk.Button()
        btn.add_css_class("folder-tile")
        btn.set_size_request(128, 128)
        btn.set_tooltip_text(folder_info["name"])

        vbox = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=8,
            halign=Gtk.Align.CENTER,
            valign=Gtk.Align.CENTER
        )

        # 2x2 preview grid
        grid = Gtk.Grid(column_spacing=6, row_spacing=6)
        grid.add_css_class("folder-preview-grid")
        grid.set_size_request(52, 52)
        grid.set_halign(Gtk.Align.CENTER)

        apps = folder_info["apps"]
        for idx in range(min(4, len(apps))):
            app = apps[idx]
            sub_img = Gtk.Image.new_from_icon_name(app["icon"])
            sub_img.set_pixel_size(18)
            grid.attach(sub_img, idx % 2, idx // 2, 1, 1)

        vbox.append(grid)

        lbl = Gtk.Label(label=folder_info["name"])
        lbl.add_css_class("title-4")
        lbl.set_ellipsize(Pango.EllipsizeMode.END)
        lbl.set_max_width_chars(14)
        vbox.append(lbl)

        btn.set_child(vbox)

        # Popover on click
        popover = Gtk.Popover()
        popover.add_css_class("folder-popover")
        popover.set_parent(btn)
        popover.set_has_arrow(True)

        pbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        pbox.set_size_request(280, -1)

        top_h = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)
        p_title = Gtk.Label(label=folder_info["name"], halign=Gtk.Align.START)
        p_title.add_css_class("title-3")
        p_title.set_hexpand(True)
        top_h.append(p_title)

        p_close = Gtk.Button(icon_name="window-close-symbolic")
        p_close.add_css_class("flat")
        p_close.connect("clicked", lambda *_: popover.popdown())
        top_h.append(p_close)
        pbox.append(top_h)

        # Inner app list
        for item in folder_info["apps"]:
            row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
            row.add_css_class("list-row")
            row_img = Gtk.Image.new_from_icon_name(item["icon"])
            row_img.set_pixel_size(24)
            row.append(row_img)

            txt_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)
            lbl_name = Gtk.Label(label=item["name"], halign=Gtk.Align.START)
            lbl_name.add_css_class("title-4")
            txt_box.append(lbl_name)

            lbl_d = Gtk.Label(label=item["desc"], halign=Gtk.Align.START)
            lbl_d.add_css_class("caption")
            txt_box.append(lbl_d)
            txt_box.set_hexpand(True)
            row.append(txt_box)

            click_ctrl = Gtk.GestureClick()
            click_ctrl.connect("released", lambda *_, info=item: self._on_folder_app_click(popover, info))
            row.add_controller(click_ctrl)

            pbox.append(row)

        popover.set_child(pbox)
        btn.connect("clicked", lambda *_: popover.popup())
        return btn

    def _on_folder_app_click(self, popover, app_info):
        popover.popdown()
        self.close_launcher()
        self.open_app_by_id(app_info["id"])

    # -------------------------------------------------------------------------
    # Live Search Filtering
    # -------------------------------------------------------------------------
    def _on_search_changed(self, entry):
        query = entry.get_text().strip().lower()
        if not query:
            # Show recent section and folders
            self.recent_section.set_visible(True)
            self.folders_section.set_visible(True)
            for item in self.flowbox_app_widgets:
                item["widget"].set_visible(True)
        else:
            # Hide recent and folders during search
            self.recent_section.set_visible(False)
            self.folders_section.set_visible(False)

            for item in self.flowbox_app_widgets:
                info = item["info"]
                match = (
                    query in info["name"].lower()
                    or query in info.get("desc", "").lower()
                    or query in info.get("category", "").lower()
                )
                item["widget"].set_visible(match)

    # -------------------------------------------------------------------------
    # Launcher Animation (250ms fade + 0.96 -> 1.0 scale respecting animations)
    # -------------------------------------------------------------------------
    def toggle_launcher(self):
        if self.launcher_overlay.get_visible():
            self.close_launcher()
        else:
            self.open_launcher()

    def open_launcher(self):
        self.launcher_overlay.set_visible(True)
        self.search_entry.set_text("")
        self.search_entry.grab_focus()

        # Check gtk-enable-animations setting
        gtk_settings = Gtk.Settings.get_default()
        enable_anim = gtk_settings.get_property("gtk-enable-animations") if gtk_settings else True

        if not enable_anim:
            self.launcher_overlay.set_opacity(1.0)
            self.launcher_progress = 1.0
            return

        self._start_launcher_anim(target=1.0)

    def close_launcher(self):
        gtk_settings = Gtk.Settings.get_default()
        enable_anim = gtk_settings.get_property("gtk-enable-animations") if gtk_settings else True

        if not enable_anim:
            self.launcher_overlay.set_visible(False)
            self.launcher_overlay.set_opacity(0.0)
            self.launcher_progress = 0.0
            return

        self._start_launcher_anim(target=0.0)

    def _start_launcher_anim(self, target):
        if self.launcher_anim_id:
            GLib.source_remove(self.launcher_anim_id)
            self.launcher_anim_id = None

        self.launcher_anim_target = target
        self.launcher_anim_start = time.time()
        self.launcher_initial_progress = self.launcher_progress
        self.launcher_anim_id = GLib.timeout_add(16, self._step_launcher_anim)

    def _step_launcher_anim(self):
        duration = 0.250 # 250ms
        elapsed = time.time() - self.launcher_anim_start
        frac = min(1.0, elapsed / duration)

        # Smooth cubic ease-out
        eased = 1.0 - math.pow(1.0 - frac, 3)

        if self.launcher_anim_target == 1.0:
            self.launcher_progress = self.launcher_initial_progress + (1.0 - self.launcher_initial_progress) * eased
        else:
            self.launcher_progress = self.launcher_initial_progress * (1.0 - eased)

        # Fade opacity 0.0 -> 1.0
        self.launcher_overlay.set_opacity(self.launcher_progress)

        # Scale 0.96 -> 1.0 via margin adjustment
        scale_offset = int((1.0 - self.launcher_progress) * 24)
        self.launcher_content_box.set_margin_top(48 + scale_offset)

        if frac >= 1.0:
            self.launcher_anim_id = None
            if self.launcher_anim_target == 0.0:
                self.launcher_overlay.set_visible(False)
            return False
        return True

    def _on_key_pressed(self, controller, keyval, keycode, state):
        # Escape key closes launcher
        if keyval == Gdk.KEY_Escape:
            if self.launcher_overlay.get_visible():
                self.close_launcher()
                return True
        # Super key or F4 toggles launcher
        elif keyval in (Gdk.KEY_Super_L, Gdk.KEY_Super_R, Gdk.KEY_F4):
            self.toggle_launcher()
            return True
        return False

    def _on_theme_changed(self, is_dark):
        """
        Handles dynamic ThemeManager scheme changes without restart:
        Clears Cairo blurred cache and requests redraw.
        """
        self.blurred_cache.clear()
        self.bg_area.queue_draw()
        self.launcher_bg.queue_draw()

    # -------------------------------------------------------------------------
    # App Execution Handlers
    # -------------------------------------------------------------------------
    def open_app_by_id(self, app_id):
        # Add to recent
        if app_id in self.recent_apps:
            self.recent_apps.remove(app_id)
        self.recent_apps.insert(0, app_id)
        self._populate_recent_apps()

        # If launcher is open, close it
        if self.launcher_overlay.get_visible():
            self.close_launcher()

        # Direct window handling for built-in tools
        if app_id == "settings":
            win = SettingsWindow(self.get_application())
            win.present()
        elif app_id == "installer":
            win = InstallerWindow(self.get_application())
            win.present()
        elif app_id == "terminal":
            for term_cmd in ["axis-terminal", "/usr/local/bin/axis-terminal", "/usr/bin/axis-terminal", "x-terminal-emulator"]:
                if shutil.which(term_cmd) or os.path.exists(term_cmd):
                    try:
                        subprocess.Popen([term_cmd])
                        return
                    except Exception as e:
                        print(f"[Shell] Terminal launch error: {e}")
        else:
            # Find in catalog
            app_info = next((a for a in APPLICATIONS if a["id"] == app_id), None)
            cmd = app_info["exec"] if app_info else app_id
            try:
                subprocess.Popen(cmd, shell=True)
            except Exception as e:
                print(f"[Shell] Launched {cmd}: {e}")


class AxisDesktopApplication(Adw.Application):
    def __init__(self):
        super().__init__(
            application_id="org.axisos.desktop.shell",
            flags=Gio.ApplicationFlags.FLAGS_NONE
        )

    def do_startup(self):
        Adw.Application.do_startup(self)
        ThemeManager.get_default().init_app(self)

    def do_activate(self):
        win = self.props.active_window
        if not win:
            win = HarmonyDesktopShell(self)
        win.present()


def main():
    app = AxisDesktopApplication()
    return app.run(sys.argv)


if __name__ == "__main__":
    sys.exit(main())
