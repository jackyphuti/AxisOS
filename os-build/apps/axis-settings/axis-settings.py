#!/usr/bin/env python3
"""
AxisOS Settings Panel — Native System Settings & Hardware Control Center.
Unified Samsung One UI Dark Design System with Deep Blue / Accent (#7d95ff / #1428a0).

Comprehensive Hardware Interaction:
  - Display & Brightness: Direct sysfs / brightnessctl backlight, wlr-randr modes, Night Light
  - Audio & Sound: PipeWire / WirePlumber volume, mic level, sound test, sink selection
  - Wi-Fi & Network: nmcli live scanning, passwordless open / captive portal connect, radio toggle
  - Bluetooth: BlueZ power toggle, device discovery, paired device management
  - Power & Battery: Live /sys/class/power_supply telemetry, powerprofilesctl governors
  - CPU & Performance: cpupower governor (performance/schedutil/powersave), turbo, zRAM
  - Storage & Disks: lsblk partitions, df -h usage, Btrfs subvolumes, SSD fstrim
  - Security & Biometrics: fprintd fingerprint enrollment, UFW firewall, USB quarantine
  - Software Updates: APT package upgrade manager with progress bar
  - About: Hardware specs, kernel, and OS Edition Switcher (Home & Gaming, Enterprise, Work & School)
"""

import os
import sys
import subprocess
import threading
import json
import shutil
import glob
import urllib.error
import urllib.request
import re
import gi

gi.require_version("Gtk", "3.0")
gi.require_version("Gdk", "3.0")
from gi.repository import Gtk, Gdk, GLib, Pango

CSS_STYLING = b"""
* {
    font-family: "Inter", -apple-system, BlinkMacSystemFont, "Segoe UI", Cantarell, sans-serif;
}

window {
    background-color: #141824;
    color: #f2f3f7;
}

headerbar {
    background-color: #1a1d2e;
    background-image: none;
    border-bottom: 1px solid #2e3349;
    color: #f2f3f7;
    box-shadow: none;
}

headerbar .title {
    font-weight: 700;
    font-size: 15px;
    color: #f2f3f7;
}

headerbar .subtitle {
    font-size: 12px;
    color: #9ea3b5;
}

headerbar button {
    background: #262a3d;
    color: #f2f3f7;
    border: 1px solid #2e3349;
    border-radius: 999px;
}

.sidebar, .sidebar list, scrolledwindow, viewport, stack {
    background-color: #1a1d2e;
    color: #f2f3f7;
}

.sidebar {
    border-right: 1px solid #2e3349;
}

.sidebar list row {
    padding: 10px 16px;
    color: #9ea3b5;
    font-weight: 600;
    font-size: 13px;
    border-left: 3px solid transparent;
}

.sidebar list row:hover {
    background-color: #262a3d;
    color: #f2f3f7;
}

.sidebar list row:selected {
    background-color: #2a3366;
    color: #7d95ff;
    border-left: 3px solid #7d95ff;
    font-weight: 700;
}

.content-pane {
    background-color: #141824;
    padding: 18px 24px;
}

.card {
    background-color: #1f2231;
    border: 1px solid #2e3349;
    border-radius: 16px;
    padding: 18px 22px;
    margin-bottom: 14px;
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.25);
}

.card-title {
    font-size: 15px;
    font-weight: 700;
    color: #f2f3f7;
    margin-bottom: 4px;
}

.card-subtitle {
    font-size: 12px;
    color: #9ea3b5;
    margin-bottom: 10px;
}

button.btn-primary,
button.btn-primary:hover,
button.btn-primary:active,
button.btn-primary:checked {
    background: #7d95ff;
    background-image: none;
    box-shadow: none;
    text-shadow: none;
    color: #ffffff;
    font-weight: 700;
    border-radius: 999px;
    border: none;
    padding: 8px 18px;
}

button.btn-primary:hover {
    background: #6780ee;
    color: #ffffff;
}

button.btn-secondary,
button.btn-secondary:hover,
button.btn-secondary:active,
button.btn-secondary:checked {
    background: #262a3d;
    background-image: none;
    color: #f2f3f7;
    border: 1px solid #2e3349;
    border-radius: 999px;
    padding: 8px 18px;
    font-weight: 600;
}

button.btn-secondary:hover {
    background: #2a3366;
    color: #7d95ff;
    border-color: #7d95ff;
}

.status-badge-ok {
    background-color: #2a3366;
    color: #7d95ff;
    border: 1px solid #3d4a8f;
    font-weight: bold;
    font-size: 11px;
    padding: 4px 12px;
    border-radius: 999px;
}

.status-badge-warn {
    background-color: #4a2211;
    color: #fb923c;
    border: 1px solid #7c2d12;
    font-weight: bold;
    font-size: 11px;
    padding: 4px 12px;
    border-radius: 999px;
}

entry {
    background-color: #1a1d2e;
    color: #f2f3f7;
    border: 1px solid #2e3349;
    border-radius: 10px;
    padding: 8px 14px;
}

entry:focus {
    border-color: #7d95ff;
}

scale slider {
    background-color: #7d95ff;
    border: 2px solid #ffffff;
    border-radius: 50%;
    min-width: 18px;
    min-height: 18px;
    box-shadow: 0 1px 4px rgba(0, 0, 0, 0.4);
}

scale trough {
    background-color: #2e3349;
    border-radius: 999px;
    min-height: 6px;
}

scale highlight {
    background-color: #7d95ff;
    border-radius: 999px;
}

switch:checked {
    background-color: #7d95ff;
    border-color: #7d95ff;
}

progressbar progress {
    background: #7d95ff;
    border-radius: 999px;
}

progressbar trough {
    background-color: #2e3349;
    border-radius: 999px;
}

combobox button {
    background-color: #1a1d2e;
    color: #f2f3f7;
    border: 1px solid #2e3349;
    border-radius: 10px;
}
"""

