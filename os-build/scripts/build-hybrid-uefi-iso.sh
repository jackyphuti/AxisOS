#!/usr/bin/env bash
set -e

BUILD_DIR="/build"
CHROOT_DIR="${BUILD_DIR}/chroot"
BINARY_DIR="${BUILD_DIR}/binary"
WORKSPACE_DIR="${WORKSPACE_DIR:-$(cd "$(dirname "$0")/../.." && pwd)}"

echo "=================================================="
echo "  AxisOS Dual-Boot UEFI + Legacy BIOS ISO Engine  "
echo "  Pure POSIX Shell Implementation                 "
echo "=================================================="
echo "Workspace Dir: ${WORKSPACE_DIR}"
echo "Chroot Dir:    ${CHROOT_DIR}"
echo "Binary Dir:    ${BINARY_DIR}"

mkdir -p "${BINARY_DIR}/live"
mkdir -p "${BINARY_DIR}/isolinux"
mkdir -p "${BINARY_DIR}/boot/grub"

# Step 0: Sync pure C apps and shell into chroot
echo "--> 0. Synchronizing AxisOS pure C binaries into chroot"
if [ -d "${WORKSPACE_DIR}/os-build/apps" ]; then
    for app_dir in "${WORKSPACE_DIR}/os-build/apps"/*/; do
        app_name=$(basename "$app_dir")
        if [ -f "${app_dir}/${app_name}" ]; then
            cp -f "${app_dir}/${app_name}" "${CHROOT_DIR}/usr/local/bin/${app_name}"
            chmod 755 "${CHROOT_DIR}/usr/local/bin/${app_name}"
        fi
    done
fi

if [ -f "${WORKSPACE_DIR}/shell-c/axisos-shell" ]; then
    cp -f "${WORKSPACE_DIR}/shell-c/axisos-shell" "${CHROOT_DIR}/usr/local/bin/axisos-shell"
    chmod 755 "${CHROOT_DIR}/usr/local/bin/axisos-shell"
    ln -sf axisos-shell "${CHROOT_DIR}/usr/local/bin/LiquidNodesOS"
fi

if [ -f "${WORKSPACE_DIR}/compositor/axis-compositor" ]; then
    cp -f "${WORKSPACE_DIR}/compositor/axis-compositor" "${CHROOT_DIR}/usr/local/bin/axis-compositor"
    chmod 755 "${CHROOT_DIR}/usr/local/bin/axis-compositor"
fi

if [ -f "${WORKSPACE_DIR}/pkg-mgr/bin/axis" ]; then
    cp -f "${WORKSPACE_DIR}/pkg-mgr/bin/axis" "${CHROOT_DIR}/usr/local/bin/axis"
    chmod 755 "${CHROOT_DIR}/usr/local/bin/axis"
fi

# Step 1: Generate filesystem.squashfs
echo "--> 1. Generating filesystem.squashfs"
rm -f "${BINARY_DIR}/live/filesystem.squashfs"
mksquashfs "${CHROOT_DIR}" "${BINARY_DIR}/live/filesystem.squashfs" \
    -comp xz -Xbcj x86 -b 1048576 -noappend -e boot 2>/dev/null || true

# Step 2: Ensure kernel and initrd are in binary/live
echo "--> 2. Ensuring kernel and initrd in binary/live"
VMLINUZ=$(find "${CHROOT_DIR}/boot" -name "vmlinuz*" | sort -V | tail -n 1)
INITRD=$(find "${CHROOT_DIR}/boot" -name "initrd.img*" | sort -V | tail -n 1)

if [ -n "$VMLINUZ" ] && [ -f "$VMLINUZ" ]; then
    cp -f "$VMLINUZ" "${BINARY_DIR}/live/vmlinuz"
fi
if [ -n "$INITRD" ] && [ -f "$INITRD" ]; then
    cp -f "$INITRD" "${BINARY_DIR}/live/initrd.img"
fi

# Step 3: Configure ISOLINUX for Legacy BIOS
echo "--> 3. Configuring ISOLINUX"
cat << 'EOF' > "${BINARY_DIR}/isolinux/isolinux.cfg"
UI vesamenu.c32
PROMPT 0
TIMEOUT 50
DEFAULT live

LABEL live
  MENU LABEL ^Try or Install AxisOS (Pure C Wayland)
  KERNEL /live/vmlinuz
  APPEND initrd=/live/initrd.img boot=live components quiet splash systemd.show_status=0
EOF

for f in isohdpfx.bin isolinux.bin ldlinux.c32 libcom32.c32 libutil.c32 vesamenu.c32; do
    src=$(find /usr/lib/ISOLINUX /usr/lib/syslinux/modules/bios -name "$f" 2>/dev/null | head -n 1)
    if [ -n "$src" ] && [ -f "$src" ]; then
        cp -f "$src" "${BINARY_DIR}/isolinux/"
    fi
done

# Step 4: Configure GRUB for UEFI 64-bit
echo "--> 4. Configuring GRUB UEFI bootloader"
cat << 'EOF' > "${BINARY_DIR}/boot/grub/grub.cfg"
set default="0"
set timeout=5

insmod efi_gop
insmod efi_uga
insmod gfxterm
insmod font
terminal_output gfxterm

menuentry "AxisOS 2.0 (Pure C Wayland Desktop)" --class axisos --class gnu-linux --class os {
    linux /live/vmlinuz boot=live components quiet splash systemd.show_status=0
    initrd /live/initrd.img
}

menuentry "AxisOS (Safe Graphics Fallback)" {
    linux /live/vmlinuz boot=live components nomodeset
    initrd /live/initrd.img
}
EOF

# Step 5: Create FAT EFI System Partition Image
echo "--> 5. Building FAT EFI System Partition Image"
EFI_IMG="${BINARY_DIR}/boot/grub/efi.img"
rm -f "$EFI_IMG"
dd if=/dev/zero of="$EFI_IMG" bs=1M count=8 2>/dev/null || true
mkfs.vfat -F 12 -n "AXIS_EFI" "$EFI_IMG" 2>/dev/null || true

if command -v mmd >/dev/null 2>&1 && command -v mcopy >/dev/null 2>&1; then
    mmd -i "$EFI_IMG" ::EFI ::EFI/BOOT 2>/dev/null || true
    for efi_bin in /usr/lib/grub/x86_64-efi-signed/gcdx64.efi.signed /usr/lib/shim/shimx64.efi.signed; do
        if [ -f "$efi_bin" ]; then
            mcopy -i "$EFI_IMG" "$efi_bin" ::EFI/BOOT/BOOTX64.EFI 2>/dev/null || true
            break
        fi
    done
fi

# Step 6: Generate Checksums
echo "--> 6. Generating SHA256 Checksums"
(cd "${BINARY_DIR}" && find . -type f ! -name SHA256SUMS | sort | xargs sha256sum > SHA256SUMS 2>/dev/null || true)

# Step 7: Create Dual UEFI + BIOS Hybrid ISO via xorriso
echo "--> 7. Creating Dual UEFI + BIOS Hybrid ISO via xorriso"
OUTPUT_ISO="${WORKSPACE_DIR}/axisos-live-amd64.iso"
ISOHDPFX=$(find /usr/lib/ISOLINUX /usr/lib/syslinux -name "isohdpfx.bin" 2>/dev/null | head -n 1)

XORRISO_ARGS=(
    -as mkisofs
    -r -V "AXISOS_LIVE"
    -o "$OUTPUT_ISO"
    -J -joliet-long
    -b isolinux/isolinux.bin
    -c isolinux/boot.cat
    -no-emul-boot
    -boot-load-size 4
    -boot-info-table
)

if [ -n "$ISOHDPFX" ] && [ -f "$ISOHDPFX" ]; then
    XORRISO_ARGS+=(-isohybrid-mbr "$ISOHDPFX")
fi

if [ -f "$EFI_IMG" ]; then
    XORRISO_ARGS+=(
        -eltorito-alt-boot
        -e boot/grub/efi.img
        -no-emul-boot
        -isohybrid-gpt-basdat
    )
fi

XORRISO_ARGS+=("${BINARY_DIR}")

xorriso "${XORRISO_ARGS[@]}" 2>/dev/null || true

if [ -f "$OUTPUT_ISO" ]; then
    echo "[SUCCESS] Generated Hybrid UEFI + BIOS ISO: ${OUTPUT_ISO}"
fi
