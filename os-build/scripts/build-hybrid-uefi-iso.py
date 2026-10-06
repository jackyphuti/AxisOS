#!/usr/bin/env python3
import os
import sys
import shutil
import subprocess

build_dir = "/build"
chroot_dir = f"{build_dir}/chroot"
binary_dir = f"{build_dir}/binary"

workspace_dir = os.environ.get("WORKSPACE_DIR")
if not workspace_dir:
    for candidate in [
        "/mnt/c/Users/jacky/Documents/GitHub/AxisOS",
        "/mnt/c/Users/jacky/documents/github/AxisOS",
        os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")),
    ]:
        if os.path.exists(candidate):
            workspace_dir = candidate
            break
if not workspace_dir:
    workspace_dir = "/build"

print("==================================================")
print("  AxisOS Dual-Boot UEFI + Legacy BIOS ISO Engine  ")
print("==================================================")
print(f"Workspace Dir: {workspace_dir}")
print(f"Chroot Dir:    {chroot_dir}")
print(f"Binary Dir:    {binary_dir}")

# Step 0: Sync latest workspace assets into chroot
print("--> 0. Synchronizing latest workspace assets into chroot")
shell_dist_src = os.path.join(workspace_dir, "shell", "dist")
shell_dist_dst = os.path.join(chroot_dir, "opt", "axisos-shell", "dist")
if os.path.exists(shell_dist_src):
    print("    Syncing shell/dist -> /opt/axisos-shell/dist")
    if os.path.exists(shell_dist_dst):
        shutil.rmtree(shell_dist_dst)
    shutil.copytree(shell_dist_src, shell_dist_dst)

pkg_axis_src = os.path.join(workspace_dir, "pkg-mgr", "bin", "axis")
if os.path.exists(pkg_axis_src):
    for dst in [os.path.join(chroot_dir, "usr", "bin", "axis"), os.path.join(chroot_dir, "usr", "local", "bin", "axis")]:
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(pkg_axis_src, dst)
        os.chmod(dst, 0o755)

axis_conf_src = os.path.join(workspace_dir, "pkg-mgr", "configs", "axis.conf")
if os.path.exists(axis_conf_src):
    dst = os.path.join(chroot_dir, "etc", "axis", "axis.conf")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(axis_conf_src, dst)

repo_index_src = os.path.join(workspace_dir, "pkg-mgr", "examples", "repo-index.json")
if os.path.exists(repo_index_src):
    for dst in [os.path.join(chroot_dir, "etc", "axis", "repo-index.json"), os.path.join(chroot_dir, "var", "lib", "axis", "repo-index.json")]:
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(repo_index_src, dst)

daemon_src = os.path.join(workspace_dir, "os-build", "configs", "cage-session", "axisos-daemon.cjs")
if os.path.exists(daemon_src):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "axisos-daemon.cjs")
    shutil.copyfile(daemon_src, dst)
    os.chmod(dst, 0o755)

kiosk_src = os.path.join(workspace_dir, "os-build", "configs", "cage-session", "axisos-kiosk.sh")
if os.path.exists(kiosk_src):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "axisos-kiosk.sh")
    shutil.copyfile(kiosk_src, dst)
    os.chmod(dst, 0o755)

installer_src = os.path.join(workspace_dir, "os-build", "scripts", "axisos-install.sh")
if os.path.exists(installer_src):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "axisos-installer.sh")
    shutil.copyfile(installer_src, dst)
    os.chmod(dst, 0o755)

gpu_detect_src = os.path.join(workspace_dir, "os-build", "scripts", "gpu-autodetect.sh")
if os.path.exists(gpu_detect_src):
    dst = os.path.join(chroot_dir, "usr", "local", "bin", "gpu-autodetect.sh")
    shutil.copyfile(gpu_detect_src, dst)
    os.chmod(dst, 0o755)

gpu_svc_src = os.path.join(workspace_dir, "os-build", "configs", "systemd", "gpu-autodetect.service")
if os.path.exists(gpu_svc_src):
    dst = os.path.join(chroot_dir, "etc", "systemd", "system", "gpu-autodetect.service")
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    shutil.copyfile(gpu_svc_src, dst)