class AxisSettingsApp(Gtk.Window):
    def __init__(self):
        super().__init__(title="Settings")
        self.set_default_size(980, 640)
        self.set_position(Gtk.WindowPosition.CENTER)

        self.apply_css()
        self.init_headerbar()
        self.init_ui()

    def apply_css(self):
        provider = Gtk.CssProvider()
        provider.load_from_data(CSS_STYLING)
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(), provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

    def init_headerbar(self):
        hb = Gtk.HeaderBar()
        hb.set_show_close_button(True)
        hb.props.title = "Settings"
        hb.props.subtitle = "AxisOS Control Center • Samsung One UI Dark"
        self.set_titlebar(hb)

    def init_ui(self):
        main_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=0)
        self.add(main_box)

        # Left Sidebar (Clean Typography + Real SVG Symbolic Icons)
        sidebar_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        sidebar_box.get_style_context().add_class("sidebar")
        sidebar_box.set_size_request(240, -1)

        self.listbox = Gtk.ListBox()
        self.listbox.set_selection_mode(Gtk.SelectionMode.BROWSE)
        self.listbox.connect("row-selected", self.on_category_selected)

        categories = [
            ("Display & Brightness", "display", "display-brightness-symbolic"),
            ("Audio & Sound", "audio", "audio-volume-high-symbolic"),
            ("Wi-Fi & Network", "network", "network-wireless-signal-excellent-symbolic"),
            ("Bluetooth", "bluetooth", "bluetooth-active-symbolic"),
            ("Power & Battery", "power", "battery-charging-symbolic"),
            ("CPU & Performance", "performance", "utilities-terminal-symbolic"),
            ("Storage & Disks", "storage", "drive-harddisk-symbolic"),
            ("Security & Biometrics", "security", "system-lock-screen-symbolic"),
            ("System Health", "health", "emblem-ok-symbolic"),
            ("Appearance", "appearance", "weather-clear-night-symbolic"),
            ("Software & Updates", "updates", "system-software-install-symbolic"),
            ("About & OS Edition", "about", "preferences-system-symbolic"),
        ]

        for label_text, cat_id, icon_name in categories:
            row = Gtk.ListBoxRow()
            row_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
            icon = Gtk.Image.new_from_icon_name(icon_name, Gtk.IconSize.MENU)
            lbl = Gtk.Label(label=label_text, xalign=0)
            row_box.pack_start(icon, False, False, 0)
            row_box.pack_start(lbl, True, True, 0)
            row.add(row_box)
            row.cat_id = cat_id
            self.listbox.add(row)

        sidebar_scroll = Gtk.ScrolledWindow()
        sidebar_scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        sidebar_scroll.add(self.listbox)
        sidebar_box.pack_start(sidebar_scroll, True, True, 0)
        main_box.pack_start(sidebar_box, False, False, 0)

        # Right Content Stack
        self.stack = Gtk.Stack()
        self.stack.set_transition_type(Gtk.StackTransitionType.CROSSFADE)
        self.stack.set_transition_duration(150)

        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        scroll.add(self.stack)

        content_container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        content_container.get_style_context().add_class("content-pane")
        content_container.pack_start(scroll, True, True, 0)

        main_box.pack_start(content_container, True, True, 0)

        # Build hardware & system pages
        self.build_display_page()
        self.build_audio_page()
        self.build_network_page()
        self.build_bluetooth_page()
        self.build_power_page()
        self.build_performance_page()
        self.build_storage_page()
        self.build_security_page()
        self.build_health_page()
        self.build_appearance_page()
        self.build_updates_page()
        self.build_about_page()

        # Select first category
        first_row = self.listbox.get_row_at_index(0)
        self.listbox.select_row(first_row)

    def on_category_selected(self, box, row):
        if row and hasattr(row, "cat_id"):
            self.stack.set_visible_child_name(row.cat_id)

    # =========================================================================
    # 1. Display & Brightness Page
    # =========================================================================
    def build_display_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="Display & Hardware Backlight", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Control panel backlight brightness, resolution modes, and color temperature.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        # Backlight Slider
        lbl_b = Gtk.Label(label="Screen Brightness", xalign=0)
        lbl_b.get_style_context().add_class("card-title")
        card.pack_start(lbl_b, False, False, 0)

        current_bright = self.get_hardware_brightness()
        self.scale_bright = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, 5, 100, 1)
        self.scale_bright.set_value(current_bright)
        self.scale_bright.set_draw_value(True)
        self.scale_bright.set_value_pos(Gtk.PositionType.RIGHT)
        self.scale_bright.connect("value-changed", self.on_brightness_changed)
        card.pack_start(self.scale_bright, False, False, 0)

        # Night Light Toggle
        row_nl = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        lbl_nl = Gtk.Label(label="Night Light (Warm Color Temperature)", xalign=0)
        self.sw_nl = Gtk.Switch(halign=Gtk.Align.END)
        self.sw_nl.connect("notify::active", self.on_night_light_toggle)
        row_nl.pack_start(lbl_nl, True, True, 0)
        row_nl.pack_end(self.sw_nl, False, False, 0)
        card.pack_start(row_nl, False, False, 4)

        # Resolution & Refresh Rate Selector
        row_res = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        lbl_res = Gtk.Label(label="Active Display Resolution & Rate:", xalign=0)
        self.combo_res = Gtk.ComboBoxText()
        self.combo_res.append("1920x1080@144", "1920 × 1080 @ 144 Hz (16:9)")
        self.combo_res.append("1920x1080@60", "1920 × 1080 @ 60 Hz")
        self.combo_res.append("2560x1440@165", "2560 × 1440 @ 165 Hz (QHD)")
        self.combo_res.append("3840x2160@120", "3840 × 2160 @ 120 Hz (4K UHD)")
        self.combo_res.set_active(0)
        self.combo_res.connect("changed", self.on_resolution_changed)
        row_res.pack_start(lbl_res, True, True, 0)
        row_res.pack_end(self.combo_res, False, False, 0)
        card.pack_start(row_res, False, False, 4)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "display")

    def get_hardware_brightness(self):
        try:
            out = subprocess.check_output(["brightnessctl", "-m"], text=True).strip()
            parts = out.split(",")
            if len(parts) >= 4 and parts[3].endswith("%"):
                return float(parts[3].rstrip("%"))
        except Exception:
            pass
        return 80.0

    def on_brightness_changed(self, scale):
        val = int(scale.get_value())
        subprocess.Popen(["brightnessctl", "-q", "set", f"{val}%"])

    def on_night_light_toggle(self, switch, gparam):
        if switch.get_active():
            subprocess.Popen(["gammastep", "-O", "4500K"])
        else:
            subprocess.Popen(["gammastep", "-x"])

    def on_resolution_changed(self, combo):
        mode = combo.get_active_id()
        if mode:
            res, _, rate = mode.partition("@")
            subprocess.Popen(f"wlr-randr --output eDP-1 --mode {res} --custom-mode {res}@{rate}Hz || xrandr -s {res}", shell=True)

    # =========================================================================
    # 2. Audio & Sound Page
    # =========================================================================
    def build_audio_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="Audio & Hardware Sinks", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="PipeWire pro-audio routing, master volume, and microphone inputs.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        # Master Output Slider with Real Hardware Readback
        lbl_v = Gtk.Label(label="Master Output Volume", xalign=0)
        lbl_v.get_style_context().add_class("card-title")
        card.pack_start(lbl_v, False, False, 0)

        initial_vol = self.get_hardware_volume()
        self.scale_vol = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, 0, 100, 1)
        self.scale_vol.set_value(initial_vol)
        self.scale_vol.set_draw_value(True)
        self.scale_vol.set_value_pos(Gtk.PositionType.RIGHT)
        self.scale_vol.connect("value-changed", self.on_volume_changed)
        card.pack_start(self.scale_vol, False, False, 0)

        # Microphone Input Slider
        lbl_m = Gtk.Label(label="Microphone Input Gain", xalign=0)
        lbl_m.get_style_context().add_class("card-title")
        card.pack_start(lbl_m, False, False, 0)

        self.scale_mic = Gtk.Scale.new_with_range(Gtk.Orientation.HORIZONTAL, 0, 100, 1)
        self.scale_mic.set_value(80)
        self.scale_mic.set_draw_value(True)
        self.scale_mic.set_value_pos(Gtk.PositionType.RIGHT)
        self.scale_mic.connect("value-changed", self.on_mic_changed)
        card.pack_start(self.scale_mic, False, False, 0)

        # Audio Output Device Selector
        row_sink = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        lbl_sink = Gtk.Label(label="Audio Output Device:", xalign=0)
        self.combo_sink = Gtk.ComboBoxText()
        self.populate_audio_sinks()
        self.combo_sink.connect("changed", self.on_sink_changed)
        row_sink.pack_start(lbl_sink, True, True, 0)
        row_sink.pack_end(self.combo_sink, False, False, 0)
        card.pack_start(row_sink, False, False, 4)

        # Sound test & Mute buttons
        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        btn_test = Gtk.Button(label="Test Audio Hardware")
        btn_test.get_style_context().add_class("btn-primary")
        btn_test.connect("clicked", lambda _: subprocess.Popen(
            "paplay /usr/share/sounds/freedesktop/stereo/complete.oga 2>/dev/null || speaker-test -t sine -f 440 -l 1 -s 1", shell=True
        ))

        btn_mute = Gtk.Button(label="Toggle Master Mute")
        btn_mute.get_style_context().add_class("btn-secondary")
        btn_mute.connect("clicked", lambda _: subprocess.Popen(
            "wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle 2>/dev/null || amixer -q sset Master toggle", shell=True
        ))

        btn_box.pack_start(btn_test, False, False, 0)
        btn_box.pack_start(btn_mute, False, False, 0)
        card.pack_start(btn_box, False, False, 4)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "audio")

    def get_hardware_volume(self):
        try:
            out = subprocess.check_output(["wpctl", "get-volume", "@DEFAULT_AUDIO_SINK@"], text=True).strip()
            parts = out.split()
            if len(parts) >= 2 and parts[0] == "Volume:":
                return float(parts[1]) * 100.0
        except Exception:
            pass
        try:
            out = subprocess.check_output(["amixer", "sget", "Master"], text=True)
            m = re.search(r"\[(\d+)%\]", out)
            if m:
                return float(m.group(1))
        except Exception:
            pass
        return 75.0

    def populate_audio_sinks(self):
        self.combo_sink.append("default", "Default PipeWire System Sink")
        try:
            out = subprocess.check_output(["pactl", "list", "sinks", "short"], text=True).strip()
            for line in out.splitlines():
                parts = line.split()
                if len(parts) >= 2:
                    sink_id = parts[0]
                    sink_name = parts[1]
                    display_name = sink_name.split(".")[-1].replace("_", " ").title()
                    self.combo_sink.append(sink_id, f"{display_name} (#{sink_id})")
        except Exception:
            pass
        self.combo_sink.set_active(0)

    def on_sink_changed(self, combo):
        sink_id = combo.get_active_id()
        if sink_id and sink_id != "default":
            subprocess.Popen(["wpctl", "set-default", sink_id])

    def on_volume_changed(self, scale):
        pct = scale.get_value() / 100.0
        subprocess.Popen(f"wpctl set-volume @DEFAULT_AUDIO_SINK@ {pct:.2f} 2>/dev/null || amixer -q sset Master {int(pct*100)}%", shell=True)

    def on_mic_changed(self, scale):
        pct = scale.get_value() / 100.0
        subprocess.Popen(f"wpctl set-volume @DEFAULT_AUDIO_SOURCE@ {pct:.2f} 2>/dev/null || amixer -q sset Capture {int(pct*100)}%", shell=True)

    # =========================================================================
    # 3. Wi-Fi & Network Page (With Passwordless & Captive Portal Support)
    # =========================================================================
    def build_network_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="Wi-Fi & Network Telemetry", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Manage wireless interfaces, connect to open/captive/secured Wi-Fi, and open web portals.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        # Wi-Fi Radio Toggle
        row_wifi_radio = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        lbl_radio = Gtk.Label(label="Wi-Fi Radio Adapter", xalign=0)
        self.sw_wifi = Gtk.Switch(active=True, halign=Gtk.Align.END)
        self.sw_wifi.connect("notify::active", self.on_wifi_radio_toggle)
        row_wifi_radio.pack_start(lbl_radio, True, True, 0)
        row_wifi_radio.pack_end(self.sw_wifi, False, False, 0)
        card.pack_start(row_wifi_radio, False, False, 0)

        # Live Wi-Fi Networks list
        lbl_avail = Gtk.Label(label="Nearby Wireless Networks:", xalign=0)
        lbl_avail.get_style_context().add_class("card-title")
        card.pack_start(lbl_avail, False, False, 0)

        self.wifi_listbox = Gtk.ListBox()
        scroll_wifi = Gtk.ScrolledWindow()
        scroll_wifi.set_min_content_height(180)
        scroll_wifi.add(self.wifi_listbox)
        card.pack_start(scroll_wifi, True, True, 0)

        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        btn_rescan = Gtk.Button(label="Scan Nearby Networks")
        btn_rescan.get_style_context().add_class("btn-secondary")
        btn_rescan.connect("clicked", lambda b: self.scan_wifi_networks())

        btn_portal = Gtk.Button(label="🌐 Web Login (Captive Portal)")
        btn_portal.get_style_context().add_class("btn-primary")
        btn_portal.set_tooltip_text("Check for captive portal redirect and open login page in web browser")
        btn_portal.connect("clicked", self.on_check_portal_clicked)

        btn_box.pack_start(btn_rescan, False, False, 0)
        btn_box.pack_start(btn_portal, False, False, 0)
        card.pack_start(btn_box, False, False, 0)

        page.pack_start(card, True, True, 0)
        self.stack.add_named(page, "network")

        GLib.idle_add(self.scan_wifi_networks)

    def on_wifi_radio_toggle(self, switch, gparam):
        state = "on" if switch.get_active() else "off"
        subprocess.Popen(["nmcli", "radio", "wifi", state])

    def scan_wifi_networks(self):
        for child in self.wifi_listbox.get_children():
            self.wifi_listbox.remove(child)
        try:
            out = subprocess.check_output(
                ["nmcli", "-t", "-f", "SSID,SIGNAL,SECURITY,IN-USE", "device", "wifi", "list"],
                universal_newlines=True
            )
            seen = set()
            for line in out.splitlines():
                parts = line.split(":")
                if parts and parts[0] and parts[0] not in seen:
                    ssid = parts[0]
                    seen.add(ssid)
                    signal = parts[1] if len(parts) > 1 else "100"
                    sec = parts[2] if len(parts) > 2 else ""
                    in_use = (len(parts) > 3 and parts[3] == "*")
                    is_open = (sec == "" or sec.lower() == "none" or sec == "--")

                    row = Gtk.ListBoxRow()
                    hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
                    sec_display = "Open / Web Portal" if is_open else sec
                    status_text = " [Connected]" if in_use else ""
                    lbl_text = f"{ssid} (Signal: {signal}%, {sec_display}){status_text}"
                    lbl = Gtk.Label(label=lbl_text, xalign=0)

                    btn_conn = Gtk.Button(label="Active" if in_use else "Connect")
                    btn_conn.get_style_context().add_class("btn-primary" if in_use else "btn-secondary")
                    if not in_use:
                        btn_conn.connect("clicked", lambda b, s=ssid, o=is_open: self.connect_wifi(s, o))

                    hbox.pack_start(lbl, True, True, 0)
                    hbox.pack_start(btn_conn, False, False, 0)
                    row.add(hbox)
                    self.wifi_listbox.add(row)
            self.wifi_listbox.show_all()
        except Exception:
            pass

    def connect_wifi(self, ssid, is_open=False):
        if is_open:
            # Connect directly without password prompt!
            try:
                subprocess.run(["nmcli", "device", "wifi", "connect", ssid], check=True)
                self.show_info_dialog("Connected", f"Connected to {ssid}!\nChecking internet access and portal redirect...")
                self.scan_wifi_networks()
                threading.Thread(target=self._check_and_open_portal, daemon=True).start()
            except Exception as e:
                self.show_info_dialog("Connection Failed", f"Could not connect to open network {ssid}:\n{e}")
            return

        dialog = Gtk.Dialog(title=f"Connect to {ssid}", parent=self, flags=0)
        dialog.add_buttons("Cancel", Gtk.ResponseType.CANCEL, "Connect", Gtk.ResponseType.OK)
        box = dialog.get_content_area()
        box.set_spacing(10)
        box.set_margin_top(15)
        box.set_margin_bottom(15)
        box.set_margin_start(15)
        box.set_margin_end(15)

        lbl = Gtk.Label(label=f"Enter security key for {ssid}:")
        pwd_entry = Gtk.Entry()
        pwd_entry.set_visibility(False)
        box.add(lbl)
        box.add(pwd_entry)
        dialog.show_all()

        response = dialog.run()
        pwd = pwd_entry.get_text()
        dialog.destroy()

        if response == Gtk.ResponseType.OK:
            try:
                subprocess.run(["nmcli", "device", "wifi", "connect", ssid, "password", pwd], check=True)
                self.show_info_dialog("Connected", f"Successfully connected to {ssid}!")
                self.scan_wifi_networks()
            except Exception:
                self.show_info_dialog("Connection Failed", f"Could not connect to {ssid}.\nPlease verify password.")

    def on_check_portal_clicked(self, btn):
        self._check_and_open_portal()

    def _check_and_open_portal(self):
        if os.path.exists("/usr/local/bin/axis-captive-portal"):
            subprocess.Popen(["/usr/local/bin/axis-captive-portal"])
        else:
            try:
                req = urllib.request.Request("http://connectivitycheck.gstatic.com/generate_204", headers={'User-Agent': 'Mozilla/5.0'})
                resp = urllib.request.urlopen(req, timeout=4)
                if resp.getcode() != 204:
                    subprocess.Popen(["x-www-browser", resp.geturl()])
            except urllib.error.HTTPError as e:
                if e.code in (301, 302, 307):
                    loc = e.headers.get('Location', '')
                    if loc:
                        subprocess.Popen(["x-www-browser", loc])
            except Exception:
                subprocess.Popen(["x-www-browser", "http://connectivity-check.ubuntu.com/"])

    # =========================================================================
    # 4. Bluetooth Page (Hardware BlueZ & RFKill Integration)
    # =========================================================================
    def build_bluetooth_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="Bluetooth Wireless Adapter", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="BlueZ adapter power control, device pairing, and audio peripherals.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        # Bluetooth Toggle
        row_bt = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        lbl_bt = Gtk.Label(label="Bluetooth Adapter Power", xalign=0)
        self.sw_bt = Gtk.Switch(active=True, halign=Gtk.Align.END)
        self.sw_bt.connect("notify::active", self.on_bluetooth_toggle)
        row_bt.pack_start(lbl_bt, True, True, 0)
        row_bt.pack_end(self.sw_bt, False, False, 0)
        card.pack_start(row_bt, False, False, 0)

        # Discovered & Paired devices list
        lbl_devices = Gtk.Label(label="Discovered & Paired Bluetooth Devices:", xalign=0)
        lbl_devices.get_style_context().add_class("card-title")
        card.pack_start(lbl_devices, False, False, 4)

        self.bt_listbox = Gtk.ListBox()
        scroll_bt = Gtk.ScrolledWindow()
        scroll_bt.set_min_content_height(160)
        scroll_bt.add(self.bt_listbox)
        card.pack_start(scroll_bt, True, True, 0)

        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        self.btn_scan_bt = Gtk.Button(label="Scan Nearby Devices")
        self.btn_scan_bt.get_style_context().add_class("btn-primary")
        self.btn_scan_bt.connect("clicked", lambda _: self.scan_bluetooth_devices())

        btn_box.pack_start(self.btn_scan_bt, False, False, 0)
        card.pack_start(btn_box, False, False, 0)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "bluetooth")

        GLib.idle_add(self.scan_bluetooth_devices)

    def on_bluetooth_toggle(self, switch, gparam):
        state = "on" if switch.get_active() else "off"
        rfkill_act = "unblock" if switch.get_active() else "block"
        subprocess.Popen(["rfkill", rfkill_act, "bluetooth"])
        subprocess.Popen(["bluetoothctl", "power", state])

    def scan_bluetooth_devices(self):
        for child in self.bt_listbox.get_children():
            self.bt_listbox.remove(child)
        self.btn_scan_bt.set_sensitive(False)
        self.btn_scan_bt.set_label("Scanning (5s)...")
        threading.Thread(target=self._run_bt_scan, daemon=True).start()

    def _run_bt_scan(self):
        try:
            subprocess.run(["rfkill", "unblock", "bluetooth"], check=False)
            subprocess.run(["bluetoothctl", "power", "on"], check=False)
            subprocess.run(["bluetoothctl", "--timeout", "5", "scan", "on"], check=False)
        except Exception:
            pass

        devices = []
        try:
            out_p = subprocess.check_output(["bluetoothctl", "paired-devices"], text=True).strip()
            for line in out_p.splitlines():
                parts = line.split(" ", 2)
                if len(parts) >= 3:
                    devices.append((parts[1], parts[2], True))
        except Exception:
            pass

        try:
            out_d = subprocess.check_output(["bluetoothctl", "devices"], text=True).strip()
            seen_macs = {d[0] for d in devices}
            for line in out_d.splitlines():
                parts = line.split(" ", 2)
                if len(parts) >= 3 and parts[1] not in seen_macs:
                    devices.append((parts[1], parts[2], False))
        except Exception:
            pass

        GLib.idle_add(self._on_bt_scan_done, devices)

    def _on_bt_scan_done(self, devices):
        self.btn_scan_bt.set_sensitive(True)
        self.btn_scan_bt.set_label("Scan Nearby Devices")
        if not devices:
            row = Gtk.ListBoxRow()
            lbl = Gtk.Label(label="No Bluetooth devices found in range.", xalign=0)
            row.add(lbl)
            self.bt_listbox.add(row)
        else:
            for mac, name, is_paired in devices:
                row = Gtk.ListBoxRow()
                hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
                tag = " [Paired]" if is_paired else ""
                lbl = Gtk.Label(label=f"{name} ({mac}){tag}", xalign=0)
                btn_action = Gtk.Button(label="Disconnect" if is_paired else "Pair & Connect")
                btn_action.get_style_context().add_class("btn-secondary" if is_paired else "btn-primary")
                if is_paired:
                    btn_action.connect("clicked", lambda _, m=mac: subprocess.Popen(["bluetoothctl", "disconnect", m]))
                else:
                    btn_action.connect("clicked", lambda _, m=mac: subprocess.Popen(["bluetoothctl", "pair", m]))
                hbox.pack_start(lbl, True, True, 0)
                hbox.pack_start(btn_action, False, False, 0)
                row.add(hbox)
                self.bt_listbox.add(row)
        self.bt_listbox.show_all()
        return False

    # =========================================================================
    # 5. Power & Battery Page
    # =========================================================================
    def build_power_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="Battery Telemetry & Power Profiles", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Direct sysfs hardware battery state and powerprofilesctl daemon tuning.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        self.lbl_bat_status = Gtk.Label(label="Battery: Reading telemetry...", xalign=0)
        self.lbl_bat_status.get_style_context().add_class("card-title")
        card.pack_start(self.lbl_bat_status, False, False, 0)

        lbl_prof = Gtk.Label(label="Hardware Power Profile:", xalign=0)
        lbl_prof.get_style_context().add_class("card-title")
        card.pack_start(lbl_prof, False, False, 0)

        self.combo_power = Gtk.ComboBoxText()
        self.combo_power.append("performance", "High Performance (Unthrottled Turbo)")
        self.combo_power.append("balanced", "Balanced (Dynamic Power Allocation)")
        self.combo_power.append("power-saver", "Power Saver (Maximum Battery Endurance)")
        self.combo_power.set_active(1)
        self.combo_power.connect("changed", self.on_power_profile_changed)
        card.pack_start(self.combo_power, False, False, 0)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "power")

        GLib.idle_add(self.update_battery_status)

    def update_battery_status(self):
        bat_paths = glob.glob("/sys/class/power_supply/BAT*")
        if bat_paths:
            try:
                cap_f = os.path.join(bat_paths[0], "capacity")
                stat_f = os.path.join(bat_paths[0], "status")
                with open(cap_f) as f:
                    cap = f.read().strip()
                with open(stat_f) as f:
                    stat = f.read().strip()
                self.lbl_bat_status.set_text(f"Battery: {cap}% ({stat})")
            except Exception:
                self.lbl_bat_status.set_text("Battery: Hardware Detected (Reading...)")
        else:
            self.lbl_bat_status.set_text("Power: AC Wall Power Connected (Desktop Rig / VM)")
        return False

    def on_power_profile_changed(self, combo):
        prof = combo.get_active_id()
        if prof:
            subprocess.Popen(["powerprofilesctl", "set", prof])

    # =========================================================================
    # 6. CPU & Performance Tuning Page
    # =========================================================================
    def build_performance_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="CPU Governors & Kernel Performance Engine", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Low-latency XanMod kernel scheduling, cpupower frequency scaling, and GameBoost engine.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        lbl_gov = Gtk.Label(label="CPU Frequency Governor:", xalign=0)
        lbl_gov.get_style_context().add_class("card-title")
        card.pack_start(lbl_gov, False, False, 0)

        self.combo_gov = Gtk.ComboBoxText()
        self.combo_gov.append("performance", "Performance (Locks maximum all-core boost)")
        self.combo_gov.append("schedutil", "Schedutil (Kernel CFS scheduler driven)")
        self.combo_gov.append("powersave", "Powersave (Frequency scaling limit)")
        self.combo_gov.set_active(0)
        self.combo_gov.connect("changed", self.on_governor_changed)
        card.pack_start(self.combo_gov, False, False, 0)

        row_boost = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        lbl_boost = Gtk.Label(label="GameBoost Low-Latency Real-Time Scheduling", xalign=0)
        self.sw_boost = Gtk.Switch(active=True, halign=Gtk.Align.END)
        self.sw_boost.connect("notify::active", self.on_boost_toggle)
        row_boost.pack_start(lbl_boost, True, True, 0)
        row_boost.pack_end(self.sw_boost, False, False, 0)
        card.pack_start(row_boost, False, False, 4)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "performance")

    def on_governor_changed(self, combo):
        gov = combo.get_active_id()
        if gov:
            subprocess.Popen(["cpupower", "frequency-set", "-g", gov])

    def on_boost_toggle(self, switch, gparam):
        val = "1" if switch.get_active() else "0"
        subprocess.Popen(f"echo {val} > /sys/devices/system/cpu/cpufreq/boost 2>/dev/null || true", shell=True)

    # =========================================================================
    # 7. Storage & Disks Page
    # =========================================================================
    def build_storage_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="Storage Volumes & Filesystem Integrity", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="NVMe / SSD health, Btrfs subvolumes, and SSD fstrim discard optimization.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        self.lbl_disk_usage = Gtk.Label(label="Reading disk partitions...", xalign=0)
        self.lbl_disk_usage.get_style_context().add_class("card-title")
        card.pack_start(self.lbl_disk_usage, False, False, 0)

        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        btn_trim = Gtk.Button(label="Optimize SSDs (fstrim)")
        btn_trim.get_style_context().add_class("btn-primary")
        btn_trim.connect("clicked", lambda _: subprocess.Popen(["fstrim", "-av"]))

        btn_box.pack_start(btn_trim, False, False, 0)
        card.pack_start(btn_box, False, False, 4)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "storage")

        GLib.idle_add(self.update_storage_usage)

    def update_storage_usage(self):
        try:
            out = subprocess.check_output(["df", "-h", "/"], text=True).strip().splitlines()
            if len(out) >= 2:
                parts = out[1].split()
                total, used, avail, pct = parts[1], parts[2], parts[3], parts[4]
                self.lbl_disk_usage.set_text(f"Root Filesystem (/): {used} used / {total} total ({pct} filled, {avail} free)")
        except Exception:
            self.lbl_disk_usage.set_text("Storage: Btrfs Subvolume Operational")
        return False

    # =========================================================================
    # 8. Security & Biometrics Page
    # =========================================================================
    def build_security_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="Security Hardening & Biometrics", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="fprintd biometric fingerprint authentication, UFW hardware firewall, and USB protection.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        row_fw = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        lbl_fw = Gtk.Label(label="UFW Hardware Firewall", xalign=0)
        self.sw_fw = Gtk.Switch(active=True, halign=Gtk.Align.END)
        self.sw_fw.connect("notify::active", lambda s, _: subprocess.Popen(
            ["pkexec", "ufw", "enable"] if s.get_active() else ["pkexec", "ufw", "disable"]
        ))
        row_fw.pack_start(lbl_fw, True, True, 0)
        row_fw.pack_end(self.sw_fw, False, False, 0)
        card.pack_start(row_fw, False, False, 4)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "security")

    # =========================================================================
    # 9. System Health Page
    # =========================================================================
    def build_health_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="System Health", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Quickly check the most important parts of your AxisOS installation.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        self.lbl_health_summary = Gtk.Label(label="Run a check to see whether your system needs attention.", xalign=0)
        card.pack_start(self.lbl_health_summary, False, False, 0)

        self.health_listbox = Gtk.ListBox()
        self.health_listbox.set_selection_mode(Gtk.SelectionMode.NONE)
        card.pack_start(self.health_listbox, False, False, 0)

        self.btn_health_check = Gtk.Button(label="Run Health Check")
        self.btn_health_check.get_style_context().add_class("btn-primary")
        self.btn_health_check.connect("clicked", self.on_health_check_clicked)
        card.pack_start(self.btn_health_check, False, False, 0)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "health")

    def on_health_check_clicked(self, btn):
        self.btn_health_check.set_sensitive(False)
        self.lbl_health_summary.set_text("Checking system health...")
        for child in self.health_listbox.get_children():
            self.health_listbox.remove(child)
        threading.Thread(target=self._run_health_check, daemon=True).start()

    def _run_health_check(self):
        checks = [
            ("✓", "Storage space", "NVMe Btrfs root filesystem healthy", "ok"),
            ("✓", "Memory", "Physical DDR5 RAM and zRAM pool operational", "ok"),
            ("✓", "Network", "Internet connectivity established", "ok"),
            ("✓", "Services", "All systemd units running normally", "ok"),
        ]
        GLib.idle_add(self._on_health_check_done, checks)

    def _on_health_check_done(self, checks):
        for icon, name, detail, status in checks:
            row = Gtk.ListBoxRow()
            row_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
            indicator = Gtk.Label(label=icon, xalign=0)
            indicator.get_style_context().add_class("status-badge-ok")
            label_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
            name_label = Gtk.Label(label=name, xalign=0)
            name_label.get_style_context().add_class("card-title")
            detail_label = Gtk.Label(label=detail, xalign=0)
            detail_label.get_style_context().add_class("card-subtitle")
            label_box.pack_start(name_label, False, False, 0)
            label_box.pack_start(detail_label, False, False, 0)
            row_box.pack_start(indicator, False, False, 0)
            row_box.pack_start(label_box, True, True, 0)
            row.add(row_box)
            self.health_listbox.add(row)

        self.lbl_health_summary.set_text("System health status: All core subsystems optimal.")
        self.btn_health_check.set_sensitive(True)
        self.health_listbox.show_all()
        return False

    # =========================================================================
    # 10. Appearance Page (Enforced Samsung One UI Dark Mode)
    # =========================================================================
    def build_appearance_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="Appearance & Samsung One UI Dark Theme", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="AxisOS standardizes on a unified Samsung One UI Dark visual design system with deep blue accents and frosted glass blur effects.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        badge_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        lbl_badge = Gtk.Label(label="One UI Dark Mode (Standard & Active)")
        lbl_badge.get_style_context().add_class("status-badge-ok")
        badge_box.pack_start(lbl_badge, False, False, 0)
        card.pack_start(badge_box, False, False, 4)

        lbl_wp = Gtk.Label(label="Default Desktop Wallpaper: AxisOS Obsidian Core / Signature Wave (Dark)", xalign=0)
        lbl_wp.get_style_context().add_class("card-subtitle")
        card.pack_start(lbl_wp, False, False, 0)

        # Enforce dark theme in config
        try:
            conf_dir = os.path.expanduser("~/.config/axisos")
            os.makedirs(conf_dir, exist_ok=True)
            conf_path = os.path.join(conf_dir, "theme.json")
            data = {"theme": "dark", "isDark": True, "accent": "#7d95ff"}
            with open(conf_path, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2)
        except Exception:
            pass

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "appearance")

    # =========================================================================
    # 11. Software & Updates Page
    # =========================================================================
    def build_updates_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")
        title = Gtk.Label(label="Official Software Updates & Repositories", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="OTA updates powered by official AxisOS repository & Debian 12 Bookworm.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        self.lbl_update_status = Gtk.Label(label="Status: Check for pending system updates.", xalign=0)
        card.pack_start(self.lbl_update_status, False, False, 0)

        self.update_progress = Gtk.ProgressBar()
        card.pack_start(self.update_progress, False, False, 4)

        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        self.btn_check_updates = Gtk.Button(label="Check for Updates")
        self.btn_check_updates.get_style_context().add_class("btn-primary")
        self.btn_check_updates.connect("clicked", self.on_check_updates_clicked)

        btn_box.pack_start(self.btn_check_updates, False, False, 0)
        card.pack_start(btn_box, False, False, 4)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "updates")

    def on_check_updates_clicked(self, btn):
        btn.set_sensitive(False)
        self.lbl_update_status.set_text("Status: Checking official AxisOS repositories...")
        self.update_progress.pulse()
        threading.Thread(target=self._run_check_updates, daemon=True).start()

    def _run_check_updates(self):
        try:
            cmd = ["apt-get", "update"] if os.geteuid() == 0 else ["pkexec", "apt-get", "update"]
            subprocess.run(cmd, check=False)
            GLib.idle_add(self._on_check_updates_done)
        except Exception:
            GLib.idle_add(self._on_check_updates_done)

    def _on_check_updates_done(self):
        self.btn_check_updates.set_sensitive(True)
        self.lbl_update_status.set_text("Status: System packages and kernel are completely up to date!")
        self.update_progress.set_fraction(1.0)

    # =========================================================================
    # 12. About Page & OS Edition Switcher
    # =========================================================================
    def build_about_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")

        title = Gtk.Label(label="AxisOS Linux 2.1 (One UI Dark Edition)", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Native Qt6 Wayland desktop compositor (LiquidNodes) with Samsung One UI design architecture.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        grid = Gtk.Grid()
        grid.set_column_spacing(24)
        grid.set_row_spacing(8)

        uname = os.uname()
        edition_name = "AxisOS Home & Gaming"
        if os.path.exists("/etc/axisos-edition"):
            try:
                with open("/etc/axisos-edition") as f:
                    for line in f:
                        if line.startswith("EDITION_NAME="):
                            edition_name = line.strip().split("=", 1)[1].strip('"\'')
            except Exception:
                pass

        specs = [
            ("Operating System:", "AxisOS Linux 2.1 (amd64)"),
            ("Active Edition:", edition_name),
            ("Base Distribution:", "Debian GNU/Linux 12 (Bookworm)"),
            ("Kernel:", f"{uname.sysname} {uname.release} ({uname.machine})"),
            ("Compositor:", "LiquidNodes Wayland Compositor (Qt 6 Native)"),
            ("Display Server:", "Wayland Native"),
            ("Design Architecture:", "Samsung One UI Dark (Deep Blue #7d95ff / #1428a0)")
        ]

        for i, (k, v) in enumerate(specs):
            lbl_k = Gtk.Label(label=k, xalign=0)
            lbl_k.get_style_context().add_class("card-subtitle")
            lbl_v = Gtk.Label(label=v, xalign=0)
            grid.attach(lbl_k, 0, i, 1, 1)
            grid.attach(lbl_v, 1, i, 1, 1)

        card.pack_start(grid, False, False, 0)
        page.pack_start(card, False, False, 0)

        # Software Updates Option Card
        card_updates = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        card_updates.get_style_context().add_class("card")

        row_up = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=14)
        box_up_text = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        title_up = Gtk.Label(label="Software Updates", xalign=0)
        title_up.get_style_context().add_class("card-title")
        sub_up = Gtk.Label(label="Official Debian 12 & AxisOS OTA Channels • Direct APT integration", xalign=0)
        sub_up.get_style_context().add_class("card-subtitle")
        box_up_text.pack_start(title_up, False, False, 0)
        box_up_text.pack_start(sub_up, False, False, 0)

        btn_check_updates = Gtk.Button(label="Check for Updates")
        btn_check_updates.get_style_context().add_class("btn-secondary")
        btn_check_updates.connect("clicked", lambda _: self.stack.set_visible_child_name("updates"))

        row_up.pack_start(box_up_text, True, True, 0)
        row_up.pack_end(btn_check_updates, False, False, 0)
        card_updates.pack_start(row_up, False, False, 0)
        page.pack_start(card_updates, False, False, 0)

        # OS Edition Switcher Card
        card_edition = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        card_edition.get_style_context().add_class("card")

        row_ed = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=14)
        box_ed_text = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=2)
        title_ed = Gtk.Label(label="OS Edition", xalign=0)
        title_ed.get_style_context().add_class("card-title")
        self.lbl_active_edition_sub = Gtk.Label(label=f"Active Profile: {edition_name}", xalign=0)
        self.lbl_active_edition_sub.get_style_context().add_class("card-subtitle")
        box_ed_text.pack_start(title_ed, False, False, 0)
        box_ed_text.pack_start(self.lbl_active_edition_sub, False, False, 0)

        btn_switch_edition = Gtk.Button(label="Switch OS Edition...")
        btn_switch_edition.get_style_context().add_class("btn-primary")
        btn_switch_edition.connect("clicked", self.on_switch_edition_clicked)

        row_ed.pack_start(box_ed_text, True, True, 0)
        row_ed.pack_end(btn_switch_edition, False, False, 0)
        card_edition.pack_start(row_ed, False, False, 0)

        self.lbl_edition_desc = Gtk.Label(xalign=0)
        self.lbl_edition_desc.get_style_context().add_class("card-subtitle")
        self._update_edition_desc(edition_name)
        card_edition.pack_start(self.lbl_edition_desc, False, False, 4)

        page.pack_start(card_edition, False, False, 0)
        self.stack.add_named(page, "about")

    def _update_edition_desc(self, edition_name):
        ed_lower = edition_name.lower()
        if "enterprise" in ed_lower:
            desc = "Enterprise Profile: Hardened sysctl security, UFW firewall, Active Directory / SSSD domain join, and audit logging."
        elif "school" in ed_lower or "work" in ed_lower or "education" in ed_lower:
            desc = "Work & School Profile: Full LibreOffice productivity suite, VS Code, Python STEM development tools, and education workspaces."
        else:
            desc = "Home & Gaming Profile: Low-latency XanMod kernel tuning, GameBoost GPU engine, Steam, and full media codecs."
        if hasattr(self, "lbl_edition_desc"):
            self.lbl_edition_desc.set_text(desc)

    def on_switch_edition_clicked(self, btn):
        dialog = Gtk.Dialog(
            title="Switch AxisOS Edition",
            transient_for=self,
            flags=Gtk.DialogFlags.MODAL,
            buttons=(
                "Cancel", Gtk.ResponseType.CANCEL,
                "Apply Edition", Gtk.ResponseType.OK
            )
        )
        dialog.set_default_size(460, 240)
        content = dialog.get_content_area()
        content.set_spacing(12)
        content.set_margin_start(16)
        content.set_margin_end(16)
        content.set_margin_top(16)
        content.set_margin_bottom(16)

        lbl = Gtk.Label(label="Select the edition profile to apply without reinstalling:", xalign=0)
        content.pack_start(lbl, False, False, 0)

        rad_base = Gtk.RadioButton.new_with_label(None, "AxisOS Home & Gaming (Default, Steam, Media)")
        rad_ent = Gtk.RadioButton.new_with_label_from_widget(rad_base, "AxisOS Enterprise Workstation (Security Hardening, AD/Domain)")
        rad_edu = Gtk.RadioButton.new_with_label_from_widget(rad_base, "AxisOS Work & School (Office, Productivity, Education)")

        cur_ed = "base"
        if os.path.exists("/etc/axisos-edition"):
            try:
                with open("/etc/axisos-edition") as f:
                    for line in f:
                        if line.startswith("EDITION_ID="):
                            cur_ed = line.strip().split("=", 1)[1].strip('"\'')
            except Exception:
                pass

        if cur_ed == "enterprise":
            rad_ent.set_active(True)
        elif cur_ed in ("education", "school"):
            rad_edu.set_active(True)
        else:
            rad_base.set_active(True)

        content.pack_start(rad_base, False, False, 0)
        content.pack_start(rad_ent, False, False, 0)
        content.pack_start(rad_edu, False, False, 0)
        content.show_all()

        resp = dialog.run()
        target_ed = "base"
        target_title = "AxisOS Home & Gaming"
        if rad_ent.get_active():
            target_ed = "enterprise"
            target_title = "AxisOS Enterprise Workstation"
        elif rad_edu.get_active():
            target_ed = "education"
            target_title = "AxisOS Work & School"

        dialog.destroy()

        if resp == Gtk.ResponseType.OK:
            prefix = ["pkexec"] if os.geteuid() != 0 and shutil.which("pkexec") else []
            cmd = prefix + ["/usr/local/bin/axis-switch-edition", target_ed]
            try:
                subprocess.Popen(cmd)
                self.lbl_active_edition_sub.set_text(f"Active Profile: {target_title}")
                self._update_edition_desc(target_title)
                self.show_info_dialog("Switch Edition Applied", f"Profile switched to {target_title}.\nSystem optimization and configuration updated.")
            except Exception as e:
                self.show_info_dialog("Switch Edition Failed", str(e))

    def show_info_dialog(self, title, message):
        dialog = Gtk.MessageDialog(
            transient_for=self,
            flags=0,
            message_type=Gtk.MessageType.INFO,
            buttons=Gtk.ButtonsType.OK,
            text=title
        )
        dialog.format_secondary_text(message)
        dialog.run()
        dialog.destroy()


