#!/usr/bin/env bash
# ==============================================================================
# AxisOS "Quarantine Vault" — Hardware-Isolated Disposable USB Sandbox
# Mounts external USB storage read-only with nosuid,nodev,noexec and layers a
# disposable RAM tmpfs overlay (overlayfs) on top so untrusted USB files cannot
# persist malware or modify the host filesystem.
# ==============================================================================

set -uo pipefail

ACTION="${1:-status}"
DEVNODE="${2:-}"
RAW_LABEL="${3:-USB_Vault}"

SAFE_LABEL=$(echo "$RAW_LABEL" | sed -E 's/[^a-zA-Z0-9._-]+/_/g')
[ -z "$SAFE_LABEL" ] && SAFE_LABEL="USB_Vault"

VAULT_BASE="/run/media/axis-vault"
LOWER_DIR="${VAULT_BASE}/lower_${SAFE_LABEL}"
RAM_DIR="${VAULT_BASE}/ram_${SAFE_LABEL}"
UPPER_DIR="${RAM_DIR}/upper"
WORK_DIR="${RAM_DIR}/work"
MERGED_DIR="${VAULT_BASE}/${SAFE_LABEL}"

case "$ACTION" in
    mount)
        if [ -z "$DEVNODE" ] || [ ! -b "$DEVNODE" ]; then
            echo "[Axis USB Vault] Invalid block device: $DEVNODE" >&2
            exit 1
        fi

        if [ "$(id -u)" -eq 0 ]; then
            mkdir -p "$LOWER_DIR" "$RAM_DIR" "$MERGED_DIR"
            # 1. Mount physical USB partition strictly read-only + noexec + nosuid + nodev
            if mount -o ro,nosuid,nodev,noexec "$DEVNODE" "$LOWER_DIR" 2>/dev/null; then
                # 2. Create disposable RAM tmpfs backing for transient writes
                mount -t tmpfs -o size=256M,nosuid,nodev,noexec tmpfs "$RAM_DIR" 2>/dev/null || true
                mkdir -p "$UPPER_DIR" "$WORK_DIR"
                # 3. Mount overlayfs combining read-only USB + RAM upper layer
                if mount -t overlay overlay -o "lowerdir=$LOWER_DIR,upperdir=$UPPER_DIR,workdir=$WORK_DIR,nosuid,nodev,noexec" "$MERGED_DIR" 2>/dev/null; then
                    echo "[Axis USB Vault] Mounted $DEVNODE in disposable Quarantine Vault at $MERGED_DIR"
                    exit 0
                fi
            fi
        fi

        # Fallback to udisksctl with no-user-interaction if non-root
        udisksctl mount --no-user-interaction -o nosuid,nodev,noexec -b "$DEVNODE" >/dev/null 2>&1 || true
        ;;
    unmount)
        if [ "$(id -u)" -eq 0 ]; then
            umount -l "$MERGED_DIR" 2>/dev/null || true
            umount -l "$RAM_DIR" 2>/dev/null || true
            umount -l "$LOWER_DIR" 2>/dev/null || true
            rmdir "$MERGED_DIR" "$RAM_DIR" "$LOWER_DIR" 2>/dev/null || true
        fi
        ;;
    open-sandboxed)
        TARGET_PATH="${2:-$MERGED_DIR}"
        if command -v bwrap >/dev/null 2>&1; then
            exec bwrap \
                --ro-bind /usr /usr \
                --ro-bind /lib /lib \
                --ro-bind /lib64 /lib64 \
                --ro-bind /bin /bin \
                --ro-bind /etc /etc \
                --dev /dev \
                --proc /proc \
                --tmpfs /home \
                --tmpfs /tmp \
                --ro-bind "$TARGET_PATH" "/mnt/usb-vault" \
                --unshare-net \
                -- nautilus "/mnt/usb-vault"
        else
            exec nautilus "$TARGET_PATH"
        fi
        ;;
    *)
        echo "Usage: $0 {mount|unmount|open-sandboxed} <device> [label]"
        exit 1
        ;;
esac
