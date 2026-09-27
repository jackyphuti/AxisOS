#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/os-build/work"

echo "=================================================="
echo "          AxisOS Bootable ISO Builder"
echo "=================================================="

# Step 1: Build the AxisOS Shell bundle
echo "[1/4] Building AxisOS Desktop Shell..."
"$SCRIPT_DIR/build-shell.sh"

# Step 2: Prepare workspace
echo "[2/4] Setting up build workspace in $BUILD_DIR..."
mkdir -p "$BUILD_DIR"

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
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
CMD ["bash"]
EOF

    echo "Building container image 'axisos-builder'..."
    $CONTAINER_TOOL build -t axisos-builder -f "$BUILD_DIR/Containerfile" "$BUILD_DIR"

    echo "[4/4] Generating AxisOS Hybrid Bootable ISO inside container..."
    $CONTAINER_TOOL run --privileged --rm \
        -v "$ROOT_DIR:/workspace" \
        axisos-builder \
        /workspace/os-build/scripts/inner-build.sh
else
    echo "[3/4] Checking for native live-build..."
    if command -v lb >/dev/null 2>&1; then
        echo "[4/4] Generating AxisOS Hybrid Bootable ISO natively..."
        sudo WORKSPACE_DIR="$ROOT_DIR" "$SCRIPT_DIR/inner-build.sh"
    else
        echo "Error: Neither podman, docker, nor live-build ('lb') were found."
        echo "To build AxisOS ISO, please install Docker, Podman, or live-build."
        exit 1
    fi
fi

echo "=================================================="
echo " ISO build finished successfully!"
echo " Bootable image: $ROOT_DIR/axisos-live-amd64.iso"
echo "=================================================="