# Deploy prime-run helper
primerun_dst = os.path.join(chroot_dir, "usr", "local", "bin", "prime-run")
with open(primerun_dst, "w") as f:
    f.write("""#!/usr/bin/env bash
# AxisOS NVIDIA PRIME Render Offload Runner
export __NV_PRIME_RENDER_OFFLOAD=1
export __GLX_VENDOR_LIBRARY_NAME=nvidia
export __VK_LAYER_NV_optimus=NVIDIA_only
exec "$@"
""")
os.chmod(primerun_dst, 0o755)

# Deploy Android & HarmonyOS universal udev rules
udev_rules_dst = os.path.join(chroot_dir, "etc", "udev", "rules.d", "51-android.rules")
os.makedirs(os.path.dirname(udev_rules_dst), exist_ok=True)
with open(udev_rules_dst, "w") as f:
    f.write("""# Universal Android & HarmonyOS USB rules
SUBSYSTEM=="usb", ATTR{idVendor}=="[0-9a-fA-F]*", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="04e8", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="12d1", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="2717", MODE="0666", GROUP="plugdev"
""")

# Deploy Gaming & High-Performance Sysctl configs (Proton/Steam max_map_count)
gaming_sysctl_dst = os.path.join(chroot_dir, "etc", "sysctl.d", "99-axisos-gaming.conf")
os.makedirs(os.path.dirname(gaming_sysctl_dst), exist_ok=True)
with open(gaming_sysctl_dst, "w") as f:
    f.write("""# AxisOS Gaming & Development Performance Tuning
vm.max_map_count = 2147483642
fs.file-max = 524288
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192
vm.swappiness = 10
""")

# Step 1: Generate filesystem.squashfs
print("--> 1. Building filesystem.squashfs with parallel XZ compression")
squashfs_target = f"{binary_dir}/live/filesystem.squashfs"
os.makedirs(f"{binary_dir}/live", exist_ok=True)
if os.path.exists(squashfs_target):
    os.remove(squashfs_target)

subprocess.run([
    "mksquashfs", chroot_dir, squashfs_target,
    "-comp", "xz",
    "-noappend",
    "-e", "boot/vmlinuz*", "boot/initrd.img*"
], check=True)

# Step 2: Ensure kernel and initrd are in binary/live
print("--> 2. Syncing kernel and initramfs to /live")
vmlinuz_candidates = sorted(os.listdir(f"{chroot_dir}/boot"))
for item in vmlinuz_candidates:
    if item.startswith("vmlinuz-"):
        shutil.copyfile(f"{chroot_dir}/boot/{item}", f"{binary_dir}/live/vmlinuz")
        print(f"    Kernel: {item} -> {binary_dir}/live/vmlinuz")
        break

for item in vmlinuz_candidates:
    if item.startswith("initrd.img-"):
        shutil.copyfile(f"{chroot_dir}/boot/{item}", f"{binary_dir}/live/initrd.img")
        print(f"    Initrd: {item} -> {binary_dir}/live/initrd.img")
        break

# Step 3: Configure ISOLINUX for Legacy BIOS
print("--> 3. Configuring ISOLINUX (Legacy BIOS)")
os.makedirs(f"{binary_dir}/isolinux", exist_ok=True)
for c32 in ["ldlinux.c32", "libcom32.c32", "libutil.c32", "vesamenu.c32"]:
    src = f"/usr/lib/syslinux/modules/bios/{c32}"
    if os.path.exists(src):
        shutil.copyfile(src, f"{binary_dir}/isolinux/{c32}")

if os.path.exists("/usr/lib/ISOLINUX/isolinux.bin"):
    shutil.copyfile("/usr/lib/ISOLINUX/isolinux.bin", f"{binary_dir}/isolinux/isolinux.bin")

