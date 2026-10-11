#!/usr/bin/env python3
"""
AxisOS HarmonyOS-Inspired Installer
Built with GTK 4 and libadwaita using single-source design tokens.
"""

import sys
import os
import gi

gi.require_version("Gtk", "4.0")
gi.require_version("Adw", "1")
from gi.repository import Gtk, Adw, Gdk, GLib, Gio

# Import shared theme manager
sys.path.insert(0, os.path.dirname(__file__))
from theme_manager import ThemeManager


class InstallerWindow(Adw.ApplicationWindow):
    PAGES = ["welcome", "disk", "user", "review", "install", "done"]
    INSTALL_STEPS = [
        "Preparing NVMe and storage partitions",
        "Copying AxisOS system root and kernel",
        "Deploying Mesa Vulkan and graphics drivers",
        "Configuring user credentials and security",
        "Finishing installation and preparing live system",
    ]

    def __init__(self, app):
        super().__init__(
            application=app,
            title="Install AxisOS",
            default_width=780,
            default_height=560
        )
        self.index = 0
        self.progress_value = 0.0
        self.config = {"disk": "", "username": "", "hostname": "axisos"}

        main_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL)

        # Headerbar with clean step dots
        header = Adw.HeaderBar()
        self.dots = Gtk.Box(spacing=8, halign=Gtk.Align.CENTER)
        self.dot_widgets = []
        for _ in self.PAGES:
            dot = Gtk.Box()
            dot.add_css_class("step-dot")
            self.dots.append(dot)
            self.dot_widgets.append(dot)
        header.set_title_widget(self.dots)
        main_box.append(header)

        # Multi-page Stack
        self.stack = Gtk.Stack(
            transition_type=Gtk.StackTransitionType.SLIDE_LEFT_RIGHT,
            transition_duration=280,
            vexpand=True,
            hexpand=True
        )
        self.stack.add_named(self.build_welcome(), "welcome")
        self.stack.add_named(self.build_disk(), "disk")
        self.stack.add_named(self.build_user(), "user")
        self.stack.add_named(self.build_review(), "review")
        self.stack.add_named(self.build_install(), "install")
        self.stack.add_named(self.build_done(), "done")
        main_box.append(self.stack)

        # Footer Actions
        footer = Gtk.Box(
            spacing=12,
            margin_top=12,
            margin_bottom=18,
            margin_start=28,
            margin_end=28
        )
        self.back_btn = Gtk.Button(label="Back")
        self.back_btn.add_css_class("secondary-button")
        self.back_btn.connect("clicked", lambda *_: self.go(-1))

        self.next_btn = Gtk.Button(label="Continue")
        self.next_btn.add_css_class("accent-button")
        self.next_btn.connect("clicked", lambda *_: self.go(1))

        footer.append(self.back_btn)
        footer.append(Gtk.Box(hexpand=True))
        footer.append(self.next_btn)
        main_box.append(footer)

        self.set_content(main_box)
        self.refresh()

    def build_welcome(self):
        page = Adw.StatusPage(
            icon_name="system-software-install-symbolic",
            title="Welcome to AxisOS",
            description="A high-performance, private, and fluid operating system.\nSetup takes approximately five to ten minutes.",
        )
        return page

    def build_disk(self):
        page = Adw.Clamp(maximum_size=560, margin_top=24)
        group = Adw.PreferencesGroup(
            title="Target Installation Drive",
            description="Select the drive to install AxisOS. All existing data on this drive will be replaced."
        )

        # Scan available disks or provide default list
        disks = [
            "/dev/nvme0n1 · 512 GB High-Speed NVMe SSD",
            "/dev/sda · 1.0 TB Internal SATA SSD",
            "/dev/sdb · 256 GB Portable Flash Drive"
        ]
        self.disk_row = Adw.ComboRow(title="Install Disk")
        self.disk_row.set_model(Gtk.StringList.new(disks))
        group.add(self.disk_row)

        row_fs = Adw.ActionRow(title="Filesystem Type", subtitle="Btrfs with Transparent ZSTD Compression")
        group.add(row_fs)

        page.set_child(group)
        return page

    def build_user(self):
        page = Adw.Clamp(maximum_size=560, margin_top=24)
        group = Adw.PreferencesGroup(
            title="User Account &amp; Security",
            description="Create your initial user account. Root administrative privileges will be synchronized."
        )
        self.user_row = Adw.EntryRow(title="Username")
        self.user_row.set_text("axis")

        self.host_row = Adw.EntryRow(title="Computer Name")
        self.host_row.set_text("axisos-pc")

        self.pass_row = Adw.PasswordEntryRow(title="Password")
        self.pass_row.set_text("axis")

        group.add(self.user_row)
        group.add(self.host_row)
        group.add(self.pass_row)

        page.set_child(group)
        return page

    def build_review(self):
        page = Adw.Clamp(maximum_size=560, margin_top=24)
        self.review_group = Adw.PreferencesGroup(
            title="Ready to Install?",
            description="Please review your choices before proceeding with disk partitioning."
        )
        page.set_child(self.review_group)
        self.review_rows = []
        for title in ("Target Disk", "User Account", "Computer Name", "Architecture"):
            row = Adw.ActionRow(title=title)
            self.review_group.add(row)
            self.review_rows.append(row)
        return page

    def build_install(self):
        box = Gtk.Box(
            orientation=Gtk.Orientation.VERTICAL,
            spacing=18,
            valign=Gtk.Align.CENTER,
            halign=Gtk.Align.CENTER
        )
        title = Gtk.Label(label="Installing AxisOS")
        title.add_css_class("title-1")

        self.progress = Gtk.ProgressBar(width_request=440)

        self.status = Gtk.Label(label="Starting installation sequence…")
        self.status.add_css_class("dim-label")

        box.append(title)
        box.append(self.progress)
        box.append(self.status)
        return box

    def build_done(self):
        status = Adw.StatusPage(
            icon_name="emblem-ok-symbolic",
            title="AxisOS is Installed",
            description="Installation has completed successfully.\nPlease remove the USB installation drive and restart your PC.",
        )
        restart = Gtk.Button(label="Restart Now")
        restart.add_css_class("accent-button")
        restart.set_halign(Gtk.Align.CENTER)
        restart.connect("clicked", self.on_restart_clicked)
        status.set_child(restart)
        return status

    def on_restart_clicked(self, _):
        # In a real environment: systemctl reboot
        print("Reboot triggered from Installer.")
        self.close()

    def go(self, delta):
        target = self.index + delta
        if target < 0 or target >= len(self.PAGES):
            return
        if delta > 0 and not self.validate(self.PAGES[self.index]):
            return

        self.index = target
        name = self.PAGES[target]
        self.stack.set_visible_child_name(name)
        self.refresh()

        if name == "review":
            self.fill_review()
        elif name == "install":
            self.start_install()

    def validate(self, page):
        if page == "user":
            username = self.user_row.get_text().strip()
            if not username:
                self.user_row.add_css_class("error")
                return False
            self.user_row.remove_css_class("error")
        return True

    def refresh(self):
        page = self.PAGES[self.index]
        for i, dot in enumerate(self.dot_widgets):
            if i == self.index:
                dot.add_css_class("active")
            else:
                dot.remove_css_class("active")

        busy = page in ("install", "done")
        self.back_btn.set_visible(self.index > 0 and not busy)
        self.next_btn.set_visible(not busy)
        self.next_btn.set_label("Begin Installation" if page == "review" else "Continue")

    def fill_review(self):
        item = self.disk_row.get_selected_item()
        disk = item.get_string() if item else "/dev/nvme0n1"
        self.config["disk"] = disk
        self.config["username"] = self.user_row.get_text().strip()
        self.config["hostname"] = self.host_row.get_text().strip() or "axisos"

        values = [
            disk,
            self.config["username"],
            self.config["hostname"],
            "x86_64 UEFI Dual-Boot Live"
        ]
        for row, value in zip(self.review_rows, values):
            row.set_subtitle(value)

    def start_install(self):
        self.progress_value = 0.0
        self.progress.set_fraction(0.0)
        GLib.timeout_add(70, self.tick_install)

    def tick_install(self):
        self.progress_value = min(1.0, self.progress_value + 0.015)
        self.progress.set_fraction(self.progress_value)

        step_idx = min(
            int(self.progress_value * len(self.INSTALL_STEPS)),
            len(self.INSTALL_STEPS) - 1
        )
        self.status.set_text(self.INSTALL_STEPS[step_idx] + "…")

        if self.progress_value >= 1.0:
            self.index = self.PAGES.index("done")
            self.stack.set_visible_child_name("done")
            self.refresh()
            return False
        return True


class AxisInstallerApp(Adw.Application):
    def __init__(self):
        super().__init__(
            application_id="org.axisos.Installer",
            flags=Gio.ApplicationFlags.DEFAULT_FLAGS
        )
        self.connect("activate", self.on_activate)

    def on_activate(self, app):
        ThemeManager.get_default().init_app(app)
        win = InstallerWindow(app)
        win.present()


def main():
    app = AxisInstallerApp()
    return app.run(sys.argv)


if __name__ == "__main__":
    sys.exit(main())