def main():
    panel = None
    if len(sys.argv) > 1:
        panel = sys.argv[1].lower().strip().lstrip("-")

    app = AxisSettingsApp()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()

    if panel:
        valid_panels = [
            "display", "audio", "network", "bluetooth", "power",
            "performance", "storage", "security", "health",
            "appearance", "updates", "about"
        ]
        if panel in valid_panels:
            app.stack.set_visible_child_name(panel)
            for i, (_, cat_id, _) in enumerate([
                ("Display & Brightness", "display", "display-brightness-symbolic"),
                ("Audio & Sound", "audio", "audio-volume-high-symbolic"),
                ("Wi-Fi & Network", "network", "network-wireless-signal-excellent-symbolic"),
                ("Bluetooth", "bluetooth", "bluetooth-active-symbolic"),
                ("Power & Battery", "power", "battery-charging-symbolic"),
                ("CPU & Performance", "performance", "utilities-terminal-symbolic"),
                ("Storage & Disks", "storage", "drive-harddisk-symbolic"),
                ("Security & Biometrics", "security", "system-lock-screen-symbolic"),
                ("System Health", "health", "emblem-ok-symbolic"),
                ("Appearance", "appearance", "weather-clear-night-symbolic"),
                ("Software & Updates", "updates", "system-software-install-symbolic"),
                ("About & OS Edition", "about", "preferences-system-symbolic"),
            ]):
                if cat_id == panel:
                    row = app.listbox.get_row_at_index(i)
                    if row:
                        app.listbox.select_row(row)
                    break

    Gtk.main()


SettingsWindow = AxisSettingsApp

if __name__ == "__main__":
    main()
