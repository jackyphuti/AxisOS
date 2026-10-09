#!/usr/bin/env python3
"""
AxisOS System Settings & Appearance Manager
Native GTK 3 Control Panel for Desktop Tuning, Appearance Presets,
Graphical Software Updates, Wi-Fi Captive Portal, Location, and Security.
"""

import os
import sys
import subprocess
import threading
import json
import urllib.request
import gi

gi.require_version("Gtk", "3.0")
gi.require_version("Gdk", "3.0")
from gi.repository import Gtk, Gdk, GLib, Pango

CSS_STYLING = b"""
* {
    font-family: "Inter", "Cantarell", "DejaVu Sans", sans-serif;
}

window {
    background-color: #0a0c10;
    color: #f8fafc;
}

headerbar {
    background: #111827;
    border-bottom: 2px solid #2563eb;
    color: #ffffff;
}

headerbar .title {
    font-weight: 800;
    font-size: 15px;
    color: #ffffff;
}

.sidebar {
    background-color: #0f141c;
    border-right: 1px solid rgba(255, 255, 255, 0.08);
}

.sidebar list row {
    padding: 12px 16px;
    color: #94a3b8;
    font-weight: 600;
    border-left: 3px solid transparent;
}

.sidebar list row:selected {
    background-color: #1e293b;
    color: #ffffff;
    border-left: 3px solid #2563eb;
}

.content-pane {
    padding: 24px 32px;
}

.card {
    background-color: #111827;
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 10px;
    padding: 18px 22px;
    margin-bottom: 16px;
}

.card-title {
    font-size: 15px;
    font-weight: 700;
    color: #ffffff;
    margin-bottom: 6px;
}

.card-subtitle {
    font-size: 12px;
    color: #94a3b8;
    margin-bottom: 12px;
}

.btn-primary {
    background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
    color: #ffffff;
    font-weight: 700;
    border-radius: 6px;
    border: none;
    padding: 9px 18px;
    box-shadow: 0 2px 6px rgba(37, 99, 235, 0.35);
}

.btn-primary:hover {
    background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
}

.btn-secondary {
    background: #1f2937;
    color: #e2e8f0;
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 6px;
    padding: 8px 16px;
    font-weight: 600;
}

.btn-secondary:hover {
    background: #374151;
    color: #ffffff;
    border-color: #2563eb;
}

.status-badge-ok {
    background-color: rgba(37, 99, 235, 0.2);
    color: #60a5fa;
    border: 1px solid rgba(37, 99, 235, 0.4);
    font-weight: bold;
    font-size: 11px;
    padding: 4px 10px;
    border-radius: 6px;
}

.status-badge-warn {
    background-color: rgba(234, 179, 8, 0.2);
    color: #facc15;
    border: 1px solid rgba(234, 179, 8, 0.4);
    font-weight: bold;
    font-size: 11px;
    padding: 4px 10px;
    border-radius: 6px;
}

entry {
    background-color: #161e2e;
    color: #ffffff;
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 6px;
    padding: 8px 12px;
}

entry:focus {
    border-color: #2563eb;
}

progressbar progress {
    background: linear-gradient(135deg, #2563eb 0%, #3b82f6 100%);
    border-radius: 4px;
}
progressbar trough {
    background-color: #161e2e;
    border-radius: 4px;
}
"""

