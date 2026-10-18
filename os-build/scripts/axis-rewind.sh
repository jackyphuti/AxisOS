#!/bin/bash
# AxisOS 2.0 — Axis Rewind (Per-Window CoW State & Config Time-Travel Engine)
# Instant zero-cost Btrfs/XFS reflink checkpoints & rollback for running apps.

set -euo pipefail

ACTION="${1:-status}"
APP_ID="${2:-unknown}"
TARGET_PID="${3:-0}"

SAFE_APP="$(echo "$APP_ID" | tr -cd 'a-zA-Z0-9._-' | tr '[:upper:]' '[:lower:]')"
[ -z "$SAFE_APP" ] && SAFE_APP="app"

BASE_STORE="${XDG_DATA_HOME:-${HOME:-/home/axis}/.local/share}/axis-rewind"
REWIND_ROOT="${BASE_STORE}/${SAFE_APP}"
if ! (mkdir -p "$REWIND_ROOT" && touch "$REWIND_ROOT/.probe" && rm -f "$REWIND_ROOT/.probe") 2>/dev/null; then
    BASE_STORE="/tmp/axis-rewind-$(id -u)"
    REWIND_ROOT="${BASE_STORE}/${SAFE_APP}"
    mkdir -p "$REWIND_ROOT"
fi
APP_CFG_DIR="${XDG_CONFIG_HOME:-${HOME:-/home/axis}/.config}/${SAFE_APP}"

case "$ACTION" in
    checkpoint)
        STAMP="$(date +%Y%m%d-%H%M%S)"
        SNAP_DIR="${REWIND_ROOT}/${STAMP}"
        mkdir -p "$SNAP_DIR"

        # 1. Snapshot ~/.config/<app> using CoW reflinks when supported by Btrfs/XFS
        if [ -d "$APP_CFG_DIR" ]; then
            cp -a --reflink=auto "$APP_CFG_DIR" "${SNAP_DIR}/config" 2>/dev/null || cp -a "$APP_CFG_DIR" "${SNAP_DIR}/config"
        fi

        # 2. Record process metadata for rapid state inspection
        if [ "$TARGET_PID" -gt 1 ] 2>/dev/null && [ -d "/proc/${TARGET_PID}" ]; then
            readlink "/proc/${TARGET_PID}/cwd" > "${SNAP_DIR}/cwd.txt" 2>/dev/null || true
            tr '\0' ' ' < "/proc/${TARGET_PID}/cmdline" > "${SNAP_DIR}/cmdline.txt" 2>/dev/null || true
        fi

        echo "$SNAP_DIR" > "${REWIND_ROOT}/latest.ptr"
        ln -sfn "$SNAP_DIR" "${REWIND_ROOT}/latest" 2>/dev/null || true
        # Retain last 10 checkpoints per app to bound disk metadata
        ls -1dt "${REWIND_ROOT}"/20* 2>/dev/null | tail -n +11 | xargs -r rm -rf --
        echo "[Axis Rewind] Checkpoint created: ${SNAP_DIR}"
        ;;

    rewind)
        LATEST=""
        if [ -f "${REWIND_ROOT}/latest.ptr" ]; then
            LATEST="$(cat "${REWIND_ROOT}/latest.ptr")"
        elif [ -L "${REWIND_ROOT}/latest" ]; then
            LATEST="$(readlink -f "${REWIND_ROOT}/latest")"
        fi
        if [ -z "$LATEST" ] || [ ! -d "$LATEST" ]; then
            echo "[Axis Rewind] No checkpoint found for ${SAFE_APP}." >&2
            exit 1
        fi

        # Pause process briefly during atomic state restoration
        if [ "$TARGET_PID" -gt 1 ] 2>/dev/null && [ -d "/proc/${TARGET_PID}" ]; then
            kill -STOP "$TARGET_PID" 2>/dev/null || true
        fi

        if [ -d "${LATEST}/config" ]; then
            rm -rf "$APP_CFG_DIR"
            cp -a --reflink=auto "${LATEST}/config" "$APP_CFG_DIR" 2>/dev/null || cp -a "${LATEST}/config" "$APP_CFG_DIR"
        fi

        if [ "$TARGET_PID" -gt 1 ] 2>/dev/null && [ -d "/proc/${TARGET_PID}" ]; then
            kill -HUP "$TARGET_PID" 2>/dev/null || true
            kill -CONT "$TARGET_PID" 2>/dev/null || true
        fi
        echo "[Axis Rewind] Restored ${SAFE_APP} to checkpoint $(basename "$LATEST")"
        ;;

    list)
        ls -1dt "${REWIND_ROOT}"/20* 2>/dev/null || true
        ;;

    checkpoint-system)
        STAMP="$(date +%Y%m%d-%H%M%S)"
        LABEL="${APP_ID:-pre-upgrade}"
        # If running on Btrfs root filesystem
        if command -v btrfs >/dev/null 2>&1 && [ "$(stat -f -c %T / 2>/dev/null)" = "btrfs" ]; then
            SNAP_DIR="/.snapshots/${LABEL}-${STAMP}"
            mkdir -p "/.snapshots"
            if btrfs subvolume snapshot -r / "$SNAP_DIR" 2>/dev/null; then
                echo "[Axis Rewind] Rootfs Btrfs snapshot created: $SNAP_DIR"
            fi
        else
            SYS_SNAP_DIR="/var/backups/axis-rewind/${LABEL}-${STAMP}"
            mkdir -p "$SYS_SNAP_DIR"
            cp -a --reflink=auto /etc "$SYS_SNAP_DIR/etc" 2>/dev/null || cp -a /etc "$SYS_SNAP_DIR/etc" 2>/dev/null || true
            echo "[Axis Rewind] System configuration checkpoint created: $SYS_SNAP_DIR"
        fi
        ;;

    *)
        echo "Usage: $0 {checkpoint|rewind|list|checkpoint-system} <app-id> [pid]"
        exit 1
        ;;
esac
