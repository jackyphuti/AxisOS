#!/usr/bin/env bash
# ==============================================================================
# AxisOS Virtual Machine Launcher (QEMU / KVM)
# Supports live ISO booting, disk installation, and booting installed drive
# ==============================================================================
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

ISO_PATH="${1:-$ROOT_DIR/axisos-live-amd64.iso}"
DISK_IMAGE="${2:-$ROOT_DIR/axisos-disk.qcow2}"
RAM="${RAM:-4096}"
CORES="${CORES:-4}"

echo "========================================"
echo "    AxisOS Virtual Machine Launcher"
echo "========================================"

# Create virtual hard disk for testing OS installation if it doesn't exist
if [ ! -f "$DISK_IMAGE" ]; then
    echo "Creating 20GB virtual disk image at $DISK_IMAGE for installer testing..."
    qemu-img create -f qcow2 "$DISK_IMAGE" 20G
fi

# Determine boot drive
BOOT_ARGS=""
if [ -f "$ISO_PATH" ]; then
    echo "  Mode:    Booting from Live ISO ($ISO_PATH)"
    BOOT_ARGS="-cdrom $ISO_PATH -boot d"
else
    echo "  Notice:  Live ISO not found, attempting to boot installed disk ($DISK_IMAGE)..."
    BOOT_ARGS="-boot c"
fi

echo "  Drive:   $DISK_IMAGE (20GB VirtIO Disk)"
echo "  RAM:     ${RAM}MB"
echo "  CPU:     ${CORES} cores"
echo "  Display: VirtIO GPU"

# Check if KVM is available for hardware acceleration
KVM_FLAG=""
if [ -w /dev/kvm ]; then
    echo "  KVM:     Enabled (Hardware acceleration active)"
    KVM_FLAG="-enable-kvm -cpu host"
else
    echo "  KVM:     Disabled (Software emulation)"
    KVM_FLAG="-cpu max"
fi

exec qemu-system-x86_64 \
    $KVM_FLAG \
    -m "$RAM" \
    -smp "$CORES" \
    -drive "file=$DISK_IMAGE,if=virtio,format=qcow2" \
    $BOOT_ARGS \
    -vga virtio \
    -display gtk,gl=on \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0 \
    -device intel-hda -device hda-duplex \
    -usb -device usb-tablet
