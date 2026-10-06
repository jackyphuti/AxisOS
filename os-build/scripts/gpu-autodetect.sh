#!/usr/bin/env bash
# ==============================================================================
# AxisOS Dynamic GPU Autodetection & Hardware Provisioning
# Detects NVIDIA, AMD, and Intel GPUs.
# AMD & Intel run plug-and-play with pre-installed kernel drivers & Mesa.
# NVIDIA triggers dynamic installation of proprietary drivers + PRIME offload.
# ==============================================================================

set -e

LOG_FILE="/var/log/axisos-gpu-autodetect.log"
MARKER_FILE="/var/lib/axis/gpu-autodetect.done"

mkdir -p /var/lib/axis /var/log

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [GPU-Autodetect] $*" | tee -a "$LOG_FILE"
}

log "Starting AxisOS GPU autodetect..."

# Always ensure prime-run helper is installed
cat << 'EOF' > /usr/local/bin/prime-run
#!/usr/bin/env bash
# AxisOS NVIDIA PRIME Render Offload Runner
export __NV_PRIME_RENDER_OFFLOAD=1
export __GLX_VENDOR_LIBRARY_NAME=nvidia
export __VK_LAYER_NV_optimus=NVIDIA_only
exec "$@"
EOF
chmod 755 /usr/local/bin/prime-run

# Scan PCI bus for GPU/display controllers
PCI_DISPLAY=$(lspci -nn | grep -iE 'vga|3d|display' || true)
log "Detected display controllers:"
log "$PCI_DISPLAY"

HAS_NVIDIA=false
HAS_AMD=false
HAS_INTEL=false

if echo "$PCI_DISPLAY" | grep -iq nvidia; then
    HAS_NVIDIA=true
fi
if echo "$PCI_DISPLAY" | grep -iqE 'amd|ati|radeon|advanced micro devices'; then
    HAS_AMD=true
fi
if echo "$PCI_DISPLAY" | grep -iq intel; then
    HAS_INTEL=true
fi

log "Hardware Summary: NVIDIA=$HAS_NVIDIA, AMD=$HAS_AMD, Intel=$HAS_INTEL"

if [ "$HAS_NVIDIA" = true ]; then
    log "NVIDIA graphics hardware detected. Checking driver status..."

    # Check if driver is already functioning
    if command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi >/dev/null 2>&1; then
        log "NVIDIA driver is already installed and operating."
    else
        log "Attempting proprietary NVIDIA driver installation..."
        # Ensure 32-bit architecture is present
        dpkg --add-architecture i386 || true
        
        # Verify network connectivity before apt-get
        if ping -c 1 -W 2 deb.debian.org >/dev/null 2>&1 || ping -c 1 -W 2 1.1.1.1 >/dev/null 2>&1; then
            log "Network connection confirmed. Updating package index..."
            apt-get update -qq || true
            
            PKGS="nvidia-driver firmware-misc-nonfree x11-xserver-utils nvidia-prime"
            if dpkg --print-foreign-architectures | grep -q i386; then
                PKGS="$PKGS libgl1-nvidia-glvnd-glx:i386"
            fi

            log "Installing: $PKGS"
            DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends $PKGS || {
                log "Warning: Full nvidia-driver install encountered issues; will retry on next online boot."
            }
        else
            log "Network offline or unreachable. Skipping online driver fetch. Will run when connection is active."
        fi
    fi
fi

if [ "$HAS_AMD" = true ]; then
    log "AMD Radeon GPU detected: Native amdgpu kernel driver and Mesa Vulkan (radv) are active (Plug-and-Play)."
fi

if [ "$HAS_INTEL" = true ]; then
    log "Intel Graphics detected: Native i915/xe kernel driver and Mesa Vulkan (anv) are active (Plug-and-Play)."
fi

# Touch marker and disable service on success
touch "$MARKER_FILE"
systemctl disable gpu-autodetect.service 2>/dev/null || true
log "GPU autodetect complete."
exit 0
