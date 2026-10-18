#!/usr/bin/env python3
"""
AxisOS 2.0 Welcome & System Telemetry Center (axis-welcome)
===========================================================
Native Python 3 + GTK 3 implementation guaranteed to launch across X11, Wayland,
GNOME, Cinnamon, and VM environments without requiring Qt6 QML modules.
"""

import os
import sys
import shutil
import platform
import subprocess
from pathlib import Path
import gi

gi.require_version("Gtk", "3.0")
gi.require_version("Gdk", "3.0")
from gi.repository import Gtk, Gdk, GLib

CSS = b"""
* {
    font-family: "Inter", "Segoe UI", "Ubuntu", "Cantarell", sans-serif;
}

window,
.background {
    background-color: #0B0E17;
    color: #F8FAFC;
}

headerbar {
    background-color: #111624;
    background-image: none;
    color: #F8FAFC;
    border-bottom: 1px solid #1E293B;
    box-shadow: none;
}

headerbar .title {
    color: #FFFFFF;
    font-weight: 700;
    font-size: 14px;
}

headerbar .subtitle {
    color: #38BDF8;
    font-size: 11px;
}

headerbar button {
    background-color: #1E293B;
    background-image: none;
    color: #F8FAFC;
    border: 1px solid #334155;
    border-radius: 6px;
    box-shadow: none;
    text-shadow: none;
}

headerbar button:hover {
    background-color: #334155;
    color: #FFFFFF;
    border-color: #00F2FE;
}

.sidebar,
.sidebar scrolledwindow,
.sidebar viewport,
.sidebar list {
    background-color: #0F1420;
    color: #CBD5E1;
    border-right: 1px solid #1E293B;
}

.sidebar row,
.sidebar list row {
    background-color: transparent;
    background-image: none;
    color: #CBD5E1;
    padding: 12px 16px;
    margin: 4px 8px;
    border-radius: 8px;
    border-left: 3px solid transparent;
}

.sidebar row label,
.sidebar list row label {
    color: #CBD5E1;
    font-weight: 600;
    font-size: 13px;
}

.sidebar row:hover,
.sidebar list row:hover {
    background-color: #161F33;
    color: #FFFFFF;
}

.sidebar row:hover label,
.sidebar list row:hover label {
    color: #FFFFFF;
}

.sidebar row:selected,
.sidebar list row:selected {
    background-color: #1A2640;
    background-image: none;
    color: #00F2FE;
    border-left: 3px solid #00F2FE;
}

.sidebar row:selected label,
.sidebar list row:selected label {
    color: #00F2FE;
    font-weight: 700;
}

scrolledwindow,
viewport,
stack {
    background-color: #0B0E17;
    color: #F8FAFC;
}

.card {
    background-color: #151C2C;
    border: 1px solid #24324D;
    border-radius: 12px;
    padding: 16px 20px;
}

.card-header {
    font-size: 14px;
    font-weight: 700;
    color: #FFFFFF;
}

.title-hero {
    font-size: 20pt;
    font-weight: bold;
    color: #FFFFFF;
}

.subtitle-hero {
    font-size: 10.5pt;
    color: #94A3B8;
}

button.primary-btn,
button.primary-btn:hover,
button.primary-btn:active,
button.primary-btn:checked {
    background-color: #00F2FE;
    background-image: none;
    color: #0A0D14;
    font-weight: 700;
    border: 1px solid #00F2FE;
    border-radius: 8px;
    padding: 9px 18px;
    box-shadow: none;
    text-shadow: none;
}

button.primary-btn label {
    color: #0A0D14;
    font-weight: 700;
}

button.primary-btn:hover {
    background-color: #38F8FF;
    border-color: #FFFFFF;
}

button.secondary-btn,
button.secondary-btn:hover,
button.secondary-btn:active,
button.secondary-btn:checked {
    background-color: #1E293B;
    background-image: none;
    color: #F8FAFC;
    border: 1px solid #334155;
    border-radius: 8px;
    padding: 8px 16px;
    font-weight: 600;
    box-shadow: none;
    text-shadow: none;
}

button.secondary-btn label {
    color: #F8FAFC;
    font-weight: 600;
}

button.secondary-btn:hover {
    background-color: #28354D;
    border-color: #00F2FE;
    color: #FFFFFF;
}

button.secondary-btn:hover label {
    color: #FFFFFF;
}

.footer-bar {
    background-color: #0F1420;
    border-top: 1px solid #1E293B;
    padding: 10px 18px;
}

checkbutton,
checkbutton label {
    color: #CBD5E1;
    font-size: 12px;
}

checkbutton check {
    background-color: #151C2C;
    background-image: none;
    border: 1px solid #475569;
    border-radius: 4px;
    color: #00F2FE;
}

checkbutton check:checked {
    background-color: #00F2FE;
    border-color: #00F2FE;
    color: #0A0D14;
}

.status-label {
    color: #94A3B8;
    font-size: 12px;
}
"""


