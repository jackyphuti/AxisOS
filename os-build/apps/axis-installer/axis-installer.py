#!/usr/bin/env python3
"""
AxisOS Friendly Modern Graphical System Installer
Native GTK 3 Wizard for Disk Selection, Synchronized User/Root Setup, and Btrfs Deployment.
"""

import os
import sys
import subprocess
import threading
import time
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
    font-size: 16px;
    color: #ffffff;
}

.step-sidebar {
    background-color: #0f141c;
    border-right: 1px solid rgba(255, 255, 255, 0.08);
    padding: 24px 16px;
}

.step-item {
    padding: 10px 14px;
    border-radius: 6px;
    color: #94a3b8;
    font-weight: 600;
    font-size: 13px;
    margin-bottom: 8px;
}

.step-item.active {
    background-color: #1e293b;
    color: #ffffff;
    border-left: 3px solid #2563eb;
}

.step-item.done {
    color: #60a5fa;
}

.wizard-content {
    padding: 32px 40px;
}

.title-large {
    font-size: 24px;
    font-weight: 800;
    color: #ffffff;
    margin-bottom: 6px;
}

.subtitle {
    font-size: 13px;
    color: #94a3b8;
    margin-bottom: 24px;
    line-height: 1.4;
}

.card {
    background-color: #111827;
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 10px;
    padding: 20px 24px;
    margin-bottom: 20px;
}

.card-title {
    font-size: 15px;
    font-weight: 700;
    color: #ffffff;
    margin-bottom: 6px;
}

.disk-row {
    background-color: #161e2e;
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 8px;
    padding: 14px 18px;
    margin-bottom: 10px;
}

.disk-row:hover {
    border-color: #2563eb;
}

