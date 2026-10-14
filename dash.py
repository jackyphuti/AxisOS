#!/usr/bin/env python3
"""
AxisOS Terminal System Dashboard
Real-time terminal dashboard built with Rich for AxisOS Linux.
Run with: python3 dash.py
"""
import os, platform, socket, time
from datetime import timedelta
import psutil
from rich.console import Console
from rich.layout import Layout
from rich.live import Live
from rich.panel import Panel
from rich.progress_bar import ProgressBar
from rich.table import Table
from rich.text import Text

console = Console()
_prev_net = psutil.net_io_counters()
_prev_time = time.time()

def get_os_version():
    if os.path.exists("/etc/os-release"):
        try:
            with open("/etc/os-release") as f:
                for line in f:
                    if line.startswith("PRETTY_NAME="):
                        return line.split("=", 1)[1].strip().strip('"')
        except Exception:
            pass
    return "AxisOS 2.0 (Liquid Nodes Edition)"

def fmt_bytes(b):
    for u in ["B", "KB", "MB", "GB", "TB"]:
        if b < 1024 or u == "TB": return f"{b:.1f} {u}"
        b /= 1024

def make_bar(pct, width=16):
    c = "bright_cyan" if pct < 60 else "bright_yellow" if pct < 85 else "bright_red"
    return ProgressBar(total=100, completed=max(0, min(100, pct)), width=width, complete_style=c, finished_style=c)

def build_header():
    uptime = str(timedelta(seconds=int(time.time() - psutil.boot_time())))
    g = Table.grid(expand=True)
    g.add_column(ratio=1); g.add_column(justify="center", ratio=2); g.add_column(justify="right", ratio=1)
    g.add_row(f"[bold white]Host:[/] [cyan]{socket.gethostname()}[/]",
              f"[bold cyan]▲ AXIS[/][bold white]OS[/] [dim]•[/] [bold #00f2fe]{get_os_version()}[/]",
              f"[bold white]Kernel:[/] [cyan]{platform.release()}[/]")
    g.add_row(f"[bold white]Uptime:[/] [green]{uptime}[/]",
              f"[dim white]Real-Time Telemetry • Refresh: 1s[/]",
              f"[bold white]Time:[/] [yellow]{time.strftime('%H:%M:%S')}[/]")
    return Panel(g, style="bold #00f2fe", border_style="#0088ff", padding=(0, 1))

def build_cpu_mem():
    pcts = psutil.cpu_percent(percpu=True)
    t = Table(box=None, expand=True, padding=(0, 1), show_header=False)
    for _ in range(2):
        t.add_column(ratio=1); t.add_column(ratio=2); t.add_column(justify="right", ratio=1)
    half = (len(pcts) + 1) // 2
    for i in range(half):
        c2 = [f"Core {i+half:02d}", make_bar(pcts[i+half], 12), f"{pcts[i+half]:5.1f}%"] if i+half < len(pcts) else ["", "", ""]
        t.add_row(f"Core {i:02d}", make_bar(pcts[i], 12), f"{pcts[i]:5.1f}%", *c2)

    mem, swap = psutil.virtual_memory(), psutil.swap_memory()
    mt = Table(box=None, expand=True, padding=(0, 1), show_header=False)
    mt.add_column(ratio=2); mt.add_column(ratio=5); mt.add_column(justify="right", ratio=5)
    mt.add_row("RAM", make_bar(mem.percent, 20), f"{fmt_bytes(mem.used)} / {fmt_bytes(mem.total)} ({mem.percent}%)")
    mt.add_row("SWAP", make_bar(swap.percent, 20), f"{fmt_bytes(swap.used)} / {fmt_bytes(swap.total)} ({swap.percent}%)")

    g = Table.grid(expand=True)
    g.add_column(ratio=1); g.add_row(t); g.add_row(Text("─" * 40, style="dim")); g.add_row(mt)
    return Panel(g, title="[bold #00f2fe]CPU & Memory Utilization[/]", border_style="#0088ff", padding=(0, 1))

