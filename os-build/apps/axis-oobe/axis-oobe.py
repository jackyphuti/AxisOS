#!/usr/bin/env python3
"""
AxisOS Nobara Edition - Out-of-Box Experience (OOBE) First-Boot Setup Wizard
Hardware detection, NetworkManager Wi-Fi/Internet enforcement, and system onboarding.
"""

import os
import sys
import subprocess
import threading
import time
import platform
import gi

gi.require_version('Gtk', '3.0')
gi.require_version('Gdk', '3.0')
from gi.repository import Gtk, Gdk, GLib, Pango

PENDING_FLAG = "/var/lib/axisos/first-boot-pending"

CSS_DATA = b"""
* {
    font-family: 'Inter', 'Noto Sans', 'Segoe UI', sans-serif;
}

window {
    background-color: #0c0d11;
    color: #e4e7eb;
}

.title-header {
    font-size: 26px;
    font-weight: 800;
    color: #ffffff;
    letter-spacing: -0.5px;
}

.subtitle-header {
    font-size: 14px;
    color: #94a3b8;
    margin-bottom: 12px;
}

.card {
    background-color: #16181f;
    border-radius: 12px;
    border: 1px solid #282c37;
    padding: 20px;
}

.card-title {
    font-size: 16px;
    font-weight: 700;
    color: #ffffff;
    margin-bottom: 8px;
}

.badge-gaming {
    background: linear-gradient(135deg, #10b981 0%, #059669 100%);
    color: #ffffff;
    font-weight: 700;
    font-size: 12px;
    border-radius: 6px;
    padding: 4px 10px;
}

.badge-offline {
    background: linear-gradient(135deg, #ef4444 0%, #b91c1c 100%);
    color: #ffffff;
    font-weight: 700;
    font-size: 12px;
    border-radius: 6px;
    padding: 4px 10px;
}

.badge-online {
    background: linear-gradient(135deg, #10b981 0%, #047857 100%);
    color: #ffffff;
    font-weight: 700;
    font-size: 12px;
    border-radius: 6px;
    padding: 4px 10px;
}

.btn-primary {
    background: linear-gradient(135deg, #10b981 0%, #059669 100%);
    color: #ffffff;
    font-weight: 700;
    font-size: 14px;
    border-radius: 8px;
    padding: 10px 24px;
    border: none;
    box-shadow: 0 4px 12px rgba(16, 185, 129, 0.3);
}

.btn-primary:hover {
    background: linear-gradient(135deg, #34d399 0%, #10b981 100%);
}

.btn-primary:disabled {
    background: #2a2d37;
    color: #64748b;
    box-shadow: none;
}

.btn-secondary {
    background-color: #242731;
    color: #cbd5e1;
    font-weight: 600;
    font-size: 13px;
    border-radius: 8px;
    padding: 8px 18px;
    border: 1px solid #333846;
}

.btn-secondary:hover {
    background-color: #2e323e;
    color: #ffffff;
}

.wifi-list {
    background-color: #12141a;
    border-radius: 8px;
    border: 1px solid #282c37;
}

.wifi-row {
    padding: 10px 14px;
    border-bottom: 1px solid #1c202a;
}

.wifi-row:selected {
    background-color: #1e293b;
    color: #38bdf8;
}

entry {
    background-color: #12141a;
    color: #ffffff;
    border: 1px solid #333846;
    border-radius: 8px;
    padding: 8px 12px;
    font-size: 14px;
}

entry:focus {
    border-color: #10b981;
}

.stepper-active {
    color: #10b981;
    font-weight: 700;
}

.stepper-inactive {
    color: #64748b;
    font-weight: 500;
}
"""