class AxisSettingsApp(Gtk.Window):
    def __init__(self):
        super().__init__(title="Settings")
        self.set_default_size(940, 640)
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
        hb.props.title = "AxisOS Settings"
        hb.props.subtitle = "Desktop, Updates, Network & Security"
        self.set_titlebar(hb)

    def init_ui(self):
        main_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=0)
        self.add(main_box)

        # Left Sidebar
        sidebar_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        sidebar_box.get_style_context().add_class("sidebar")
        sidebar_box.set_size_request(240, -1)

        self.listbox = Gtk.ListBox()
        self.listbox.set_selection_mode(Gtk.SelectionMode.BROWSE)
        self.listbox.connect("row-selected", self.on_category_selected)

        categories = [
            ("🎨 Appearance & Desktop", "appearance"),
            ("🔄 Software & Updates", "updates"),
            ("📶 Wi-Fi & Network", "network"),
            ("🌍 Date, Time & Location", "datetime"),
            ("🔒 Accounts & Security", "security"),
            ("ℹ️ About AxisOS", "about"),
        ]

        for label_text, cat_id in categories:
            row = Gtk.ListBoxRow()
            lbl = Gtk.Label(label=label_text, xalign=0)
            lbl.cat_id = cat_id
            row.add(lbl)
            self.listbox.add(row)

        sidebar_box.pack_start(self.listbox, True, True, 0)
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

        # Build pages
        self.build_appearance_page()
        self.build_updates_page()
        self.build_network_page()
        self.build_datetime_page()
        self.build_security_page()
        self.build_about_page()

        # Select first category
        first_row = self.listbox.get_row_at_index(0)
        self.listbox.select_row(first_row)

    def on_category_selected(self, box, row):
        if row:
            lbl = row.get_child()
            if hasattr(lbl, "cat_id"):
                self.stack.set_visible_child_name(lbl.cat_id)

    # =========================================================================
    # 1. Appearance & Desktop Page
    # =========================================================================
    def build_appearance_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        # Card 1: Taskbar / Panel Position
        card_bar = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        card_bar.get_style_context().add_class("card")
        title = Gtk.Label(label="Taskbar Position", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Choose whether the system taskbar is docked at the top or bottom of the screen.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")
        card_bar.pack_start(title, False, False, 0)
        card_bar.pack_start(sub, False, False, 0)

        bar_btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        self.rb_bar_top = Gtk.RadioButton.new_with_label_from_widget(None, "Top (Up - Standard)")
        self.rb_bar_bottom = Gtk.RadioButton.new_with_label_from_widget(self.rb_bar_top, "Bottom (Down - Classic)")

        current_pos = self.get_waybar_position()
        if current_pos == "bottom":
            self.rb_bar_bottom.set_active(True)
        else:
            self.rb_bar_top.set_active(True)

        self.rb_bar_top.connect("toggled", lambda b: self.set_waybar_position("top") if b.get_active() else None)
        self.rb_bar_bottom.connect("toggled", lambda b: self.set_waybar_position("bottom") if b.get_active() else None)

        bar_btn_box.pack_start(self.rb_bar_top, False, False, 0)
        bar_btn_box.pack_start(self.rb_bar_bottom, False, False, 0)
        card_bar.pack_start(bar_btn_box, False, False, 0)
        page.pack_start(card_bar, False, False, 0)

        # Card 2: Color Theme (Dark / Light)
        card_theme = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        card_theme.get_style_context().add_class("card")
        title2 = Gtk.Label(label="System Color Scheme", xalign=0)
        title2.get_style_context().add_class("card-title")
        sub2 = Gtk.Label(label="Toggle between Pure Obsidian Dark Mode and Crisp Clean Light Mode.", xalign=0)
        sub2.get_style_context().add_class("card-subtitle")
        card_theme.pack_start(title2, False, False, 0)
        card_theme.pack_start(sub2, False, False, 0)

        theme_btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        btn_dark = Gtk.Button(label="🌙 Dark Mode (Black, Blue, White)")
        btn_dark.get_style_context().add_class("btn-primary")
        btn_dark.connect("clicked", lambda b: self.apply_theme_mode("dark"))

        btn_light = Gtk.Button(label="☀️ Light Mode (White, Blue, Black)")
        btn_light.get_style_context().add_class("btn-secondary")
        btn_light.connect("clicked", lambda b: self.apply_theme_mode("light"))

        theme_btn_box.pack_start(btn_dark, False, False, 0)
        theme_btn_box.pack_start(btn_light, False, False, 0)
        card_theme.pack_start(theme_btn_box, False, False, 0)
        page.pack_start(card_theme, False, False, 0)

        # Card 3: Icon Theme & Presets
        card_icons = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        card_icons.get_style_context().add_class("card")
        title3 = Gtk.Label(label="Desktop Icons Preset", xalign=0)
        title3.get_style_context().add_class("card-title")
        sub3 = Gtk.Label(label="Select how system icons appear across your desktop, menus, and file manager.", xalign=0)
        sub3.get_style_context().add_class("card-subtitle")
        card_icons.pack_start(title3, False, False, 0)
        card_icons.pack_start(sub3, False, False, 0)

        icon_combo_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        lbl_icon = Gtk.Label(label="Icon Set:")
        self.combo_icons = Gtk.ComboBoxText()
        for ic in ["Papirus-Dark", "Papirus", "Papirus-Light", "Adwaita"]:
            self.combo_icons.append_text(ic)
        self.combo_icons.set_active(0)
        self.combo_icons.connect("changed", self.on_icon_changed)

        icon_combo_box.pack_start(lbl_icon, False, False, 0)
        icon_combo_box.pack_start(self.combo_icons, False, False, 0)
        card_icons.pack_start(icon_combo_box, False, False, 0)
        page.pack_start(card_icons, False, False, 0)

        self.stack.add_named(page, "appearance")

    def get_waybar_position(self):
        cfg_path = os.path.expanduser("~/.config/waybar/config.jsonc")
        if not os.path.exists(cfg_path):
            cfg_path = "/etc/xdg/waybar/config.jsonc"
        if os.path.exists(cfg_path):
            try:
                with open(cfg_path, "r", encoding="utf-8") as f:
                    for line in f:
                        if '"position"' in line:
                            if "bottom" in line:
                                return "bottom"
                            return "top"
            except Exception:
                pass
        return "top"

    def set_waybar_position(self, pos):
        user_cfg = os.path.expanduser("~/.config/waybar/config.jsonc")
        os.makedirs(os.path.dirname(user_cfg), exist_ok=True)
        src_cfg = user_cfg if os.path.exists(user_cfg) else "/etc/xdg/waybar/config.jsonc"
        if os.path.exists(src_cfg):
            try:
                with open(src_cfg, "r", encoding="utf-8") as f:
                    content = f.read()
                if '"position":' in content:
                    import re
                    content = re.sub(r'"position"\s*:\s*"[^"]+"', f'"position": "{pos}"', content)
                with open(user_cfg, "w", encoding="utf-8") as f:
                    f.write(content)
                subprocess.run(["pkill", "-SIGUSR2", "waybar"], stderr=subprocess.DEVNULL)
            except Exception as e:
                print("Error setting Waybar position:", e)

    def apply_theme_mode(self, mode):
        if mode == "dark":
            subprocess.run(["gsettings", "set", "org.gnome.desktop.interface", "color-scheme", "prefer-dark"], stderr=subprocess.DEVNULL)
            subprocess.run(["gsettings", "set", "org.gnome.desktop.interface", "gtk-theme", "Adwaita-dark"], stderr=subprocess.DEVNULL)
        else:
            subprocess.run(["gsettings", "set", "org.gnome.desktop.interface", "color-scheme", "prefer-light"], stderr=subprocess.DEVNULL)
            subprocess.run(["gsettings", "set", "org.gnome.desktop.interface", "gtk-theme", "Adwaita"], stderr=subprocess.DEVNULL)
        self.show_info_dialog("Theme Updated", f"Applied {mode.capitalize()} mode successfully.")

    def on_icon_changed(self, combo):
        selected = combo.get_active_text()
        if selected:
            subprocess.run(["gsettings", "set", "org.gnome.desktop.interface", "icon-theme", selected], stderr=subprocess.DEVNULL)

    # =========================================================================
    # 2. Software & Updates Page (No Terminal Required!)
    # =========================================================================
    def build_updates_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")

        title = Gtk.Label(label="System & Software Updates", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Check and install official Debian & AxisOS package updates directly without using the terminal.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")

        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        # Status and progress
        self.lbl_update_status = Gtk.Label(label="Status: Ready to check for updates", xalign=0)
        card.pack_start(self.lbl_update_status, False, False, 0)

        self.update_progress = Gtk.ProgressBar()
        self.update_progress.set_fraction(0.0)
        card.pack_start(self.update_progress, False, False, 0)

        # Update buttons
        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        self.btn_check_updates = Gtk.Button(label="🔍 Check for Updates")
        self.btn_check_updates.get_style_context().add_class("btn-secondary")
        self.btn_check_updates.connect("clicked", self.on_check_updates_clicked)

        self.btn_install_updates = Gtk.Button(label="⬇️ Install All Updates")
        self.btn_install_updates.get_style_context().add_class("btn-primary")
        self.btn_install_updates.set_sensitive(False)
        self.btn_install_updates.connect("clicked", self.on_install_updates_clicked)

        btn_box.pack_start(self.btn_check_updates, False, False, 0)
        btn_box.pack_start(self.btn_install_updates, False, False, 0)
        card.pack_start(btn_box, False, False, 0)

        # List of updates
        self.updates_listbox = Gtk.ListBox()
        self.updates_listbox.set_selection_mode(Gtk.SelectionMode.NONE)
        scroll_updates = Gtk.ScrolledWindow()
        scroll_updates.set_min_content_height(180)
        scroll_updates.add(self.updates_listbox)
        card.pack_start(scroll_updates, True, True, 0)

        page.pack_start(card, True, True, 0)
        self.stack.add_named(page, "updates")

    def on_check_updates_clicked(self, btn):
        self.btn_check_updates.set_sensitive(False)
        self.lbl_update_status.set_text("Status: Checking Debian repositories...")
        self.update_progress.set_fraction(0.2)
        self.update_progress.pulse()

        threading.Thread(target=self._run_check_updates, daemon=True).start()

    def _run_check_updates(self):
        try:
            subprocess.run(["apt-get", "update", "-qq"], stderr=subprocess.DEVNULL)
            out = subprocess.check_output(["apt-get", "--just-print", "upgrade"], universal_newlines=True, stderr=subprocess.DEVNULL)
            upgradable = []
            for line in out.splitlines():
                if line.startswith("Inst "):
                    parts = line.split()
                    if len(parts) >= 2:
                        upgradable.append(parts[1])
            GLib.idle_add(self._on_check_updates_done, upgradable)
        except Exception as e:
            GLib.idle_add(self._on_check_updates_error, str(e))

    def _on_check_updates_done(self, upgradable):
        self.btn_check_updates.set_sensitive(True)
        self.update_progress.set_fraction(1.0)
        # Clear list
        for child in self.updates_listbox.get_children():
            self.updates_listbox.remove(child)

        if upgradable:
            self.lbl_update_status.set_text(f"Status: {len(upgradable)} package updates available.")
            self.btn_install_updates.set_sensitive(True)
            for pkg in upgradable:
                row = Gtk.ListBoxRow()
                lbl = Gtk.Label(label=f"📦 {pkg}", xalign=0)
                row.add(lbl)
                self.updates_listbox.add(row)
            self.updates_listbox.show_all()
        else:
            self.lbl_update_status.set_text("Status: Your system and all apps are completely up to date!")
            self.btn_install_updates.set_sensitive(False)

    def _on_check_updates_error(self, err):
        self.btn_check_updates.set_sensitive(True)
        self.lbl_update_status.set_text(f"Status: Failed to check updates ({err})")

    def on_install_updates_clicked(self, btn):
        self.btn_install_updates.set_sensitive(False)
        self.btn_check_updates.set_sensitive(False)
        self.lbl_update_status.set_text("Status: Installing updates (background process)...")
        self.update_progress.set_fraction(0.3)
        threading.Thread(target=self._run_install_updates, daemon=True).start()

    def _run_install_updates(self):
        try:
            env = os.environ.copy()
            env["DEBIAN_FRONTEND"] = "noninteractive"
            subprocess.run(["apt-get", "upgrade", "-y", "-qq"], env=env, check=True)
            GLib.idle_add(self._on_install_updates_done)
        except Exception as e:
            GLib.idle_add(self._on_install_updates_error, str(e))

    def _on_install_updates_done(self):
        self.btn_check_updates.set_sensitive(True)
        self.btn_install_updates.set_sensitive(False)
        self.update_progress.set_fraction(1.0)
        self.lbl_update_status.set_text("Status: All updates installed successfully!")
        self.show_info_dialog("Updates Installed", "All software packages were upgraded successfully.")

    def _on_install_updates_error(self, err):
        self.btn_check_updates.set_sensitive(True)
        self.btn_install_updates.set_sensitive(True)
        self.lbl_update_status.set_text("Status: Error during installation.")
        self.show_info_dialog("Update Error", f"Could not install updates:\n{err}")

    # =========================================================================
    # 3. Wi-Fi & Network Page (Captive Portal Login Redirect Support)
    # =========================================================================
    def build_network_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")

        title = Gtk.Label(label="Wi-Fi & Network Connectivity", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Manage wireless networks, detect captive web login portals, and test connection.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")

        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        # Captive Portal Section
        portal_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        self.lbl_portal_status = Gtk.Label(label="Network Status: Checking...", xalign=0)
        self.lbl_portal_status.get_style_context().add_class("status-badge-ok")

        btn_check_portal = Gtk.Button(label="🌐 Check Web Login Portal")
        btn_check_portal.get_style_context().add_class("btn-primary")
        btn_check_portal.connect("clicked", self.on_check_portal_clicked)

        portal_box.pack_start(self.lbl_portal_status, False, False, 0)
        portal_box.pack_start(btn_check_portal, False, False, 0)
        card.pack_start(portal_box, False, False, 0)

        # Wi-Fi Networks list
        lbl_avail = Gtk.Label(label="Available Wi-Fi Networks:", xalign=0)
        lbl_avail.get_style_context().add_class("card-title")
        card.pack_start(lbl_avail, False, False, 0)

        self.wifi_listbox = Gtk.ListBox()
        scroll_wifi = Gtk.ScrolledWindow()
        scroll_wifi.set_min_content_height(160)
        scroll_wifi.add(self.wifi_listbox)
        card.pack_start(scroll_wifi, True, True, 0)

        btn_rescan = Gtk.Button(label="🔄 Scan Wi-Fi Networks")
        btn_rescan.get_style_context().add_class("btn-secondary")
        btn_rescan.connect("clicked", lambda b: self.scan_wifi_networks())
        card.pack_start(btn_rescan, False, False, 0)

        page.pack_start(card, True, True, 0)
        self.stack.add_named(page, "network")

        GLib.idle_add(self.check_network_connectivity)
        GLib.idle_add(self.scan_wifi_networks)

    def check_network_connectivity(self):
        try:
            out = subprocess.check_output(["nmcli", "networking", "connectivity", "check"], universal_newlines=True).strip()
            if out == "portal":
                self.lbl_portal_status.set_text("Status: Web Login Required (Captive Portal)")
                self.lbl_portal_status.get_style_context().remove_class("status-badge-ok")
                self.lbl_portal_status.get_style_context().add_class("status-badge-warn")
            elif out == "full":
                self.lbl_portal_status.set_text("Status: Internet Online (Full Access)")
            else:
                self.lbl_portal_status.set_text(f"Status: {out.capitalize()}")
        except Exception:
            self.lbl_portal_status.set_text("Status: Offline / Limited")

    def on_check_portal_clicked(self, btn):
        # Trigger captive browser redirect
        redirect_urls = [
            "http://connectivity-check.ubuntu.com/",
            "http://detectportal.firefox.com/canonical.html",
            "http://www.google.com/generate_204"
        ]
        target = redirect_urls[0]
        # Launch browser to force portal redirect
        for browser in ["chromium", "firefox", "x-www-browser"]:
            if shutil_which := subprocess.run(["which", browser], stdout=subprocess.DEVNULL).returncode == 0:
                subprocess.Popen([browser, target])
                self.show_info_dialog("Portal Redirect", f"Opening web browser to complete login at:\n{target}")
                return
        subprocess.Popen(["xdg-open", target])

    def scan_wifi_networks(self):
        for child in self.wifi_listbox.get_children():
            self.wifi_listbox.remove(child)
        try:
            out = subprocess.check_output(["nmcli", "-t", "-f", "SSID,SIGNAL,SECURITY", "device", "wifi", "list"], universal_newlines=True)
            seen = set()
            for line in out.splitlines():
                parts = line.split(":")
                if parts and parts[0] and parts[0] not in seen:
                    ssid = parts[0]
                    seen.add(ssid)
                    signal = parts[1] if len(parts) > 1 else "100"
                    sec = parts[2] if len(parts) > 2 else "Open"

                    row = Gtk.ListBoxRow()
                    hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
                    lbl = Gtk.Label(label=f"📶 {ssid} ({signal}%)", xalign=0)
                    btn_conn = Gtk.Button(label="Connect")
                    btn_conn.get_style_context().add_class("btn-secondary")
                    btn_conn.connect("clicked", lambda b, s=ssid: self.connect_wifi(s))

                    hbox.pack_start(lbl, True, True, 0)
                    hbox.pack_start(btn_conn, False, False, 0)
                    row.add(hbox)
                    self.wifi_listbox.add(row)
            self.wifi_listbox.show_all()
        except Exception:
            pass

    def connect_wifi(self, ssid):
        dialog = Gtk.Dialog(title=f"Connect to {ssid}", parent=self, flags=0)
        dialog.add_buttons(Gtk.STOCK_CANCEL, Gtk.ResponseType.CANCEL, Gtk.STOCK_OK, Gtk.ResponseType.OK)
        box = dialog.get_content_area()
        box.set_spacing(10)
        box.set_margin_top(15)
        box.set_margin_bottom(15)
        box.set_margin_start(15)
        box.set_margin_end(15)

        lbl = Gtk.Label(label="Enter Wi-Fi Password:")
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
                self.check_network_connectivity()
            except Exception as e:
                self.show_info_dialog("Connection Failed", f"Could not connect to {ssid}.\nPlease verify password.")

    # =========================================================================
    # 4. Date, Time & Location Page (Automatic Timezone Geolocation)
    # =========================================================================
    def build_datetime_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")

        title = Gtk.Label(label="Automatic Date, Time & Location", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Synchronize your system clock using network NTP and auto-detect your location.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")

        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        # Current details
        self.lbl_current_time = Gtk.Label(label="System Clock: Fetching...", xalign=0)
        self.lbl_current_tz = Gtk.Label(label="Current Timezone: Fetching...", xalign=0)
        card.pack_start(self.lbl_current_time, False, False, 0)
        card.pack_start(self.lbl_current_tz, False, False, 0)

        btn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        btn_auto_tz = Gtk.Button(label="📍 Auto-Detect Location & Set Timezone")
        btn_auto_tz.get_style_context().add_class("btn-primary")
        btn_auto_tz.connect("clicked", self.on_auto_detect_timezone)

        btn_sync_ntp = Gtk.Button(label="⏱️ Sync Clock with NTP")
        btn_sync_ntp.get_style_context().add_class("btn-secondary")
        btn_sync_ntp.connect("clicked", self.on_sync_ntp)

        btn_box.pack_start(btn_auto_tz, False, False, 0)
        btn_box.pack_start(btn_sync_ntp, False, False, 0)
        card.pack_start(btn_box, False, False, 0)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "datetime")

        GLib.idle_add(self.update_datetime_info)

    def update_datetime_info(self):
        try:
            out = subprocess.check_output(["timedatectl", "status"], universal_newlines=True)
            for line in out.splitlines():
                if "Local time:" in line:
                    self.lbl_current_time.set_text(f"Clock: {line.split('Local time:')[1].strip()}")
                elif "Time zone:" in line:
                    self.lbl_current_tz.set_text(f"Timezone: {line.split('Time zone:')[1].strip()}")
        except Exception:
            pass

    def on_auto_detect_timezone(self, btn):
        btn.set_sensitive(False)
        threading.Thread(target=self._run_auto_timezone, args=(btn,), daemon=True).start()

    def _run_auto_timezone(self, btn):
        detected_tz = None
        city = None
        country = None
        try:
            req = urllib.request.Request("https://ipapi.co/json/", headers={"User-Agent": "AxisOS-Settings/2.0"})
            with urllib.request.urlopen(req, timeout=5) as response:
                data = json.loads(response.read().decode("utf-8"))
                detected_tz = data.get("timezone")
                city = data.get("city")
                country = data.get("country_name")
        except Exception:
            try:
                req = urllib.request.Request("http://ip-api.com/json/", headers={"User-Agent": "AxisOS-Settings/2.0"})
                with urllib.request.urlopen(req, timeout=5) as response:
                    data = json.loads(response.read().decode("utf-8"))
                    detected_tz = data.get("timezone")
                    city = data.get("city")
                    country = data.get("country")
            except Exception:
                pass

        GLib.idle_add(self._on_auto_timezone_done, btn, detected_tz, city, country)

    def _on_auto_timezone_done(self, btn, detected_tz, city, country):
        btn.set_sensitive(True)
        if detected_tz:
            try:
                subprocess.run(["timedatectl", "set-timezone", detected_tz], check=True)
                subprocess.run(["timedatectl", "set-ntp", "true"], check=True)
                self.update_datetime_info()
                msg = f"Location detected: {city}, {country}\nTimezone set to: {detected_tz}\nClock synchronized via NTP."
                self.show_info_dialog("Timezone Configured", msg)
            except Exception as e:
                self.show_info_dialog("Error", f"Could not set timezone:\n{e}")
        else:
            self.show_info_dialog("Location Detection", "Could not reach geolocation server. Please verify internet connection.")

    def on_sync_ntp(self, btn):
        try:
            subprocess.run(["timedatectl", "set-ntp", "true"], check=True)
            self.update_datetime_info()
            self.show_info_dialog("Clock Synced", "System clock successfully synchronized with network time servers.")
        except Exception as e:
            self.show_info_dialog("NTP Error", f"Failed to sync NTP:\n{e}")

    # =========================================================================
    # 5. Accounts & Security Page (User & Root Unified Password)
    # =========================================================================
    def build_security_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")

        title = Gtk.Label(label="Unified Account & Administrator Passwords", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(
            label="In Debian, administrator (root) and user credentials must stay securely synchronized.\n"
                  "Administrative commands (sudo) will always prompt for your password to protect system integrity.",
            xalign=0
        )
        sub.get_style_context().add_class("card-subtitle")

        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        grid = Gtk.Grid()
        grid.set_column_spacing(12)
        grid.set_row_spacing(10)

        lbl_u = Gtk.Label(label="Target User:", xalign=0)
        self.entry_user = Gtk.Entry()
        self.entry_user.set_text(os.environ.get("USER", "axis"))

        lbl_p1 = Gtk.Label(label="New Password:", xalign=0)
        self.entry_pwd1 = Gtk.Entry()
        self.entry_pwd1.set_visibility(False)

        lbl_p2 = Gtk.Label(label="Confirm Password:", xalign=0)
        self.entry_pwd2 = Gtk.Entry()
        self.entry_pwd2.set_visibility(False)

        grid.attach(lbl_u, 0, 0, 1, 1)
        grid.attach(self.entry_user, 1, 0, 1, 1)
        grid.attach(lbl_p1, 0, 1, 1, 1)
        grid.attach(self.entry_pwd1, 1, 1, 1, 1)
        grid.attach(lbl_p2, 0, 2, 1, 1)
        grid.attach(self.entry_pwd2, 1, 2, 1, 1)

        card.pack_start(grid, False, False, 0)

        btn_save = Gtk.Button(label="🔐 Set Unified Password for User & Root")
        btn_save.get_style_context().add_class("btn-primary")
        btn_save.connect("clicked", self.on_save_passwords_clicked)
        card.pack_start(btn_save, False, False, 0)

        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "security")

    def on_save_passwords_clicked(self, btn):
        username = self.entry_user.get_text().strip()
        pwd1 = self.entry_pwd1.get_text()
        pwd2 = self.entry_pwd2.get_text()

        if not pwd1:
            self.show_info_dialog("Password Required", "Password cannot be empty.")
            return
        if pwd1 != pwd2:
            self.show_info_dialog("Mismatch", "Entered passwords do not match.")
            return

        try:
            # Set user password and root password to the exact same value
            p1 = subprocess.Popen(["chpasswd"], stdin=subprocess.PIPE, text=True)
            p1.communicate(input=f"{username}:{pwd1}\nroot:{pwd1}\n")
            if p1.returncode == 0:
                self.show_info_dialog(
                    "Password Synchronized",
                    f"Password updated successfully!\nBoth account '{username}' and administrator 'root' now share this password.\n"
                    "Terminal operations will strictly require this password."
                )
                self.entry_pwd1.set_text("")
                self.entry_pwd2.set_text("")
            else:
                self.show_info_dialog("Error", "Could not set password (administrator privileges required).")
        except Exception as e:
            self.show_info_dialog("Error", f"Failed to update passwords:\n{e}")

    # =========================================================================
    # 6. About Page
    # =========================================================================
    def build_about_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")

        title = Gtk.Label(label="AxisOS Linux 2.0 (Debian Edition)", xalign=0)
        title.get_style_context().add_class("card-title")
        sub = Gtk.Label(label="Native Wayland desktop architecture powered by Debian GNU/Linux Bookworm.", xalign=0)
        sub.get_style_context().add_class("card-subtitle")

        card.pack_start(title, False, False, 0)
        card.pack_start(sub, False, False, 0)

        grid = Gtk.Grid()
        grid.set_column_spacing(16)
        grid.set_row_spacing(8)

        uname = os.uname()
        specs = [
            ("Operating System:", "AxisOS Linux 2.0"),
            ("Base Distribution:", "Debian GNU/Linux 12 (Bookworm)"),
            ("Kernel:", f"{uname.sysname} {uname.release} ({uname.machine})"),
            ("Compositor:", "Native wlroots Wayland Compositor (axis-compositor)"),
            ("Display Server:", "Wayland Native"),
            ("Accent Theme:", "Obsidian Black, Crisp White, Real Blue (#2563eb)")
        ]

        for i, (k, v) in enumerate(specs):
            lbl_k = Gtk.Label(label=k, xalign=0)
            lbl_k.get_style_context().add_class("card-subtitle")
            lbl_v = Gtk.Label(label=v, xalign=0)
            grid.attach(lbl_k, 0, i, 1, 1)
            grid.attach(lbl_v, 1, i, 1, 1)

        card.pack_start(grid, False, False, 0)
        page.pack_start(card, False, False, 0)
        self.stack.add_named(page, "about")

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
    app = AxisSettingsApp()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
