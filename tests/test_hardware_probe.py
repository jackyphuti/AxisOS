#!/usr/bin/env python3
import os
import platform
import subprocess
import shutil

def probe():
    print("=== Probing Hardware ===")
    
    # 1. Device Model
    model = "PC Device"
    for p in ["/sys/class/dmi/id/product_name", "/sys/devices/virtual/dmi/id/product_name"]:
        if os.path.exists(p):
            try:
                with open(p, "r") as f:
                    val = f.read().strip()
                    if val and val != "None":
                        model = val
                        break
            except Exception:
                pass
    vendor = ""
    for p in ["/sys/class/dmi/id/sys_vendor", "/sys/devices/virtual/dmi/id/sys_vendor"]:
        if os.path.exists(p):
            try:
                with open(p, "r") as f:
                    val = f.read().strip()
                    if val and val != "None":
                        vendor = val
                        break
            except Exception:
                pass
    if vendor and vendor not in model:
        device_str = f"{vendor} {model}"
    else:
        device_str = model
    print(f"Device: {device_str}")

    # 2. CPU
    cpu_model = "x86_64 Processor"
    if os.path.exists("/proc/cpuinfo"):
        try:
            with open("/proc/cpuinfo", "r") as f:
                for line in f:
                    if "model name" in line:
                        cpu_model = line.split(":", 1)[1].strip()
                        break
        except Exception:
            pass
    cores = os.cpu_count() or 1
    print(f"Processor: {cpu_model} ({cores} cores)")

    # 3. RAM
    ram_str = "Unknown"
    if os.path.exists("/proc/meminfo"):
        try:
            with open("/proc/meminfo", "r") as f:
                for line in f:
                    if line.startswith("MemTotal:"):
                        kb = int(line.split()[1])
                        gb = kb / (1024.0 * 1024.0)
                        ram_str = f"{gb:.1f} GB"
                        break
        except Exception:
            pass
    print(f"Memory (RAM): {ram_str}")

    # 4. GPU
    gpus = []
    if shutil.which("lspci"):
        try:
            out = subprocess.check_output(["lspci"], text=True)
            for line in out.splitlines():
                if any(k in line.lower() for k in ["vga", "3d controller", "display controller"]):
                    parts = line.split(":", 2)
                    gpu_name = parts[-1].strip() if len(parts) >= 3 else line
                    gpus.append(gpu_name)
        except Exception:
            pass
    if not gpus:
        gpus = ["Hardware Accelerated Graphics"]
    print(f"Graphics (GPU): {' / '.join(gpus)}")

    # 5. Storage
    try:
        usage = shutil.disk_usage("/")
        total_gb = usage.total / (1024**3)
        avail_gb = usage.free / (1024**3)
        storage_str = f"{total_gb:.0f} GB Storage ({avail_gb:.0f} GB Available)"
    except Exception:
        storage_str = "Internal Storage"
    print(f"Storage: {storage_str}")

    # 6. Kernel & OS
    kernel_str = platform.release()
    arch_str = platform.machine()
    print(f"Kernel: {kernel_str} ({arch_str})")

if __name__ == "__main__":
    probe()
