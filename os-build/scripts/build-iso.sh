#!/usr/bin/env bash
# ==============================================================================
# AxisOS Bootable Hybrid ISO Generator (Pure C Edition)
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/os-build/work"

echo "=================================================="
echo "          AxisOS Bootable ISO Builder (Pure C)    "
echo "=================================================="

# Step 1: Compile Pure C Native Applications & Shell
echo "[1/4] Compiling Pure C Desktop Applications & Shell..."
bash "$SCRIPT_DIR/compile-native-apps.sh"

SUDO_CMD=""
if [ "$(id -u)" -ne 0 ] && command -v sudo >/dev/null 2>&1; then
    SUDO_CMD="sudo"
fi

# Step 2: Prepare workspace
# If running on /mnt/* (Windows DrvFs in WSL), use native Linux ext4 for build chroot
if [[ "$BUILD_DIR" == /mnt/* ]]; then
    BUILD_DIR="/var/tmp/axisos-build"
fi
echo "[2/4] Setting up build workspace in $BUILD_DIR..."
if [ -n "$SUDO_CMD" ]; then
    $SUDO_CMD mkdir -p "$BUILD_DIR"
    $SUDO_CMD chown -R "$(id -u):$(id -g)" "$BUILD_DIR"
else
    mkdir -p "$BUILD_DIR"
fi

# Step 3: Determine builder environment (Container vs Native live-build)
CONTAINER_TOOL=""
if command -v podman >/dev/null 2>&1; then
    CONTAINER_TOOL="podman"
elif command -v docker >/dev/null 2>&1; then
    CONTAINER_TOOL="docker"
fi

if [ -n "$CONTAINER_TOOL" ]; then
    echo "[3/4] Using $CONTAINER_TOOL container for reproducible Debian live ISO build..."

    cat << 'EOF' > "$BUILD_DIR/Containerfile"
FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    live-build \
    debootstrap \
    squashfs-tools \
    xorriso \
    isolinux \
    syslinux-common \
    syslinux-efi \
    grub-pc-bin \
    grub-efi-amd64-bin \
    mtools \
    dosfstools \
    ca-certificates \
    curl \
    rsync \
    build-essential \
    pkg-config \
    gcc \
    make \
    libgtk-3-dev \
    libgtk-layer-shell-dev \
    libwlroots-dev \
    wayland-protocols \
    libwayland-dev \
    libwayland-bin \
    libxkbcommon-dev \
    libinput-dev \
    libpam0g-dev \
    libudev-dev \
    libvterm-dev \
    libvte-2.91-dev \
    libcairo2-dev \
    libglib2.0-dev \
    libcurl4-openssl-dev \
    libarchive-dev \
    libjson-c-dev \
    libssl-dev \
    libasound2-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
CMD ["bash"]
EOF

    SUDO_CMD=""
    if [ "$(id -u)" -ne 0 ] && command -v sudo >/dev/null 2>&1; then
        SUDO_CMD="sudo"
    fi

    echo "Building container image 'axisos-builder'..."
    $SUDO_CMD $CONTAINER_TOOL build -t axisos-builder -f "$BUILD_DIR/Containerfile" "$BUILD_DIR"

    echo "[4/4] Generating AxisOS Hybrid Bootable ISO inside container..."
    $SUDO_CMD $CONTAINER_TOOL run --privileged --rm \
        -v "$ROOT_DIR:/workspace:rw" \
        -v "$BUILD_DIR:/build:rw" \
        -e WORKSPACE_DIR=/workspace \
        -e BUILD_DIR=/build \
        axisos-builder \
        bash /workspace/os-build/scripts/inner-build.sh
else
    echo "[3/4] Container runtime not found; building native with live-build..."
    bash "$SCRIPT_DIR/inner-build.sh"
fi

echo "=================================================="
echo "          AxisOS ISO Build Complete!              "
echo "=================================================="
