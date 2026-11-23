#!/usr/bin/env bash
# ==============================================================================
# AxisOS Install-Once Bi-Monthly OTA Update & Remote Repair Engine
# Repository: https://github.com/jackyphuti/AxisOS
# Wrapped in main() so self-updates NEVER corrupt bash's file read offset!
# ==============================================================================

main() {
    set -u

    local REPO_OWNER="jackyphuti"
    local REPO_NAME="AxisOS"
    local BRANCH="main"
    local TARBALL_URL="https://github.com/${REPO_OWNER}/${REPO_NAME}/archive/refs/heads/${BRANCH}.tar.gz"
    local LOG_FILE="/var/log/axisos-ota.log"
    local STATE_FILE="/etc/axisos-ota-version"
    local LAYOUT_REV_FILE="/etc/axisos-layout-rev"
    local TARGET_LAYOUT_REV="obsidian-v2"

    local AUTO_MODE=0
    if [ "${1:-}" = "--auto" ]; then
        AUTO_MODE=1
    fi

    if [ "$(id -u)" -ne 0 ]; then
        echo "[AxisOS OTA] Please run with sudo: sudo $0"
        exit 1
    fi

    mkdir -p "$(dirname "$LOG_FILE")"
    if [ "${AXISOS_OTA_REEXEC:-0}" != "1" ]; then
        exec > >(tee -a "$LOG_FILE") 2>&1
    fi

    echo "======================================================================"
    echo "  AxisOS OTA System Update & Repair ($(date -u '+%Y-%m-%d %H:%M:%S UTC'))"
    echo "======================================================================"

    # 1. Verify connectivity
    if ! curl -fsI --connect-timeout 10 "https://github.com" >/dev/null 2>&1; then
        echo "[AxisOS OTA] No internet connection detected. Skipping OTA sync."
        exit 0
    fi

    # 2. Download latest AxisOS main archive
    local WORK_DIR
    WORK_DIR="$(mktemp -d /tmp/axisos-ota.XXXXXX)"
    trap "rm -rf '${WORK_DIR}'" EXIT

    echo "[1/8] Fetching latest AxisOS release channel (${REPO_OWNER}/${REPO_NAME}@${BRANCH})..."
    if ! curl -fsSL --retry 3 --connect-timeout 15 "$TARBALL_URL" -o "$WORK_DIR/axisos.tar.gz"; then
        echo "[AxisOS OTA] ERROR: Failed to download update archive from $TARBALL_URL"
        exit 1
    fi

    tar -xzf "$WORK_DIR/axisos.tar.gz" -C "$WORK_DIR"
    local SRC_DIR
    SRC_DIR="$(find "$WORK_DIR" -maxdepth 1 -mindepth 1 -type d | head -n1)"
    if [ -z "$SRC_DIR" ] || [ ! -d "$SRC_DIR/os-build" ]; then
        echo "[AxisOS OTA] ERROR: Extracted archive is missing os-build/"
        exit 1
    fi

    # If invoked as the installed OTA updater and a newer OTA engine was fetched,
    # atomically update and re-exec immediately so the latest repairs run on the first pass.
    case "$0" in
        /usr/local/bin/axisos-ota-update|/usr/local/bin/axisos-update)
            if [ "${AXISOS_OTA_REEXEC:-0}" != "1" ] && [ -f "$SRC_DIR/scripts/axisos-ota-update.sh" ] && ! cmp -s "$SRC_DIR/scripts/axisos-ota-update.sh" /usr/local/bin/axisos-ota-update 2>/dev/null; then
                echo "    -> Updating AxisOS OTA engine to latest release and continuing..."
                install -m 0755 "$SRC_DIR/scripts/axisos-ota-update.sh" /usr/local/bin/axisos-ota-update.new
                mv -f /usr/local/bin/axisos-ota-update.new /usr/local/bin/axisos-ota-update
                ln -sf /usr/local/bin/axisos-ota-update /usr/local/bin/axisos-update
                rm -rf "$WORK_DIR"
                export AXISOS_OTA_REEXEC=1
                exec /usr/local/bin/axisos-ota-update "$@"
            fi
            ;;
    esac

    # 3. Repair SDDM login freeze & session defaults
    echo "[2/8] Ensuring clean SDDM Plasma session defaults..."
    rm -f /usr/share/wayland-sessions/axisos-gamemode.desktop \
          /usr/share/wayland-sessions/liquidnodes.desktop \
          /usr/share/xsessions/axisos-gamemode.desktop \
          /usr/share/xsessions/liquidnodes.desktop 2>/dev/null || true

    systemctl disable --now seatd.service 2>/dev/null || true
    systemctl mask axisos.service 2>/dev/null || true
    systemctl enable sddm.service 2>/dev/null || true
    systemctl set-default graphical.target 2>/dev/null || true

    rm -f /etc/sddm.conf.d/autologin.conf /etc/sddm.conf.d/sddm.conf 2>/dev/null || true

    local EXISTING_AUTO_USER=""
    if [ -f /etc/sddm.conf ]; then
        EXISTING_AUTO_USER="$(awk -F= '/^User=/{print $2; exit}' /etc/sddm.conf 2>/dev/null || true)"
        if [ "$EXISTING_AUTO_USER" = "axis" ] && [ -f /etc/axisos-installed ]; then
            EXISTING_AUTO_USER=""
        fi
    fi

    cat << 'SDDM_EOF' > /etc/sddm.conf