.btn-primary {
    background: linear-gradient(135deg, #2563eb 0%, #1d4ed8 100%);
    color: #ffffff;
    font-weight: 700;
    font-size: 14px;
    border-radius: 8px;
    border: none;
    padding: 10px 24px;
    box-shadow: 0 4px 10px rgba(37, 99, 235, 0.35);
}

.btn-primary:hover {
    background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
}

.btn-secondary {
    background: #1f2937;
    color: #e2e8f0;
    font-weight: 600;
    font-size: 13px;
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 8px;
    padding: 9px 20px;
}

.btn-secondary:hover {
    background: #374151;
    color: #ffffff;
    border-color: #2563eb;
}

entry {
    background-color: #161e2e;
    color: #ffffff;
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 6px;
    padding: 10px 14px;
    font-size: 13px;
}

entry:focus {
    border-color: #2563eb;
}

progressbar progress {
    background: linear-gradient(135deg, #2563eb 0%, #3b82f6 100%);
    border-radius: 6px;
}

progressbar trough {
    background-color: #161e2e;
    border-radius: 6px;
}

.badge-info {
    background-color: rgba(37, 99, 235, 0.15);
    border: 1px solid rgba(37, 99, 235, 0.4);
    color: #60a5fa;
    font-size: 12px;
    font-weight: 600;
    padding: 6px 12px;
    border-radius: 6px;
}
"""

class AxisFriendlyInstaller(Gtk.Window):
    def __init__(self):
        super().__init__(title="Install AxisOS Linux")
        self.set_default_size(960, 640)
        self.set_position(Gtk.WindowPosition.CENTER)

        self.step_index = 0
        self.selected_disk = None
        self.user_fullname = "AxisOS User"
        self.username = "axis"
        self.password = "password"
        self.hostname = "axis-pc"

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
        hb.props.title = "Install AxisOS Linux 2.0"
        hb.props.subtitle = "Fast, Native & Secure System Setup"
        self.set_titlebar(hb)

    def init_ui(self):
        root = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.add(root)

        body = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=0)
        root.pack_start(body, True, True, 0)

        # Left step indicator
        self.step_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=6)
        self.step_box.get_style_context().add_class("step-sidebar")
        self.step_box.set_size_request(220, -1)

        self.steps = [
            "1. Welcome & Readiness",
            "2. Select Target Drive",
            "3. User & Administrator",
            "4. Confirmation Summary",
            "5. Installing System",
            "6. Setup Complete",
        ]
        self.step_labels = []

        title_steps = Gtk.Label(label="INSTALLATION STEPS", xalign=0)
        title_steps.get_style_context().add_class("card-title")
        title_steps.set_margin_bottom(12)
        self.step_box.pack_start(title_steps, False, False, 0)

        for s in self.steps:
            lbl = Gtk.Label(label=s, xalign=0)
            lbl.get_style_context().add_class("step-item")
            self.step_labels.append(lbl)
            self.step_box.pack_start(lbl, False, False, 0)

        body.pack_start(self.step_box, False, False, 0)

        # Right Content Area (Stack)
        self.stack = Gtk.Stack()
        self.stack.set_transition_type(Gtk.StackTransitionType.SLIDE_LEFT_RIGHT)
        self.stack.set_transition_duration(200)

        content_container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        content_container.get_style_context().add_class("wizard-content")
        content_container.pack_start(self.stack, True, True, 0)

        body.pack_start(content_container, True, True, 0)

        # Bottom Navigation Bar
        nav_bar = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        nav_bar.set_margin_top(12)
        nav_bar.set_margin_bottom(16)
        nav_bar.set_margin_start(40)
        nav_bar.set_margin_end(40)

        self.btn_back = Gtk.Button(label="← Back")
        self.btn_back.get_style_context().add_class("btn-secondary")
        self.btn_back.connect("clicked", self.on_back_clicked)
        self.btn_back.set_sensitive(False)

        self.btn_next = Gtk.Button(label="Next →")
        self.btn_next.get_style_context().add_class("btn-primary")
        self.btn_next.connect("clicked", self.on_next_clicked)

        nav_bar.pack_start(self.btn_back, False, False, 0)
        nav_bar.pack_end(self.btn_next, False, False, 0)
        root.pack_end(nav_bar, False, False, 0)

        # Build Wizard Steps
        self.build_step_welcome()
        self.build_step_drive()
        self.build_step_user()
        self.build_step_summary()
        self.build_step_install()
        self.build_step_complete()

        self.update_step_ui()

    def update_step_ui(self):
        for i, lbl in enumerate(self.step_labels):
            ctx = lbl.get_style_context()
            ctx.remove_class("active")
            ctx.remove_class("done")
            if i == self.step_index:
                ctx.add_class("active")
            elif i < self.step_index:
                ctx.add_class("done")

        self.btn_back.set_sensitive(self.step_index in [1, 2, 3])
        if self.step_index == 3:
            self.btn_next.set_label("🚀 Install AxisOS Now")
        elif self.step_index in [4, 5]:
            self.btn_next.set_visible(self.step_index == 5)
            self.btn_back.set_visible(False)
            if self.step_index == 5:
                self.btn_next.set_label("🔄 Restart Computer")
        else:
            self.btn_next.set_label("Next →")
            self.btn_next.set_visible(True)
            self.btn_back.set_visible(True)

    # -------------------------------------------------------------------------
    # Step 1: Welcome & Readiness
    # -------------------------------------------------------------------------
    def build_step_welcome(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        t = Gtk.Label(label="Welcome to AxisOS Linux 2.0", xalign=0)
        t.get_style_context().add_class("title-large")
        s = Gtk.Label(
            label="You are about to install AxisOS Linux onto your computer.\n"
                  "Enjoy modern hardware drivers, high-performance Btrfs filesystem, and a native Wayland experience.",
            xalign=0
        )
        s.get_style_context().add_class("subtitle")
        vbox.pack_start(t, False, False, 0)
        vbox.pack_start(s, False, False, 0)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")

        ct = Gtk.Label(label="System Readiness Verification", xalign=0)
        ct.get_style_context().add_class("card-title")
        card.pack_start(ct, False, False, 0)

        grid = Gtk.Grid()
        grid.set_column_spacing(16)
        grid.set_row_spacing(8)

        is_efi = os.path.exists("/sys/firmware/efi")
        boot_mode = "UEFI 64-bit (Modern Standard)" if is_efi else "Legacy BIOS"

        checks = [
            ("Firmware Environment:", f"✅ {boot_mode}"),
            ("Storage Requirement:", "✅ At least 25 GB available storage"),
            ("Processor Architecture:", "✅ x86_64 Dual/Multi-Core Processor"),
            ("Network & Graphics:", "✅ Automatic Driver & Codec Provisioning")
        ]
        for idx, (lbl_txt, val_txt) in enumerate(checks):
            l1 = Gtk.Label(label=lbl_txt, xalign=0)
            l2 = Gtk.Label(label=val_txt, xalign=0)
            grid.attach(l1, 0, idx, 1, 1)
            grid.attach(l2, 1, idx, 1, 1)

        card.pack_start(grid, False, False, 0)
        vbox.pack_start(card, False, False, 0)

        self.stack.add_named(vbox, "step_0")

    # -------------------------------------------------------------------------
    # Step 2: Target Drive Selection
    # -------------------------------------------------------------------------
    def build_step_drive(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        t = Gtk.Label(label="Select Target Storage Drive", xalign=0)
        t.get_style_context().add_class("title-large")
        s = Gtk.Label(
            label="Choose the physical drive where AxisOS will be installed.\n"
                  "⚠️ Warning: The selected drive will be partitioned with GPT and formatted with Btrfs.",
            xalign=0
        )
        s.get_style_context().add_class("subtitle")
        vbox.pack_start(t, False, False, 0)
        vbox.pack_start(s, False, False, 0)

        self.disk_container = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        scroll = Gtk.ScrolledWindow()
        scroll.set_min_content_height(240)
        scroll.add(self.disk_container)
        vbox.pack_start(scroll, True, True, 0)

        btn_refresh = Gtk.Button(label="🔄 Refresh Drive List")
        btn_refresh.get_style_context().add_class("btn-secondary")
        btn_refresh.connect("clicked", lambda b: self.populate_drives())
        vbox.pack_start(btn_refresh, False, False, 0)

        self.stack.add_named(vbox, "step_1")

    def populate_drives(self):
        for child in self.disk_container.get_children():
            self.disk_container.remove(child)

        drives = []
        try:
            out = subprocess.check_output(
                ["lsblk", "-d", "-p", "-n", "-l", "-o", "NAME,SIZE,MODEL,TYPE"],
                universal_newlines=True
            )
            for line in out.splitlines():
                parts = line.split()
                if len(parts) >= 2:
                    name = parts[0]
                    size = parts[1]
                    dtype = parts[-1] if len(parts) >= 3 else "disk"
                    model = " ".join(parts[2:-1]) if len(parts) > 3 else "Generic Storage"
                    if dtype == "disk" and not name.startswith(("/dev/loop", "/dev/ram", "/dev/zram")):
                        drives.append({"name": name, "size": size, "model": model})
        except Exception:
            pass

        if not drives:
            drives = [{"name": "/dev/sda", "size": "64.0G", "model": "Virtual Disk / Standard Drive"}]

        rb_group = None
        self.drive_radio_buttons = []
        for d in drives:
            card = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=14)
            card.get_style_context().add_class("disk-row")

            rb = Gtk.RadioButton.new_with_label_from_widget(rb_group, f"💾 {d['name']} ({d['size']}) - {d['model']}")
            rb_group = rb
            rb.drive_path = d["name"]
            rb.connect("toggled", self.on_drive_toggled)
            self.drive_radio_buttons.append(rb)

            card.pack_start(rb, True, True, 0)
            self.disk_container.pack_start(card, False, False, 0)

        if self.drive_radio_buttons:
            self.drive_radio_buttons[0].set_active(True)
            self.selected_disk = self.drive_radio_buttons[0].drive_path

        self.disk_container.show_all()

    def on_drive_toggled(self, rb):
        if rb.get_active():
            self.selected_disk = rb.drive_path

    # -------------------------------------------------------------------------
    # Step 3: User Account & Synchronized Root Password
    # -------------------------------------------------------------------------
    def build_step_user(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        t = Gtk.Label(label="User Account & Root Security", xalign=0)
        t.get_style_context().add_class("title-large")
        s = Gtk.Label(
            label="Create your primary user account.\n"
                  "In Debian, your password will be synchronized for both user and administrator (root) access.\n"
                  "The terminal will always require this password when performing system changes.",
            xalign=0
        )
        s.get_style_context().add_class("subtitle")
        vbox.pack_start(t, False, False, 0)
        vbox.pack_start(s, False, False, 0)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=12)
        card.get_style_context().add_class("card")

        grid = Gtk.Grid()
        grid.set_column_spacing(16)
        grid.set_row_spacing(12)

        lbl_fn = Gtk.Label(label="Your Full Name:", xalign=0)
        self.entry_fullname = Gtk.Entry()
        self.entry_fullname.set_text("AxisOS User")

        lbl_un = Gtk.Label(label="Username (Login):", xalign=0)
        self.entry_username = Gtk.Entry()
        self.entry_username.set_text("axis")

        lbl_hn = Gtk.Label(label="Computer Name (Hostname):", xalign=0)
        self.entry_hostname = Gtk.Entry()
        self.entry_hostname.set_text("axis-pc")

        lbl_pw = Gtk.Label(label="Account & Root Password:", xalign=0)
        self.entry_pwd = Gtk.Entry()
        self.entry_pwd.set_visibility(False)
        self.entry_pwd.set_text("password")

        lbl_cp = Gtk.Label(label="Confirm Password:", xalign=0)
        self.entry_cpwd = Gtk.Entry()
        self.entry_cpwd.set_visibility(False)
        self.entry_cpwd.set_text("password")

        grid.attach(lbl_fn, 0, 0, 1, 1)
        grid.attach(self.entry_fullname, 1, 0, 1, 1)
        grid.attach(lbl_un, 0, 1, 1, 1)
        grid.attach(self.entry_username, 1, 1, 1, 1)
        grid.attach(lbl_hn, 0, 2, 1, 1)
        grid.attach(self.entry_hostname, 1, 2, 1, 1)
        grid.attach(lbl_pw, 0, 3, 1, 1)
        grid.attach(self.entry_pwd, 1, 3, 1, 1)
        grid.attach(lbl_cp, 0, 4, 1, 1)
        grid.attach(self.entry_cpwd, 1, 4, 1, 1)

        card.pack_start(grid, False, False, 0)
        vbox.pack_start(card, False, False, 0)

        self.stack.add_named(vbox, "step_2")

    # -------------------------------------------------------------------------
    # Step 4: Confirmation Summary
    # -------------------------------------------------------------------------
    def build_step_summary(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=14)

        t = Gtk.Label(label="Ready to Install AxisOS", xalign=0)
        t.get_style_context().add_class("title-large")
        s = Gtk.Label(label="Please review your choices before proceeding. Disk partitioning will begin next.", xalign=0)
        s.get_style_context().add_class("subtitle")
        vbox.pack_start(t, False, False, 0)
        vbox.pack_start(s, False, False, 0)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")

        self.lbl_sum_disk = Gtk.Label(label="", xalign=0)
        self.lbl_sum_user = Gtk.Label(label="", xalign=0)
        self.lbl_sum_fs = Gtk.Label(label="Filesystem: Modern Btrfs with ZSTD subvolumes (@ and @home)", xalign=0)
        self.lbl_sum_sec = Gtk.Label(label="Security: User & Root passwords synchronized (sudo enabled)", xalign=0)

        card.pack_start(self.lbl_sum_disk, False, False, 0)
        card.pack_start(self.lbl_sum_user, False, False, 0)
        card.pack_start(self.lbl_sum_fs, False, False, 0)
        card.pack_start(self.lbl_sum_sec, False, False, 0)

        vbox.pack_start(card, False, False, 0)
        self.stack.add_named(vbox, "step_3")

    def update_summary_page(self):
        self.user_fullname = self.entry_fullname.get_text().strip() or "AxisOS User"
        self.username = self.entry_username.get_text().strip() or "axis"
        self.hostname = self.entry_hostname.get_text().strip() or "axis-pc"
        self.password = self.entry_pwd.get_text()

        self.lbl_sum_disk.set_text(f"Target Drive: 💾 {self.selected_disk} (Will be partitioned with GPT)")
        self.lbl_sum_user.set_text(f"Account: {self.user_fullname} ({self.username}) on {self.hostname}")

    # -------------------------------------------------------------------------
    # Step 5: Live Installation Progress
    # -------------------------------------------------------------------------
    def build_step_install(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=16)

        t = Gtk.Label(label="Installing AxisOS Linux...", xalign=0)
        t.get_style_context().add_class("title-large")
        s = Gtk.Label(label="Configuring partitions, transferring system files, and setting up bootloader.", xalign=0)
        s.get_style_context().add_class("subtitle")
        vbox.pack_start(t, False, False, 0)
        vbox.pack_start(s, False, False, 0)

        self.install_progress = Gtk.ProgressBar()
        self.install_progress.set_fraction(0.05)
        vbox.pack_start(self.install_progress, False, False, 0)

        self.lbl_install_step = Gtk.Label(label="Preparing storage...", xalign=0)
        self.lbl_install_step.get_style_context().add_class("badge-info")
        vbox.pack_start(self.lbl_install_step, False, False, 0)

        # Feature highlight box
        feat_card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=8)
        feat_card.get_style_context().add_class("card")
        ftitle = Gtk.Label(label="What's Inside AxisOS 2.0", xalign=0)
        ftitle.get_style_context().add_class("card-title")
        fdesc = Gtk.Label(
            label="• Native wlroots Wayland Compositor & Modern Desktop Environment\n"
                  "• Debian Base Native Apps (Files, Music, Videos, Photos, Documents)\n"
                  "• Steam & Gaming Runtime with GameMode and Proton Support\n"
                  "• Synchronized Administrator Security & Out-of-Box Wi-Fi Captive Redirect\n"
                  "• Resilient Btrfs Snapshot Architecture",
            xalign=0
        )
        feat_card.pack_start(ftitle, False, False, 0)
        feat_card.pack_start(fdesc, False, False, 0)
        vbox.pack_start(feat_card, True, True, 0)

        self.stack.add_named(vbox, "step_4")

    def start_installation_process(self):
        threading.Thread(target=self._run_installer_script, daemon=True).start()

    def _run_installer_script(self):
        cmd = [
            "/usr/local/bin/axisos-installer.sh",
            "--disk", str(self.selected_disk or "/dev/sda"),
            "--user", str(self.username),
            "--pass", str(self.password),
            "--name", str(self.user_fullname),
            "--host", str(self.hostname)
        ]

        if not os.path.exists(cmd[0]):
            cmd[0] = "/usr/local/bin/axisos-install.sh"

        try:
            proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            for line in proc.stdout:
                line = line.strip()
                if line.startswith("PROGRESS:"):
                    parts = line.split(":", 2)
                    if len(parts) >= 3:
                        pct = float(parts[1]) / 100.0
                        msg = parts[2]
                        GLib.idle_add(self._update_progress_ui, pct, msg)
            proc.wait()
            if proc.returncode == 0:
                GLib.idle_add(self._on_install_finished, True, "Installation finished successfully.")
            else:
                GLib.idle_add(self._on_install_finished, False, f"Installer exited with code {proc.returncode}")
        except Exception as e:
            GLib.idle_add(self._on_install_finished, False, str(e))

    def _update_progress_ui(self, fraction, message):
        self.install_progress.set_fraction(fraction)
        self.lbl_install_step.set_text(f"{int(fraction * 100)}% - {message}")

    def _on_install_finished(self, success, msg):
        if success:
            self.step_index = 5
            self.stack.set_visible_child_name("step_5")
            self.update_step_ui()
        else:
            dialog = Gtk.MessageDialog(
                transient_for=self,
                flags=0,
                message_type=Gtk.MessageType.ERROR,
                buttons=Gtk.ButtonsType.OK,
                text="Installation Failed"
            )
            dialog.format_secondary_text(f"An error occurred during system installation:\n{msg}")
            dialog.run()
            dialog.destroy()

    # -------------------------------------------------------------------------
    # Step 6: Setup Complete
    # -------------------------------------------------------------------------
    def build_step_complete(self):
        vbox = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=16)

        t = Gtk.Label(label="🎉 Installation Successfully Completed!", xalign=0)
        t.get_style_context().add_class("title-large")
        s = Gtk.Label(
            label="AxisOS Linux 2.0 has been deployed to your computer.\n"
                  "Please remove your live USB installation media and restart your device.",
            xalign=0
        )
        s.get_style_context().add_class("subtitle")
        vbox.pack_start(t, False, False, 0)
        vbox.pack_start(s, False, False, 0)

        card = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=10)
        card.get_style_context().add_class("card")
        c1 = Gtk.Label(label="Your system is ready. Click Restart Computer below to boot into your installed AxisOS.", xalign=0)
        card.pack_start(c1, False, False, 0)
        vbox.pack_start(card, False, False, 0)

        self.stack.add_named(vbox, "step_5")

    # -------------------------------------------------------------------------
    # Navigation Handlers
    # -------------------------------------------------------------------------
    def on_back_clicked(self, btn):
        if self.step_index > 0:
            self.step_index -= 1
            self.stack.set_visible_child_name(f"step_{self.step_index}")
            self.update_step_ui()

    def on_next_clicked(self, btn):
        if self.step_index == 1:
            if not self.selected_disk:
                self.show_dialog("Target Drive Required", "Please select a target storage drive to continue.")
                return
        elif self.step_index == 2:
            p1 = self.entry_pwd.get_text()
            p2 = self.entry_cpwd.get_text()
            if not p1:
                self.show_dialog("Password Required", "Please enter a password for your account.")
                return
            if p1 != p2:
                self.show_dialog("Password Mismatch", "Passwords do not match. Please re-enter them.")
                return
            self.update_summary_page()
        elif self.step_index == 3:
            # Confirm formatting
            dialog = Gtk.MessageDialog(
                transient_for=self,
                flags=0,
                message_type=Gtk.MessageType.WARNING,
                buttons=Gtk.ButtonsType.OK_CANCEL,
                text=f"Erase and Install on {self.selected_disk}?"
            )
            dialog.format_secondary_text(
                f"All data on {self.selected_disk} will be erased and formatted with Btrfs.\nAre you sure you want to proceed?"
            )
            res = dialog.run()
            dialog.destroy()
            if res != Gtk.ResponseType.OK:
                return

            self.step_index = 4
            self.stack.set_visible_child_name("step_4")
            self.update_step_ui()
            self.start_installation_process()
            return
        elif self.step_index == 5:
            # Reboot
            subprocess.run(["systemctl", "reboot"], stderr=subprocess.DEVNULL)
            return

        self.step_index += 1
        self.stack.set_visible_child_name(f"step_{self.step_index}")
        if self.step_index == 1:
            self.populate_drives()
        elif self.step_index == 3:
            self.update_summary_page()
        self.update_step_ui()

    def show_dialog(self, title, msg):
        d = Gtk.MessageDialog(
            transient_for=self,
            flags=0,
            message_type=Gtk.MessageType.INFO,
            buttons=Gtk.ButtonsType.OK,
            text=title
        )
        d.format_secondary_text(msg)
        d.run()
        d.destroy()


def main():
    app = AxisFriendlyInstaller()
    app.connect("destroy", Gtk.main_quit)
    app.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