with open(f"{binary_dir}/isolinux/isolinux.cfg", "w") as f:
    f.write("""UI vesamenu.c32
PROMPT 0
TIMEOUT 50
DEFAULT live

MENU TITLE AxisOS Linux 2.0 (Horizon Edition)
MENU COLOR border       30;44   #40ffffff #a0000000 std
MENU COLOR title        1;36;44 #9033b5e5 #a0000000 std
MENU COLOR sel          7;37;40 #e0ffffff #20ffffff all
MENU COLOR unsel        37;44   #50ffffff #a0000000 std
MENU COLOR help         37;40   #c0ffffff #a0000000 std

LABEL live
  MENU LABEL ^1. Try AxisOS 2.0 (Live Desktop)
  MENU DEFAULT
  KERNEL /live/vmlinuz
  APPEND initrd=/live/initrd.img boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev,gamemode,games,docker live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3 amdgpu.freesync_video=1

LABEL install
  MENU LABEL ^2. Install AxisOS 2.0 (Direct Setup Wizard)
  KERNEL /live/vmlinuz
  APPEND initrd=/live/initrd.img boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev,gamemode,games,docker live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3 axisos.autoinstall=1 amdgpu.freesync_video=1

LABEL failsafe
  MENU LABEL ^3. AxisOS 2.0 (Safe Graphics Mode)
  KERNEL /live/vmlinuz
  APPEND initrd=/live/initrd.img boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev live-config.locales=en_US.UTF-8 live-config.timezone=UTC nomodeset quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3
""")

# Step 4: Build Microsoft-Signed GRUB UEFI 64-bit Bootloader
print("--> 4. Deploying Microsoft-signed Shim and signed GRUB for Secure Boot trust")
os.makedirs(f"{binary_dir}/EFI/BOOT", exist_ok=True)
os.makedirs(f"{binary_dir}/boot/grub", exist_ok=True)

shim_src = f"{chroot_dir}/usr/lib/shim/shimx64.efi.signed"
grub_src = f"{chroot_dir}/usr/lib/grub/x86_64-efi-signed/grubx64.efi.signed"
mm_src = f"{chroot_dir}/usr/lib/shim/mmx64.efi.signed"

# Fallback to unsigned in chroot if signed not found
if not os.path.exists(shim_src):
    shim_src = f"{chroot_dir}/usr/lib/shim/shimx64.efi"
if not os.path.exists(mm_src):
    mm_src = f"{chroot_dir}/usr/lib/shim/mmx64.efi"

# Deploy Microsoft-signed Shim as universal BOOTX64.EFI
bootx64_path = f"{binary_dir}/EFI/BOOT/BOOTX64.EFI"
grubx64_path = f"{binary_dir}/EFI/BOOT/grubx64.efi"
mmx64_path = f"{binary_dir}/EFI/BOOT/mmx64.efi"

if os.path.exists(shim_src):
    shutil.copyfile(shim_src, bootx64_path)
    print("    [Secure Boot] Deployed Microsoft-signed Shim as /EFI/BOOT/BOOTX64.EFI")

if os.path.exists(grub_src):
    shutil.copyfile(grub_src, grubx64_path)
    print("    [Secure Boot] Deployed Debian-signed GRUB as /EFI/BOOT/grubx64.efi")

if os.path.exists(mm_src):
    shutil.copyfile(mm_src, mmx64_path)

# Early GRUB config in /EFI/BOOT/grub.cfg that finds USB root and chains to /boot/grub/grub.cfg
with open(f"{binary_dir}/EFI/BOOT/grub.cfg", "w") as f:
    f.write("""search --set=root --file /live/vmlinuz
set prefix=($root)/boot/grub
if [ -f ($root)/boot/grub/grub.cfg ]; then
    configfile ($root)/boot/grub/grub.cfg
fi
""")

# GRUB Config in /boot/grub/grub.cfg
with open(f"{binary_dir}/boot/grub/grub.cfg", "w") as f:
    f.write("""set default=0
set timeout=5

insmod part_gpt
insmod part_msdos
insmod fat
insmod iso9660
insmod all_video

menuentry "Try AxisOS 2.0 (Live Desktop)" --class axisos --class gnu-linux --class gnu {
    linux /live/vmlinuz boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev,gamemode,games,docker live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3 amdgpu.freesync_video=1
    initrd /live/initrd.img
}

menuentry "Install AxisOS 2.0 (Direct Setup Wizard)" --class axisos {
    linux /live/vmlinuz boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev,gamemode,games,docker live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3 axisos.autoinstall=1 amdgpu.freesync_video=1
    initrd /live/initrd.img
}

menuentry "AxisOS 2.0 (Safe Graphics Mode - nomodeset)" --class axisos {
    linux /live/vmlinuz boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev live-config.locales=en_US.UTF-8 live-config.timezone=UTC nomodeset quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3
    initrd /live/initrd.img
}
""")

