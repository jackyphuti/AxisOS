#!/usr/bin/env python3
import os
import sys
import shutil
import subprocess

build_dir = "/build"
chroot_dir = f"{build_dir}/chroot"
binary_dir = f"{build_dir}/binary"
workspace_dir = "/mnt/c/Users/jacky/documents/github/AxisOS"

print("==================================================")
print("  AxisOS Dual-Boot UEFI + Legacy BIOS ISO Engine  ")
print("==================================================")

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

MENU TITLE AxisOS Linux 1.0 (Sonoma Edition)
MENU COLOR border       30;44   #40ffffff #a0000000 std
MENU COLOR title        1;36;44 #9033b5e5 #a0000000 std
MENU COLOR sel          7;37;40 #e0ffffff #20ffffff all
MENU COLOR unsel        37;44   #50ffffff #a0000000 std
MENU COLOR help         37;40   #c0ffffff #a0000000 std

LABEL live
  MENU LABEL ^1. AxisOS Live Desktop (Horizon Shell)
  MENU DEFAULT
  KERNEL /live/vmlinuz
  APPEND initrd=/live/initrd.img boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash

LABEL failsafe
  MENU LABEL ^2. AxisOS Live (Safe Graphics / Failsafe)
  KERNEL /live/vmlinuz
  APPEND initrd=/live/initrd.img boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev live-config.locales=en_US.UTF-8 live-config.timezone=UTC nomodeset
""")

# Step 4: Build GRUB UEFI 64-bit Bootloader
print("--> 4. Generating standalone 64-bit UEFI GRUB bootloader")
os.makedirs(f"{binary_dir}/EFI/BOOT", exist_ok=True)
os.makedirs(f"{binary_dir}/boot/grub", exist_ok=True)

# Generate BOOTX64.EFI with embedded early configuration to find USB root
early_cfg = f"{build_dir}/early-grub.cfg"
with open(early_cfg, "w") as f:
    f.write("""search --set=root --file /live/vmlinuz
set prefix=($root)/boot/grub
if [ -f ($root)/boot/grub/grub.cfg ]; then
    configfile ($root)/boot/grub/grub.cfg
fi
""")

bootx64_path = f"{binary_dir}/EFI/BOOT/BOOTX64.EFI"
subprocess.run([
    "grub-mkstandalone",
    "-O", "x86_64-efi",
    "-o", bootx64_path,
    f"boot/grub/grub.cfg={early_cfg}",
    "--modules=part_gpt part_msdos fat ext2 iso9660 normal test echo linux search search_fs_file search_fs_uuid search_label configfile"
], check=True)

# GRUB Config
with open(f"{binary_dir}/boot/grub/grub.cfg", "w") as f:
    f.write("""set default=0
set timeout=5

insmod part_gpt
insmod part_msdos
insmod fat
insmod iso9660
insmod all_video

menuentry "AxisOS Linux 1.0 (Live Desktop - Horizon Shell)" --class axisos --class gnu-linux --class gnu {
    linux /live/vmlinuz boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev live-config.locales=en_US.UTF-8 live-config.timezone=UTC quiet splash
    initrd /live/initrd.img
}

menuentry "AxisOS Linux 1.0 (Safe Graphics / Failsafe)" --class axisos {
    linux /live/vmlinuz boot=live components username=axis user-fullname=AxisOS user-default-groups=audio,video,render,input,seat,sudo,netdev live-config.locales=en_US.UTF-8 live-config.timezone=UTC nomodeset
    initrd /live/initrd.img
}
""")

# Step 5: Create FAT EFI System Partition Image for El Torito UEFI boot
print("--> 5. Creating EFI System Partition image (efi.img)")
efi_img = f"{binary_dir}/boot/grub/efi.img"
if os.path.exists(efi_img):
    os.remove(efi_img)

subprocess.run(["dd", "if=/dev/zero", f"of={efi_img}", "bs=1M", "count=12"], check=True)
subprocess.run(["mformat", "-i", efi_img, "-C", "::"], check=True)
subprocess.run(["mmd", "-i", efi_img, "::/EFI"], check=True)
subprocess.run(["mmd", "-i", efi_img, "::/EFI/BOOT"], check=True)
subprocess.run(["mcopy", "-i", efi_img, bootx64_path, "::/EFI/BOOT/BOOTX64.EFI"], check=True)

# Step 6: Generate SHA256 Checksums
print("--> 6. Generating SHA256 checksums")
subprocess.run(
    ["sh", "-c", f"cd {binary_dir} && find . -type f ! -name SHA256SUMS | sort | xargs sha256sum > SHA256SUMS"],
    check=True
)

# Step 7: Create Dual UEFI + BIOS Hybrid ISO via xorriso
print("--> 7. Assembling Dual-Boot UEFI + Legacy BIOS Hybrid ISO with xorriso")
output_iso = f"{build_dir}/axisos-live-amd64.iso"
if os.path.exists(output_iso):
    os.remove(output_iso)

mbr_template = "/usr/lib/ISOLINUX/isohdpfx.bin"

xorriso_cmd = [
    "xorriso", "-as", "mkisofs",
    "-r", "-V", "AXISOS_LIVE",
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
dest = f"{workspace_dir}/axisos-live-amd64.iso"
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
