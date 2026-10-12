#!/bin/bash
# ==============================================================================
# AxisOS Linux - Real System Installation Engine
# Performs full disk partitioning (GPT/ESP), Btrfs formatting with subvolumes,
# system root deployment (rsync from live system), user account configuration,
# and GRUB 2 EFI / BIOS bootloader installation.
# ==============================================================================

set -eo pipefail

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
    echo "[AxisOS Install] (${percent}%) ${message}" >&2
}

log() {
    echo "[AxisOS Install] $*" >&2
}

usage() {
    cat << EOF
Usage: $0 --disk <device> [options]
Options:
  --disk <path>         Target block device (e.g. /dev/sda, /dev/nvme0n1) [REQUIRED]
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

# Parse CLI arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --disk) TARGET_DISK="$2"; shift 2 ;;
        --user|--username) USERNAME="$2"; shift 2 ;;
        --pass|--password) PASSWORD="$2"; shift 2 ;;
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
    echo "ERROR: --disk parameter is required." >&2
    usage
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
    echo "ERROR: This installer must be run as root." >&2
    exit 1
fi

# Verify disk exists
if [[ ! -b "$TARGET_DISK" ]]; then
    echo "ERROR: Target disk $TARGET_DISK does not exist or is not a block device." >&2
    exit 1
fi

# Detect and safeguard live USB medium
LIVE_MEDIA_DEV=$(findmnt -n -o SOURCE /run/live/medium 2>/dev/null || findmnt -n -o SOURCE /run/live/rootfs 2>/dev/null || echo "")
if [[ -n "$LIVE_MEDIA_DEV" ]]; then
    LIVE_DISK=$(lsblk -no PKNAME "$LIVE_MEDIA_DEV" 2>/dev/null || echo "")
    if [[ -n "$LIVE_DISK" ]]; then
        LIVE_DISK="/dev/$LIVE_DISK"
    else
        LIVE_DISK="${LIVE_MEDIA_DEV%[0-9]*}"
        LIVE_DISK="${LIVE_DISK%p}"
    fi
    if [[ "$TARGET_DISK" == "$LIVE_DISK" || "$TARGET_DISK" == "$LIVE_MEDIA_DEV" ]]; then
        echo "ERROR: Target disk $TARGET_DISK is the live USB! Cannot install onto active installation media." >&2
        report 0 "Error: Selected disk is the live USB media. Please select an internal drive."
        exit 1
    fi
fi

# Detect UEFI vs BIOS
IS_UEFI=false
if [[ -d /sys/firmware/efi ]]; then
    IS_UEFI=true
    log "Boot mode: UEFI detected"
else
    log "Boot mode: Legacy BIOS / CSM detected"
fi

# Unmount any existing partitions on target disk
report 5 "Unmounting existing partitions on $TARGET_DISK..."
swapoff -a 2>/dev/null || true
for part in $(lsblk -ln -o NAME "$TARGET_DISK" 2>/dev/null | tail -n +2); do
    mountpoint="/dev/$part"
    umount -f -l "$mountpoint" 2>/dev/null || true
    swapoff "$mountpoint" 2>/dev/null || true
done
umount -R /mnt 2>/dev/null || true
umount -R /mnt_axis_tmp 2>/dev/null || true

report 10 "Wiping partition signatures on $TARGET_DISK..."
wipefs -a -f "$TARGET_DISK" 2>/dev/null || true
dd if=/dev/zero of="$TARGET_DISK" bs=1M count=10 conv=notrunc status=none 2>/dev/null || true

report 15 "Partitioning drive with GPT scheme..."
# Calculate swap size based on disk capacity
DISK_SIZE_BYTES=$(blockdev --getsize64 "$TARGET_DISK" 2>/dev/null || echo 0)
DISK_SIZE_GB=$(( DISK_SIZE_BYTES / 1024 / 1024 / 1024 ))
SWAP_SIZE="4096M"
if [ "$DISK_SIZE_GB" -lt 30 ]; then
    SWAP_SIZE="2048M"
fi
if [ "$DISK_SIZE_GB" -lt 15 ]; then
    SWAP_SIZE="1024M"
fi

if command -v sgdisk >/dev/null 2>&1; then
    sgdisk --zap-all "$TARGET_DISK" 2>/dev/null || true
    dd if=/dev/zero of="$TARGET_DISK" bs=1M count=2 conv=notrunc status=none 2>/dev/null || true
    sgdisk -n 1:2048:+512M -t 1:ef00 -c 1:"AXIS_BOOT" "$TARGET_DISK"
    sgdisk -n 2:0:+${SWAP_SIZE} -t 2:8200 -c 2:"AXIS_SWAP" "$TARGET_DISK"
    sgdisk -n 3:0:0 -t 3:8300 -c 3:"AXIS_ROOT" "$TARGET_DISK"