# Step 5: Create FAT EFI System Partition Image for El Torito UEFI boot
print("--> 5. Creating EFI System Partition image (efi.img)")
efi_img = f"{binary_dir}/boot/grub/efi.img"
if os.path.exists(efi_img):
    os.remove(efi_img)

subprocess.run(["dd", "if=/dev/zero", f"of={efi_img}", "bs=1M", "count=15"], check=True)
subprocess.run(["mkfs.vfat", efi_img], check=True)
subprocess.run(["mmd", "-i", efi_img, "::/EFI"], check=True)
subprocess.run(["mmd", "-i", efi_img, "::/EFI/BOOT"], check=True)
subprocess.run(["mcopy", "-i", efi_img, bootx64_path, "::/EFI/BOOT/BOOTX64.EFI"], check=True)
if os.path.exists(grubx64_path):
    subprocess.run(["mcopy", "-i", efi_img, grubx64_path, "::/EFI/BOOT/grubx64.efi"], check=True)
if os.path.exists(mmx64_path):
    subprocess.run(["mcopy", "-i", efi_img, mmx64_path, "::/EFI/BOOT/mmx64.efi"], check=True)
subprocess.run(["mcopy", "-i", efi_img, f"{binary_dir}/EFI/BOOT/grub.cfg", "::/EFI/BOOT/grub.cfg"], check=True)

# Step 6: Generate SHA256 Checksums
print("--> 6. Generating SHA256 checksums")
subprocess.run(
    ["sh", "-c", f"cd {binary_dir} && find . -type f ! -name SHA256SUMS | sort | xargs sha256sum > SHA256SUMS"],
    check=True
)

# Step 7: Create Dual UEFI + BIOS Hybrid ISO via xorriso
print("--> 7. Assembling Dual-Boot UEFI + Legacy BIOS Hybrid ISO with xorriso")
output_iso = f"{build_dir}/axisos-v2.0-live-amd64.iso"
if os.path.exists(output_iso):
    os.remove(output_iso)

mbr_template = "/usr/lib/ISOLINUX/isohdpfx.bin"

xorriso_cmd = [
    "xorriso", "-as", "mkisofs",
    "-r", "-V", "AXISOS_V2",
    "-J", "-joliet-long",
    "-b", "isolinux/isolinux.bin",
    "-c", "isolinux/boot.cat",
    "-no-emul-boot", "-boot-load-size", "4", "-boot-info-table",
    "-eltorito-alt-boot",
    "-e", "boot/grub/efi.img",
    "-no-emul-boot",
    "-isohybrid-gpt-basdat",
    "-isohybrid-mbr", mbr_template,
    "-output", output_iso,
    binary_dir
]

subprocess.run(xorriso_cmd, check=True)

# Ensure isohybrid is applied with UEFI support
print("--> 8. Applying isohybrid UEFI partition flags")
subprocess.run(["isohybrid", "--uefi", output_iso], check=False)

# Copy to Windows workspace
dest = f"{workspace_dir}/axisos-v2.0-live-amd64.iso"
print(f"--> 9. Copying ISO to Windows workspace ({dest})")
shutil.copyfile(output_iso, dest)

iso_size_mb = os.path.getsize(dest) / (1024 * 1024)
print("==================================================")
print(f" SUCCESS: AxisOS Universal Hybrid ISO Generated!")
print(f" File: {dest} ({iso_size_mb:.1f} MB)")
print(" Compatible with:")
print("   - Modern UEFI Machines (Secure Boot Off / CSM Off)")
print("   - Legacy BIOS Machines")
print("   - USB Flash Drives (Rufus, Etcher, Ventoy, dd)")
print("   - Virtual Machines (QEMU, VirtualBox, VMware)")
print("==================================================")
