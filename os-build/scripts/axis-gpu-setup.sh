#!/usr/bin/env bash
# ==============================================================================
# AxisOS Universal GPU Driver & Vulkan Setup Bootstrap (Idempotent)
# Supports: AMD RDNA/GCN, Intel Arc/Xe/Iris, NVIDIA Turing/Ampere/Ada/Legacy
# Zero duplicate modifications, safe to re-run anytime.
# ==============================================================================

set -euo pipefail

log() {
    echo "[AxisOS GPU Setup] $*"
}

# 1. Check Root Privileges
if [ "$(id -u)" -ne 0 ]; then
    echo "Error: This script must be run as root (sudo)." >&2
    exit 1
fi

log "Detecting installed graphics hardware..."

# 2. Enable 32-bit Multiarch Architecture (Required for Steam, Wine & 32-bit Vulkan/GL)
if ! dpkg --print-foreign-architectures | grep -qx "i386"; then
    log "Enabling i386 32-bit multiarch architecture..."
    dpkg --add-architecture i386
    apt-get update -y || true
else
    log "i386 multiarch architecture already enabled."
fi

# 3. Ensure Non-Free and Non-Free-Firmware repositories are active
if [ -f /etc/apt/sources.list ]; then
    if ! grep -q "non-free-firmware" /etc/apt/sources.list; then
        log "Adding non-free-firmware to /etc/apt/sources.list..."
        sed -i 's/non-free/non-free non-free-firmware/g' /etc/apt/sources.list
        sed -i 's/non-free-firmware non-free-firmware/non-free-firmware/g' /etc/apt/sources.list
        apt-get update -y || true
    fi
fi

# 4. Probe PCI Bus for Graphics Cards
HAS_NVIDIA=0
HAS_AMD=0
HAS_INTEL=0
IS_TURING_OR_NEWER=0

GPU_INFO=$(lspci -nn | grep -iE 'vga|3d|display' || true)
echo "$GPU_INFO"

if echo "$GPU_INFO" | grep -qi "10de:"; then
    HAS_NVIDIA=1
    log "NVIDIA Graphics Hardware Detected."
    # Check PCI Device IDs for Turing (GTX 16xx / RTX 20xx: 1e00-21ff), Ampere (RTX 30xx: 2200-25ff), Ada (RTX 40xx: 2600-28ff)
    if echo "$GPU_INFO" | grep -qiE "10de:(1[e-f][0-9a-f]|2[0-8][0-9a-f])"; then
        IS_TURING_OR_NEWER=1
        log "NVIDIA Turing / Ampere / Ada Lovelace architecture detected (Supports nvidia-open)."
    fi
fi

if echo "$GPU_INFO" | grep -qi "1002:"; then
    HAS_AMD=1
    log "AMD Radeon Graphics Hardware Detected."
fi

if echo "$GPU_INFO" | grep -qi "8086:"; then
    HAS_INTEL=1
    log "Intel Graphics Hardware Detected."
fi

# 5. Common Vulkan & Core Mesa Libraries
log "Ensuring core Vulkan and 32-bit runtime libraries..."
CORE_PKGS=(
    vulkan-tools
    libvulkan1
    libvulkan1:i386
    mesa-vulkan-drivers
    mesa-vulkan-drivers:i386
    libgl1-mesa-dri
    libgl1-mesa-dri:i386
    mesa-va-drivers
    mesa-vdpau-drivers
)

apt-get install -y --no-install-recommends "${CORE_PKGS[@]}" || true

# 6. Configure AMD Drivers
if [ "$HAS_AMD" -eq 1 ]; then
    log "Configuring AMD GPU drivers and Vulkan (RADV)..."
    apt-get install -y --no-install-recommends firmware-amd-graphics || true
    
    mkdir -p /etc/environment.d
    cat << 'EOF' > /etc/environment.d/10-axisos-amd.conf
# AxisOS AMD Graphics Optimization
AMD_VULKAN_ICD=RADV
RADV_PERFTEST=gpl,nggc
EOF
fi