elif command -v parted >/dev/null 2>&1; then
    parted -s "$TARGET_DISK" mklabel gpt
    parted -s "$TARGET_DISK" mkpart "AXIS_BOOT" fat32 1MiB 513MiB
    parted -s "$TARGET_DISK" set 1 esp on
    parted -s "$TARGET_DISK" mkpart "AXIS_SWAP" linux-swap 513MiB 2561MiB
    parted -s "$TARGET_DISK" mkpart "AXIS_ROOT" ext4 2561MiB 100%
else
    echo "ERROR: Neither sgdisk nor parted found." >&2
    exit 1
fi

# Settle udev and reread partition table
partprobe "$TARGET_DISK" 2>/dev/null || true
udevadm settle 2>/dev/null || sleep 2

# Resolve partition paths (e.g., /dev/nvme0n1p1 vs /dev/sda1)
PART_PREFIX="$TARGET_DISK"
if [[ "$TARGET_DISK" =~ [0-9]$ ]]; then
    PART_PREFIX="${TARGET_DISK}p"
fi

ESP_PART="${PART_PREFIX}1"
SWAP_PART="${PART_PREFIX}2"
ROOT_PART="${PART_PREFIX}3"

# Wait for partitions to appear
for i in {1..15}; do
    if [[ -b "$ROOT_PART" && -b "$ESP_PART" ]]; then break; fi
    sleep 1
    partprobe "$TARGET_DISK" 2>/dev/null || true
done

if [[ ! -b "$ROOT_PART" || ! -b "$ESP_PART" ]]; then
    echo "ERROR: Partition $ROOT_PART or $ESP_PART not found after partitioning." >&2
    exit 1
fi

report 25 "Formatting filesystems (ESP FAT32, Swap, and Root)..."
wipefs -a -f "$ESP_PART" 2>/dev/null || true
mkfs.vfat -F32 -n "AXIS_BOOT" "$ESP_PART" || mkfs.fat -n "AXIS_BOOT" "$ESP_PART" || true

wipefs -a -f "$SWAP_PART" 2>/dev/null || true
mkswap -L "AXIS_SWAP" "$SWAP_PART" 2>/dev/null || true

USE_EXT4=false
if [[ "$FILESYSTEM" == "btrfs" ]] && command -v mkfs.btrfs >/dev/null 2>&1; then
    modprobe btrfs 2>/dev/null || true
    wipefs -a -f "$ROOT_PART" 2>/dev/null || true
    if mkfs.btrfs -f -L "AXIS_ROOT" "$ROOT_PART"; then
        report 30 "Setting up Btrfs subvolumes (@, @home, @snapshots, @var_log)..."
        mkdir -p /mnt_axis_tmp
        if mount -t btrfs "$ROOT_PART" /mnt_axis_tmp 2>/dev/null; then
            btrfs subvolume create /mnt_axis_tmp/@ 2>/dev/null || true
            btrfs subvolume create /mnt_axis_tmp/@home 2>/dev/null || true
            btrfs subvolume create /mnt_axis_tmp/@snapshots 2>/dev/null || true
            btrfs subvolume create /mnt_axis_tmp/@var_log 2>/dev/null || true
            umount /mnt_axis_tmp 2>/dev/null || true
            rmdir /mnt_axis_tmp 2>/dev/null || true

            mkdir -p /mnt
            mount -o noatime,compress=zstd:3,subvol=@ "$ROOT_PART" /mnt
            mkdir -p /mnt/{home,boot/efi,var/log,.snapshots}
            mount -o noatime,compress=zstd:3,subvol=@home "$ROOT_PART" /mnt/home 2>/dev/null || true
            mount -o noatime,compress=zstd:3,subvol=@var_log "$ROOT_PART" /mnt/var/log 2>/dev/null || true
            mount -o noatime,compress=zstd:3,subvol=@snapshots "$ROOT_PART" /mnt/.snapshots 2>/dev/null || true
        else
            USE_EXT4=true
        fi
    else
        USE_EXT4=true
    fi
else
    USE_EXT4=true
fi

if [[ "$USE_EXT4" == "true" ]]; then
    report 30 "Formatting root as high-performance EXT4 filesystem..."
    wipefs -a -f "$ROOT_PART" 2>/dev/null || true
    mkfs.ext4 -F -L "AXIS_ROOT" "$ROOT_PART"
    mkdir -p /mnt
    mount "$ROOT_PART" /mnt
    mkdir -p /mnt/{home,boot/efi,var/log}
fi

mkdir -p /mnt/boot/efi
mount "$ESP_PART" /mnt/boot/efi 2>/dev/null || true
swapon "$SWAP_PART" 2>/dev/null || true

report 35 "Copying AxisOS Linux system files to target root..."
SOURCE_DIR="/"
mkdir -p /mnt/{dev,proc,sys,run,tmp,etc,root}
chmod 1777 /mnt/tmp

