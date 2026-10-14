#!/usr/bin/env bash
# ==============================================================================
# AxisOS Edition - Automated NVIDIA GPU Driver First-Boot Wrapper
# Delegates to unified /usr/local/bin/gpu-autodetect.sh to prevent concurrent
# dpkg/apt lock contention on first boot.
# ==============================================================================

set -e

if [ -x /usr/local/bin/gpu-autodetect.sh ]; then
    exec /usr/local/bin/gpu-autodetect.sh "$@"
fi
exit 0
