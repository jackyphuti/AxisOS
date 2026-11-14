#!/usr/bin/env bash
# ==============================================================================
# AxisOS QEMU Live Test Runner
# Boots axisos-live-amd64.iso in QEMU with automatic acceleration detection
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ISO_PATH="${1:-$SCRIPT_DIR/axisos-live-amd64.iso}"

if [ ! -f "$ISO_PATH" ]; then
    echo "[-] Error: ISO not found at $ISO_PATH"
    echo "    Please run ./build-iso.sh first or provide the path to your ISO file."
    exit 1
fi

echo "=================================================="
echo "          AxisOS QEMU Virtual Machine Test        "
echo "=================================================="
echo "ISO Image:   $ISO_PATH"
echo "RAM Size:    4096 MB (4G)"
echo "CPU Cores:   4 vCPUs"

# Acceleration flags (KVM with fallback to TCG)
ACCEL_ARGS=("-accel" "kvm" "-accel" "tcg")
if [ -e /dev/kvm ] && [ -r /dev/kvm ] && [ -w /dev/kvm ]; then
    echo "Accelerator: Hardware KVM (/dev/kvm)"
else
    echo "Accelerator: TCG (Software Emulation / WSL2 fallback)"
fi

# Display flags (GTK window under WSLg / X11 / Wayland)
DISPLAY_ARGS=("-display" "gtk" "-vga" "virtio")

echo "Launching QEMU..."
exec qemu-system-x86_64 \
    "${ACCEL_ARGS[@]}" \
    -m 4G \
    -smp 4 \
    "${DISPLAY_ARGS[@]}" \
    -cdrom "$ISO_PATH" \
    -boot d
