#!/usr/bin/env bash
# ==============================================================================
# AxisOS Dynamic GPU Autodetection & Hardware Provisioning
# Detects NVIDIA, AMD, and Intel GPUs.
# Uses DPkg::Lock::Timeout=120 to prevent first-boot APT lock races and only
# writes the completion marker when driver provisioning succeeds.
# ==============================================================================

set -uo pipefail

LOG_FILE="/var/log/axisos-gpu-autodetect.log"
MARKER_FILE="/var/lib/axis/gpu-autodetect.done"
LEGACY_FLAG="/var/lib/nvidia-first-boot.flag"

mkdir -p /var/lib/axis /var/log

log() {
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] [GPU-Autodetect] $*" | tee -a "$LOG_FILE"
}

log "Starting AxisOS GPU autodetect..."

# Ensure prime-run helper is installed
cat << 'EOF' > /usr/local/bin/prime-run
#!/usr/bin/env bash
# AxisOS NVIDIA PRIME Render Offload Runner
export __NV_PRIME_RENDER_OFFLOAD=1
export __GLX_VENDOR_LIBRARY_NAME=nvidia
export __VK_LAYER_NV_optimus=NVIDIA_only
exec "$@"
EOF
chmod 755 /usr/local/bin/prime-run

PCI_DISPLAY=$(lspci -nn 2>/dev/null | grep -iE 'vga|3d|display' || true)
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

PROVISION_SUCCESS=true

if [ "$HAS_NVIDIA" = true ]; then
    PROVISION_SUCCESS=false
    log "NVIDIA graphics hardware detected. Checking driver status..."

    if lsmod 2>/dev/null | grep -iq '^nvidia' || (command -v nvidia-smi >/dev/null 2>&1 && nvidia-smi >/dev/null 2>&1); then
        log "NVIDIA proprietary driver is already loaded and operational."
        PROVISION_SUCCESS=true
    else
        log "Attempting proprietary NVIDIA driver installation..."
        dpkg --add-architecture i386 2>/dev/null || true

        if ping -c 1 -W 3 deb.debian.org >/dev/null 2>&1 || ping -c 1 -W 3 1.1.1.1 >/dev/null 2>&1; then
            log "Network connection confirmed. Acquiring APT lock (up to 120s timeout)..."
            export DEBIAN_FRONTEND=noninteractive
            APT_OPTS=(-o DPkg::Lock::Timeout=120 -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold")

            if apt-get "${APT_OPTS[@]}" update -qq; then
                PKG_TARGET="nvidia-driver"
                if command -v nvidia-detect >/dev/null 2>&1; then
                    CANDIDATE=$(nvidia-detect 2>/dev/null | grep -oE 'nvidia-[a-z0-9-]+driver' | head -n1 || true)
                    if [ -n "$CANDIDATE" ]; then
                        PKG_TARGET="$CANDIDATE"
                    fi
                fi

                PKGS=("$PKG_TARGET" "firmware-misc-nonfree" "x11-xserver-utils" "nvidia-prime" "nvidia-smi")
                if dpkg --print-foreign-architectures 2>/dev/null | grep -q '^i386$'; then
                    PKGS+=("libgl1-nvidia-glvnd-glx:i386")
                fi

                log "Installing packages: ${PKGS[*]}"
                if apt-get "${APT_OPTS[@]}" install -y --no-install-recommends "${PKGS[@]}"; then
                    log "NVIDIA driver installation completed successfully."
                    PROVISION_SUCCESS=true
                else
                    log "Warning: NVIDIA driver installation failed; will retry on next online boot."
                fi
            else
                log "Warning: apt-get update failed; will retry on next online boot."
            fi
        else
            log "Network offline or unreachable. Deferring NVIDIA driver installation to next online boot."
        fi
    fi
fi

if [ "$HAS_AMD" = true ]; then
    log "AMD Radeon GPU detected: Native amdgpu kernel driver and Mesa Vulkan (radv) active."
fi

if [ "$HAS_INTEL" = true ]; then
    log "Intel Graphics detected: Native i915/xe kernel driver and Mesa Vulkan (anv) active."
fi

if [ "$PROVISION_SUCCESS" = true ]; then
    touch "$MARKER_FILE" "$LEGACY_FLAG"
    systemctl disable gpu-autodetect.service 2>/dev/null || true
    systemctl disable nvidia-first-boot.service 2>/dev/null || true
    log "GPU autodetect finalized and marked complete."
    exit 0
else
    log "GPU provisioning incomplete; leaving service enabled for next boot retry."
    exit 0
fi
