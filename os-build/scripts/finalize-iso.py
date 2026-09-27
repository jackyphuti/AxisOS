#!/usr/bin/env python3
import os
import subprocess
import glob
import shutil

build_dir = "/build"
workspace_dir = "/mnt/c/Users/jacky/documents/github/AxisOS"

print("=== Generating SHA256 Checksums ===")
subprocess.run(["sh", "-c", "cd /build/binary && find . -type f ! -name SHA256SUMS | sort | xargs sha256sum > SHA256SUMS"], check=True)

print("=== Building Hybrid ISO with live-build ===")
subprocess.run(["sh", "-c", "cd /build && rm -f binary.hybrid.iso binary.iso chroot/binary.hybrid.iso && lb binary_iso"], check=True)

# Find generated ISO
candidates = [
    "/build/binary.hybrid.iso",
    "/build/chroot/binary.hybrid.iso",
    "/build/live-image-amd64.hybrid.iso",
] + glob.glob("/build/*.iso") + glob.glob("/build/chroot/*.iso")

target_iso = None
for c in candidates:
    if os.path.exists(c) and os.path.getsize(c) > 100 * 1024 * 1024:
        target_iso = c
        break

if not target_iso:
    print("Error: Could not locate generated ISO image.")
    exit(1)

print(f"Applying isohybrid to {target_iso}...")
subprocess.run(["isohybrid", target_iso], check=False)

dest = f"{workspace_dir}/axisos-live-amd64.iso"
print(f"Copying {target_iso} -> {dest}...")
shutil.copyfile(target_iso, dest)

print(f"SUCCESS: AxisOS ISO updated at {dest} ({os.path.getsize(dest) / (1024*1024):.1f} MB)")