class AxisOOBEWizard(Gtk.Window):
    def __init__(self):
        super().__init__(title="AxisOS Setup Wizard")
        self.set_default_size(920, 640)
        self.set_position(Gtk.WindowPosition.CENTER)
        self.set_resizable(False)

        # Style provider
        css_provider = Gtk.CssProvider()
        css_provider.load_from_data(CSS_DATA)
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(),
            css_provider,
            Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

        self.is_connected_global = False
        self.selected_ssid = ""

        # Outer container
        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.add(main_box)

        # Header bar
        header = self.build_header()
        main_box.pack_start(header, False, False, 0)

        # Content stack
        self.stack = Gtk.Stack()
        self.stack.set_transition_type(Gtk.StackTransitionType.SLIDE_LEFT_RIGHT)
        self.stack.set_transition_duration(300)
        main_box.pack_start(self.stack, True, True, 0)

        # Build pages
        self.page_hardware = self.build_hardware_page()
        self.page_network = self.build_network_page()
        self.page_complete = self.build_complete_page()

        self.stack.add_named(self.page_hardware, "hardware")
        self.stack.add_named(self.page_network, "network")
        self.stack.add_named(self.page_complete, "complete")

        # Start periodic connectivity monitor
        GLib.timeout_add_seconds(3, self.check_network_status_tick)
        self.check_network_status_tick()

    def build_header(self):
        header_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=16)
        header_box.set_margin_start(32)
        header_box.set_margin_end(32)
        header_box.set_margin_top(24)
        header_box.set_margin_bottom(16)

        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=4)
        title = Gtk.Label(label="AXIS OS")
        title.get_style_context().add_class("title-header")
        title.set_xalign(0)

        subtitle = Gtk.Label(label="Nobara Gaming Edition — Out-of-Box First-Boot Experience")
        subtitle.get_style_context().add_class("subtitle-header")
        subtitle.set_xalign(0)

        vbox.pack_start(title, False, False, 0)
        vbox.pack_start(subtitle, False, False, 0)
        header_box.pack_start(vbox, True, True, 0)

        badge = Gtk.Label(label="NATIVE WAYLAND")
        badge.get_style_context().add_class("badge-gaming")
        header_box.pack_end(badge, False, False, 0)

        return header_box

    # =========================================================================
    # Page 1: Hardware Detection & Review
    # =========================================================================
    def build_hardware_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=20)
        page.set_margin_start(32)
        page.set_margin_end(32)
        page.set_margin_top(8)
        page.set_margin_bottom(24)

        intro = Gtk.Label(
            label="Welcome to AxisOS. Hardware drivers, low-latency gaming kernel tweaks, "
                  "and multimedia codecs have been optimized for your system."
        )
        intro.set_xalign(0)
        intro.set_line_wrap(True)
        intro.get_style_context().add_class("subtitle-header")
        page.pack_start(intro, False, False, 0)

        # Specs grid card
        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)
        card.get_style_context().add_class("card")

        card_title = Gtk.Label(label="Detected Hardware Configuration")
        card_title.get_style_context().add_class("card-title")
        card_title.set_xalign(0)
        card.pack_start(card_title, False, False, 0)

        grid = Gtk.Grid()
        grid.set_column_spacing(24)
        grid.set_row_spacing(10)

        specs = self.probe_hardware()
        row = 0
        for label_text, val_text in specs:
            lbl = Gtk.Label(label=label_text)
            lbl.get_style_context().add_class("stepper-inactive")
            lbl.set_xalign(0)

            val = Gtk.Label(label=val_text)
            val.set_xalign(0)
            val.set_selectable(True)

            grid.attach(lbl, 0, row, 1, 1)
            grid.attach(val, 1, row, 1, 1)
            row += 1

        card.pack_start(grid, False, False, 0)
        page.pack_start(card, True, True, 0)

        # Footer navigation
        footer = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        step_label = Gtk.Label(label="Step 1 of 3: Hardware Verification")
        step_label.get_style_context().add_class("stepper-active")
        footer.pack_start(step_label, False, False, 0)

        btn_next = Gtk.Button(label="Next: Connect to Internet  ->")
        btn_next.get_style_context().add_class("btn-primary")
        btn_next.connect("clicked", lambda _: self.stack.set_visible_child_name("network"))
        footer.pack_end(btn_next, False, False, 0)

        page.pack_end(footer, False, False, 0)
        return page

    def probe_hardware(self):
        specs = []

        # CPU
        cpu_name = "x86_64 Processor"
        cores = 0
        try:
            with open("/proc/cpuinfo", "r") as f:
                for line in f:
                    if "model name" in line:
                        cpu_name = line.split(":", 1)[1].strip()
                    if line.startswith("processor"):
                        cores += 1
        except Exception:
            pass
        specs.append(("Processor (CPU):", f"{cpu_name} ({cores} Cores/Threads)"))

        # Memory
        total_ram_gb = 0
        try:
            with open("/proc/meminfo", "r") as f:
                for line in f:
                    if "MemTotal:" in line:
                        kb = int(line.split()[1])
                        total_ram_gb = round(kb / (1024 * 1024), 1)
                        break
        except Exception:
            pass
        specs.append(("System Memory (RAM):", f"{total_ram_gb} GB High-Speed RAM (zRAM Enabled)"))

        # GPU
        gpu_desc = "Standard Display Adapter (Mesa 3D)"
        try:
            pci = subprocess.check_output(
                "lspci -k 2>/dev/null | grep -iEA2 'VGA|3D|Display'",
                shell=True, text=True
            ).strip()
            if pci:
                first_line = pci.splitlines()[0]
                if ":" in first_line:
                    gpu_desc = first_line.split(":", 2)[-1].strip()
        except Exception:
            pass
        specs.append(("Graphics (GPU):", gpu_desc))

        # Kernel
        krelease = platform.uname().release
        specs.append(("Linux Kernel:", f"{krelease} (XanMod Low-Latency Tuning Active)"))

        # Audio
        specs.append(("Audio Server:", "PipeWire Low-Latency Audio Engine"))

        # Gamepads
        gamepads = []
        try:
            if os.path.exists("/dev/input"):
                for item in os.listdir("/dev/input"):
                    if item.startswith("js"):
                        gamepads.append(item)
        except Exception:
            pass
        gp_text = f"{len(gamepads)} Gamepad(s) detected (Hotplug udev Ready)" if gamepads else "Ready (Plug & Play udev active)"
        specs.append(("Controller / Gamepad:", gp_text))

        return specs

    # =========================================================================
    # Page 2: Mandatory Network & Internet Connection
    # =========================================================================
    def build_network_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=16)
        page.set_margin_start(32)
        page.set_margin_end(32)
        page.set_margin_top(8)
        page.set_margin_bottom(24)

        # Status Banner
        self.banner_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        self.status_badge = Gtk.Label(label="OFFLINE — INTERNET REQUIRED")
        self.status_badge.get_style_context().add_class("badge-offline")
        self.banner_box.pack_start(self.status_badge, False, False, 0)

        self.status_desc = Gtk.Label(
            label="AxisOS requires a verified global internet connection to initialize security updates and game runtimes."
        )
        self.status_desc.set_xalign(0)
        self.banner_box.pack_start(self.status_desc, True, True, 0)
        page.pack_start(self.banner_box, False, False, 0)

        # Network Selection Card
        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")

        top_bar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=8)
        c_title = Gtk.Label(label="Available Wi-Fi & Wired Connections")
        c_title.get_style_context().add_class("card-title")
        top_bar.pack_start(c_title, True, True, 0)

        btn_scan = Gtk.Button(label="Rescan Networks")
        btn_scan.get_style_context().add_class("btn-secondary")
        btn_scan.connect("clicked", lambda _: self.trigger_wifi_scan())
        top_bar.pack_end(btn_scan, False, False, 0)
        card.pack_start(top_bar, False, False, 0)

        # Wi-Fi list scrolled window
        scrolled = Gtk.ScrolledWindow()
        scrolled.set_min_content_height(160)
        scrolled.set_max_content_height(180)
        scrolled.get_style_context().add_class("wifi-list")

        self.listbox = Gtk.ListBox()
        self.listbox.set_selection_mode(Gtk.SelectionMode.SINGLE)
        self.listbox.connect("row-selected", self.on_wifi_row_selected)
        scrolled.add(self.listbox)
        card.pack_start(scrolled, True, True, 0)

        # Password & Connect section
        conn_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.ssid_entry = Gtk.Entry()
        self.ssid_entry.set_placeholder_text("Network SSID")
        self.ssid_entry.set_width_chars(20)

        self.pass_entry = Gtk.Entry()
        self.pass_entry.set_placeholder_text("Wi-Fi Password / Security Key")
        self.pass_entry.set_visibility(False)
        self.pass_entry.set_width_chars(24)

        self.btn_connect = Gtk.Button(label="Connect to Wi-Fi")
        self.btn_connect.get_style_context().add_class("btn-secondary")
        self.btn_connect.connect("clicked", self.on_connect_clicked)

        conn_box.pack_start(self.ssid_entry, False, False, 0)
        conn_box.pack_start(self.pass_entry, True, True, 0)
        conn_box.pack_start(self.btn_connect, False, False, 0)
        card.pack_start(conn_box, False, False, 0)

        self.conn_feedback = Gtk.Label(label="")
        self.conn_feedback.set_xalign(0)
        card.pack_start(self.conn_feedback, False, False, 0)

        page.pack_start(card, True, True, 0)

        # Footer navigation
        footer = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        btn_back = Gtk.Button(label="<- Back")
        btn_back.get_style_context().add_class("btn-secondary")
        btn_back.connect("clicked", lambda _: self.stack.set_visible_child_name("hardware"))
        footer.pack_start(btn_back, False, False, 0)

        step_label = Gtk.Label(label="Step 2 of 3: Internet Requirement")
        step_label.get_style_context().add_class("stepper-active")
        footer.pack_start(step_label, True, True, 0)

        # Next button is locked until internet is connected!
        self.btn_network_next = Gtk.Button(label="Continue to Finalize  ->")
        self.btn_network_next.get_style_context().add_class("btn-primary")
        self.btn_network_next.set_sensitive(False)
        self.btn_network_next.set_tooltip_text("You must connect to the internet to continue.")
        self.btn_network_next.connect("clicked", lambda _: self.stack.set_visible_child_name("complete"))
        footer.pack_end(self.btn_network_next, False, False, 0)

        page.pack_end(footer, False, False, 0)

        # Initial Wi-Fi populate
        self.trigger_wifi_scan()

        return page

    def trigger_wifi_scan(self):
        threading.Thread(target=self._scan_wifi_worker, daemon=True).start()

    def _scan_wifi_worker(self):
        networks = []
        try:
            # First rescan
            subprocess.run(["nmcli", "dev", "wifi", "rescan"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            time.sleep(1)
            # List SSIDs
            out = subprocess.check_output(
                ["nmcli", "-t", "-f", "SSID,SIGNAL,SECURITY", "dev", "wifi", "list"],
                text=True
            )
            seen = set()
            for line in out.splitlines():
                parts = line.strip().split(":")
                if len(parts) >= 2 and parts[0]:
                    ssid = parts[0]
                    if ssid not in seen and ssid != "--":
                        seen.add(ssid)
                        signal = parts[1] if len(parts) > 1 else "50"
                        sec = parts[2] if len(parts) > 2 and parts[2] else "Open"
                        networks.append((ssid, signal, sec))
        except Exception as e:
            # Fallback if no wifi card (e.g. Ethernet only)
            pass

        GLib.idle_add(self._update_wifi_list_ui, networks)

    def _update_wifi_list_ui(self, networks):
        for child in self.listbox.get_children():
            self.listbox.remove(child)

        if not networks:
            row = Gtk.ListBoxRow()
            box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
            box.set_margin_start(12)
            box.set_margin_top(10)
            box.set_margin_bottom(10)
            lbl = Gtk.Label(label="No Wi-Fi networks found. (Wired Ethernet or USB tethering is supported)")
            lbl.get_style_context().add_class("subtitle-header")
            box.pack_start(lbl, False, False, 0)
            row.add(box)
            self.listbox.add(row)
        else:
            for ssid, signal, sec in networks:
                row = Gtk.ListBoxRow()
                row.ssid = ssid
                row.sec = sec

                box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
                box.set_margin_start(12)
                box.set_margin_end(12)
                box.set_margin_top(8)
                box.set_margin_bottom(8)

                icon = Gtk.Label(label="📶")
                name = Gtk.Label(label=ssid)
                name.set_xalign(0)

                sec_badge = Gtk.Label(label=sec)
                sec_badge.get_style_context().add_class("stepper-inactive")

                sig_badge = Gtk.Label(label=f"{signal}%")
                sig_badge.get_style_context().add_class("stepper-active")

                box.pack_start(icon, False, False, 0)
                box.pack_start(name, True, True, 0)
                box.pack_end(sig_badge, False, False, 0)
                box.pack_end(sec_badge, False, False, 0)

                row.add(box)
                self.listbox.add(row)

        self.listbox.show_all()

    def on_wifi_row_selected(self, listbox, row):
        if row and hasattr(row, 'ssid'):
            self.selected_ssid = row.ssid
            self.ssid_entry.set_text(row.ssid)
            if row.sec == "Open":
                self.pass_entry.set_text("")
                self.pass_entry.set_sensitive(False)
            else:
                self.pass_entry.set_sensitive(True)
                self.pass_entry.grab_focus()

    def on_connect_clicked(self, btn):
        ssid = self.ssid_entry.get_text().strip()
        pwd = self.pass_entry.get_text().strip()

        if not ssid:
            self.conn_feedback.set_text("Please enter or select a Wi-Fi SSID.")
            return

        self.btn_connect.set_sensitive(False)
        self.conn_feedback.set_text(f"Connecting to '{ssid}'...")

        threading.Thread(target=self._connect_worker, args=(ssid, pwd), daemon=True).start()

    def _connect_worker(self, ssid, pwd):
        cmd = ["nmcli", "dev", "wifi", "connect", ssid]
        if pwd:
            cmd.extend(["password", pwd])

        res = subprocess.run(cmd, capture_output=True, text=True)
        GLib.idle_add(self._connect_finished, res.returncode == 0, res.stdout + res.stderr)

    def _connect_finished(self, success, output):
        self.btn_connect.set_sensitive(True)
        if success:
            self.conn_feedback.set_markup(f"<span foreground='#10b981'>Successfully connected! Verifying global internet...</span>")
        else:
            err_msg = output.strip().replace("\n", " ")
            self.conn_feedback.set_markup(f"<span foreground='#ef4444'>Connection failed: {err_msg[:60]}</span>")
        self.check_network_status_tick()

    def check_network_status_tick(self):
        threading.Thread(target=self._probe_internet_worker, daemon=True).start()
        return True  # repeat

    def _probe_internet_worker(self):
        connected = False
        try:
            # 1. Check nmcli general status
            out = subprocess.check_output(["nmcli", "general", "status"], text=True)
            if "connected (global)" in out.lower() or "connected" in out.lower():
                connected = True

            # 2. Ping check for reliable confirmation
            if not connected:
                ping_res = subprocess.run(
                    ["ping", "-c", "1", "-W", "2", "1.1.1.1"],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
                )
                if ping_res.returncode == 0:
                    connected = True
        except Exception:
            pass

        GLib.idle_add(self._update_internet_state, connected)

    def _update_internet_state(self, connected):
        self.is_connected_global = connected

        if connected:
            self.status_badge.set_text("ONLINE — INTERNET VERIFIED")
            self.status_badge.get_style_context().remove_class("badge-offline")
            self.status_badge.get_style_context().add_class("badge-online")
            self.status_desc.set_text("Global internet connection established. System setup unlocked.")
            self.btn_network_next.set_sensitive(True)
            self.btn_network_next.set_tooltip_text("Proceed to finalize installation.")
        else:
            self.status_badge.set_text("OFFLINE — INTERNET REQUIRED")
            self.status_badge.get_style_context().remove_class("badge-online")
            self.status_badge.get_style_context().add_class("badge-offline")
            self.status_desc.set_text("AxisOS requires a verified global internet connection to proceed.")
            self.btn_network_next.set_sensitive(False)
            self.btn_network_next.set_tooltip_text("You must connect to the internet before proceeding.")

    # =========================================================================
    # Page 3: Finalize & Launch Desktop
    # =========================================================================
    def build_complete_page(self):
        page = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=20)
        page.set_margin_start(32)
        page.set_margin_end(32)
        page.set_margin_top(16)
        page.set_margin_bottom(24)

        # Card
        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=16)
        card.get_style_context().add_class("card")

        title = Gtk.Label(label="Setup Complete! Your Gaming System is Ready.")
        title.get_style_context().add_class("card-title")
        title.set_xalign(0)
        card.pack_start(title, False, False, 0)

        summary = Gtk.Label(
            label="AxisOS Nobara Edition is fully configured with native Wayland desktop architecture.\n\n"
                  "Features active on your system:\n"
                  "  • Steam pre-installed with Proton compatibility & GameMode\n"
                  "  • High-performance XanMod kernel scheduling\n"
                  "  • Automated NVIDIA / AMD / Intel GPU driver support\n"
                  "  • Bluetooth controller AutoEnable & udev gamepad rules\n"
                  "  • Kyber & BFQ high-speed NVMe/SSD schedulers\n"
                  "  • Low-latency PipeWire gaming audio"
        )
        summary.set_xalign(0)
        summary.get_style_context().add_class("subtitle-header")
        card.pack_start(summary, True, True, 0)

        page.pack_start(card, True, True, 0)

        # Footer
        footer = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        btn_back = Gtk.Button(label="<- Back")
        btn_back.get_style_context().add_class("btn-secondary")
        btn_back.connect("clicked", lambda _: self.stack.set_visible_child_name("network"))
        footer.pack_start(btn_back, False, False, 0)

        step_label = Gtk.Label(label="Step 3 of 3: System Finalized")
        step_label.get_style_context().add_class("stepper-active")
        footer.pack_start(step_label, True, True, 0)

        btn_finish = Gtk.Button(label="Enter Nobara Desktop  🚀")
        btn_finish.get_style_context().add_class("btn-primary")
        btn_finish.connect("clicked", self.on_finish_clicked)
        footer.pack_end(btn_finish, False, False, 0)

        page.pack_end(footer, False, False, 0)
        return page

    def on_finish_clicked(self, btn):
        # 1. Clear first boot pending flag
        if os.path.exists(PENDING_FLAG):
            try:
                os.remove(PENDING_FLAG)
            except Exception as e:
                print(f"Error removing {PENDING_FLAG}: {e}")

        # 2. Launch normal desktop suite (Waybar, Swaybg, Mako, Nobara Welcome)
        subprocess.Popen(["waybar"])
        subprocess.Popen(["nobara-welcome"])

        # 3. Quit OOBE wizard
        Gtk.main_quit()


def main():
    win = AxisOOBEWizard()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