[Theme]
Current=breeze
CursorTheme=breeze_cursors
Font=IBM Plex Sans,10,-1,5,50,0,0,0,0,0

[General]
InputMethod=
Numlock=none

[Wayland]
SessionDir=/usr/share/wayland-sessions

[X11]
SessionDir=/usr/share/xsessions
SDDM_EOF

    if [ -n "$EXISTING_AUTO_USER" ] && id "$EXISTING_AUTO_USER" >/dev/null 2>&1; then
        cat << AUTO_EOF >> /etc/sddm.conf

[Autologin]
User=${EXISTING_AUTO_USER}
Session=plasma.desktop
Relogin=false
AUTO_EOF
    fi

    mkdir -p /var/lib/sddm
    local LAST_USER
    LAST_USER="$(ls -1 /home 2>/dev/null | grep -v '^axis$' | head -n1 || true)"
    cat << STATE_EOF > /var/lib/sddm/state.conf
[Last]
Session=/usr/share/xsessions/plasma.desktop
User=${LAST_USER}
STATE_EOF
    chown -R sddm:sddm /var/lib/sddm 2>/dev/null || true

    mkdir -p /usr/share/sddm/themes/breeze /etc/sddm.conf.d
    if [ -f "$SRC_DIR/os-build/configs/sddm/theme.conf.user" ]; then
        cp -f "$SRC_DIR/os-build/configs/sddm/theme.conf.user" /usr/share/sddm/themes/breeze/theme.conf.user
        cp -f "$SRC_DIR/os-build/configs/sddm/theme.conf.user" /etc/sddm.conf.d/theme.conf.user
        rm -f /usr/share/sddm/themes/breeze/theme.conf.use /etc/sddm.conf.d/theme.conf.use 2>/dev/null || true
    fi

    # 4. Sync AxisOS Obsidian KDE Plasma Theme, Global Theme, Colors & Layout
    echo "[3/8] Updating AxisOS Obsidian KDE Plasma UI & themes..."
    mkdir -p /usr/share/color-schemes /usr/share/plasma/desktoptheme /usr/share/plasma/look-and-feel /etc/xdg /etc/skel/.config

    if [ -d "$SRC_DIR/os-build/configs/kde" ]; then
        cp -f "$SRC_DIR"/os-build/configs/kde/*.colors /usr/share/color-schemes/ 2>/dev/null || true
        for kde_cfg in kdeglobals plasmarc kwinrc breezerc krunnerrc ksplashrc dolphinrc kglobalshortcutsrc kcminputrc kscreenlockerrc baloofilerc powermanagementprofilesrc; do
            if [ -f "$SRC_DIR/os-build/configs/kde/$kde_cfg" ]; then
                cp -f "$SRC_DIR/os-build/configs/kde/$kde_cfg" /etc/xdg/
                cp -f "$SRC_DIR/os-build/configs/kde/$kde_cfg" /etc/skel/.config/
            fi
        done

        if [ -d "$SRC_DIR/os-build/configs/kde/plasma-theme/AxisOS-Obsidian" ]; then
            cp -rf "$SRC_DIR/os-build/configs/kde/plasma-theme/AxisOS-Obsidian" /usr/share/plasma/desktoptheme/
        fi
        if [ -d "$SRC_DIR/os-build/configs/kde/look-and-feel/org.axisos.obsidian.desktop" ]; then
            cp -rf "$SRC_DIR/os-build/configs/kde/look-and-feel/org.axisos.obsidian.desktop" /usr/share/plasma/look-and-feel/
        fi
    fi

    local AXIS_PLASMA_THEME="/usr/share/plasma/desktoptheme/AxisOS-Obsidian"
    if [ -d "$AXIS_PLASMA_THEME" ] && [ -d /usr/share/plasma/desktoptheme/default/translucent ]; then
        mkdir -p "$AXIS_PLASMA_THEME/translucent/widgets" "$AXIS_PLASMA_THEME/translucent/dialogs"
        for svg_rel in widgets/panel-background.svgz dialogs/background.svgz widgets/tooltip.svgz; do
            local src_svg="/usr/share/plasma/desktoptheme/default/translucent/$svg_rel"
            local dst_svg="$AXIS_PLASMA_THEME/translucent/$svg_rel"
            if [ -f "$src_svg" ]; then
                zcat "$src_svg" \
                    | sed 's/opacity:0\.85;fill:currentColor/opacity:0.72;fill:currentColor/g; s/style="opacity:0\.5"/style="opacity:0.38;fill:#38bdf8"/g' \
                    | gzip -9 > "$dst_svg" 2>/dev/null || cp -f "$src_svg" "$dst_svg"
            fi
        done
    fi

    if [ -f /usr/share/plasma/look-and-feel/org.axisos.obsidian.desktop/contents/layouts/org.kde.plasma.desktop-layout.js ] && [ -d /usr/share/plasma/shells/org.kde.plasma.desktop/contents ]; then
        cp -f /usr/share/plasma/look-and-feel/org.axisos.obsidian.desktop/contents/layouts/org.kde.plasma.desktop-layout.js \
              /usr/share/plasma/shells/org.kde.plasma.desktop/contents/layout.js
    fi

    local PREV_LAYOUT_REV
    PREV_LAYOUT_REV="$(cat "$LAYOUT_REV_FILE" 2>/dev/null || true)"
    for user_home in /home/*; do
        [ -d "$user_home" ] || continue
        local uname_dir
        uname_dir="$(basename "$user_home")"
        id "$uname_dir" >/dev/null 2>&1 || continue

        mkdir -p "$user_home/.config"
        for kde_cfg in kdeglobals plasmarc kwinrc breezerc krunnerrc ksplashrc dolphinrc kglobalshortcutsrc kcminputrc kscreenlockerrc baloofilerc powermanagementprofilesrc; do
            if [ -f "/etc/skel/.config/$kde_cfg" ]; then
                cp -f "/etc/skel/.config/$kde_cfg" "$user_home/.config/$kde_cfg"
            fi
        done

        if [ "$PREV_LAYOUT_REV" != "$TARGET_LAYOUT_REV" ]; then
            rm -f "$user_home/.config/plasma-org.kde.plasma.desktop-appletsrc" 2>/dev/null || true
        fi

        rm -f "$user_home/.Xauthority" "$user_home/.ICEauthority" 2>/dev/null || true

        if [ -f /etc/axisos-installed ]; then
            rm -f "$user_home/Desktop/install-axisos.desktop" \
                  "$user_home/Desktop/install-debian.desktop" \
                  "$user_home/Desktop/calamares.desktop" 2>/dev/null || true
        fi

        chown -R "${uname_dir}:${uname_dir}" "$user_home" 2>/dev/null || true
    done
    echo "$TARGET_LAYOUT_REV" > "$LAYOUT_REV_FILE"

    # 5. Update AxisOS Pure C native binaries & terminal logo greeting
    echo "[4/8] Updating all Pure C11 AxisOS applications (-O3 LTO optimized)..."
    for app_bin in \
        "os-build/apps/axis-task-manager/axis-task-manager" \
        "os-build/apps/axis-welcome/axis-welcome" \
        "os-build/apps/nobara-welcome/nobara-welcome" \
        "os-build/apps/axis-terminal/axis-terminal" \
        "os-build/apps/axis-software-manager/axis-software-manager" \
        "os-build/apps/axis-update-manager/axis-update-manager" \
        "os-build/apps/axis-package-installer/axis-package-installer" \
        "os-build/apps/axis-scheduler-manager/axis-scheduler-manager" \
        "os-build/apps/axis-settings/axisos-settings-panel" \
        "os-build/apps/axis-oobe/axis-oobe" \
        "os-build/apps/axis-usb-daemon/axis-usb-daemon" \
        "os-build/apps/axis-fetch/axis-fetch" \
        "os-build/apps/axis-calc/axis-calc" \
        "os-build/apps/axis-color-picker/axis-color-picker" \
        "os-build/scripts/axis-disk-info" \
        "os-build/scripts/axis-game-boost" \
        "os-build/scripts/axis-game-runner" \
        "os-build/scripts/axis-switch-session" \
        "os-build/scripts/axis-update-notifier" \
        "os-build/scripts/axis-autotimezone" \
        "os-build/scripts/gpu-autodetect" \
        "os-build/scripts/axis-biometrics" \
        "os-build/scripts/axis-fastboot-patch" \
        "os-build/scripts/axis-captive-portal" \
        "os-build/scripts/axis-rewind" \
        "os-build/scripts/axis-gpu-setup.sh" \
        "os-build/updater/axis-update" \
        "pkg-mgr/bin/axis" \
        "pkg-mgr/bin/axis_repo_builder" \
        "shell-c/axisos-shell"; do
        if [ -f "$SRC_DIR/$app_bin" ]; then
            local bname
            bname="$(basename "$app_bin")"
            install -m 0755 "$SRC_DIR/$app_bin" "/usr/local/bin/${bname}.new"
            mv -f "/usr/local/bin/${bname}.new" "/usr/local/bin/${bname}"
        fi
    done
    ln -sf /usr/local/bin/nobara-welcome /usr/local/bin/axis-hardware-manager 2>/dev/null || true
    ln -sf /usr/local/bin/axisos-settings-panel /usr/local/bin/axis-settings 2>/dev/null || true
    ln -sf /usr/local/bin/axisos-shell /usr/local/bin/axis-shell 2>/dev/null || true

    # Configure automatic AxisOS Logo + System Details banner on every interactive terminal
    cat << 'FETCH_EOF' > /etc/profile.d/axisos-fetch.sh
if [ -t 1 ] && [ -x /usr/local/bin/axis-fetch ] && [ -z "${AXIS_FETCH_SHOWN:-}" ]; then
    export AXIS_FETCH_SHOWN=1
    /usr/local/bin/axis-fetch
fi
FETCH_EOF
    chmod 0644 /etc/profile.d/axisos-fetch.sh

    if [ -f /etc/bash.bashrc ] && ! grep -q "axis-fetch" /etc/bash.bashrc 2>/dev/null; then
        cat << 'BASHRC_EOF' >> /etc/bash.bashrc

# AxisOS Terminal System Details & Logo Greeting
if [ -t 1 ] && [ -x /usr/local/bin/axis-fetch ] && [ -z "${AXIS_FETCH_SHOWN:-}" ]; then
    export AXIS_FETCH_SHOWN=1
    /usr/local/bin/axis-fetch
fi
BASHRC_EOF
    fi

    if [ -f "$SRC_DIR/os-build/configs/sysctl/99-axisos-gaming.conf" ]; then
        mkdir -p /etc/sysctl.d /etc/modules-load.d
        echo "tcp_bbr" > /etc/modules-load.d/axisos-bbr.conf
        modprobe tcp_bbr 2>/dev/null || true
        cp -f "$SRC_DIR/os-build/configs/sysctl/99-axisos-gaming.conf" /etc/sysctl.d/
        sysctl --system >/dev/null 2>&1 || true
    fi

    # 5b. OS Speed, I/O Scheduler, ZRAM, CPU Governor & Fast-Boot Optimization
    echo "    -> Applying kernel I/O schedulers, CPU governor, ZRAM, and fast-boot tuning..."

    # Udev rule for low-latency NVMe / SSD / SATA I/O schedulers & read-ahead
    mkdir -p /etc/udev/rules.d
    cat << 'UDEV_IO_EOF' > /etc/udev/rules.d/60-axisos-ioscheduler.rules
ACTION=="add|change", KERNEL=="nvme[0-9]*n[0-9]*", ATTR{queue/scheduler}="none", ATTR{queue/read_ahead_kb}="256", ATTR{queue/rq_affinity}="2"
ACTION=="add|change", KERNEL=="sd[a-z]|mmcblk[0-9]*", ATTR{queue/rotational}=="0", ATTR{queue/scheduler}="mq-deadline", ATTR{queue/read_ahead_kb}="256", ATTR{queue/rq_affinity}="2"
ACTION=="add|change", KERNEL=="sd[a-z]", ATTR{queue/rotational}=="1", ATTR{queue/scheduler}="bfq", ATTR{queue/read_ahead_kb}="512"
UDEV_IO_EOF

    # Apply block I/O tuning live
    for blk in /sys/block/nvme* /sys/block/sd* /sys/block/vd* /sys/block/mmcblk*; do
        [ -d "$blk/queue" ] || continue
        echo 256 > "$blk/queue/read_ahead_kb" 2>/dev/null || true
        echo 2 > "$blk/queue/rq_affinity" 2>/dev/null || true
        echo 0 > "$blk/queue/add_random" 2>/dev/null || true
        if [ -f "$blk/queue/rotational" ] && [ "$(cat "$blk/queue/rotational" 2>/dev/null)" = "0" ]; then
            if grep -q "\[none\]\| none " "$blk/queue/scheduler" 2>/dev/null; then
                echo none > "$blk/queue/scheduler" 2>/dev/null || true
            elif grep -q "mq-deadline" "$blk/queue/scheduler" 2>/dev/null; then
                echo mq-deadline > "$blk/queue/scheduler" 2>/dev/null || true
            fi
        fi
    done

    # Configure ZRAM compressed RAM swap (zstd, 50% RAM, priority 100)
    cat << 'ZRAM_EOF' > /etc/default/zramswap
ALGO=zstd
PERCENT=50
PRIORITY=100
ZRAM_EOF
    systemctl enable --now zramswap.service 2>/dev/null || true

    # Persistent boot-time CPU governor & transparent hugepage tuning service
    cat << 'PERF_SVC_EOF' > /etc/systemd/system/axisos-performance.service
[Unit]
Description=AxisOS Low-Latency CPU Governor & Memory Performance Tune
After=multi-user.target

[Service]
Type=oneshot
ExecStart=/bin/sh -c 'for g in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do [ -f "$g" ] && (echo schedutil > "$g" 2>/dev/null || echo performance > "$g" 2>/dev/null || true); done; for e in /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference; do [ -f "$e" ] && (echo balance_performance > "$e" 2>/dev/null || echo performance > "$e" 2>/dev/null || true); done; [ -f /sys/kernel/mm/transparent_hugepage/enabled ] && echo madvise > /sys/kernel/mm/transparent_hugepage/enabled 2>/dev/null || true'
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
PERF_SVC_EOF
    systemctl daemon-reload 2>/dev/null || true
    systemctl enable --now axisos-performance.service 2>/dev/null || true

    # Disable heavy background indexers & slow boot wait services
    rm -f /etc/xdg/autostart/baloo_file.desktop 2>/dev/null || true
    killall -q baloo_file baloo_file_extractor 2>/dev/null || true
    systemctl disable --now NetworkManager-wait-online.service systemd-networkd-wait-online.service 2>/dev/null || true

    # Fast systemd shutdown/start timeouts & bounded journal size
    mkdir -p /etc/systemd/system.conf.d /etc/systemd/journald.conf.d
    cat << 'SYS_FAST_EOF' > /etc/systemd/system.conf.d/99-axisos-fast.conf
[Manager]
DefaultTimeoutStartSec=15s
DefaultTimeoutStopSec=8s
SYS_FAST_EOF
    cat << 'JRN_FAST_EOF' > /etc/systemd/journald.conf.d/99-axisos-journal.conf
[Journal]
SystemMaxUse=64M
RuntimeMaxUse=32M
JRN_FAST_EOF

    # 6. Repair APT Repositories, dpkg, Polkit (pkexec) & Universal Flatpak/Flathub Support
    echo "[5/8] Repairing APT sources, Polkit permissions, and Flatpak/Flathub support..."
    export DEBIAN_FRONTEND=noninteractive

    # Remove stale live-media/cdrom sources that break apt update/install
    rm -f /etc/apt/sources.list.d/debian-live-media.list /etc/apt/sources.list.d/live.list 2>/dev/null || true
    for src_f in /etc/apt/sources.list.d/*.list /etc/apt/sources.list.d/*.sources; do
        [ -f "$src_f" ] || continue
        if grep -qiE 'cdrom:|file:/|deb\.axisos\.org' "$src_f" 2>/dev/null; then
            rm -f "$src_f" 2>/dev/null || true
        fi
    done

    # Write clean, verified Debian 12 Bookworm APT repositories
    cat << 'APT_EOF' > /etc/apt/sources.list
# AxisOS / Debian 12 Bookworm Official Repositories
deb http://deb.debian.org/debian bookworm main contrib non-free non-free-firmware
deb http://deb.debian.org/debian bookworm-updates main contrib non-free non-free-firmware
deb http://security.debian.org/debian-security bookworm-security main contrib non-free non-free-firmware
deb http://deb.debian.org/debian bookworm-backports main contrib non-free non-free-firmware
APT_EOF
    chmod 0644 /etc/apt/sources.list

    # Ensure required APT & log directories exist with proper permissions
    mkdir -p /var/lib/apt/lists/partial /var/cache/apt/archives/partial /var/log/apt
    chmod 0755 /var/lib/apt/lists /var/cache/apt/archives /var/log/apt
    if id _apt >/dev/null 2>&1; then
        chown _apt:root /var/lib/apt/lists/partial /var/cache/apt/archives/partial 2>/dev/null || true
        chmod 0700 /var/lib/apt/lists/partial /var/cache/apt/archives/partial 2>/dev/null || true
    fi

    # Repair / install official Microsoft VS Code APT GPG keyring BEFORE running apt-get update
    # (prevents NO_PUBKEY EB3E94ADBE1229CF and 0-byte keyring failures in Software Manager / Discover / APT)
    install -d -m 0755 /etc/apt/keyrings /etc/apt/sources.list.d
    local ms_key_valid=0
    local ms_tmp_dir="$WORK_DIR/ms-gpg"
    mkdir -p "$ms_tmp_dir/gnupg"
    chmod 700 "$ms_tmp_dir/gnupg"

    # Remove any 0-byte / broken Microsoft keyrings and duplicate .sources definitions
    rm -f /etc/apt/sources.list.d/vscode.sources 2>/dev/null || true
    for stale_key in /etc/apt/keyrings/packages.microsoft.gpg /usr/share/keyrings/microsoft.gpg /usr/share/keyrings/packages.microsoft.gpg; do
        if [ -e "$stale_key" ] && [ ! -s "$stale_key" ]; then
            rm -f "$stale_key" 2>/dev/null || true
        fi
    done

    # Ensure curl and gpg are present; temporarily clear broken VS Code repo files if tools are missing
    if ! command -v gpg >/dev/null 2>&1 || ! command -v curl >/dev/null 2>&1; then
        for src_f in /etc/apt/sources.list.d/*.list /etc/apt/sources.list.d/*.sources; do
            [ -f "$src_f" ] || continue
            if grep -qi 'packages\.microsoft\.com/repos/code' "$src_f" 2>/dev/null; then
                rm -f "$src_f" 2>/dev/null || true
            fi
        done
        apt-get -o DPkg::Lock::Timeout=60 update 2>/dev/null || true
        apt-get -o DPkg::Lock::Timeout=60 install -y --no-install-recommends curl gpg ca-certificates apt-transport-https 2>/dev/null || true
    fi

    # Download, dearmor, and verify the Microsoft signing key in an isolated temp dir before installing
    if command -v curl >/dev/null 2>&1 && command -v gpg >/dev/null 2>&1; then
        if curl -fsSL --retry 3 --connect-timeout 15 "https://packages.microsoft.com/keys/microsoft.asc" -o "$ms_tmp_dir/microsoft.asc" 2>/dev/null \
            && [ -s "$ms_tmp_dir/microsoft.asc" ] \
            && GNUPGHOME="$ms_tmp_dir/gnupg" gpg --batch --yes --dearmor -o "$ms_tmp_dir/packages.microsoft.gpg" "$ms_tmp_dir/microsoft.asc" 2>/dev/null \
            && [ -s "$ms_tmp_dir/packages.microsoft.gpg" ] \
            && GNUPGHOME="$ms_tmp_dir/gnupg" gpg --batch --show-keys --with-colons "$ms_tmp_dir/packages.microsoft.gpg" 2>/dev/null | grep -q '^pub:'; then
            install -m 0644 -o root -g root "$ms_tmp_dir/packages.microsoft.gpg" /etc/apt/keyrings/packages.microsoft.gpg
            if [ -d /usr/share/keyrings ]; then
                install -m 0644 -o root -g root "$ms_tmp_dir/packages.microsoft.gpg" /usr/share/keyrings/microsoft.gpg
            fi
            ms_key_valid=1
        fi
    fi

    # Fallback to an existing verified non-empty keyring if Microsoft key endpoint is temporarily unreachable
    if [ "$ms_key_valid" -eq 0 ] && command -v gpg >/dev/null 2>&1; then
        for existing_key in /etc/apt/keyrings/packages.microsoft.gpg /usr/share/keyrings/microsoft.gpg /usr/share/keyrings/packages.microsoft.gpg; do
            if [ -s "$existing_key" ] && GNUPGHOME="$ms_tmp_dir/gnupg" gpg --batch --show-keys --with-colons "$existing_key" 2>/dev/null | grep -q '^pub:'; then
                if [ "$existing_key" != "/etc/apt/keyrings/packages.microsoft.gpg" ]; then
                    install -m 0644 -o root -g root "$existing_key" /etc/apt/keyrings/packages.microsoft.gpg
                fi
                if [ -d /usr/share/keyrings ] && [ "$existing_key" != "/usr/share/keyrings/microsoft.gpg" ]; then
                    install -m 0644 -o root -g root "$existing_key" /usr/share/keyrings/microsoft.gpg
                fi
                ms_key_valid=1
                break
            fi
        done
    fi

    # Deduplicate VS Code repo files and either write a clean signed entry or remove broken unsigned entries
    for src_f in /etc/apt/sources.list.d/*.list /etc/apt/sources.list.d/*.sources; do
        [ -f "$src_f" ] || continue
        if grep -qi 'packages\.microsoft\.com/repos/code' "$src_f" 2>/dev/null; then
            rm -f "$src_f" 2>/dev/null || true
        fi
    done
    if [ "$ms_key_valid" -eq 1 ]; then
        cat << 'VSCODE_EOF' > /etc/apt/sources.list.d/vscode.list
deb [arch=amd64,arm64,armhf signed-by=/etc/apt/keyrings/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main
VSCODE_EOF
        chmod 0644 /etc/apt/sources.list.d/vscode.list
    fi

    # Fix Polkit rules directory permissions (polkitd runs as UID polkitd, NOT root!)
    mkdir -p /etc/polkit-1/rules.d
    cat << 'POLKIT_EOF' > /etc/polkit-1/rules.d/10-axisos-admin.rules
polkit.addRule(function(action, subject) {
    if (subject.isInGroup("sudo") || subject.isInGroup("wheel") || subject.user == "axis") {
        return polkit.Result.YES;
    }
});
POLKIT_EOF
    chown root:root /etc/polkit-1 /etc/polkit-1/rules.d /etc/polkit-1/rules.d/10-axisos-admin.rules 2>/dev/null || true
    chmod 0755 /etc/polkit-1 /etc/polkit-1/rules.d 2>/dev/null || true
    chmod 0644 /etc/polkit-1/rules.d/10-axisos-admin.rules 2>/dev/null || true
    for suid_bin in /bin/su /bin/mount /bin/umount /usr/bin/passwd /usr/bin/chsh /usr/bin/chfn /usr/bin/gpasswd /usr/bin/newgrp /usr/bin/sudo /usr/bin/pkexec /usr/bin/fusermount3 /usr/lib/polkit-1/polkit-agent-helper-1; do
        if [ -f "$suid_bin" ]; then
            chown root:root "$suid_bin" 2>/dev/null || true
            chmod 4755 "$suid_bin" 2>/dev/null || true
        fi
    done

    # Fix KDE Plasma lockscreen ("Unlocking failed"): unix_chkpwd MUST be setgid shadow (2755 root:shadow)
    # so non-root kscreenlocker_greet can verify passwords against /etc/shadow via pam_unix.so
    chown root:shadow /etc/shadow /etc/gshadow 2>/dev/null || true
    chmod 0640 /etc/shadow /etc/gshadow 2>/dev/null || true
    for sgid_shadow_bin in /usr/sbin/unix_chkpwd /sbin/unix_chkpwd /usr/bin/chage /usr/bin/expiry; do
        if [ -f "$sgid_shadow_bin" ]; then
            chown root:shadow "$sgid_shadow_bin" 2>/dev/null || true
            chmod 2755 "$sgid_shadow_bin" 2>/dev/null || true
        fi
    done
    if [ -f /usr/bin/crontab ] && getent group crontab >/dev/null 2>&1; then
        chown root:crontab /usr/bin/crontab 2>/dev/null || true
        chmod 2755 /usr/bin/crontab 2>/dev/null || true
    fi
    if [ ! -f /etc/pam.d/kde ]; then
        cat << 'PAM_KDE_EOF' > /etc/pam.d/kde
#%PAM-1.0
@include common-auth
@include common-account
@include common-password
@include common-session
PAM_KDE_EOF
        chmod 0644 /etc/pam.d/kde
    fi
    loginctl unlock-sessions 2>/dev/null || true
    for user_home in /home/*; do
        [ -d "$user_home" ] || continue
        local uname_dir
        uname_dir="$(basename "$user_home")"
        if [ "$uname_dir" != "axis" ] && id "$uname_dir" >/dev/null 2>&1; then
            usermod -aG sudo "$uname_dir" 2>/dev/null || true
        fi
    done
    systemctl restart polkit.service 2>/dev/null || true

    # Recover any interrupted dpkg transactions and refresh APT package index
    dpkg --configure -a 2>&1 || true
    apt-get -o DPkg::Lock::Timeout=60 update || true
    apt-get -o DPkg::Lock::Timeout=60 install -f -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" || true

    if ! command -v flatpak >/dev/null 2>&1 || ! command -v pkexec >/dev/null 2>&1; then
        apt-get -o DPkg::Lock::Timeout=60 install -y flatpak pkexec ca-certificates || true
    fi
    if command -v flatpak >/dev/null 2>&1; then
        flatpak remote-add --if-not-exists --system flathub https://dl.flathub.org/repo/flathub.flatpakrepo 2>&1 || true
        flatpak remote-modify --system --enable flathub 2>/dev/null || true
        for user_home in /home/*; do
            [ -d "$user_home" ] || continue
            local uname_dir
            uname_dir="$(basename "$user_home")"
            id "$uname_dir" >/dev/null 2>&1 || continue
            su - "$uname_dir" -c "flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo && flatpak remote-modify --user --enable flathub" 2>/dev/null || true
        done
    fi

    # 6b. Ensure Visual Studio Code (native APT) & Valve Steam (multiarch i386 + Vulkan) are installed
    echo "    -> Ensuring Visual Studio Code and Steam are installed..."
    if ! command -v code >/dev/null 2>&1; then
        if [ "$ms_key_valid" -eq 1 ]; then
            apt-get -o DPkg::Lock::Timeout=60 install -y code || true
            # Remove any duplicate vscode.sources created by the code package postinst
            if [ -f /etc/apt/sources.list.d/vscode.list ] && [ -f /etc/apt/sources.list.d/vscode.sources ]; then
                rm -f /etc/apt/sources.list.d/vscode.sources 2>/dev/null || true
            fi
        fi
        if ! command -v code >/dev/null 2>&1 && command -v flatpak >/dev/null 2>&1; then
            flatpak install --system -y --noninteractive flathub com.visualstudio.code 2>&1 || true
        fi
    fi

    if ! command -v steam >/dev/null 2>&1 && ! flatpak info com.valvesoftware.Steam >/dev/null 2>&1; then
        dpkg --add-architecture i386 2>/dev/null || true
        apt-get -o DPkg::Lock::Timeout=60 update || true
        apt-get -o DPkg::Lock::Timeout=60 install -y steam-installer mesa-vulkan-drivers:i386 libgl1-mesa-dri:i386 libglx-mesa0:i386 || true
        if ! command -v steam >/dev/null 2>&1 && command -v flatpak >/dev/null 2>&1; then
            flatpak install --system -y --noninteractive flathub com.valvesoftware.Steam 2>&1 || true
        fi
    fi

    # 7. Atomically self-update OTA script & enable Bi-Monthly Systemd Time
    echo "[6/8] Installing AxisOS Bi-Monthly OTA Auto-Update Timer (every 2 months)..."
    install -m 0755 "$SRC_DIR/scripts/axisos-ota-update.sh" /usr/local/bin/axisos-ota-update.new
    mv -f /usr/local/bin/axisos-ota-update.new /usr/local/bin/axisos-ota-update
    ln -sf /usr/local/bin/axisos-ota-update /usr/local/bin/axisos-update

    cat << 'SVC_EOF' > /etc/systemd/system/axisos-ota.service
[Unit]
Description=AxisOS Bi-Monthly Over-The-Air System Update
Wants=network-online.target
After=network-online.target

[Service]
Type=oneshot
ExecStart=/usr/local/bin/axisos-ota-update --auto
Nice=10
IOSchedulingClass=idle
SVC_EOF

    rm -f /etc/systemd/system/axisos-ota.time 2>/dev/null || true
    cat << 'TMR_EOF' > /etc/systemd/system/axisos-ota.timer
[Unit]
Description=Run AxisOS Over-The-Air System Update Every 2 Months

[Timer]
# Runs on the 1st of Jan, Mar, May, Jul, Sep, Nov at 03:00 (every 2 months)
OnCalendar=*-01,03,05,07,09,11-01 03:00:00
RandomizedDelaySec=1h
Persistent=true
Unit=axisos-ota.service

[Install]
WantedBy=timers.target
TMR_EOF

    systemctl daemon-reload 2>/dev/null || true
    systemctl enable axisos-ota.timer 2>/dev/null || true
    systemctl start axisos-ota.timer 2>/dev/null || true

    # 8. Update Debian base system & security packages
    echo "[7/8] Updating Debian Bookworm base packages & security patches..."
    apt-get -o DPkg::Lock::Timeout=60 update || true
    apt-get -o DPkg::Lock::Timeout=60 upgrade -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" || true
    apt-get -o DPkg::Lock::Timeout=60 autoremove -y --purge 2>/dev/null || true
    apt-get clean 2>/dev/null || true
    systemctl try-restart packagekit.service 2>/dev/null || true

    date -u '+%Y-%m-%dT%H:%M:%SZ' > "$STATE_FILE"

    echo "[8/8] Update complete!"
    echo "======================================================================"
    echo "  AxisOS OTA Update Complete!"
    echo "  - Windows 11 Task Manager (valid GTK 3 dark colors + 3D icons) updated"
    echo "  - Terminal AxisOS Logo + System Details enabled"
    echo "  - Package Installer & Software Manager with Flathub + APT ready"
    echo "  - Bi-monthly auto-updates active: systemctl status axisos-ota.timer"
    echo "  - Manual update anytime: sudo axisos-update"
    echo "======================================================================"

    # Restart SDDM only if invoked interactively from a TTY with no graphical session
    if [ "$AUTO_MODE" -eq 0 ] && [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
        echo ""
        echo "Restarting SDDM display manager in 3 seconds..."
        sleep 3
        systemctl restart sddm.service 2>/dev/null || true
    fi
}

main "$@"
