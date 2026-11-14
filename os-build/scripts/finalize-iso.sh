#!/usr/bin/env bash
set -e

BUILD_DIR="/build"
WORKSPACE="${WORKSPACE_DIR:-$(cd "$(dirname "$0")/../.." && pwd)}"

echo "=== Generating SHA256 Checksums ==="
if [ -d "${BUILD_DIR}/binary" ]; then
    (cd "${BUILD_DIR}/binary" && find . -type f ! -name SHA256SUMS | sort | xargs sha256sum > SHA256SUMS)
fi

echo "=== Building Hybrid ISO with live-build ==="
if [ -d "$BUILD_DIR" ]; then
    (cd "$BUILD_DIR" && rm -f .build/binary_iso binary.hybrid.iso binary.iso chroot/binary.hybrid.iso && lb binary_iso)
fi

TARGET_ISO=""
for cand in "${BUILD_DIR}/binary.hybrid.iso" "${BUILD_DIR}/chroot/binary.hybrid.iso" "${BUILD_DIR}/live-image-amd64.hybrid.iso" ${BUILD_DIR}/*.iso ${BUILD_DIR}/chroot/*.iso; do
    if [ -f "$cand" ]; then
        sz=$(stat -c%s "$cand" 2>/dev/null || echo 0)
        if [ "$sz" -gt 104857600 ]; then
            TARGET_ISO="$cand"
            break
        fi
    fi
done

if [ -n "$TARGET_ISO" ] && [ -f "$TARGET_ISO" ]; then
    echo "Applying isohybrid to ${TARGET_ISO}..."
    isohybrid "$TARGET_ISO" 2>/dev/null || true

    DEST="${WORKSPACE}/axisos-live-amd64.iso"
    echo "Copying ${TARGET_ISO} -> ${DEST}..."
    cp -f "$TARGET_ISO" "$DEST"
    echo "SUCCESS: AxisOS ISO updated at ${DEST}"
else
    echo "Notice: Final ISO build phase ready."
fi