def build_io():
    global _prev_net, _prev_time
    cur_net, cur_time = psutil.net_io_counters(), time.time()
    dt = max(0.1, cur_time - _prev_time)
    rx = (cur_net.bytes_recv - _prev_net.bytes_recv) / dt
    tx = (cur_net.bytes_sent - _prev_net.bytes_sent) / dt
    _prev_net, _prev_time = cur_net, cur_time

    net_info = Text.from_markup(
        f"[bold white]Network:[/] [green]▼ In:[/] [bold green]{fmt_bytes(rx)}/s[/]  "
        f"[blue]▲ Out:[/] [bold blue]{fmt_bytes(tx)}/s[/]  [dim]({fmt_bytes(cur_net.bytes_recv)} rx, {fmt_bytes(cur_net.bytes_sent)} tx)[/]"
    )
    dtbl = Table(box=None, expand=True, padding=(0, 1))
    dtbl.add_column("Mount", style="bold white", ratio=2)
    dtbl.add_column("Device", style="dim", ratio=2)
    dtbl.add_column("Type", style="dim", ratio=1)
    dtbl.add_column("Usage", ratio=3)
    dtbl.add_column("Used / Total", justify="right", ratio=3)
    seen = set()
    for p in psutil.disk_partitions(all=False):
        if p.mountpoint in seen or "loop" in p.device: continue
        try:
            u = psutil.disk_usage(p.mountpoint); seen.add(p.mountpoint)
            dtbl.add_row(p.mountpoint, p.device, p.fstype, make_bar(u.percent, 10), f"{fmt_bytes(u.used)} / {fmt_bytes(u.total)} ({u.percent}%)")
        except (PermissionError, OSError): pass

    g = Table.grid(expand=True)
    g.add_column(ratio=1); g.add_row(net_info); g.add_row(Text("─" * 40, style="dim")); g.add_row(dtbl)
    return Panel(g, title="[bold #00f2fe]Storage & Network Throughput[/]", border_style="#0088ff", padding=(0, 1))

def build_procs():
    pt = Table(box=None, expand=True, padding=(0, 1))
    pt.add_column("PID", style="cyan", justify="right", ratio=1)
    pt.add_column("User", style="dim white", ratio=2)
    pt.add_column("CPU%", style="bold yellow", justify="right", ratio=1)
    pt.add_column("Mem%", style="bold green", justify="right", ratio=1)
    pt.add_column("Process Name", style="bold white", ratio=4)
    procs = []
    for p in psutil.process_iter(["pid", "name", "username", "cpu_percent", "memory_percent"]):
        try:
            if p.info["name"] and p.info["cpu_percent"] is not None:
                procs.append(p.info)
        except (psutil.NoSuchProcess, psutil.AccessDenied): pass
    procs.sort(key=lambda x: (x["cpu_percent"] or 0.0, x["memory_percent"] or 0.0), reverse=True)
    for p in procs[:10]:
        pt.add_row(str(p["pid"]), str(p["username"] or "-")[:12], f"{p['cpu_percent'] or 0.0:5.1f}%", f"{p['memory_percent'] or 0.0:4.1f}%", str(p["name"])[:32])
    return Panel(pt, title="[bold #00f2fe]Top 10 Processes by CPU[/]", border_style="#0088ff", padding=(0, 1))

def render():
    layout = Layout()
    layout.split_column(Layout(name="header", size=4), Layout(name="body", ratio=1))
    layout["body"].split_row(Layout(name="left", ratio=1), Layout(name="right", ratio=1))
    layout["left"].split_column(Layout(name="cpu_mem", ratio=1), Layout(name="io", ratio=1))
    layout["header"].update(build_header())
    layout["cpu_mem"].update(build_cpu_mem())
    layout["io"].update(build_io())
    layout["right"].update(build_procs())
    return layout

def main():
    # Warm up CPU measurement
    psutil.cpu_percent(percpu=True)
    with Live(render(), console=console, screen=True, refresh_per_second=1) as live:
        try:
            while True:
                time.sleep(1)
                live.update(render())
        except KeyboardInterrupt:
            pass

if __name__ == "__main__":
    main()
