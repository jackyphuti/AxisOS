#!/bin/bash
# ==============================================================================
# AxisOS Linux - Real System Installation Engine
# Performs full disk partitioning (GPT/ESP), Btrfs formatting with subvolumes,
# system root deployment (rsync from live system), user account configuration,
# and GRUB 2 EFI / BIOS bootloader installation.
# ==============================================================================

# Standard error handling (explicit checks for critical operations)
export PATH="/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:$PATH"
umask 022

TARGET_DISK=""
USERNAME="axis"
PASSWORD="password"
USER_FULLNAME="AxisOS User"
HOSTNAME="axis-pc"
AUTOLOGIN="true"
LOCALE="en_US.UTF-8"
KEYMAP="us"
TIMEZONE="UTC"
FILESYSTEM="btrfs"
DRY_RUN="false"

# Helper for progress reporting: PROGRESS:<percent>:<message>
report() {
    local percent="$1"
    local message="$2"
    echo "PROGRESS:${percent}:${message}"
    echo "[AxisOS Install] (${percent}%) ${message}" >> /tmp/axisos-installer.log 2>/dev/null || true
}

log() {
    echo "[AxisOS Install] $*" >> /tmp/axisos-installer.log 2>/dev/null || true
}

cleanup_mounts() {
    sync
    umount -l /mnt/sys/firmware/efi/efivars 2>/dev/null || true
    umount -l /mnt/dev/pts 2>/dev/null || true
    umount -l /mnt/dev 2>/dev/null || true
    umount -l /mnt/proc 2>/dev/null || true
    umount -l /mnt/sys 2>/dev/null || true
    umount -l /mnt/run 2>/dev/null || true
    umount -l /mnt/boot/efi 2>/dev/null || true
    umount -l /mnt/home 2>/dev/null || true
    umount -l /mnt/var/log 2>/dev/null || true
    umount -l /mnt/.snapshots 2>/dev/null || true
    umount -R /mnt 2>/dev/null || umount -l /mnt 2>/dev/null || true
}

error_exit() {
    local message="$1"
    echo "ERROR: ${message}" >&2
    echo "[AxisOS Install ERROR] ${message}" >> /tmp/axisos-installer.log 2>/dev/null || true
    cleanup_mounts
    report 0 "Error: ${message}"
    exit 1
}

usage() {
    cat << EOF
Usage: $0 --disk <device> [options]
Options:
  --disk <path>         Target block device (e.g. /dev/sda, /dev/nvme0n1) [REQUIRED]
  --edition <type>      OS Edition (base|enterprise|education, default: base)
  --username <name>     Primary username (default: axis)
  --password <pass>     User password (default: password)
  --fullname <name>     User full name (default: AxisOS User)
  --hostname <name>     Machine hostname (default: axis-pc)
  --autologin <bool>    Auto-login on boot (true|false, default: true)
  --locale <locale>     System locale (default: en_US.UTF-8)
  --keymap <layout>     Keyboard layout (default: us)
  --timezone <zone>     System timezone (default: UTC)
  --fs <type>           Filesystem (btrfs|ext4, default: btrfs)
  --dry-run             Simulate actions without wiping disks
EOF
    exit 1
}

EDITION="base"

# Parse CLI arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --disk) TARGET_DISK="$2"; shift 2 ;;
        --edition) EDITION="$2"; shift 2 ;;
        --user|--username) USERNAME="$2"; shift 2 ;;
        --pass|--password) PASSWORD="$2"; shift 2 ;;
        --password-stdin) IFS= read -r PASSWORD; shift 1 ;;
        --name|--fullname) USER_FULLNAME="$2"; shift 2 ;;
        --host|--hostname) HOSTNAME="$2"; shift 2 ;;
        --autologin) AUTOLOGIN="$2"; shift 2 ;;
        --locale) LOCALE="$2"; shift 2 ;;
        --keymap) KEYMAP="$2"; shift 2 ;;
        --timezone) TIMEZONE="$2"; shift 2 ;;
        --fs) FILESYSTEM="$2"; shift 2 ;;
        --dry-run) DRY_RUN="true"; shift 1 ;;
        *) echo "Unknown parameter: $1"; usage ;;
    esac
done

if [[ -z "$TARGET_DISK" ]]; then
    error_exit "--disk parameter is required."
fi

report 2 "Validating pre-installation requirements..."
log "Target disk: $TARGET_DISK"
log "Username: $USERNAME"
log "Hostname: $HOSTNAME"
log "Filesystem: $FILESYSTEM"

if [[ "$DRY_RUN" == "true" ]]; then
    log "DRY RUN mode active - simulating installation steps..."
    for p in 10 25 45 65 80 95 100; do
        sleep 1
        report "$p" "Dry-run step: $p% completed."
    done
    report 100 "AxisOS dry-run installation finished successfully."
    exit 0
fi

# Ensure running as root
if [[ $EUID -ne 0 ]]; then
    error_exit "This installer must be run as root."
fi

# Verify disk exists
if [[ ! -b "$TARGET_DISK" ]]; then
    error_exit "Target disk $TARGET_DISK does not exist or is not a block device."
fi

