#!/usr/bin/env python3
"""
AxisOS Theme & Style Manager (HarmonyOS Architecture)
Manages single-source design tokens, Adw.StyleManager integration,
System / Light / Dark switching, Gtk.CssProvider lifecycles, and icon themes.
"""

import os
import configparser
import math
import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
from gi.repository import Gtk, Gdk, Adw, GLib

CONFIG_DIR = os.path.expanduser("~/.config/axis")
CONFIG_PATH = os.path.join(CONFIG_DIR, "appearance.conf")

def get_theme_dir():
    # Priority: local source tree first, system install second
    candidate = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "data", "theme"))
    if os.path.isdir(candidate):
        return candidate
    installed = "/usr/share/axis/theme"
    if os.path.isdir(installed):
        return installed
    return candidate

def get_icons_dir():
    candidate = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "data", "icons"))
    if os.path.isdir(candidate):
        return candidate
    installed = "/usr/share/icons/AxisHarmony"
    if os.path.isdir(installed):
        return installed
    return candidate

def read_config():
    config = configparser.ConfigParser()
    if os.path.exists(CONFIG_PATH):
        try:
            config.read(CONFIG_PATH)
        except Exception:
            pass
    if not config.has_section("Appearance"):
        config.add_section("Appearance")
    return config

def write_config(config):
    os.makedirs(CONFIG_DIR, exist_ok=True)
    with open(CONFIG_PATH, "w", encoding="utf-8") as f:
        config.write(f)

def get_saved_color_scheme():
    config = read_config()
    return config.get("Appearance", "color_scheme", fallback="system").lower()

def set_saved_color_scheme(scheme):
    config = read_config()
    config.set("Appearance", "color_scheme", scheme.lower())
    write_config(config)

def get_saved_wallpaper():
    config = read_config()
    return config.get("Appearance", "wallpaper", fallback="")

def set_saved_wallpaper(path):
    config = read_config()
    config.set("Appearance", "wallpaper", path)
    write_config(config)

# Relative luminance and contrast calculation according to WCAG 2.1
def channel_lum(c):
    c_s = c / 255.0
    return c_s / 12.92 if c_s <= 0.03928 else ((c_s + 0.055) / 1.055) ** 2.4

def relative_luminance(r, g, b):
    return 0.2126 * channel_lum(r) + 0.7152 * channel_lum(g) + 0.0722 * channel_lum(b)

def contrast_ratio(lum1, lum2):
    lighter = max(lum1, lum2)
    darker = min(lum1, lum2)
    return (lighter + 0.05) / (darker + 0.05)


class ThemeManager:
    _instance = None

    @classmethod
    def get_default(cls):
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance

    def __init__(self):
        self.current_provider = None
        self.style_manager = None
        self.display = None
        self.current_dark = None
        self.on_scheme_changed_callbacks = []

    def init_app(self, app=None):
        self.display = Gdk.Display.get_default()
        self.style_manager = Adw.StyleManager.get_default()

        # 1. Register icon search paths and icon theme
        icons_dir = get_icons_dir()
        if self.display:
            icon_theme = Gtk.IconTheme.get_for_display(self.display)
            icon_theme.add_search_path(icons_dir)
            icon_theme.add_search_path(os.path.join(icons_dir, "AxisHarmony"))
            gtk_settings = Gtk.Settings.get_for_display(self.display)
            if gtk_settings:
                gtk_settings.set_property("gtk-icon-theme-name", "AxisHarmony")

        # 2. Read saved preference: system, light, dark
        saved_scheme = get_saved_color_scheme()
        self.apply_color_scheme_preference(saved_scheme)

        # 3. Listen to style manager notify::dark
        self.style_manager.connect("notify::dark", self._on_dark_changed)

        # 4. Apply initial stylesheet
        self.apply_stylesheet(self.style_manager.get_dark())

    def connect_scheme_changed(self, callback):
        self.on_scheme_changed_callbacks.append(callback)

    def _on_dark_changed(self, style_manager, pspec):
        is_dark = style_manager.get_dark()
        self.apply_stylesheet(is_dark)
        for cb in self.on_scheme_changed_callbacks:
            try:
                cb(is_dark)
            except Exception as e:
                print(f"[ThemeManager] Callback error: {e}")

    def apply_color_scheme_preference(self, scheme):
        if not self.style_manager:
            self.style_manager = Adw.StyleManager.get_default()
        scheme = scheme.lower()
        if scheme == "light":
            self.style_manager.set_color_scheme(Adw.ColorScheme.FORCE_LIGHT)
        elif scheme == "dark":
            self.style_manager.set_color_scheme(Adw.ColorScheme.FORCE_DARK)
        else:
            self.style_manager.set_color_scheme(Adw.ColorScheme.DEFAULT)

    def set_user_color_scheme(self, scheme):
        set_saved_color_scheme(scheme)
        self.apply_color_scheme_preference(scheme)

    def calculate_wallpaper_accent(self, is_dark):
        """
        Calculates wallpaper-derived accent color.
        Ensures contrast against the base color (#0E1411 in dark, #F4F7F3 in light).
        Falls back to theme accent (#8BC34A in dark, #4E8A1E in light).
        """
        wall_path = get_saved_wallpaper()
        base_lum = relative_luminance(14, 20, 17) if is_dark else relative_luminance(244, 247, 243)
        fallback_accent = "#8BC34A" if is_dark else "#4E8A1E"

        if not wall_path or not os.path.exists(wall_path):
            return fallback_accent

        # If wallpaper sampling is available, calculate dominant RGB and verify contrast
        try:
            from PIL import Image
            with Image.open(wall_path) as im:
                im = im.resize((32, 32))
                colors = im.getcolors(32 * 32)
                if colors:
                    dominant = max(colors, key=lambda item: item[0])[1]
                    r, g, b = dominant[0], dominant[1], dominant[2]
                    lum = relative_luminance(r, g, b)
                    contrast = contrast_ratio(lum, base_lum)
                    if contrast >= 4.5:
                        return f"#{r:02x}{g:02x}{b:02x}"
        except Exception:
            pass
        return fallback_accent

    def apply_stylesheet(self, is_dark):
        if not self.display:
            self.display = Gdk.Display.get_default()
        if not self.display:
            return

        theme_dir = get_theme_dir()
        sheet_filename = "axis-dark.css" if is_dark else "axis-light.css"
        sheet_path = os.path.join(theme_dir, sheet_filename)

        if not os.path.exists(sheet_path):
            print(f"[ThemeManager] Warning: {sheet_path} not found.")
            return

        # Crucial requirement: Remove old provider before adding new one so they never stack
        if self.current_provider is not None:
            Gtk.StyleContext.remove_provider_for_display(self.display, self.current_provider)
            self.current_provider = None

        new_provider = Gtk.CssProvider()
        try:
            new_provider.load_from_path(sheet_path)
            Gtk.StyleContext.add_provider_for_display(
                self.display,
                new_provider,
                Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
            )
            self.current_provider = new_provider
            self.current_dark = is_dark
        except Exception as e:
            print(f"[ThemeManager] Error loading stylesheet {sheet_path}: {e}")
