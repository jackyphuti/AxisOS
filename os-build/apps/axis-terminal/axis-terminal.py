#!/usr/bin/env python3
"""
AxisOS 2.0 Native Terminal Emulator (axis-terminal)
===================================================
- Automatically uses Vte.Terminal (gir1.2-vte-2.91) when available, or falls back
  to a built-in native Linux PTY (pty.openpty) + GTK 3 interactive terminal widget
  with zero external dependencies beyond python3-gi + gir1.2-gtk-3.0.
- Supports Multi-Tab sessions, Ctrl+Shift+C (Copy), Ctrl+Shift+V (Paste),
  and the AxisOS 2.0 Offline Self-Healing Terminal Pill ("⚡ Fix & Retry").
"""

import os
import sys
import pty
import re
import fcntl
import struct
import termios
import signal
import subprocess
import gi

gi.require_version("Gtk", "3.0")
gi.require_version("Gdk", "3.0")
from gi.repository import Gtk, Gdk, GLib, Pango

HAS_VTE = False
try:
    gi.require_version("Vte", "2.91")
    from gi.repository import Vte
    HAS_VTE = True
except Exception:
    HAS_VTE = False

ANSI_ESCAPE_RE = re.compile(r'\x1b\[[0-9;?]*[a-zA-Z]|\x1b\][^\x07]*\x07|\x1b\][^\x1b]*\x1b\\|\r')

CSS = b"""
window {
    background-color: #1E1E2E;
    color: #F2F3F7;
}
headerbar {
    background-color: #181825;
    color: #F2F3F7;
    border-bottom: 1px solid #2E3349;
}
notebook header {
    background-color: #181825;
    border-bottom: 1px solid #2E3349;
}
notebook tab {
    background-color: #181825;
    color: #9EA3B5;
    padding: 6px 16px;
    border: 1px solid #2E3349;
    border-radius: 8px 8px 0 0;
}
notebook tab:checked {
    background-color: #1E1E2E;
    color: #7D95FF;
    border-bottom: 2px solid #7D95FF;
}
textview, textview text {
    background-color: #1E1E2E;
    color: #F2F3F7;
    font-family: "JetBrains Mono", "Fira Code", "DejaVu Sans Mono", monospace;
    font-size: 11pt;
}
entry.cmd-entry {
    background-color: #181825;
    color: #7D95FF;
    border: 1px solid #2E3349;
    border-radius: 999px;
    padding: 6px 14px;
    font-family: "JetBrains Mono", "DejaVu Sans Mono", monospace;
    font-size: 11pt;
}
.heal-pill {
    background-color: #2A3366;
    border: 1px solid #7D95FF;
    border-radius: 12px;
    padding: 8px 14px;
}
button.heal-btn {
    background: #1428A0;
    color: #FFFFFF;
    border-radius: 999px;
    padding: 6px 14px;
    font-weight: bold;
}
button.action-btn {
    background-color: #181825;
    color: #F2F3F7;
    border: 1px solid #2E3349;
    border-radius: 999px;
    padding: 4px 12px;
}
"""

def analyze_terminal_error(text):
    lower = text.lower()
    if "could not get lock /var/lib/dpkg/lock" in lower or "unable to lock directory /var/lib/apt/" in lower:
        return (
            "APT/dpkg package manager lock detected",
            "sudo rm -f /var/lib/apt/lists/lock /var/cache/apt/archives/lock /var/lib/dpkg/lock* && sudo dpkg --configure -a"
        )
    m_cmd = re.search(r"([a-zA-Z0-9_.+-]+):\s*command not found", text)
    if m_cmd:
        cmd = m_cmd.group(1)
        return (
            f"Missing command '{cmd}'",
            f"sudo apt-get update && sudo apt-get install -y {cmd}"
        )
    m_port = re.search(r"(?:EADDRINUSE|address already in use).*?:(\d{2,5})", text, re.IGNORECASE)
    if m_port:
        port = m_port.group(1)
        return (
            f"Port {port} is already in use (EADDRINUSE)",
            f"sudo fuser -k {port}/tcp"
        )
    if "error while loading shared libraries:" in lower:
        return (
            "Missing shared library cache or package",
            "sudo ldconfig && sudo apt-get install -f -y"
        )
    if "no space left on device" in lower or "enospc" in lower:
        return (
            "Disk space exhausted (ENOSPC)",
            "sudo apt-get clean && sudo journalctl --vacuum-size=50M"
        )
    if "permission denied" in lower and ("/usr/" in lower or "/etc/" in lower or "/var/" in lower or "apt" in lower):
        return (
            "Elevated privileges required (Permission denied)",
            "sudo !!"
        )
    return None, None