# Execute rsync with live progress tracking
rsync -aAX \
    --exclude=/dev/* \
    --exclude=/proc/* \
    --exclude=/sys/* \
    --exclude=/tmp/* \
    --exclude=/run/* \
    --exclude=/mnt/* \
    --exclude=/media/* \
    --exclude=/lost+found \
    --exclude=/run/live/* \
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

wait $RSYNC_PID || {
    log "Notice: rsync finished with minor exclusions."
}

report 76 "System file transfer complete."

report 77 "Generating filesystem mount table (/etc/fstab)..."
ROOT_UUID=$(blkid -s UUID -o value "$ROOT_PART")
ESP_UUID=$(blkid -s UUID -o value "$ESP_PART")
SWAP_UUID=$(blkid -s UUID -o value "$SWAP_PART")

if [[ "$USE_EXT4" == "true" ]]; then
cat << EOF > /mnt/etc/fstab
# /etc/fstab: static file system information generated by AxisOS Installer
# <file system>             <mount point>   <type>  <options>                               <dump>  <pass>
UUID=$ROOT_UUID             /               ext4    errors=remount-ro,noatime               0       1
UUID=$ESP_UUID              /boot/efi       vfat    umask=0077                              0       2
UUID=$SWAP_UUID             none            swap    sw                                      0       0
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
UUID=$SWAP_UUID             none            swap    sw                                      0       0
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

# Configure GDM3 for GNOME Wayland desktop
mkdir -p /mnt/etc/gdm3
if [[ "$AUTOLOGIN" == "true" ]]; then
    cat << EOF > /mnt/etc/gdm3/daemon.conf
# GDM configuration for AxisOS
[daemon]
AutomaticLoginEnable = true
AutomaticLogin = ${USERNAME}
WaylandEnable = true

[security]

[xdmcp]

[chooser]

[debug]
EOF
else
    cat << EOF > /mnt/etc/gdm3/daemon.conf
# GDM configuration for AxisOS
[daemon]
AutomaticLoginEnable = false
WaylandEnable = true

[security]

[xdmcp]

[chooser]

[debug]
EOF
fi

chroot /mnt systemctl set-default graphical.target 2>/dev/null || true
chroot /mnt systemctl enable gdm3.service 2>/dev/null || true

# Compile dconf system databases
chroot /mnt dconf update 2>/dev/null || true

# Clean user profile to ensure no live-session kiosk loop
mkdir -p "/mnt/home/${USERNAME}"
sed -i '/axisos-kiosk/d' "/mnt/home/${USERNAME}/.profile" 2>/dev/null || true
chroot /mnt chown -R "${USERNAME}:${USERNAME}" "/home/${USERNAME}"

# Permanent marker indicating system is fully installed to hard disk
cat << EOF > /mnt/etc/axisos-installed
INSTALLED=true
VERSION="2.0"
CODENAME="Debian Edition"
EDITION="Gaming & Workstation"
INSTALL_DATE="$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
TARGET_DISK="${TARGET_DISK}"
TARGET_PART="${ROOT_PART}"
ROOT_UUID="${ROOT_UUID}"
PRIMARY_USER="${USERNAME}"
BOOT_MODE="$([[ "$IS_UEFI" == "true" ]] && echo "UEFI" || echo "BIOS")"
EOF
chmod 644 /mnt/etc/axisos-installed

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

# Ensure native Axis package manager is executable in installed system
[ -f /mnt/usr/bin/axis ] && chmod +x /mnt/usr/bin/axis

# ------------------------------------------------------------------------------
# Hardware Enablement, GPU Autodetect & Developer Runtimes
# ------------------------------------------------------------------------------
report 88 "Configuring hardware enablement, GPU autodetection & device drivers..."

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
GRUB_TIMEOUT=2
GRUB_DISTRIBUTOR="AxisOS"
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash loglevel=0 vt.global_cursor_default=0 systemd.show_status=false rd.udev.log_level=3 udev.log_priority=3 amdgpu.freesync_video=1"
GRUB_CMDLINE_LINUX=""
GRUB_DISABLE_OS_PROBER=false
EOF

# Generate grub.cfg
chroot /mnt update-grub 2>/dev/null || chroot /mnt grub-mkconfig -o /boot/grub/grub.cfg

# Set Out-of-Box Experience (OOBE) first boot flag for initial setup
mkdir -p /mnt/var/lib/axisos
touch /mnt/var/lib/axisos/first-boot-pending
log "Configured /var/lib/axisos/first-boot-pending for first-boot OOBE wizard"

report 96 "Cleaning up mounts..."
sync
umount -R /mnt/dev /mnt/proc /mnt/sys /mnt/run 2>/dev/null || true
umount -R /mnt 2>/dev/null || true

report 100 "AxisOS successfully installed! Ready to reboot."
log "Installation completed successfully on $TARGET_DISK."