# 7. Configure Intel Drivers
if [ "$HAS_INTEL" -eq 1 ]; then
    log "Configuring Intel GPU drivers and Vulkan (ANV)..."
    apt-get install -y --no-install-recommends \
        intel-media-va-driver-non-free \
        firmware-misc-nonfree || true

    mkdir -p /etc/environment.d
    cat << 'EOF' > /etc/environment.d/10-axisos-intel.conf
# AxisOS Intel Graphics Optimization
ANV_ENABLE_PIPELINE_CACHE=1
LIBVA_DRIVER_NAME=iHD
EOF

    # Check for Intel Arc (Alchemist/Battlemage)
    if echo "$GPU_INFO" | grep -qiE "56[a-f][0-9a-f]|7d[0-9a-f]{2}"; then
        log "Intel Arc GPU detected. Enabling Xe driver configuration..."
        mkdir -p /etc/modprobe.d
        cat << 'EOF' > /etc/modprobe.d/xe-graphics.conf
# Enable Xe kernel module for Intel Arc GPUs
options xe force_probe=*
EOF
    fi
fi

# 8. Configure NVIDIA Drivers
if [ "$HAS_NVIDIA" -eq 1 ]; then
    log "Configuring NVIDIA proprietary driver and DKMS..."
    
    # Kernel headers needed for DKMS compilation
    apt-get install -y --no-install-recommends \
        build-essential \
        dkms \
        linux-headers-$(uname -r) || true

    if [ "$IS_TURING_OR_NEWER" -eq 1 ]; then
        log "Installing nvidia-open-kernel-dkms (Turing+ Open GPU Kernel Modules)..."
        apt-get install -y --no-install-recommends \
            nvidia-open-kernel-dkms \
            nvidia-driver \
            nvidia-vulkan-icd \
            nvidia-vulkan-icd:i386 \
            libgl1-nvidia-glvnd-glx:i386 || {
                log "Fallback: installing standard nvidia-kernel-dkms..."
                apt-get install -y --no-install-recommends nvidia-kernel-dkms nvidia-driver
            }
    else
        log "Installing standard nvidia-kernel-dkms and driver..."
        apt-get install -y --no-install-recommends \
            nvidia-kernel-dkms \
            nvidia-driver \
            nvidia-vulkan-icd \
            nvidia-vulkan-icd:i386 \
            libgl1-nvidia-glvnd-glx:i386 || true
    fi

    # Set nvidia-drm modeset=1 and fbdev=1
    log "Setting up /etc/modprobe.d/nvidia-modeset.conf (nvidia-drm.modeset=1)..."
    mkdir -p /etc/modprobe.d
    cat << 'EOF' > /etc/modprobe.d/nvidia-modeset.conf
# AxisOS NVIDIA Modeset & Wayland fbdev
options nvidia-drm modeset=1 fbdev=1
options nvidia NVreg_PreserveVideoMemoryAllocations=1
options nvidia NVreg_TemporaryFilePath=/var/tmp
EOF

    # Configure Wayland environment for NVIDIA
    mkdir -p /etc/environment.d
    cat << 'EOF' > /etc/environment.d/10-axisos-nvidia.conf
# AxisOS NVIDIA Wayland & Vulkan Integration
LIBVA_DRIVER_NAME=nvidia
GBM_BACKEND=nvidia-drm
__GLX_VENDOR_LIBRARY_NAME=nvidia
WLR_NO_HARDWARE_CURSORS=1
EOF

    # Idempotently update GRUB command line with nvidia-drm.modeset=1
    if [ -f /etc/default/grub ]; then
        if ! grep -q "nvidia-drm.modeset=1" /etc/default/grub; then
            log "Appending nvidia-drm.modeset=1 to GRUB_CMDLINE_LINUX_DEFAULT..."
            sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="\([^"]*\)"/GRUB_CMDLINE_LINUX_DEFAULT="\1 nvidia-drm.modeset=1"/' /etc/default/grub
            update-grub 2>/dev/null || true
        fi
    fi

    # Update initramfs so nvidia-drm is loaded early
    if command -v update-initramfs >/dev/null 2>&1; then
        update-initramfs -u 2>/dev/null || true
    fi
fi

log "GPU driver & Vulkan bootstrap completed successfully."
exit 0