class PtyTab(Gtk.Box):
    """Interactive terminal tab using Vte.Terminal or built-in PTY + Gtk.TextView."""
    def __init__(self, on_error_detected):
        super().__init__(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.on_error_detected = on_error_detected
        self.master_fd = None
        self.child_pid = None
        self.vte = None
        self.history = []
        self.hist_idx = -1

        if HAS_VTE:
            self._init_vte()
        else:
            self._init_native_pty()

    def _init_vte(self):
        self.vte = Vte.Terminal()
        self.vte.set_scrollback_lines(10000)
        self.vte.set_font(Pango.FontDescription("Monospace 11"))
        fg = Gdk.RGBA()
        fg.parse("#E2E8F0")
        bg = Gdk.RGBA()
        bg.parse("#0A0D14")
        self.vte.set_color_foreground(fg)
        self.vte.set_color_background(bg)

        scroll = Gtk.ScrolledWindow()
        scroll.set_hexpand(True)
        scroll.set_vexpand(True)
        scroll.add(self.vte)
        self.pack_start(scroll, True, True, 0)

        envv = [f"{k}={v}" for k, v in os.environ.items()]
        envv.append("TERM=xterm-256color")
        envv.append("COLORTERM=truecolor")
        try:
            self.vte.spawn_sync(
                Vte.PtyFlags.DEFAULT,
                os.environ.get("HOME", "/home/axis"),
                ["/bin/bash"],
                envv,
                GLib.SpawnFlags.DO_NOT_REAP_CHILD,
                None,
                None,
            )
        except Exception:
            pass

    def _init_native_pty(self):
        self.textview = Gtk.TextView()
        self.textview.set_editable(False)
        self.textview.set_cursor_visible(True)
        self.textview.set_wrap_mode(Gtk.WrapMode.CHAR)
        self.textview.set_left_margin(10)
        self.textview.set_right_margin(10)
        self.textview.set_top_margin(8)
        self.textview.set_bottom_margin(8)
        self.buffer = self.textview.get_buffer()

        scroll = Gtk.ScrolledWindow()
        scroll.set_hexpand(True)
        scroll.set_vexpand(True)
        scroll.add(self.textview)
        self.pack_start(scroll, True, True, 0)

        # Interactive command bar at bottom for full line editing + raw key forwarding on TextView
        cmd_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=6)
        cmd_box.set_margin_start(8)
        cmd_box.set_margin_end(8)
        cmd_box.set_margin_top(4)
        cmd_box.set_margin_bottom(6)

        prompt_lbl = Gtk.Label(label="axis@axisos:~$")
        prompt_lbl.get_style_context().add_class("action-btn")
        cmd_box.pack_start(prompt_lbl, False, False, 0)

        self.entry = Gtk.Entry()
        self.entry.set_placeholder_text("Type a command and press Enter (Up/Down for history, Ctrl+C to interrupt)...")
        self.entry.get_style_context().add_class("cmd-entry")
        self.entry.connect("activate", self._on_entry_activate)
        self.entry.connect("key-press-event", self._on_entry_key)
        cmd_box.pack_start(self.entry, True, True, 0)

        int_btn = Gtk.Button(label="Ctrl+C")
        int_btn.get_style_context().add_class("action-btn")
        int_btn.connect("clicked", lambda *_: self.send_command("\x03", append_newline=False))
        cmd_box.pack_start(int_btn, False, False, 0)

        clr_btn = Gtk.Button(label="Clear")
        clr_btn.get_style_context().add_class("action-btn")
        clr_btn.connect("clicked", lambda *_: self.buffer.set_text(""))
        cmd_box.pack_start(clr_btn, False, False, 0)

        self.pack_start(cmd_box, False, False, 0)

        # Spawn real bash shell inside PTY
        try:
            pid, master_fd = pty.fork()
            if pid == 0:
                os.environ["TERM"] = "dumb"
                os.environ["PS1"] = "axis@axisos:\\w\\$ "
                os.environ["PATH"] = "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
                home = os.environ.get("HOME", "/home/axis")
                if os.path.isdir(home):
                    os.chdir(home)
                os.execvp("/bin/bash", ["/bin/bash", "--norc", "-i"])
            else:
                self.child_pid = pid
                self.master_fd = master_fd
                fl = fcntl.fcntl(self.master_fd, fcntl.F_GETFL)
                fcntl.fcntl(self.master_fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)
                GLib.io_add_watch(
                    self.master_fd,
                    GLib.IOCondition.IN | GLib.IOCondition.HUP | GLib.IOCondition.ERR,
                    self._on_pty_output
                )
        except Exception as e:
            self._append_text(f"[AxisOS Terminal] Failed to allocate PTY: {e}\n")

    def _append_text(self, raw_text):
        clean = ANSI_ESCAPE_RE.sub("", raw_text)
        end_iter = self.buffer.get_end_iter()
        self.buffer.insert(end_iter, clean)
        mark = self.buffer.create_mark(None, self.buffer.get_end_iter(), False)
        self.textview.scroll_to_mark(mark, 0.0, False, 0.0, 1.0)
        reason, fix_cmd = analyze_terminal_error(clean)
        if reason and fix_cmd:
            self.on_error_detected(reason, fix_cmd)

    def _on_pty_output(self, fd, condition):
        if condition & GLib.IOCondition.IN:
            try:
                data = os.read(fd, 4096)
                if data:
                    self._append_text(data.decode("utf-8", errors="replace"))
                    return True
            except BlockingIOError:
                return True
            except OSError:
                return False
        return False

    def _on_entry_activate(self, entry):
        cmd = entry.get_text()
        entry.set_text("")
        if cmd.strip():
            self.history.append(cmd)
            self.hist_idx = len(self.history)
        self.send_command(cmd, append_newline=True)

    def _on_entry_key(self, widget, event):
        if event.keyval == Gdk.KEY_Up and self.history:
            self.hist_idx = max(0, self.hist_idx - 1)
            self.entry.set_text(self.history[self.hist_idx])
            self.entry.set_position(-1)
            return True
        if event.keyval == Gdk.KEY_Down and self.history:
            self.hist_idx = min(len(self.history), self.hist_idx + 1)
            if self.hist_idx < len(self.history):
                self.entry.set_text(self.history[self.hist_idx])
            else:
                self.entry.set_text("")
            self.entry.set_position(-1)
            return True
        if (event.state & Gdk.ModifierType.CONTROL_MASK) and event.keyval in (Gdk.KEY_c, Gdk.KEY_C):
            if not (event.state & Gdk.ModifierType.SHIFT_MASK):
                self.send_command("\x03", append_newline=False)
                return True
        return False

    def send_command(self, cmd, append_newline=True):
        payload = (cmd + "\n") if append_newline else cmd
        if self.vte is not None:
            self.vte.feed_child(payload.encode("utf-8"))
        elif self.master_fd is not None:
            try:
                os.write(self.master_fd, payload.encode("utf-8"))
            except OSError:
                pass

    def copy_selection(self):
        clip = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
        if self.vte is not None:
            self.vte.copy_clipboard_format(Vte.Format.TEXT)
        else:
            bounds = self.buffer.get_selection_bounds()
            if bounds:
                text = self.buffer.get_text(bounds[0], bounds[1], True)
            else:
                text = self.buffer.get_text(self.buffer.get_start_iter(), self.buffer.get_end_iter(), True)
            clip.set_text(text, -1)
            clip.store()

    def paste_clipboard(self):
        if self.vte is not None:
            self.vte.paste_clipboard()
        else:
            clip = Gtk.Clipboard.get(Gdk.SELECTION_CLIPBOARD)
            text = clip.wait_for_text()
            if text:
                self.entry.insert_text(text, self.entry.get_position())

    def cleanup(self):
        if self.master_fd is not None:
            try:
                os.close(self.master_fd)
            except OSError:
                pass
            self.master_fd = None
        if self.child_pid is not None:
            try:
                os.kill(self.child_pid, signal.SIGTERM)
            except OSError:
                pass
            self.child_pid = None


