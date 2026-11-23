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
    exec > >(tee -a "$LOG_FILE") 2>&1

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
    trap 'rm -rf "$WORK_DIR"' EXIT

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
        cp -f "$SRC_DIR/os-build/configs/sddm/theme.conf.user" /usr/share/sddm/themes/breeze/theme.conf.use
        cp -f "$SRC_DIR/os-build/configs/sddm/theme.conf.user" /etc/sddm.conf.d/theme.conf.use
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
        local uname_di
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
    echo "[4/8] Updating Pure C AxisOS applications (Task Manager, Terminal, Package Installer, Fetch)..."
    for app_bin in \
        "os-build/apps/axis-task-manager/axis-task-manager" \
        "os-build/apps/axis-welcome/axis-welcome" \
        "os-build/apps/nobara-welcome/nobara-welcome" \
        "os-build/apps/axis-terminal/axis-terminal" \
        "os-build/apps/axis-software-manager/axis-software-manager" \
        "os-build/apps/axis-update-manager/axis-update-manager" \
        "os-build/apps/axis-package-installer/axis-package-installer" \
        "os-build/apps/axis-scheduler-manager/axis-scheduler-manager" \
        "os-build/apps/axis-fetch/axis-fetch" \
        "os-build/apps/axis-calc/axis-calc" \
        "os-build/apps/axis-color-picker/axis-color-picker"; do
        if [ -f "$SRC_DIR/$app_bin" ]; then
            local bname
            bname="$(basename "$app_bin")"
            install -m 0755 "$SRC_DIR/$app_bin" "/usr/local/bin/${bname}.new"
            mv -f "/usr/local/bin/${bname}.new" "/usr/local/bin/${bname}"
        fi
    done
    ln -sf /usr/local/bin/nobara-welcome /usr/local/bin/axis-hardware-manager 2>/dev/null || true

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
        mkdir -p /etc/sysctl.d
        cp -f "$SRC_DIR/os-build/configs/sysctl/99-axisos-gaming.conf" /etc/sysctl.d/
        sysctl --system >/dev/null 2>&1 || true
    fi

    # 6. Enable Universal Flatpak + Flathub Support (System & User)
    echo "[5/8] Configuring Flatpak & Flathub universal software support..."
    export DEBIAN_FRONTEND=noninteractive
    if ! command -v flatpak >/dev/null 2>&1; then
        apt-get update -qq 2>/dev/null || true
        apt-get install -y flatpak 2>/dev/null || true
    fi
    if command -v flatpak >/dev/null 2>&1; then
        flatpak remote-add --if-not-exists --system flathub https://dl.flathub.org/repo/flathub.flatpakrepo 2>/dev/null || true
        for user_home in /home/*; do
            [ -d "$user_home" ] || continue
            local uname_di
            uname_dir="$(basename "$user_home")"
            id "$uname_dir" >/dev/null 2>&1 || continue
            su - "$uname_dir" -c "flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo" 2>/dev/null || true
        done
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

    cat << 'TMR_EOF' > /etc/systemd/system/axisos-ota.time
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
    apt-get update -qq 2>/dev/null || true
    apt-get upgrade -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" 2>/dev/null || true
    apt-get autoremove -y --purge 2>/dev/null || true
    apt-get clean 2>/dev/null || true

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
