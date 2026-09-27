#!/usr/bin/env bash
set -e

ISO_PATH="${1:-axisos-live-amd64.iso}"
RAM="${RAM:-4096}"
CORES="${CORES:-4}"

echo "========================================"
echo "    AxisOS Virtual Machine Launcher"
echo "========================================"

if [ ! -f "$ISO_PATH" ]; then
    echo "Notice: ISO file '$ISO_PATH' not found."
    echo "You can build the ISO using: sudo ./os-build/scripts/build-iso.sh"
    echo ""
    echo "To test the AxisOS UI right now without building the full ISO:"
    echo "  cd shell && npm run dev"
    echo "========================================"
    exit 1
fi

echo "Starting QEMU with:"
echo "  ISO:     $ISO_PATH"
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

qemu-system-x86_64 \
    $KVM_FLAG \
    -m "$RAM" \
    -smp "$CORES" \
    -cdrom "$ISO_PATH" \
    -boot d \
    -vga virtio \
    -display gtk,gl=on \
    -device virtio-net-pci,netdev=net0 \
    -netdev user,id=net0 \
    -device intel-hda -device hda-duplex \
    -usb -device usb-tablet