class AxisTerminalWindow(Gtk.Window):
    def __init__(self, initial_cmd=None):
        super().__init__(title="AxisOS Terminal")
        self.set_default_size(940, 580)
        self.set_position(Gtk.WindowPosition.CENTER)

        provider = Gtk.CssProvider()
        provider.load_from_data(CSS)
        Gtk.StyleContext.add_provider_for_screen(
            Gdk.Screen.get_default(), provider, Gtk.STYLE_PROVIDER_PRIORITY_APPLICATION
        )

        hb = Gtk.HeaderBar()
        hb.set_show_close_button(True)
        hb.set_title("AxisOS Terminal")
        hb.set_subtitle("Liquid Nodes Native Shell • Self-Healing Engine Active")
        self.set_titlebar(hb)

        new_tab_btn = Gtk.Button(label="+ New Tab")
        new_tab_btn.get_style_context().add_class("action-btn")
        new_tab_btn.connect("clicked", lambda *_: self.add_tab())
        hb.pack_start(new_tab_btn)

        copy_btn = Gtk.Button(label="Copy")
        copy_btn.get_style_context().add_class("action-btn")
        copy_btn.connect("clicked", lambda *_: self.copy_active())
        hb.pack_end(copy_btn)

        paste_btn = Gtk.Button(label="Paste")
        paste_btn.get_style_context().add_class("action-btn")
        paste_btn.connect("clicked", lambda *_: self.paste_active())
        hb.pack_end(paste_btn)

        root_box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.add(root_box)

        self.notebook = Gtk.Notebook()
        self.notebook.set_scrollable(True)
        root_box.pack_start(self.notebook, True, True, 0)

        # Offline Self-Healing Terminal Pill (hidden until an error is detected)
        self.heal_revealer = Gtk.Revealer()
        self.heal_revealer.set_transition_type(Gtk.RevealerTransitionType.SLIDE_UP)
        self.heal_revealer.set_reveal_child(False)

        heal_box = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=12)
        heal_box.get_style_context().add_class("heal-pill")
        heal_box.set_margin_start(12)
        heal_box.set_margin_end(12)
        heal_box.set_margin_top(6)
        heal_box.set_margin_bottom(8)

        self.heal_label = Gtk.Label(label="")
        self.heal_label.set_xalign(0.0)
        self.heal_label.set_hexpand(True)
        heal_box.pack_start(self.heal_label, True, True, 0)

        fix_btn = Gtk.Button(label="⚡ Fix & Retry")
        fix_btn.get_style_context().add_class("heal-btn")
        fix_btn.connect("clicked", self._on_apply_fix)
        heal_box.pack_start(fix_btn, False, False, 0)

        dismiss_btn = Gtk.Button(label="✕")
        dismiss_btn.get_style_context().add_class("action-btn")
        dismiss_btn.connect("clicked", lambda *_: self.heal_revealer.set_reveal_child(False))
        heal_box.pack_start(dismiss_btn, False, False, 0)

        self.heal_revealer.add(heal_box)
        root_box.pack_start(self.heal_revealer, False, False, 0)

        self.pending_fix_cmd = None
        self.connect("key-press-event", self._on_window_key)
        self.connect("destroy", self._on_destroy)

        first_tab = self.add_tab()
        if initial_cmd:
            GLib.timeout_add(250, lambda: (first_tab.send_command(initial_cmd), False)[1])

    def add_tab(self):
        idx = self.notebook.get_n_pages() + 1
        tab = PtyTab(self._show_heal_pill)
        lbl = Gtk.Label(label=f"Shell {idx}")
        self.notebook.append_page(tab, lbl)
        self.notebook.show_all()
        self.notebook.set_current_page(self.notebook.get_n_pages() - 1)
        if hasattr(tab, "entry"):
            tab.entry.grab_focus()
        return tab

    def _active_tab(self):
        page = self.notebook.get_current_page()
        if page >= 0:
            return self.notebook.get_nth_page(page)
        return None

    def copy_active(self):
        t = self._active_tab()
        if t:
            t.copy_selection()

    def paste_active(self):
        t = self._active_tab()
        if t:
            t.paste_clipboard()

    def _show_heal_pill(self, reason, fix_cmd):
        self.pending_fix_cmd = fix_cmd
        self.heal_label.set_markup(
            f"<b>Self-Healing Suggestion:</b> {GLib.markup_escape_text(reason)}  "
            f"<span foreground='#00F2FE'><tt>{GLib.markup_escape_text(fix_cmd)}</tt></span>"
        )
        self.heal_revealer.set_reveal_child(True)

    def _on_apply_fix(self, *_):
        t = self._active_tab()
        if t and self.pending_fix_cmd:
            t.send_command(self.pending_fix_cmd, append_newline=True)
        self.heal_revealer.set_reveal_child(False)

    def _on_window_key(self, widget, event):
        mods = event.state & Gtk.accelerator_get_default_mod_mask()
        if mods == (Gdk.ModifierType.CONTROL_MASK | Gdk.ModifierType.SHIFT_MASK):
            if event.keyval in (Gdk.KEY_C, Gdk.KEY_c):
                self.copy_active()
                return True
            if event.keyval in (Gdk.KEY_V, Gdk.KEY_v):
                self.paste_active()
                return True
            if event.keyval in (Gdk.KEY_T, Gdk.KEY_t):
                self.add_tab()
                return True
        return False

    def _on_destroy(self, *_):
        for i in range(self.notebook.get_n_pages()):
            page = self.notebook.get_nth_page(i)
            if isinstance(page, PtyTab):
                page.cleanup()
        Gtk.main_quit()


def main():
    init_cmd = None
    args = sys.argv[1:]
    if "-e" in args:
        i = args.index("-e")
        init_cmd = " ".join(args[i + 1:])
    elif "-c" in args:
        i = args.index("-c")
        init_cmd = " ".join(args[i + 1:])
    win = AxisTerminalWindow(initial_cmd=init_cmd)
    win.show_all()
    Gtk.main()


if __name__ == "__main__":
    main()
