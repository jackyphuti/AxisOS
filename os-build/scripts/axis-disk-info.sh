#!/bin/bash
# axis-disk-info.sh - Discovers block devices, accurately calculates capacity vs true unallocated free space,
# and detects installed operating systems (e.g. Windows NTFS, Linux Ext4/Btrfs).
# Output format per line:
# DEVICE|CAPACITY|FREE_SPACE|MODEL_AND_OS

set -e

# Human readable format helper
format_bytes() {
    local bytes=$1
    if [ "$bytes" -ge 1099511627776 ]; then
        awk -v b="$bytes" 'BEGIN {printf "%.1f TB", b / 1099511627776}'
    elif [ "$bytes" -ge 1073741824 ]; then
        awk -v b="$bytes" 'BEGIN {printf "%.1f GB", b / 1073741824}'
    elif [ "$bytes" -ge 1048576 ]; then
        awk -v b="$bytes" 'BEGIN {printf "%.1f MB", b / 1048576}'
    elif [ "$bytes" -gt 0 ]; then
        awk -v b="$bytes" 'BEGIN {printf "%.0f KB", b / 1024}'
    else
        echo "0 B"
    fi
}

# Find all disk block devices (excluding loops, zram, cdrom/sr)
for dev in $(lsblk -d -p -n -l -o NAME,TYPE 2>/dev/null | awk '$2=="disk"{print $1}'); do
    # Skip virtual/ram/optical devices
    case "$dev" in
        /dev/loop*|/dev/zram*|/dev/ram*|/dev/sr*) continue ;;
    esac

    # Total disk size
    disk_bytes=$(blockdev --getsize64 "$dev" 2>/dev/null || true)
    if [ -z "$disk_bytes" ] || [ "$disk_bytes" -le 0 ]; then
        disk_bytes=$(lsblk -b -d -n -o SIZE "$dev" 2>/dev/null || echo 0)
    fi

    if [ "$disk_bytes" -le 0 ]; then
        continue
    fi

    disk_cap_hr=$(format_bytes "$disk_bytes")

    # Read disk model / bus info
    model=$(lsblk -d -n -o MODEL,TRAN "$dev" 2>/dev/null | sed -e 's/^[ \t]*//' -e 's/[ \t]*$//' -e 's/  */ /g')
    [ -z "$model" ] && model="System Storage Drive"

    # Inspect whole disk directly for direct fs
    disk_direct_fs=$(lsblk -d -n -o FSTYPE "$dev" 2>/dev/null | tr -d '[:space:]')

    # Inspect partitions
    part_count=0
    part_bytes_total=0
    has_windows=0
    has_linux=0
    has_efi=0

    # Get all partitions for this device
    while IFS= read -r line; do
        [ -z "$line" ] && continue
        p_type=$(echo "$line" | awk '{print $1}')
        p_size=$(echo "$line" | awk '{print $2}')
        [ "$p_type" != "part" ] && continue

        part_count=$((part_count + 1))
        if [ -n "$p_size" ] && [ "$p_size" -gt 0 ] 2>/dev/null; then
            part_bytes_total=$((part_bytes_total + p_size))
        fi

        line_lower=$(echo "$line" | tr '[:upper:]' '[:lower:]')
        if [[ "$line_lower" =~ ntfs|bitlocker|microsoft|recovery|windows ]]; then
            has_windows=1
        fi
        if [[ "$line_lower" =~ ext4|btrfs|xfs|f2fs ]]; then
            has_linux=1
        fi
        if [[ "$line_lower" =~ vfat|fat32|efi ]]; then
            has_efi=1
        fi
    done < <(lsblk -b -r -n -o TYPE,SIZE,FSTYPE,PARTTYPE,PARTLABEL,LABEL "$dev" 2>/dev/null || true)

    # Secondary check with blkid if not yet detected as windows
    if [ "$has_windows" -eq 0 ]; then
        if blkid "${dev}"* 2>/dev/null | grep -qiE 'ntfs|bitlocker'; then
            has_windows=1
        fi
    fi

    # Calculate unallocated free space
    free_bytes=$((disk_bytes - part_bytes_total))
    if [ "$free_bytes" -lt 0 ]; then
        free_bytes=0
    fi

    # Determine OS status description
    if [ "$has_windows" -eq 1 ]; then
        os_desc="[Windows NTFS - In Use]"
    elif [ "$has_linux" -eq 1 ]; then
        os_desc="[Linux System Detected]"
    elif [ "$part_count" -gt 0 ]; then
        os_desc="[Partitioned Drive (${part_count} part)]"
    elif [ -n "$disk_direct_fs" ]; then
        os_desc="[Direct ${disk_direct_fs^^}]"
    else
        os_desc="[Clean / Empty Drive]"
    fi

    # Determine free space string
    if [ "$part_count" -eq 0 ]; then
        if [ -n "$disk_direct_fs" ]; then
            free_hr="0 B (Formatted ${disk_direct_fs})"
        else
            free_hr="${disk_cap_hr} (100% Free)"
        fi
    elif [ "$free_bytes" -lt 1073741824 ]; then
        free_hr="0 B (Fully Allocated)"
    else
        free_hr=$(format_bytes "$free_bytes")
        pct=$(( free_bytes * 100 / disk_bytes ))
        free_hr="${free_hr} (${pct}% Free)"
    fi

    full_desc="${model} ${os_desc}"
    echo "${dev}|${disk_cap_hr}|${free_hr}|${full_desc}"
done
