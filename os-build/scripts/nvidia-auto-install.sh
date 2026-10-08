#!/usr/bin/env bash
# ==============================================================================
# AxisOS Nobara Edition - Automated NVIDIA GPU Driver First-Boot Installer
# Detects NVIDIA GPU via PCI bus and non-interactively installs proprietary drivers.
# AMD and Intel GPUs are natively powered by in-kernel Mesa / amdgpu / i915 drivers.
# ==============================================================================

set -e

LOG_FILE="/var/log/nvidia-auto-install.log"
FLAG_FILE="/var/lib/nvidia-first-boot.flag"

mkdir -p /var/log /var/lib

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [NVIDIA-AutoInstall] $*" | tee -a "$LOG_FILE"
}

log "Starting graphics hardware probe..."

# 1. Probe PCI bus for display controllers
GPU_CONTROLLER=$(lspci -k 2>/dev/null | grep -iEA3 'VGA|3D|Display' || true)
log "Detected Graphics Controllers:"
echo "$GPU_CONTROLLER" | tee -a "$LOG_FILE"

# 2. Check if NVIDIA hardware exists
if ! echo "$GPU_CONTROLLER" | grep -iq 'nvidia'; then
    log "Intel / AMD GPU detected. Kernel Mesa drivers active. No proprietary NVIDIA drivers needed."
    touch "$FLAG_FILE"
    exit 0
fi

log "NVIDIA Graphics Processor detected!"

# 3. Check if proprietary driver is already active
if lsmod 2>/dev/null | grep -iq 'nvidia' || command -v nvidia-smi >/dev/null 2>&1; then
    log "NVIDIA proprietary driver is already active and running."
    touch "$FLAG_FILE"
    exit 0
fi

# 4. Determine target driver package
PKG_TARGET="nvidia-driver"
if command -v nvidia-detect >/dev/null 2>&1; then
    DETECT_OUT=$(nvidia-detect 2>/dev/null || true)
    log "nvidia-detect output: $DETECT_OUT"
    CANDIDATE=$(echo "$DETECT_OUT" | grep -oE 'nvidia-[a-z0-9-]+' | head -n1 || true)
    if [ -n "$CANDIDATE" ]; then
        PKG_TARGET="$CANDIDATE"
    fi
fi

log "Target package identified: $PKG_TARGET"
log "Installing $PKG_TARGET and non-free firmware non-interactively..."

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq || true
apt-get install -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" \
    "$PKG_TARGET" firmware-misc-nonfree libgl1-nvidia-glvnd-glx nvidia-smi || {
    log "Direct installation of $PKG_TARGET encountered issues, attempting generic fallback..."
    apt-get install -y nvidia-driver || true
}

log "NVIDIA driver installation procedure complete."
touch "$FLAG_FILE"
exit 0