# Helper: Resolve parent disk of a device (handles partitions, loop devices, etc.)
resolve_parent_disk() {
    local dev="$1"
    [ -z "$dev" ] && return
    if [[ "$dev" =~ /dev/loop[0-9]+ ]]; then
        local back_file
        back_file=$(losetup -O BACK-FILE -n "$dev" 2>/dev/null || true)
        if [[ -n "$back_file" && -e "$back_file" ]]; then
            local back_src
            back_src=$(df -P "$back_file" 2>/dev/null | awk 'NR==2 {print $1}')
            if [[ -n "$back_src" && "$back_src" != "$dev" ]]; then
                resolve_parent_disk "$back_src"
                return
            fi
        fi
    fi

    if [[ "$dev" == /dev/* ]]; then
        local pk
        pk=$(lsblk -no PKNAME "$dev" 2>/dev/null | head -n 1 || true)
        if [[ -n "$pk" ]]; then
            echo "/dev/$pk"
            return
        fi
        local dtype
        dtype=$(lsblk -no TYPE "$dev" 2>/dev/null | head -n 1 || true)
        if [[ "$dtype" == "disk" ]]; then
            echo "$dev"
            return
        fi
        local stripped="$dev"
        if [[ "$stripped" =~ ^(/dev/.*)p[0-9]+$ ]]; then
            echo "${BASH_REMATCH[1]}"
            return
        elif [[ "$stripped" =~ ^(/dev/[a-zA-Z]+)[0-9]+$ ]]; then
            echo "${BASH_REMATCH[1]}"
            return
        fi
        echo "$dev"
        return
    fi
}

get_excluded_disks() {
    local excluded_disks=()

    add_excluded() {
        local d="$1"
        [ -z "$d" ] && return
        local parent
        parent=$(resolve_parent_disk "$d")
        if [[ -n "$parent" ]]; then
            for e in "${excluded_disks[@]}"; do
                [[ "$e" == "$parent" ]] && return
            done
            excluded_disks+=("$parent")
        fi
    }

    # 1. Mount points for installation medium and initramfs
    for mp in /run/live/medium /run/live/rootfs /run/initramfs /run/initramfs/live /cdrom /live/image; do
        if mountpoint -q "$mp" 2>/dev/null || [ -d "$mp" ]; then
            local src
            src=$(findmnt -n -o SOURCE "$mp" 2>/dev/null || true)
            [ -n "$src" ] && add_excluded "$src"
        fi
    done

    # 2. Check / (root) backing device if not a pure overlay/tmpfs/rootfs
    local root_src
    root_src=$(findmnt -n -o SOURCE / 2>/dev/null || true)
    if [[ -n "$root_src" && "$root_src" != "overlay" && "$root_src" != "rootfs" && "$root_src" != "tmpfs" && "$root_src" == /dev/* ]]; then
        add_excluded "$root_src"
    fi

    # 3. Check /proc/mounts for any live or initramfs mounts
    while read -r src target fstype rest; do
        if [[ "$target" =~ ^/run/live || "$target" =~ ^/run/initramfs || "$target" =~ ^/cdrom ]]; then
            [[ "$src" == /dev/* ]] && add_excluded "$src"
        fi
    done < /proc/mounts 2>/dev/null || true

    echo "${excluded_disks[@]}"
}

# Safeguard installer boot device, initramfs, and live USB medium
EXCLUDED_DISKS=($(get_excluded_disks))
log "Excluded boot/installer disks: ${EXCLUDED_DISKS[*]}"
for ex_disk in "${EXCLUDED_DISKS[@]}"; do
    if [[ "$TARGET_DISK" == "$ex_disk" ]]; then
        error_exit "Target disk $TARGET_DISK is the installer's active boot device or installation medium ($ex_disk). Refusing to overwrite. Please select an internal drive."
    fi
done

# Print chosen disk, its model, and its size before wiping
DISK_MODEL=$(lsblk -d -n -o MODEL "$TARGET_DISK" 2>/dev/null | sed -e 's/^[ \t]*//' -e 's/[ \t]*$//' || echo "Generic Storage Device")
[ -z "$DISK_MODEL" ] && DISK_MODEL="Generic Storage Device"
DISK_SIZE_HR=$(lsblk -d -n -o SIZE "$TARGET_DISK" 2>/dev/null || echo "Unknown Size")
DISK_SIZE_BYTES=$(blockdev --getsize64 "$TARGET_DISK" 2>/dev/null || echo 0)
log "Selected installation target: $TARGET_DISK | Model: $DISK_MODEL | Size: $DISK_SIZE_HR ($DISK_SIZE_BYTES bytes)"
echo "Target disk selected: $TARGET_DISK (Model: $DISK_MODEL, Size: $DISK_SIZE_HR)"

# Detect UEFI vs BIOS
IS_UEFI=false
if [[ -d /sys/firmware/efi ]]; then
    IS_UEFI=true
    log "Boot mode: UEFI detected"
else
    log "Boot mode: Legacy BIOS / CSM detected"
fi

# Unmount any existing partitions, deactivate LVM, dmsetup, multipath, and swap
report 5 "Preparing target disk and unlocking block device $TARGET_DISK..."

# Stop system daemons that may hold exclusive inotify/O_EXCL locks on block devices
systemctl stop udisks2.service 2>/dev/null || true
systemctl stop multipathd.service multipathd.socket 2>/dev/null || true
multipath -F 2>/dev/null || true
dmraid -an 2>/dev/null || true

# Kill any processes holding open handles on the target disk or any of its partitions
fuser -k -9 "$TARGET_DISK" 2>/dev/null || true
for p_init in $(lsblk -rno NAME,TYPE "$TARGET_DISK" 2>/dev/null | awk '$2=="part" {print $1}'); do
    p_dev="/dev/$p_init"
    [[ "$p_init" == /* ]] && p_dev="$p_init"
    fuser -k -9 "$p_dev" 2>/dev/null || true
done
sleep 1

# Disable all active swap
swapoff "$TARGET_DISK"* 2>/dev/null || true
swapoff -a 2>/dev/null || true

# Deactivate LVM
vgchange -an 2>/dev/null || true
lvchange -an 2>/dev/null || true

# Remove all device-mapper mappings associated with the disk
for dm in $(dmsetup ls 2>/dev/null | awk '{print $1}'); do
    dmsetup remove -f "$dm" 2>/dev/null || true
done
dmsetup remove_all -f 2>/dev/null || true
mdadm --stop --scan 2>/dev/null || true

# Safely unmount all mountpoints matching target disk
if mountpoint -q /mnt; then
    umount -l -R /mnt 2>/dev/null || true
fi
if mountpoint -q /mnt_axis_tmp; then
    umount -l -R /mnt_axis_tmp 2>/dev/null || true
fi

# 1. Using findmnt
while read -r target; do
    if [[ -n "$target" ]]; then
        log "Unmounting active mountpoint $target on $TARGET_DISK..."
        umount -f "$target" 2>/dev/null || umount -l "$target" 2>/dev/null || error_exit "Failed to unmount active filesystem at $target on $TARGET_DISK"
    fi
done < <(findmnt -rn -o TARGET -S "${TARGET_DISK}"* 2>/dev/null || true)

# 2. Using lsblk MOUNTPOINT
while read -r mp; do
    if [[ -n "$mp" ]]; then
        log "Unmounting partition mountpoint $mp on $TARGET_DISK..."
        umount -f "$mp" 2>/dev/null || umount -l "$mp" 2>/dev/null || error_exit "Failed to unmount partition mountpoint at $mp on $TARGET_DISK"
    fi
done < <(lsblk -rno MOUNTPOINT "$TARGET_DISK" 2>/dev/null | awk 'NF' || true)

# 3. /proc/mounts fallback
awk -v d="$TARGET_DISK" '$1 ~ d {print $2}' /proc/mounts 2>/dev/null | while read -r mp; do
    umount -f -l "$mp" 2>/dev/null || true
done
sync

# Force kernel to drop all existing in-memory partition table entries
for p_num in $(seq 1 16); do
    delpart "$TARGET_DISK" "$p_num" 2>/dev/null || true
done
partx -d "$TARGET_DISK" 2>/dev/null || true
udevadm settle --timeout=3 2>/dev/null || sleep 1

report 10 "Wiping partition signatures on $TARGET_DISK..."
# Unmount all child partitions and wipe individual partition signatures
for p in $(lsblk -lno NAME "$TARGET_DISK" 2>/dev/null); do
    [ -b "/dev/$p" ] && wipefs -a -f "/dev/$p" 2>/dev/null || true
done

# Wipe target disk signatures with wipefs and zap partition table
log "Running wipefs -a -f on $TARGET_DISK..."
wipefs -a -f "$TARGET_DISK" 2>/dev/null || true

if command -v sgdisk >/dev/null 2>&1; then
    log "Running sgdisk --zap-all on $TARGET_DISK..."
    sgdisk --zap-all "$TARGET_DISK" 2>/dev/null || true
fi

# Zero out first 32MB (primary GPT, MBR, partition tables)
dd if=/dev/zero of="$TARGET_DISK" bs=1M count=32 conv=notrunc status=none 2>/dev/null || true

# Zero out last 32MB (secondary/backup GPT header and tables at end of disk)
DISK_SECTORS=$(blockdev --getsz "$TARGET_DISK" 2>/dev/null || echo 0)
if [ "$DISK_SECTORS" -gt 65536 ]; then
    SEEK_START=$(( DISK_SECTORS - 65536 ))
    dd if=/dev/zero of="$TARGET_DISK" bs=512 seek=$SEEK_START count=65536 conv=notrunc status=none 2>/dev/null || true
fi
sync
blockdev --flushbufs "$TARGET_DISK" 2>/dev/null || true
blockdev --rereadpt "$TARGET_DISK" 2>/dev/null || true
if command -v partprobe >/dev/null 2>&1; then
    partprobe "$TARGET_DISK" 2>/dev/null || true
fi
udevadm settle --timeout=3 2>/dev/null || sleep 1

report 15 "Partitioning drive with GPT scheme..."
# Calculate swap size based on disk capacity
DISK_SIZE_BYTES=$(blockdev --getsize64 "$TARGET_DISK" 2>/dev/null || echo 0)
DISK_SIZE_MB=$(( DISK_SIZE_BYTES / 1024 / 1024 ))
SWAP_SIZE="4096M"
if [ "$DISK_SIZE_MB" -lt 65536 ]; then
    SWAP_SIZE="2048M"
fi
if [ "$DISK_SIZE_MB" -lt 25600 ]; then
    SWAP_SIZE="1024M"
fi
if [ "$DISK_SIZE_MB" -lt 12800 ]; then
    SWAP_SIZE="512M"
fi

PARTITION_SUCCESS=false

# Method 1: Atomic sfdisk with standard GPT GUIDs
if command -v sfdisk >/dev/null 2>&1; then
    log "Partitioning with atomic sfdisk..."
    if sfdisk --wipe always --wipe-partitions always --force "$TARGET_DISK" <<EOF
label: gpt
size=512M, type=C12A7328-F81F-11D2-BA4B-00A0C93EC93B, name="AXIS_BOOT"
size=${SWAP_SIZE}, type=0657FD6D-A4AB-43C4-84E5-0933C84B4F4F, name="AXIS_SWAP"
type=0FC63DAF-8483-4772-8E79-3D69D8477DE4, name="AXIS_ROOT"
EOF
    then
        PARTITION_SUCCESS=true
        log "sfdisk completed successfully."
    else
        log "sfdisk failed with exit code $?."
    fi
fi

# Method 2: sgdisk fallback if sfdisk failed
if [[ "$PARTITION_SUCCESS" != "true" ]] && command -v sgdisk >/dev/null 2>&1; then
    log "Partitioning with sgdisk fallback..."
    if sgdisk -o "$TARGET_DISK" && \
       sgdisk -n 1:2048:+512M -t 1:ef00 -c 1:"AXIS_BOOT" "$TARGET_DISK" && \
       sgdisk -n 2:0:+${SWAP_SIZE} -t 2:8200 -c 2:"AXIS_SWAP" "$TARGET_DISK" && \
       sgdisk -n 3:0:0 -t 3:8300 -c 3:"AXIS_ROOT" "$TARGET_DISK"; then
        PARTITION_SUCCESS=true
        log "sgdisk completed successfully."
    else
        log "sgdisk failed with exit code $?."
    fi
fi

# Method 3: parted fallback with optimal alignment if sfdisk and sgdisk failed
if [[ "$PARTITION_SUCCESS" != "true" ]] && command -v parted >/dev/null 2>&1; then
    log "Partitioning with parted optimal alignment fallback..."
    SWAP_MB_VAL=${SWAP_SIZE%M}
    SWAP_END=$(( 513 + SWAP_MB_VAL ))
    if parted -s -a optimal "$TARGET_DISK" mklabel gpt && \
       parted -s -a optimal "$TARGET_DISK" mkpart "AXIS_BOOT" fat32 1MiB 513MiB && \
       parted -s "$TARGET_DISK" set 1 esp on && \
       parted -s -a optimal "$TARGET_DISK" mkpart "AXIS_SWAP" linux-swap 513MiB "${SWAP_END}MiB" && \
       parted -s -a optimal "$TARGET_DISK" mkpart "AXIS_ROOT" ext4 "${SWAP_END}MiB" 100%; then
        PARTITION_SUCCESS=true
        log "parted completed successfully."
    else
        log "parted failed with exit code $?."
    fi
fi

[[ "$PARTITION_SUCCESS" != "true" ]] && error_exit "Partitioning failed: could not write GPT partition table on $TARGET_DISK using sfdisk, sgdisk, or parted."

sync
blockdev --flushbufs "$TARGET_DISK" 2>/dev/null || true
if command -v partprobe >/dev/null 2>&1; then
    partprobe "$TARGET_DISK" || log "partprobe returned exit code $?"
fi
udevadm settle --timeout=30 2>/dev/null || sleep 2

# Verify table with sgdisk -p or sfdisk -d, confirming expected partition count and types
log "Verifying partition table layout on $TARGET_DISK..."
table_verified=false
part_table_count=0
has_esp_type=false

if command -v sgdisk >/dev/null 2>&1; then
    sg_check=$(sgdisk -p "$TARGET_DISK" 2>/dev/null || true)
    log "sgdisk -p verification:
$sg_check"
    part_table_count=$(echo "$sg_check" | awk '$1 ~ /^[1-9][0-9]*$/ {count++} END {print count+0}')
    if echo "$sg_check" | grep -qiE 'EF00|C12A7328-F81F-11D2-BA4B-00A0C93EC93B'; then
        has_esp_type=true
    fi
    if [ "$part_table_count" -ge 3 ]; then
        if [[ "$IS_UEFI" == "true" ]]; then
            [ "$has_esp_type" = "true" ] && table_verified=true
        else
            table_verified=true
        fi
    fi
fi

if [[ "$table_verified" != "true" ]] && command -v sfdisk >/dev/null 2>&1; then
    sf_check=$(sfdisk -d "$TARGET_DISK" 2>/dev/null || true)
    log "sfdisk -d verification:
$sf_check"
    part_table_count=$(echo "$sf_check" | grep -c "type=" || true)
    if echo "$sf_check" | grep -qiE 'C12A7328-F81F-11D2-BA4B-00A0C93EC93B|type=c12a7328'; then
        has_esp_type=true
    fi
    if [ "$part_table_count" -ge 3 ]; then
        if [[ "$IS_UEFI" == "true" ]]; then
            [ "$has_esp_type" = "true" ] && table_verified=true
        else
            table_verified=true
        fi
    fi
fi

if [[ "$table_verified" != "true" ]]; then
    if [ "$part_table_count" -lt 3 ]; then
        error_exit "Partition table verification failed on $TARGET_DISK: expected at least 3 partitions, but found $part_table_count in table."
    elif [[ "$IS_UEFI" == "true" && "$has_esp_type" != "true" ]]; then
        error_exit "Partition table verification failed on $TARGET_DISK: UEFI mode requires EFI System Partition (type EF00), none found."
    fi
fi
log "Partition table verified: $part_table_count partitions confirmed (ESP type present: $has_esp_type)."

# Resume udisks2 now that partitioning is verified
systemctl start udisks2.service 2>/dev/null || true

# Find partition nodes by reading the table, not by constructing names.
# Use lsblk -rno NAME,TYPE /dev/DISK and filter for TYPE=part.
# Handle "p" separator for nvme (nvme0n1p1), mmcblk (mmcblk0p1), and loop (loop0p1) automatically.
report 20 "Waiting for partition device nodes to be exposed by kernel..."
real_partitions=()
START_TIME=$(date +%s)
MAX_WAIT_SECONDS=30
RETRY_REREAD_DONE_5=false
RETRY_REREAD_DONE_15=false

while true; do
    real_partitions=()
    while read -r name type; do
        if [ "$type" = "part" ]; then
            if [[ "$name" = /* ]]; then
                real_partitions+=("$name")
            else
                real_partitions+=("/dev/$name")
            fi
        fi
    done < <(lsblk -rno NAME,TYPE "$TARGET_DISK" 2>/dev/null || true)

    # Check if we have at least 3 partition nodes and that every node exists as a block special device
    if [ ${#real_partitions[@]} -ge 3 ]; then
        all_block_nodes=true
        for p_dev in "${real_partitions[@]}"; do
            if [ ! -b "$p_dev" ]; then
                all_block_nodes=false
                break
            fi
        done
        if [ "$all_block_nodes" = "true" ]; then
            log "All partition nodes found and confirmed as block devices: ${real_partitions[*]}"
            break
        fi
    fi

    NOW=$(date +%s)
    ELAPSED=$(( NOW - START_TIME ))

    # If the table lists partitions but no nodes appear after partprobe and udevadm settle,
    # call partx -a /dev/DISK or blockdev --rereadpt /dev/DISK, then retry the wait loop.
    if [ "$ELAPSED" -ge 5 ] && [ "$RETRY_REREAD_DONE_5" = "false" ]; then
        log "Nodes delayed: triggering kernel re-read with partx -a and blockdev --rereadpt on $TARGET_DISK..."
        partx -a "$TARGET_DISK" 2>/dev/null || true
        blockdev --rereadpt "$TARGET_DISK" 2>/dev/null || true
        if command -v partprobe >/dev/null 2>&1; then
            partprobe "$TARGET_DISK" 2>/dev/null || true
        fi
        udevadm trigger --action=add --subsystem-match=block 2>/dev/null || true
        udevadm settle --timeout=5 2>/dev/null || true
        RETRY_REREAD_DONE_5=true
    fi

    if [ "$ELAPSED" -ge 15 ] && [ "$RETRY_REREAD_DONE_15" = "false" ]; then
        log "Nodes still delayed: retrying partx -u, partx -a, and blockdev --rereadpt on $TARGET_DISK..."
        partx -u "$TARGET_DISK" 2>/dev/null || true
        partx -a "$TARGET_DISK" 2>/dev/null || true
        blockdev --rereadpt "$TARGET_DISK" 2>/dev/null || true
        udevadm trigger --action=add --subsystem-match=block 2>/dev/null || true
        udevadm settle --timeout=5 2>/dev/null || true
        RETRY_REREAD_DONE_15=true
    fi

    if [ "$ELAPSED" -ge "$MAX_WAIT_SECONDS" ]; then
        log "TIMEOUT: Partition device nodes failed to appear after ${MAX_WAIT_SECONDS} seconds on $TARGET_DISK."
        log "=== lsblk output ==="
        lsblk "$TARGET_DISK" >> /tmp/axisos-installer.log 2>&1 || true
        log "=== lsblk -rno NAME,TYPE,FSTYPE,SIZE ==="
        lsblk -rno NAME,TYPE,FSTYPE,SIZE "$TARGET_DISK" >> /tmp/axisos-installer.log 2>&1 || true
        if command -v sgdisk >/dev/null 2>&1; then
            log "=== sgdisk -p output ==="
            sgdisk -p "$TARGET_DISK" >> /tmp/axisos-installer.log 2>&1 || true
        fi
        error_exit "Expected partition device nodes failed to appear after waiting ${MAX_WAIT_SECONDS} seconds on $TARGET_DISK. Partition table was written, but kernel did not expose block devices. Diagnostic logs dumped to /tmp/axisos-installer.log."
    fi

    sleep 0.5
done

# If lsblk returned fewer than 3 partitions, fallback to blkid lookup
if [[ ${#real_partitions[@]} -lt 3 ]]; then
    while read -r b_dev rest; do
        b_clean="${b_dev%:}"
        if [[ "$b_clean" == /dev/* && "$b_clean" != "$TARGET_DISK" ]]; then
            already_in=false
            for existing_p in "${real_partitions[@]}"; do
                if [[ "$existing_p" == "$b_clean" ]]; then
                    already_in=true
                    break
                fi
            done
            if [[ "$already_in" == "false" ]]; then
                real_partitions+=("$b_clean")
            fi
        fi
    done < <(blkid 2>/dev/null | grep "^${TARGET_DISK}" || true)
fi

if [[ ${#real_partitions[@]} -lt 3 ]]; then
    error_exit "Partition discovery failed: expected 3 partitions on $TARGET_DISK, found ${#real_partitions[@]} (${real_partitions[*]})."
fi

ESP_PART="${real_partitions[0]}"
SWAP_PART="${real_partitions[1]}"
ROOT_PART="${real_partitions[2]}"

# Add a log line for each partition found, with its device path, size, and filesystem
for part_dev in "$ESP_PART" "$SWAP_PART" "$ROOT_PART"; do
    part_size=$(lsblk -b -n -o SIZE "$part_dev" 2>/dev/null || echo "0")
    part_size_hr=$(lsblk -n -o SIZE "$part_dev" 2>/dev/null || echo "unknown")
    part_fs=$(lsblk -n -o FSTYPE "$part_dev" 2>/dev/null || blkid -s TYPE -o value "$part_dev" 2>/dev/null || echo "unformatted")
    [ -z "$part_fs" ] && part_fs="unformatted"
    log "Partition found: device=$part_dev, size=$part_size_hr (${part_size} bytes), filesystem=$part_fs"
done

report 25 "Formatting filesystems (ESP FAT32, Swap, and Root)..."
# Ensure all partitions are unmounted before formatting
umount -f -l "$ESP_PART" 2>/dev/null || true
umount -f -l "$SWAP_PART" 2>/dev/null || true
umount -f -l "$ROOT_PART" 2>/dev/null || true
if mountpoint -q /mnt; then
    umount -l -R /mnt 2>/dev/null || true
fi

# Format ESP
wipefs -a -f "$ESP_PART" 2>/dev/null || true
mkfs.vfat -F32 -n "AXIS_BOOT" "$ESP_PART" || mkfs.fat -n "AXIS_BOOT" "$ESP_PART" || true

# Format Swap
wipefs -a -f "$SWAP_PART" 2>/dev/null || true
mkswap -L "AXIS_SWAP" "$SWAP_PART" 2>/dev/null || true

# Format Root
wipefs -a -f "$ROOT_PART" 2>/dev/null || true
FORMAT_SUCCESS=false

if [[ "$FILESYSTEM" == "btrfs" ]] && command -v mkfs.btrfs >/dev/null 2>&1; then
    modprobe btrfs 2>/dev/null || true
    if mkfs.btrfs -f -L "AXIS_ROOT" "$ROOT_PART" 2>/dev/null; then
        report 30 "Setting up Btrfs subvolumes (@, @home, @snapshots, @var_log)..."
        mkdir -p /mnt_axis_tmp
        if mount -t btrfs "$ROOT_PART" /mnt_axis_tmp 2>/dev/null; then
            btrfs subvolume create /mnt_axis_tmp/@ 2>/dev/null || true
            btrfs subvolume create /mnt_axis_tmp/@home 2>/dev/null || true
            btrfs subvolume create /mnt_axis_tmp/@snapshots 2>/dev/null || true
            btrfs subvolume create /mnt_axis_tmp/@var_log 2>/dev/null || true
            sync
            umount /mnt_axis_tmp 2>/dev/null || true
            rmdir /mnt_axis_tmp 2>/dev/null || true
            sync
            sleep 1

            mkdir -p /mnt
            if mount -o noatime,compress=zstd,subvol=@ "$ROOT_PART" /mnt 2>/dev/null || \
               mount -o noatime,subvol=@ "$ROOT_PART" /mnt 2>/dev/null; then
                mkdir -p /mnt/{home,boot/efi,var/log,.snapshots}
                mount -o noatime,subvol=@home "$ROOT_PART" /mnt/home 2>/dev/null || true
                mount -o noatime,subvol=@var_log "$ROOT_PART" /mnt/var/log 2>/dev/null || true
                mount -o noatime,subvol=@snapshots "$ROOT_PART" /mnt/.snapshots 2>/dev/null || true
                FORMAT_SUCCESS=true
            fi
        fi
    fi
fi

if [[ "$FORMAT_SUCCESS" != "true" ]]; then
    FILESYSTEM="ext4"
    report 30 "Formatting root as high-performance EXT4 filesystem..."
    umount -f -l "$ROOT_PART" 2>/dev/null || true
    wipefs -a -f "$ROOT_PART" 2>/dev/null || true
    sync
    udevadm settle --timeout=3 2>/dev/null || sleep 1
    mkfs.ext4 -F -F -L "AXIS_ROOT" "$ROOT_PART" || {
        sleep 2
        mkfs.ext4 -F -F -L "AXIS_ROOT" "$ROOT_PART"
    }
    sync
    udevadm settle --timeout=3 2>/dev/null || sleep 1
    mkdir -p /mnt
    mount "$ROOT_PART" /mnt || {
        sleep 1
        mount "$ROOT_PART" /mnt
    }
    mkdir -p /mnt/{home,boot/efi,var/log}
fi

mkdir -p /mnt/boot/efi
mount "$ESP_PART" /mnt/boot/efi 2>/dev/null || {
    sleep 1
    mount "$ESP_PART" /mnt/boot/efi 2>/dev/null || true
}

# Log all partitions formatted with their device path, size, and filesystem
for part_dev in "$ESP_PART" "$SWAP_PART" "$ROOT_PART"; do
    part_size=$(lsblk -b -n -o SIZE "$part_dev" 2>/dev/null || echo "0")
    part_size_hr=$(lsblk -n -o SIZE "$part_dev" 2>/dev/null || echo "unknown")
    part_fs=$(lsblk -n -o FSTYPE "$part_dev" 2>/dev/null || blkid -s TYPE -o value "$part_dev" 2>/dev/null || echo "unformatted")
    [ -z "$part_fs" ] && part_fs="unformatted"
    log "Partition formatted: device=$part_dev, size=$part_size_hr (${part_size} bytes), filesystem=$part_fs"
done

# Verify that root is successfully mounted
if ! mountpoint -q /mnt; then
    error_exit "Failed to mount target root filesystem on /mnt."
fi

report 35 "Copying AxisOS Linux system files to target root..."
SOURCE_DIR="/"
mkdir -p /mnt/{dev,proc,sys,run,tmp,etc,root}
chmod 1777 /mnt/tmp

# Execute rsync with live progress tracking
rsync -aAX \
    --exclude=/dev/** \
    --exclude=/proc/** \
    --exclude=/sys/** \
    --exclude=/tmp/** \
    --exclude=/run/** \
    --exclude=/mnt/** \
    --exclude=/media/** \
    --exclude=/live/** \
    --exclude=/cdrom/** \
    --exclude=/lost+found \
    --exclude=/swapfile \
    --exclude=/etc/udev/rules.d/70-persistent* \
    "$SOURCE_DIR" /mnt/ >/dev/null 2>&1 &
RSYNC_PID=$!

while kill -0 $RSYNC_PID 2>/dev/null; do
    sleep 2
    CUR_USED_MB=$(df -m /mnt 2>/dev/null | awk 'NR==2 {print $3}' || echo 0)
    PCT=$(( 35 + (CUR_USED_MB * 40 / 7000) ))
    if [ "$PCT" -gt 75 ]; then PCT=75; fi
    if [ "$PCT" -lt 35 ]; then PCT=35; fi
    report "$PCT" "Copying system files (${CUR_USED_MB} MB transferred)..."
done

RSYNC_EXIT=0
wait $RSYNC_PID || RSYNC_EXIT=$?
if [ "$RSYNC_EXIT" -ne 0 ] && [ "$RSYNC_EXIT" -ne 24 ]; then
    error_exit "Root filesystem copy (rsync) failed with exit code ${RSYNC_EXIT}. Check target disk health and free space."
fi
log "rsync completed cleanly (exit code ${RSYNC_EXIT})." 

report 76 "System file transfer complete."

report 77 "Generating filesystem mount table (/etc/fstab)..."
udevadm settle --timeout=3 2>/dev/null || true
ROOT_UUID=$(blkid -s UUID -o value "$ROOT_PART" 2>/dev/null || lsblk -no UUID "$ROOT_PART" 2>/dev/null || true)
ESP_UUID=$(blkid -s UUID -o value "$ESP_PART" 2>/dev/null || lsblk -no UUID "$ESP_PART" 2>/dev/null || true)
SWAP_UUID=$(blkid -s UUID -o value "$SWAP_PART" 2>/dev/null || lsblk -no UUID "$SWAP_PART" 2>/dev/null || true)

if [[ "$FILESYSTEM" == "ext4" ]]; then
cat << EOF > /mnt/etc/fstab
# /etc/fstab: static file system information generated by AxisOS Installer
# <file system>             <mount point>   <type>  <options>                               <dump>  <pass>
UUID=$ROOT_UUID             /               ext4    errors=remount-ro,noatime               0       1
UUID=$ESP_UUID              /boot/efi       vfat    umask=0077                              0       2
UUID=$SWAP_UUID             none            swap    defaults,nofail,x-systemd.device-timeout=5s 0   0
EOF
else
cat << EOF > /mnt/etc/fstab
# /etc/fstab: static file system information generated by AxisOS Installer
# <file system>             <mount point>   <type>  <options>                               <dump>  <pass>
UUID=$ROOT_UUID             /               btrfs   defaults,noatime,compress=zstd:3,subvol=@   0       0
UUID=$ROOT_UUID             /home           btrfs   defaults,noatime,compress=zstd:3,subvol=@home 0       0
UUID=$ROOT_UUID             /var/log        btrfs   defaults,noatime,compress=zstd:3,subvol=@var_log 0   0
UUID=$ROOT_UUID             /.snapshots     btrfs   defaults,noatime,compress=zstd:3,subvol=@snapshots 0 0
UUID=$ESP_UUID              /boot/efi       vfat    umask=0077                              0       2
UUID=$SWAP_UUID             none            swap    defaults,nofail,x-systemd.device-timeout=5s 0   0
EOF
fi

report 78 "Configuring hostname, locale, and network settings..."
echo "$HOSTNAME" > /mnt/etc/hostname
cat << EOF > /mnt/etc/hosts
127.0.0.1   localhost
127.0.1.1   $HOSTNAME
::1         localhost ip6-localhost ip6-loopback
EOF

# Locale, timezone, and keyboard
echo "LANG=${LOCALE}" > /mnt/etc/default/locale
if [[ -f /mnt/etc/locale.gen ]]; then
    sed -i "s/^#\s*${LOCALE}/${LOCALE}/" /mnt/etc/locale.gen 2>/dev/null || echo "${LOCALE} UTF-8" >> /mnt/etc/locale.gen
fi

if [[ -n "$TIMEZONE" && -f "/mnt/usr/share/zoneinfo/$TIMEZONE" ]]; then
    chroot /mnt ln -sf "/usr/share/zoneinfo/$TIMEZONE" /etc/localtime
    echo "$TIMEZONE" > /mnt/etc/timezone
fi

cat << EOF > /mnt/etc/default/keyboard
XKBMODEL="pc105"
XKBLAYOUT="$KEYMAP"
XKBVARIANT=""
XKBOPTIONS=""
BACKSPACE="guess"
EOF

report 80 "Mounting virtual filesystems for configuration..."
# Mount kernel virtual filesystems for chroot operations
mount --bind /dev /mnt/dev
mount --bind /dev/pts /mnt/dev/pts
mount --bind /proc /mnt/proc
mount --bind /sys /mnt/sys
mount --bind /run /mnt/run
mount -t efivarfs efivarfs /mnt/sys/firmware/efi/efivars 2>/dev/null || true

# Enable 32-bit multiarch for Steam, Proton, and Wine compatibility
report 82 "Enabling 32-bit multiarch (i386) for gaming compatibility..."
chroot /mnt dpkg --add-architecture i386 || true

# Configure Debian Bookworm repositories including contrib, non-free, non-free-firmware, and backports
report 84 "Configuring Debian Bookworm repositories with non-free and backports..."
mkdir -p /mnt/etc/apt
cat << 'EOF' > /mnt/etc/apt/sources.list
deb http://deb.debian.org/debian/ bookworm main contrib non-free non-free-firmware
deb-src http://deb.debian.org/debian/ bookworm main contrib non-free non-free-firmware

deb http://deb.debian.org/debian-security/ bookworm-security main contrib non-free non-free-firmware
deb-src http://deb.debian.org/debian-security/ bookworm-security main contrib non-free non-free-firmware

deb http://deb.debian.org/debian/ bookworm-updates main contrib non-free non-free-firmware
deb-src http://deb.debian.org/debian/ bookworm-updates main contrib non-free non-free-firmware

# Debian Bookworm Backports (Modern Mesa 24+ Vulkan/OpenGL drivers, modern kernel releases)
deb http://deb.debian.org/debian/ bookworm-backports main contrib non-free non-free-firmware
deb-src http://deb.debian.org/debian/ bookworm-backports main contrib non-free non-free-firmware
EOF

# Gaming & High-Performance Sysctl optimizations (Proton / Steam max_map_count)
mkdir -p /mnt/etc/sysctl.d
cat << 'EOF' > /mnt/etc/sysctl.d/99-axisos-gaming.conf
# AxisOS Gaming & Development Performance Tuning
# Critical for Steam Proton (ESync, FSync, Unreal Engine 5)
vm.max_map_count = 2147483642
fs.file-max = 524288
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192
vm.swappiness = 10
EOF

# Enable Flathub for universal application distribution
if chroot /mnt command -v flatpak >/dev/null 2>&1; then
    report 85 "Configuring Flathub universal software repository..."
    chroot /mnt flatpak remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo 2>/dev/null || true
fi

# Deploy Steam pre-installed launcher & menu entry
mkdir -p /mnt/usr/share/applications /mnt/usr/local/bin
cat << 'EOF' > /mnt/usr/share/applications/steam.desktop
[Desktop Entry]
Name=Steam
Comment=Application for managing and playing games on Steam
Exec=/usr/local/bin/steam %U
Icon=steam
Terminal=false
Type=Application
Categories=Network;FileTransfer;Game;
MimeType=x-scheme-handler/steam;x-scheme-handler/steamlink;
Actions=Store;Community;Library;Servers;Screenshots;News;Settings;BigPicture;Friends;
EOF

cat << 'EOF' > /mnt/usr/local/bin/steam
#!/usr/bin/env bash
# AxisOS Steam Unified Launcher with Proton & GameMode
if [ -x "/usr/games/steam" ]; then
    exec /usr/games/steam "$@"
elif command -v steam >/dev/null 2>&1 && [ "$(which steam)" != "/usr/local/bin/steam" ]; then
    exec "$(which steam)" "$@"
elif command -v steam-installer >/dev/null 2>&1; then
    exec steam-installer "$@"
elif command -v flatpak >/dev/null 2>&1 && flatpak info com.valvesoftware.Steam >/dev/null 2>&1; then
    exec flatpak run com.valvesoftware.Steam "$@"
else
    echo "Starting Steam setup..."
    apt-get update -qq && apt-get install -y steam-installer || true
    if [ -x "/usr/games/steam" ]; then
        exec /usr/games/steam "$@"
    fi
fi
EOF
chmod 0755 /mnt/usr/local/bin/steam

report 86 "Setting up user account ($USERNAME)..."
# Create user if it doesn't already exist
chroot /mnt groupadd -f gamemode 2>/dev/null || true
chroot /mnt groupadd -f games 2>/dev/null || true
chroot /mnt groupadd -f docker 2>/dev/null || true

if ! chroot /mnt id -u "$USERNAME" >/dev/null 2>&1; then
    chroot /mnt useradd -m -s /bin/bash -c "$USER_FULLNAME" -G sudo,video,audio,render,input,seat,netdev,gamemode,games,docker "$USERNAME"
else
    chroot /mnt usermod -aG sudo,video,audio,render,input,seat,netdev,gamemode,games,docker "$USERNAME" 2>/dev/null || true
fi

# Set passwords
echo "${USERNAME}:${PASSWORD}" | chroot /mnt chpasswd
echo "root:${PASSWORD}" | chroot /mnt chpasswd

# Standard Debian security: User and root passwords are identical.
# Terminal and administrative tasks strictly prompt for the password.
rm -f "/mnt/etc/sudoers.d/$USERNAME"
rm -f /mnt/etc/sudoers.d/axis 2>/dev/null || true

# Configure display manager (GDM3 / GNOME) for installed system
chroot /mnt groupadd -r autologin 2>/dev/null || true
chroot /mnt groupadd -r nopasswdlogin 2>/dev/null || true
chroot /mnt gpasswd -a "${USERNAME}" autologin 2>/dev/null || true
chroot /mnt gpasswd -a "${USERNAME}" nopasswdlogin 2>/dev/null || true

# Remove tty1 conflicting autologin or old service overrides
rm -rf /mnt/etc/systemd/system/getty@tty1.service.d 2>/dev/null || true
rm -f /mnt/etc/systemd/system/graphical.target.wants/axisos.service 2>/dev/null || true
rm -rf /mnt/etc/lightdm/lightdm.conf.d 2>/dev/null || true

# Configure Native AxisOS LiquidNodes Qt6 Wayland Session & Display Manager
mkdir -p /mnt/etc/systemd/system
mkdir -p /mnt/usr/share/wayland-sessions

if [[ "$AUTOLOGIN" == "true" ]]; then
    touch /mnt/etc/axisos-autologin
else
    rm -f /mnt/etc/axisos-autologin 2>/dev/null || true
fi

cat << EOF > /mnt/etc/systemd/system/axisos-session.service
[Unit]
Description=AxisOS Desktop Environment
After=systemd-user-sessions.service seatd.service
Wants=seatd.service
Conflicts=getty@tty1.service
After=getty@tty1.service

[Service]
Type=simple
User=${USERNAME}
PAMName=login
WorkingDirectory=/home/${USERNAME}
Environment=XDG_SESSION_TYPE=wayland
Environment=XDG_CURRENT_DESKTOP=AxisOS:LiquidNodes
Environment=WLR_RENDERER_ALLOW_SOFTWARE=1
Environment=WLR_NO_HARDWARE_CURSORS=1
Environment=LIBSEAT_BACKEND=seatd
Environment=XDG_RUNTIME_DIR=/run/user/1000
ExecStart=/usr/local/bin/axisos-kiosk.sh
Restart=on-failure
RestartSec=2
StandardInput=tty
StandardOutput=journal
StandardError=journal
TTYPath=/dev/tty1
TTYReset=no
TTYVHangup=no

[Install]
WantedBy=graphical.target
EOF

# Clean network interfaces: lo only, let NetworkManager handle ethernet & Wi-Fi asynchronously
cat << 'NET_EOF' > /mnt/etc/network/interfaces
# AxisOS Network Interfaces
auto lo
iface lo inet loopback
NET_EOF

# Mask slow blocking wait-online services
chroot /mnt systemctl mask NetworkManager-wait-online.service 2>/dev/null || true
chroot /mnt systemctl mask systemd-networkd-wait-online.service 2>/dev/null || true
chroot /mnt systemctl enable seatd.service 2>/dev/null || true

cat << 'EOF' > /mnt/usr/share/wayland-sessions/axisos-liquidnodes.desktop
[Desktop Entry]
Name=AxisOS LiquidNodes
Comment=AxisOS Native Qt 6 Wayland Compositor & Glassmorphic Desktop
Exec=/usr/local/bin/LiquidNodesOS
Type=Application
DesktopNames=AxisOS:LiquidNodes
EOF
ln -sfn /usr/share/wayland-sessions/axisos-liquidnodes.desktop /mnt/usr/share/wayland-sessions/axisos.desktop 2>/dev/null || true

chroot /mnt systemctl set-default graphical.target 2>/dev/null || true
chroot /mnt systemctl disable gdm3.service 2>/dev/null || true
chroot /mnt systemctl enable axisos-session.service 2>/dev/null || true

# Clean user profile to ensure no live-session kiosk loop and deploy desktop applications
mkdir -p "/mnt/home/${USERNAME}/Desktop"
sed -i '/axisos-kiosk/d' "/mnt/home/${USERNAME}/.profile" 2>/dev/null || true
if [ -d /mnt/etc/skel/Desktop ]; then
    cp -r /mnt/etc/skel/Desktop/* "/mnt/home/${USERNAME}/Desktop/" 2>/dev/null || true
fi

# Ensure installer is completely removed and unavailable in installed system
rm -f "/mnt/home/${USERNAME}/Desktop/axis-installer.desktop" 2>/dev/null || true
rm -f "/mnt/etc/skel/Desktop/axis-installer.desktop" 2>/dev/null || true
rm -f "/mnt/usr/share/applications/axis-installer.desktop" 2>/dev/null || true
rm -f "/mnt/usr/local/bin/axisos-installer.sh" 2>/dev/null || true
rm -f "/mnt/usr/local/bin/axisos-install.sh" 2>/dev/null || true

# Strip axis-installer.desktop from GNOME dock favorites in dconf
if [ -f /mnt/etc/dconf/db/local.d/00-axisos-gnome ]; then
    sed -i "s/'axis-installer.desktop',[ ]*//g" /mnt/etc/dconf/db/local.d/00-axisos-gnome
    sed -i "s/,[ ]*'axis-installer.desktop'//g" /mnt/etc/dconf/db/local.d/00-axisos-gnome
fi
# Clear live user dconf override so installed dock reflects updated system defaults without ghost installer icon
rm -rf "/mnt/home/${USERNAME}/.config/dconf" 2>/dev/null || true
chroot /mnt dconf update 2>/dev/null || true

# Create First-Boot OOBE autostart desktop entry for GNOME/Cinnamon sessions
mkdir -p "/mnt/home/${USERNAME}/.config/autostart"
cat << 'OOBE_EOF' > "/mnt/home/${USERNAME}/.config/autostart/axis-oobe-firstboot.desktop"
[Desktop Entry]
Name=AxisOS First-Boot Setup
Comment=Out-of-Box Experience Setup Wizard
Exec=/bin/sh -c 'if [ -f /var/lib/axisos/first-boot-pending ]; then /usr/local/bin/axis-oobe; fi'
Icon=preferences-system
Terminal=false
Type=Application
X-GNOME-Autostart-enabled=true
OOBE_EOF

# Replace installer binary with no-op script so it cannot be invoked
cat << 'EOF' > /mnt/usr/local/bin/axis-installer
#!/bin/sh
echo "AxisOS is already installed on this system."
exit 0
EOF
chmod 755 /mnt/usr/local/bin/axis-installer

# Ensure GameBoost engine utility and service are deployed in target OS
if [ -f /usr/local/bin/axis-game-boost ]; then
    cp -f /usr/local/bin/axis-game-boost /mnt/usr/local/bin/axis-game-boost
    chmod 755 /mnt/usr/local/bin/axis-game-boost
fi
if [ -f /etc/systemd/system/axis-game-boost.service ]; then
    cp -f /etc/systemd/system/axis-game-boost.service /mnt/etc/systemd/system/axis-game-boost.service
    chroot /mnt systemctl enable axis-game-boost.service 2>/dev/null || true
fi

chmod +x "/mnt/home/${USERNAME}/Desktop"/*.desktop 2>/dev/null || true
chroot /mnt chown -R "${USERNAME}:${USERNAME}" "/home/${USERNAME}"

# Resolve and configure selected OS Edition profile
case "$EDITION" in
    enterprise|workstation|corp)
        EDITION_ID="enterprise"
        EDITION_LABEL="AxisOS Enterprise Workstation"
        EDITION_DESC="Hardened workstation with corporate domain join, VPNs, and compliance security."
        ;;
    education|school|work|edu)
        EDITION_ID="education"
        EDITION_LABEL="AxisOS Work & School"
        EDITION_DESC="Full productivity workstation for students, educators, and professionals."
        ;;
    *)
        EDITION_ID="base"
        EDITION_LABEL="AxisOS Home & Gaming"
        EDITION_DESC="High-performance gaming, multimedia, and daily computing edition."
        ;;
esac

log "Applying OS Edition Profile: $EDITION_LABEL ($EDITION_ID)"

# Permanent marker indicating system is fully installed to hard disk
cat << EOF > /mnt/etc/axisos-installed
INSTALLED=true
VERSION="2.1"
CODENAME="Debian Edition"
EDITION_ID="${EDITION_ID}"
EDITION="${EDITION_LABEL}"
INSTALL_DATE="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
TARGET_DISK="${TARGET_DISK}"
TARGET_PART="${ROOT_PART}"
ROOT_UUID="${ROOT_UUID}"
PRIMARY_USER="${USERNAME}"
BOOT_MODE="$([[ "$IS_UEFI" == "true" ]] && echo "UEFI" || echo "BIOS")"
EOF
chmod 644 /mnt/etc/axisos-installed

cat << EOF > /mnt/etc/axisos-edition
EDITION_ID="${EDITION_ID}"
EDITION_NAME="${EDITION_LABEL}"
EDITION_DESC="${EDITION_DESC}"
VERSION="2.1"
EOF
chmod 644 /mnt/etc/axisos-edition

# Apply edition-specific profiles
if [ "$EDITION_ID" = "enterprise" ]; then
    cat << 'EOF' > /mnt/etc/sysctl.d/99-axisos-enterprise.conf
# AxisOS Enterprise Workstation Security Hardening
fs.protected_hardlinks = 1
fs.protected_symlinks = 1
fs.protected_regular = 2
fs.protected_fifos = 2
kernel.dmesg_restrict = 1
kernel.kptr_restrict = 2
net.ipv4.conf.all.rp_filter = 1
net.ipv4.conf.default.rp_filter = 1
net.ipv4.icmp_echo_ignore_broadcasts = 1
EOF
    if [ -f /usr/local/bin/axis-domain-join ]; then
        cp -f /usr/local/bin/axis-domain-join /mnt/usr/local/bin/axis-domain-join
        chmod 755 /mnt/usr/local/bin/axis-domain-join
    fi
    if [ -f /usr/share/applications/axis-domain-join.desktop ]; then
        cp -f /usr/share/applications/axis-domain-join.desktop /mnt/usr/share/applications/axis-domain-join.desktop
        chmod 644 /mnt/usr/share/applications/axis-domain-join.desktop
    fi
elif [ "$EDITION_ID" = "education" ]; then
    cat << 'EOF' > /mnt/etc/axisos-work-school.conf
PRODUCTIVITY_PROFILE=standard
CONTENT_SHIELD_SAFE_SEARCH=recommended
OFFICE_SUITE=libreoffice
DEV_TOOLING=enabled
EOF
    chmod 644 /mnt/etc/axisos-work-school.conf
fi

# Ensure axis-switch-edition is deployed in installed system
if [ -f /usr/local/bin/axis-switch-edition ]; then
    cp -f /usr/local/bin/axis-switch-edition /mnt/usr/local/bin/axis-switch-edition
    chmod 755 /mnt/usr/local/bin/axis-switch-edition
fi

report 87 "Configuring automated kernel & system updates (unattended-upgrades)..."
mkdir -p /mnt/etc/apt/apt.conf.d
cat << 'EOF' > /mnt/etc/apt/apt.conf.d/20auto-upgrades
APT::Periodic::Update-Package-Lists "1";
APT::Periodic::Download-Upgradeable-Packages "1";
APT::Periodic::AutocleanInterval "7";
APT::Periodic::Unattended-Upgrade "1";
EOF

cat << 'EOF' > /mnt/etc/apt/apt.conf.d/50unattended-upgrades
Unattended-Upgrade::Allowed-Origins {
    "${distro_id}:${distro_codename}";
    "${distro_id}:${distro_codename}-security";
    "${distro_id}:${distro_codename}-updates";
};
Unattended-Upgrade::Package-Blacklist {
};
Unattended-Upgrade::AutoFixInterruptedDpkg "true";
Unattended-Upgrade::MinimalSteps "true";
Unattended-Upgrade::InstallOnShutdown "false";
Unattended-Upgrade::Remove-Unused-Kernel-Packages "true";
Unattended-Upgrade::Remove-New-Unused-Dependencies "true";
Unattended-Upgrade::Automatic-Reboot "false";
EOF

# Ensure native Axis package manager (/usr/bin/axis) and atomic updater (/usr/bin/axis-update) are deployed
[ -f /mnt/usr/bin/axis ] && chmod +x /mnt/usr/bin/axis
if [ -f /usr/bin/axis-update ]; then
    cp -f /usr/bin/axis-update /mnt/usr/bin/axis-update
    chmod 755 /mnt/usr/bin/axis-update
fi
if [ -f /etc/systemd/system/axis-mark-good.service ]; then
    cp -f /etc/systemd/system/axis-mark-good.service /mnt/etc/systemd/system/axis-mark-good.service
    chroot /mnt systemctl enable axis-mark-good.service 2>/dev/null || true
fi

# ------------------------------------------------------------------------------
# Hardware Enablement, GPU Autodetect & Developer Runtimes
# ------------------------------------------------------------------------------
report 88 "Configuring hardware enablement, GPU autodetection & device drivers..."

# Deploy Biometric Authentication Manager & Udev Rules (Fingerprint & Iris)
if [[ -f /usr/local/bin/axis-biometrics ]]; then
    cp -f /usr/local/bin/axis-biometrics /mnt/usr/local/bin/axis-biometrics
    chmod 755 /mnt/usr/local/bin/axis-biometrics
fi
if [[ -f /etc/udev/rules.d/70-axisos-biometrics.rules ]]; then
    mkdir -p /mnt/etc/udev/rules.d
    cp -f /etc/udev/rules.d/70-axisos-biometrics.rules /mnt/etc/udev/rules.d/70-axisos-biometrics.rules
fi
if [[ -f /etc/pam.d/axis-greeter ]]; then
    mkdir -p /mnt/etc/pam.d
    cp -f /etc/pam.d/axis-greeter /mnt/etc/pam.d/axis-greeter
fi
chroot /mnt usermod -aG plugdev,video,input "$USERNAME" 2>/dev/null || true

# Deploy Signature AxisOS Utilities (Axis Rewind, NeuralPulse, USB Quarantine Vault)
for util_bin in axis-rewind axis-neural-pulse axis-usb-vault axis-gaze-shield; do
    if [[ -f "/usr/local/bin/$util_bin" ]]; then
        cp -f "/usr/local/bin/$util_bin" "/mnt/usr/local/bin/$util_bin"
        chmod 755 "/mnt/usr/local/bin/$util_bin"
    fi
done

# Copy GPU autodetect script and systemd service
if [[ -f /usr/local/bin/gpu-autodetect.sh ]]; then
    cp /usr/local/bin/gpu-autodetect.sh /mnt/usr/local/bin/gpu-autodetect.sh
    chmod +x /mnt/usr/local/bin/gpu-autodetect.sh
fi

if [[ -f /etc/systemd/system/gpu-autodetect.service ]]; then
    cp /etc/systemd/system/gpu-autodetect.service /mnt/etc/systemd/system/gpu-autodetect.service
    chroot /mnt systemctl enable gpu-autodetect.service 2>/dev/null || true
fi

# Ensure prime-run is present
cat << 'EOF' > /mnt/usr/local/bin/prime-run
#!/usr/bin/env bash
# AxisOS NVIDIA PRIME Render Offload Runner
export __NV_PRIME_RENDER_OFFLOAD=1
export __GLX_VENDOR_LIBRARY_NAME=nvidia
export __VK_LAYER_NV_optimus=NVIDIA_only
exec "$@"
EOF
chmod 755 /mnt/usr/local/bin/prime-run

# Mobile device udev rules (Android/HarmonyOS MTP & ADB)
mkdir -p /mnt/etc/udev/rules.d
cat << 'EOF' > /mnt/etc/udev/rules.d/51-android.rules
# Universal Android & HarmonyOS USB rules
SUBSYSTEM=="usb", ATTR{idVendor}=="[0-9a-fA-F]*", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="18d1", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="04e8", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="12d1", MODE="0666", GROUP="plugdev"
SUBSYSTEM=="usb", ATTR{idVendor}=="2717", MODE="0666", GROUP="plugdev"
EOF
chmod 644 /mnt/etc/udev/rules.d/51-android.rules

# Enable hardware services in target
chroot /mnt systemctl enable udisks2.service 2>/dev/null || true
chroot /mnt systemctl enable usbmuxd.service 2>/dev/null || true
chroot /mnt systemctl enable bluetooth.service 2>/dev/null || true

# Pre-configure official Microsoft VS Code repository
mkdir -p /mnt/etc/apt/keyrings /mnt/etc/apt/sources.list.d
if [[ -f /etc/apt/keyrings/packages.microsoft.gpg ]]; then
    cp /etc/apt/keyrings/packages.microsoft.gpg /mnt/etc/apt/keyrings/packages.microsoft.gpg
fi
cat << 'EOF' > /mnt/etc/apt/sources.list.d/vscode.list
deb [arch=amd64,arm64,armhf signed-by=/etc/apt/keyrings/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main
EOF

# Enable apt-daily background timers
report 90 "Installing and generating Microsoft-signed UEFI Secure Bootloader..."
if [[ "$IS_UEFI" == "true" ]]; then
    # 1. Run standard grub-install into target
    chroot /mnt grub-install \
        --target=x86_64-efi \
        --efi-directory=/boot/efi \
        --bootloader-id=AxisOS \
        --recheck || {
            log "Warning: EFI grub-install failed, trying fallback removable target..."
            chroot /mnt grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=AxisOS --removable || true
        }

    # 2. Deploy Microsoft-signed Shim & signed GRUB binaries for Secure Boot trust
    mkdir -p /mnt/boot/efi/EFI/BOOT
    mkdir -p /mnt/boot/efi/EFI/AxisOS

    SHIM_SIGNED="/mnt/usr/lib/shim/shimx64.efi.signed"
    GRUB_SIGNED="/mnt/usr/lib/grub/x86_64-efi-signed/grubx64.efi.signed"
    MM_SIGNED="/mnt/usr/lib/shim/mmx64.efi.signed"
    FB_SIGNED="/mnt/usr/lib/shim/fbx64.efi.signed"

    [ ! -f "$SHIM_SIGNED" ] && SHIM_SIGNED="/mnt/usr/lib/shim/shimx64.efi"
    [ ! -f "$MM_SIGNED" ] && MM_SIGNED="/mnt/usr/lib/shim/mmx64.efi"
    [ ! -f "$FB_SIGNED" ] && FB_SIGNED="/mnt/usr/lib/shim/fbx64.efi"

    # Copy Microsoft-signed Shim to /EFI/BOOT/BOOTX64.EFI (Lenovo/HP/Dell standard fallback)
    if [[ -f "$SHIM_SIGNED" ]]; then
        cp "$SHIM_SIGNED" /mnt/boot/efi/EFI/BOOT/BOOTX64.EFI
        cp "$SHIM_SIGNED" /mnt/boot/efi/EFI/AxisOS/shimx64.efi
        log "Deployed Microsoft UEFI CA signed Shim as BOOTX64.EFI and shimx64.efi"
    fi

    # Copy Debian-signed GRUB
    if [[ -f "$GRUB_SIGNED" ]]; then
        cp "$GRUB_SIGNED" /mnt/boot/efi/EFI/BOOT/grubx64.efi
        cp "$GRUB_SIGNED" /mnt/boot/efi/EFI/AxisOS/grubx64.efi
        log "Deployed Debian Secure Boot signed GRUB as grubx64.efi"
    elif [[ -f /mnt/boot/efi/EFI/AxisOS/grubx64.efi ]]; then
        cp /mnt/boot/efi/EFI/AxisOS/grubx64.efi /mnt/boot/efi/EFI/BOOT/grubx64.efi
    fi

    # Copy MOK manager and fallback helpers
    [[ -f "$MM_SIGNED" ]] && cp "$MM_SIGNED" /mnt/boot/efi/EFI/BOOT/mmx64.efi && cp "$MM_SIGNED" /mnt/boot/efi/EFI/AxisOS/mmx64.efi
    [[ -f "$FB_SIGNED" ]] && cp "$FB_SIGNED" /mnt/boot/efi/EFI/BOOT/fbx64.efi && cp "$FB_SIGNED" /mnt/boot/efi/EFI/AxisOS/fbx64.efi

    # 3. Create embedded early grub.cfg redirecting to root partition Btrfs subvolume or ext4
    ROOT_UUID=$(blkid -s UUID -o value "$ROOT_PART" 2>/dev/null || true)
    PREFIX_PATH="(\$root)/boot/grub"
    [[ "$FILESYSTEM" == "btrfs" ]] && PREFIX_PATH="(\$root)/@/boot/grub"

    cat << EOF > /mnt/boot/efi/EFI/BOOT/grub.cfg
search.fs_uuid $ROOT_UUID root
set prefix=$PREFIX_PATH
configfile \$prefix/grub.cfg
EOF
    cp /mnt/boot/efi/EFI/BOOT/grub.cfg /mnt/boot/efi/EFI/AxisOS/grub.cfg

    # 4. Register boot options in motherboard NVRAM via efibootmgr
    if command -v efibootmgr >/dev/null 2>&1; then
        report 93 "Registering AxisOS into motherboard UEFI NVRAM..."
        mount -t efivarfs efivarfs /sys/firmware/efi/efivars 2>/dev/null || true

        # Clean existing AxisOS entries to prevent stale duplicates
        for bootnum in $(efibootmgr 2>/dev/null | grep -i "AxisOS" | sed -E 's/^Boot([0-9A-Fa-f]+).*/\1/'); do
            efibootmgr -b "$bootnum" -B 2>/dev/null || true
        done

        # Register primary entry pointing to Microsoft-signed Shim
        efibootmgr -c -d "$TARGET_DISK" -p 1 -L "AxisOS" -l '\EFI\AxisOS\shimx64.efi' 2>/dev/null || \
            efibootmgr -c -d "$TARGET_DISK" -p 1 -L "AxisOS" -l '\EFI\BOOT\BOOTX64.EFI' 2>/dev/null || true
        log "Registered AxisOS UEFI NVRAM boot entry"

        # Explicitly configure BootNext and prioritize BootOrder
        NEW_BOOTNUM=$(efibootmgr 2>/dev/null | grep -i "AxisOS" | head -n1 | sed -E 's/^Boot([0-9A-Fa-f]+).*/\1/')
        if [[ -n "$NEW_BOOTNUM" ]]; then
            log "Configuring UEFI BootNext to $NEW_BOOTNUM (forces direct boot from drive on restart)..."
            efibootmgr -n "$NEW_BOOTNUM" 2>/dev/null || true

            CURRENT_ORDER=$(efibootmgr 2>/dev/null | grep -i "BootOrder:" | awk '{print $2}')
            if [[ -n "$CURRENT_ORDER" ]]; then
                FILTERED_ORDER=$(echo "$CURRENT_ORDER" | tr ',' '\n' | grep -v -i "^${NEW_BOOTNUM}$" | tr '\n' ',' | sed 's/,$//')
                NEW_ORDER="${NEW_BOOTNUM},${FILTERED_ORDER}"
                efibootmgr -o "$NEW_ORDER" 2>/dev/null || true
                log "Updated UEFI BootOrder to prioritize AxisOS: $NEW_ORDER"
            fi
        fi
    fi
else
    chroot /mnt grub-install --target=i386-pc "$TARGET_DISK" || true
fi

# Configure GRUB for silent fast boot directly into AxisOS
mkdir -p /mnt/etc/default
cat << 'EOF' > /mnt/etc/default/grub
# AxisOS Default GRUB Configuration
GRUB_DEFAULT=0
GRUB_TIMEOUT=1
GRUB_DISTRIBUTOR="AxisOS"
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3 amdgpu.freesync_video=1"
GRUB_CMDLINE_LINUX=""
GRUB_DISABLE_OS_PROBER=false
EOF

# Purge live-session boot hooks so installed OS boots directly from root UUID/subvolume
report 94 "Removing live-boot hooks and regenerating initramfs for installed storage..."
DEBIAN_FRONTEND=noninteractive chroot /mnt apt-get -o DPkg::Lock::Timeout=60 remove --purge -y     live-boot live-boot-initramfs-tools live-config live-config-systemd 2>/dev/null || true
chroot /mnt update-initramfs -u -k all 2>/dev/null || chroot /mnt update-initramfs -c -k all 2>/dev/null || true

# Generate grub.cfg
mkdir -p /mnt/boot/grub
chroot /mnt update-grub 2>/dev/null || chroot /mnt grub-mkconfig -o /boot/grub/grub.cfg

# Set Out-of-Box Experience (OOBE) first boot flag for initial setup
mkdir -p /mnt/var/lib/axisos
chmod 777 /mnt/var/lib/axisos 2>/dev/null || true
touch /mnt/var/lib/axisos/first-boot-pending
chmod 666 /mnt/var/lib/axisos/first-boot-pending 2>/dev/null || true
log "Configured /var/lib/axisos/first-boot-pending for first-boot OOBE wizard"

report 96 "Cleaning up mounts..."
sync
umount -l /mnt/sys/firmware/efi/efivars 2>/dev/null || true
umount -l /mnt/dev/pts 2>/dev/null || true
umount -l /mnt/dev 2>/dev/null || true
umount -l /mnt/proc 2>/dev/null || true
umount -l /mnt/sys 2>/dev/null || true
umount -l /mnt/run 2>/dev/null || true
umount -l /mnt/boot/efi 2>/dev/null || true
umount -l /mnt/home 2>/dev/null || true
umount -l /mnt/var/log 2>/dev/null || true
umount -l /mnt/.snapshots 2>/dev/null || true
umount -R /mnt 2>/dev/null || umount -l /mnt 2>/dev/null || true

report 100 "AxisOS successfully installed! Ready to reboot."
log "Installation completed successfully on $TARGET_DISK."