def read_cmd(cmd, fallback="Available"):
    try:
        out = subprocess.check_output(cmd, shell=True, stderr=subprocess.DEVNULL, text=True, timeout=2).strip()
        return out if out else fallback
    except Exception:
        return fallback


class AxisWelcomeWindow(Gtk.Window):
    def __init__(self):
        super().__init__(title="Welcome to AxisOS 2.0")
        self.set_default_size(940, 620)
        self.set_position(Gtk.WindowPosition.CENTER)

        settings = Gtk.Settings.get_default()
        if settings is not None:
            settings.set_property("gtk-application-prefer-dark-theme", True)

        provider = Gtk.CssProvider()
        provider.load_from_data(CSS)
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(), provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

        hb = Gtk.HeaderBar()
        hb.set_show_close_button(True)
        hb.set_title("Welcome to AxisOS 2.0 (Liquid Nodes Edition)")
        hb.set_subtitle("System Telemetry, Quick Setup, Biometrics & Signature Features")
        self.set_titlebar(hb)

        root_vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.add(root_vbox)

        main_hbox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=0)
        root_vbox.pack_start(main_hbox, True, True, 0)

        self.stack = Gtk.Stack()
        self.stack.set_transition_type(Gtk.StackTransitionType.SLIDE_UP_DOWN)
        self.stack.set_transition_duration(180)

        sidebar = Gtk.StackSidebar()
        sidebar.set_stack(self.stack)
        sidebar.set_size_request(230, -1)
        sidebar.get_style_context().add_class("sidebar")

        main_hbox.pack_start(sidebar, False, False, 0)
        main_hbox.pack_start(self.stack, True, True, 0)

        self.stack.add_titled(self._build_overview_tab(), "overview", "🏠  Overview & Telemetry")
        self.stack.add_titled(self._build_setup_tab(), "setup", "🚀  Quick Setup & Apps")
        self.stack.add_titled(self._build_gaming_tab(), "gaming", "⚡  Gaming & Kernel Boost")
        self.stack.add_titled(self._build_signature_tab(), "signature", "✨  Signature Features")

        # Footer with Launch at Startup toggle
        footer = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        footer.get_style_context().add_class("footer-bar")

        autostart_file = Path.home() / ".config/autostart/axis-welcome.desktop"
        self.startup_chk = Gtk.CheckButton(label="Show AxisOS Welcome on startup")
        self.startup_chk.set_active(autostart_file.exists())
        self.startup_chk.connect("toggled", self._on_toggle_autostart)
        footer.pack_start(self.startup_chk, False, False, 0)

        self.status_lbl = Gtk.Label(label="Ready.")
        self.status_lbl.set_xalign(0.0)
        self.status_lbl.get_style_context().add_class("status-label")
        footer.pack_start(self.status_lbl, True, True, 0)

        close_btn = Gtk.Button(label="Close")
        close_btn.get_style_context().add_class("secondary-btn")
        close_btn.connect("clicked", lambda *_: self.destroy())
        footer.pack_end(close_btn, False, False, 0)

        root_vbox.pack_start(footer, False, False, 0)

    def _make_page(self, title, subtitle):
        scroll = Gtk.ScrolledWindow()
        scroll.set_policy(Gtk.PolicyType.NEVER, Gtk.PolicyType.AUTOMATIC)
        box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)
        box.set_margin_start(24)
        box.set_margin_end(24)
        box.set_margin_top(20)
        box.set_margin_bottom(20)

        t_lbl = Gtk.Label(label=title)
        t_lbl.set_xalign(0.0)
        t_lbl.get_style_context().add_class("title-hero")
        box.pack_start(t_lbl, False, False, 0)

        s_lbl = Gtk.Label(label=subtitle)
        s_lbl.set_xalign(0.0)
        s_lbl.set_line_wrap(True)
        s_lbl.get_style_context().add_class("subtitle-hero")
        box.pack_start(s_lbl, False, False, 4)

        scroll.add(box)
        return scroll, box

    def _make_card(self, title, desc):
        frame = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        frame.get_style_context().add_class("card")
        lbl = Gtk.Label(label=title)
        lbl.set_xalign(0.0)
        lbl.get_style_context().add_class("card-header")
        frame.pack_start(lbl, False, False, 0)
        d_lbl = Gtk.Label(label=desc)
        d_lbl.set_xalign(0.0)
        d_lbl.set_line_wrap(True)
        d_lbl.get_style_context().add_class("subtitle-hero")
        frame.pack_start(d_lbl, False, False, 0)
        return frame

    def _launch(self, cmd):
        self.status_lbl.set_text(f"Launching: {cmd} ...")
        subprocess.Popen(cmd, shell=True)

    def _build_overview_tab(self):
        scroll, box = self._make_page(
            "Welcome to AxisOS 2.0",
            "A high-reliability Debian 12 Bookworm workstation & gaming OS with Liquid Nodes architecture."
        )
        cpu = read_cmd("grep -m1 'model name' /proc/cpuinfo | cut -d: -f2", platform.processor() or "x86_64 CPU")
        mem = read_cmd("free -h | awk '/^Mem:/ {print $3 \" used / \" $2 \" total\"}'", "Memory Active")
        kern = platform.release()
        gpu = read_cmd("lspci 2>/dev/null | grep -iE 'vga|3d|display' | head -n1 | cut -d: -f3", "Mesa / DRM Graphics Pipeline")

        hw_card = self._make_card(
            "Live Hardware Telemetry",
            f"• Processor: {cpu}\n• Graphics: {gpu}\n• Memory: {mem}\n• Kernel: Linux {kern}"
        )
        box.pack_start(hw_card, False, False, 0)

        act_card = self._make_card("Essential System Actions", "Jump straight into installing or configuring AxisOS.")
        btn_row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        for label, cmd, primary in [
            ("💿 Install AxisOS to Disk", "/usr/local/bin/axis-installer", True),
            ("⚙️ Open Settings", "gnome-control-center", False),
            ("💻 Open AxisOS Terminal", "/usr/local/bin/axis-terminal", False),
        ]:
            b = Gtk.Button(label=label)
            b.get_style_context().add_class("primary-btn" if primary else "secondary-btn")
            b.connect("clicked", lambda _, c=cmd: self._launch(c))
            btn_row.pack_start(b, False, False, 0)
        act_card.pack_start(btn_row, False, False, 4)
        box.pack_start(act_card, False, False, 0)
        return scroll

    def _build_setup_tab(self):
        scroll, box = self._make_page(
            "Quick Setup & Applications",
            "Manage packages, atomic system updates, hardware drivers, and biometric unlock."
        )
        apps = [
            ("Software Manager", "Browse and install Debian APT, Flatpak, and .axis packages.", "/usr/local/bin/axis-software-manager"),
            ("Update Manager", "Apply security updates with automatic Btrfs snapshot protection.", "/usr/local/bin/axis-update-manager"),
            ("Package Installer", "Install standalone .deb and .axis packages.", "/usr/local/bin/axis-package-installer"),
            ("Hardware Drivers & Codecs", "Configure GPU drivers and multimedia codecs.", "/usr/local/bin/nobara-welcome"),
            ("Task Manager", "Inspect CPU, RAM, and running processes in real time.", "/usr/local/bin/axis-task-manager"),
        ]
        for name, desc, cmd in apps:
            c = self._make_card(name, desc)
            b = Gtk.Button(label=f"Launch {name}")
            b.get_style_context().add_class("secondary-btn")
            b.set_halign(Gtk.Align.START)
            b.connect("clicked", lambda _, command=cmd: self._launch(command))
            c.pack_start(b, False, False, 0)
            box.pack_start(c, False, False, 0)
        return scroll

    def _build_gaming_tab(self):
        scroll, box = self._make_page(
            "Gaming & Kernel Scheduler",
            "Unlock peak CPU/GPU clocks, GameMode optimizations, and sched-ext kernel profiles."
        )
        c1 = self._make_card(
            "AxisOS Game Boost & CPU Governor",
            "Pins CPU scaling governors to performance and reduces scheduling latency for gaming and heavy workloads."
        )
        row = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        for lbl, cmd in [
            ("Enable Game Boost", "pkexec /usr/local/bin/axis-game-boost start || /usr/local/bin/axis-game-boost start"),
            ("Disable Game Boost", "pkexec /usr/local/bin/axis-game-boost stop || /usr/local/bin/axis-game-boost stop"),
            ("Open Kernel Scheduler Manager", "/usr/local/bin/axis-scheduler-manager"),
        ]:
            b = Gtk.Button(label=lbl)
            b.get_style_context().add_class("secondary-btn")
            b.connect("clicked", lambda _, command=cmd: self._launch(command))
            row.pack_start(b, False, False, 0)
        c1.pack_start(row, False, False, 0)
        box.pack_start(c1, False, False, 0)
        return scroll

    def _build_signature_tab(self):
        scroll, box = self._make_page(
            "AxisOS 2.0 Signature Exclusive Features",
            "Features engineered specifically for AxisOS 2.0 that set it apart from every other Linux distribution."
        )
        features = [
            ("1. Liquid Nodes Window Piping", "Connect windows with live Bezier pipes in the Qt6 compositor to stream stdout/PipeWire audio between apps.", "/usr/local/bin/axisos-settings-panel signature"),
            ("2. Axis Rewind (CoW Time-Travel)", "Create instant Copy-on-Write checkpoints of app configs/state and roll back in <10ms.", "/usr/local/bin/axis-rewind checkpoint axis-welcome"),
            ("3. NeuralPulse Focus Priority", "Automatically boosts the focused window's CPU nice (-10), I/O priority, and cgroup v2 weight.", "/usr/local/bin/axis-neural-pulse status"),
            ("4. Shoulder-Surfing & Gaze Privacy Shield", "Detects onlookers via V4L2 camera and blurs protected windows automatically.", "/usr/local/bin/axis-gaze-shield scan-once"),
            ("5. One-Click USB Quarantine Vault", "Mounts untrusted USB drives read-only underneath a RAM tmpfs overlayfs + Bubblewrap sandbox.", "/usr/local/bin/axis-usb-vault status"),
            ("6. Offline Self-Healing Terminal Pill", "Detects dpkg locks, missing commands, port conflicts, and disk-full errors with a one-click Fix & Retry pill.", "/usr/local/bin/axis-terminal"),
        ]
        for title, desc, cmd in features:
            c = self._make_card(title, desc)
            b = Gtk.Button(label="Test / Configure")
            b.get_style_context().add_class("secondary-btn")
            b.set_halign(Gtk.Align.START)
            b.connect("clicked", lambda _, command=cmd: self._launch(command))
            c.pack_start(b, False, False, 0)
            box.pack_start(c, False, False, 0)
        return scroll

    def _on_toggle_autostart(self, chk):
        auto_dir = Path.home() / ".config/autostart"
        auto_file = auto_dir / "axis-welcome.desktop"
        if chk.get_active():
            auto_dir.mkdir(parents=True, exist_ok=True)
            auto_file.write_text(
                "[Desktop Entry]\n"
                "Name=AxisOS Welcome\n"
                "Comment=Welcome to AxisOS\n"
                "Exec=/usr/local/bin/axis-welcome\n"
                "Icon=axisos\n"
                "Terminal=false\n"
                "Type=Application\n",
                encoding="utf-8"
            )
            self.status_lbl.set_text("Enabled AxisOS Welcome at login.")
        else:
            if auto_file.exists():
                auto_file.unlink()
            self.status_lbl.set_text("Disabled AxisOS Welcome at login.")


if __name__ == "__main__":
    win = AxisWelcomeWindow()
    win.connect("destroy", Gtk.main_quit)
    win.show_all()
    Gtk.main()
