#!/usr/bin/env python3
"""
AxisOS HarmonyOS-Inspired Settings Application
Built with GTK 4 and libadwaita.
Supports live System / Light / Dark switching, persisted in ~/.config/axis/appearance.conf.
"""

import sys
import os
import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
from gi.repository import Gtk, Adw, Gdk, Gio

sys.path.insert(0, os.path.dirname(__file__))
from theme_manager import ThemeManager, get_saved_color_scheme
from hardware_info import HardwareInfo


class SettingsWindow(Adw.PreferencesWindow):
    def __init__(self, app):
        super().__init__(
            application=app,
            title="Settings",
            default_width=720,
            default_height=580
        )

        self.theme_mgr = ThemeManager.get_default()

        # ---------------------------------------------------------------------
        # Page 1: Appearance & Desktop
        # ---------------------------------------------------------------------
        page_appear = Adw.PreferencesPage(
            title="Appearance",
            icon_name="weather-clear-symbolic"
        )
        self.add(page_appear)

        group_theme = Adw.PreferencesGroup(
            title="Color Scheme",
            description="Choose how AxisOS appears across all native applications."
        )
        page_appear.add(group_theme)

        # Combo row for System, Light, Dark
        self.scheme_row = Adw.ComboRow(title="Theme Mode")
        modes = ["System", "Light", "Dark"]
        self.scheme_row.set_model(Gtk.StringList.new(modes))

        saved = get_saved_color_scheme()
        if saved == "light":
            self.scheme_row.set_selected(1)
        elif saved == "dark":
            self.scheme_row.set_selected(2)
        else:
            self.scheme_row.set_selected(0)

        self.scheme_row.connect("notify::selected", self.on_scheme_selected)
        group_theme.add(self.scheme_row)

        # Accent Color Info Row
        group_accent = Adw.PreferencesGroup(
            title="Monochrome Harmony Palette",
            description="Active color tokens and verified WCAG contrast."
        )
        page_appear.add(group_accent)

        row_primary = Adw.ActionRow(
            title="Primary Accent",
            subtitle="Crisp White (#FFFFFF in Dark / Deep Jet #0F172A in Light)"
        )
        group_accent.add(row_primary)

        row_secondary = Adw.ActionRow(
            title="Secondary Accent",
            subtitle="Titanium Slate (#94A3B8 in Dark / Balanced Slate #64748B in Light)"
        )
        group_accent.add(row_secondary)

        # Dock & Launcher Tuning
        group_dock = Adw.PreferencesGroup(
            title="Harmony Dock &amp; Launcher",
            description="Configure desktop magnification and launcher behaviors."
        )
        page_appear.add(group_dock)

        row_dock_mag = Adw.ActionRow(title="Dock Magnification Effect")
        switch_mag = Gtk.Switch(active=True, valign=Gtk.Align.CENTER)
        row_dock_mag.add_suffix(switch_mag)
        group_dock.add(row_dock_mag)

        row_blur = Adw.ActionRow(title="Frosted Wallpaper Blur", subtitle="Pre-rendered wallpaper blur with Cairo")
        switch_blur = Gtk.Switch(active=True, valign=Gtk.Align.CENTER)
        row_blur.add_suffix(switch_blur)
        group_dock.add(row_blur)

        # ---------------------------------------------------------------------
        # Page 2: Screen Lock & Biometrics (Fingerprint, Iris & Passwordless)
        # ---------------------------------------------------------------------
        page_security = Adw.PreferencesPage(
            title="Security & Lock",
            icon_name="emblem-ok-symbolic"
        )
        self.add(page_security)

        group_lock = Adw.PreferencesGroup(
            title="Screen Lock &amp; Biometric Unlock",
            description="Unlock with Fingerprint (fprintd), Infrared Iris scan (V4L2 IR), or disable the lock screen password."
        )
        page_security.add(group_lock)

        self.lock_mode_row = Adw.ComboRow(title="Screen Lock Method")
        lock_modes = [
            "Biometrics (Fingerprint / Iris) or Password",
            "Fingerprint Unlock Only (No Password Prompt)",
            "Iris / IR Scanner Unlock Only (No Password Prompt)",
            "Password / PIN Only",
            "No Password (Click / Slide to Unlock)"
        ]
        self.lock_mode_row.set_model(Gtk.StringList.new(lock_modes))
        self.lock_mode_row.set_selected(0)
        self.lock_mode_row.connect("notify::selected", self.on_lock_mode_changed)
        group_lock.add(self.lock_mode_row)

        row_fp = Adw.ActionRow(
            title="Fingerprint Recognition (fprintd)",
            subtitle="Goodix, Synaptics, ELAN, Validity &amp; Broadcom sensors"
        )
        self.sw_fp = Gtk.Switch(active=True, valign=Gtk.Align.CENTER)
        btn_enroll_fp = Gtk.Button(label="Enroll Fingerprint", valign=Gtk.Align.CENTER)
        btn_enroll_fp.connect("clicked", self.on_enroll_fingerprint)
        row_fp.add_suffix(self.sw_fp)
        row_fp.add_suffix(btn_enroll_fp)
        group_lock.add(row_fp)

        row_iris = Adw.ActionRow(
            title="Iris &amp; Infrared Eye Recognition (V4L2 IR)",
            subtitle="Unlock hands-free using infrared iris or camera scan"
        )
        self.sw_iris = Gtk.Switch(active=True, valign=Gtk.Align.CENTER)
        btn_enroll_iris = Gtk.Button(label="Enroll Iris", valign=Gtk.Align.CENTER)
        btn_enroll_iris.connect("clicked", self.on_enroll_iris)
        row_iris.add_suffix(self.sw_iris)
        row_iris.add_suffix(btn_enroll_iris)
        group_lock.add(row_iris)

        # ---------------------------------------------------------------------
        # Page 3: About System
        # ---------------------------------------------------------------------
        page_about = Adw.PreferencesPage(
            title="About",
            icon_name="preferences-system-symbolic"
        )
        self.add(page_about)

        specs = HardwareInfo.get_specs_dict()

        # Hardware Specifications Group
        group_hw = Adw.PreferencesGroup(
            title="Hardware Specifications",
            description="Verified components and hardware telemetry."
        )
        page_about.add(group_hw)

        row_model = Adw.ActionRow(title="Device Model", subtitle=specs["device_model"])
        group_hw.add(row_model)

        row_cpu = Adw.ActionRow(title="Processor", subtitle=specs["processor"])
        group_hw.add(row_cpu)

        row_mem = Adw.ActionRow(title="Memory (RAM)", subtitle=specs["memory"])
        group_hw.add(row_mem)

        row_gpu = Adw.ActionRow(title="Graphics (GPU)", subtitle=specs["graphics"])
        group_hw.add(row_gpu)

        row_disk = Adw.ActionRow(title="Disk Capacity", subtitle=specs["storage"])
        group_hw.add(row_disk)

        # Operating System Details Group
        group_os = Adw.PreferencesGroup(
            title="System &amp; Software Details",
            description="Operating system version, kernel, and desktop session."
        )
        page_about.add(group_os)

        group_os.add(Adw.ActionRow(title="Operating System", subtitle=specs["os_name"]))
        group_os.add(Adw.ActionRow(title="OS Architecture", subtitle=specs["os_type"]))
        group_os.add(Adw.ActionRow(title="Linux Kernel", subtitle=specs["kernel"]))
        group_os.add(Adw.ActionRow(title="Windowing System", subtitle=specs["windowing"]))
        group_os.add(Adw.ActionRow(title="Desktop Environment", subtitle=specs["desktop"]))
        group_os.add(Adw.ActionRow(title="Icon Theme", subtitle="AxisHarmony 1.5px Stroke Symbolic Set"))

    def on_scheme_selected(self, row, pspec):
        idx = row.get_selected()
        mode_map = {0: "system", 1: "light", 2: "dark"}
        choice = mode_map.get(idx, "system")
        print(f"[Settings] Color scheme selected: {choice}")
        self.theme_mgr.set_user_color_scheme(choice)

    def on_lock_mode_changed(self, row, pspec):
        import subprocess
        idx = row.get_selected()
        mode_ids = {
            0: "biometric_or_password",
            1: "fingerprint_only",
            2: "iris_only",
            3: "password_only",
            4: "none"
        }
        mode = mode_ids.get(idx, "biometric_or_password")
        fp = "true" if self.sw_fp.get_active() else "false"
        iris = "true" if self.sw_iris.get_active() else "false"
        subprocess.Popen(["/usr/local/bin/axis-biometrics", "configure", "--mode", mode, "--fingerprint", fp, "--iris", iris])

    def on_enroll_fingerprint(self, btn):
        import subprocess
        subprocess.Popen(["/usr/local/bin/axis-biometrics", "enroll-fingerprint"])

    def on_enroll_iris(self, btn):
        import subprocess
        subprocess.Popen(["/usr/local/bin/axis-biometrics", "enroll-iris"])


class AxisSettingsApp(Adw.Application):
    def __init__(self):
        super().__init__(
            application_id="org.axisos.Settings",
            flags=Gio.ApplicationFlags.DEFAULT_FLAGS
        )
        self.connect("activate", self.on_activate)

    def on_activate(self, app):
        ThemeManager.get_default().init_app(app)
        win = SettingsWindow(app)
        win.present()


def main():
    app = AxisSettingsApp()
    return app.run(sys.argv)


if __name__ == "__main__":
    sys.exit(main())
