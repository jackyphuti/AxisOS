#!/bin/bash
# ==============================================================================
# AxisOS Universal Edition Switcher
# Allows seamless switching between Home & Gaming, Enterprise, and Work & School editions.
# Author: Jacky Mpoka <jackympoka22@gmail.com>
# ==============================================================================

set -euo pipefail

EDITION_FILE="/etc/axisos-edition"
INSTALLED_FILE="/etc/axisos-installed"

# Helper for notifications
notify() {
    local title="$1"
    local msg="$2"
    if command -v notify-send >/dev/null 2>&1; then
        notify-send --app-name="AxisOS Edition Manager" --icon="preferences-system" "$title" "$msg" 2>/dev/null || true
    fi
    echo "[$title] $msg"
}

get_current_edition() {
    if [ -f "$EDITION_FILE" ]; then
        # shellcheck disable=SC1090
        source "$EDITION_FILE"
        echo "${EDITION_ID:-base}"
    elif [ -f "$INSTALLED_FILE" ]; then
        grep -oP '^EDITION_ID="\K[^"]+' "$INSTALLED_FILE" 2>/dev/null || echo "base"
    else
        echo "base"
    fi
}

show_status() {
    local cur
    cur=$(get_current_edition)
    echo "=================================================="
    echo "             AxisOS Edition Manager               "
    echo "=================================================="
    case "$cur" in
        enterprise)
            echo "Current Edition: 🏢 AxisOS Enterprise Workstation"
            echo "Profile Features: Active Directory/SSSD, UFW Firewall, Enterprise VPNs, Audit Logging"
            ;;
        education|school|work)
            echo "Current Edition: 🎓 AxisOS Work & School"
            echo "Profile Features: Full LibreOffice Suite, VS Code & Python STEM Tools, PDF Tools"
            ;;
        *)
            echo "Current Edition: 🚀 AxisOS Home & Gaming (Base)"
            echo "Profile Features: XanMod Low-Latency Kernel, GameBoost GPU Engine, Steam, Media Codecs"
            ;;
    esac
    echo "=================================================="
    echo ""
    echo "Available Editions to switch to:"
    echo "  1) base        - AxisOS Home & Gaming"
    echo "  2) enterprise  - AxisOS Enterprise Workstation"
    echo "  3) school      - AxisOS Work & School"
    echo ""
    echo "Usage: sudo axis-switch-edition <base|enterprise|school>"
}

apply_base_edition() {
    echo "--> Configuring AxisOS Home & Gaming Profile..."
    cat << 'EOF' > "$EDITION_FILE"
EDITION_ID="base"
EDITION_NAME="AxisOS Home & Gaming"
EDITION_DESC="High-performance gaming, multimedia, and daily computing edition."
VERSION="2.1"
EOF
    chmod 644 "$EDITION_FILE"

    # Enable GameBoost daemon if available
    if command -v systemctl >/dev/null 2>&1 && systemctl list-unit-files axis-game-boost.service >/dev/null 2>&1; then
        systemctl enable axis-game-boost.service 2>/dev/null || true
        systemctl start axis-game-boost.service 2>/dev/null || true
    fi

    # Low-latency sysctl gaming tuning
    cat << 'EOF' > /etc/sysctl.d/99-axisos-gaming.conf
# AxisOS Low-Latency Gaming & Multimedia Tuning
vm.max_map_count = 2147483642
vm.swappiness = 10
fs.file-max = 2097152
EOF
    sysctl -p /etc/sysctl.d/99-axisos-gaming.conf 2>/dev/null || true

    notify "AxisOS Edition Changed" "Switched to AxisOS Home & Gaming Edition. GameBoost and high-performance gaming profiles engaged."
}

apply_enterprise_edition() {
    echo "--> Configuring AxisOS Enterprise Workstation Profile..."
    cat << 'EOF' > "$EDITION_FILE"
EDITION_ID="enterprise"
EDITION_NAME="AxisOS Enterprise Workstation"
EDITION_DESC="Hardened workstation with corporate domain join, VPNs, and compliance security."
VERSION="2.1"
EOF
    chmod 644 "$EDITION_FILE"

    # Enterprise security sysctl hardening
    cat << 'EOF' > /etc/sysctl.d/99-axisos-enterprise.conf
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
    sysctl -p /etc/sysctl.d/99-axisos-enterprise.conf 2>/dev/null || true

    # Enable firewall if installed
    if command -v ufw >/dev/null 2>&1; then
        ufw default deny incoming 2>/dev/null || true
        ufw default allow outgoing 2>/dev/null || true
        ufw --force enable 2>/dev/null || true
    fi

    notify "AxisOS Edition Changed" "Switched to AxisOS Enterprise Workstation. Security hardening and corporate domain join tools activated."
}

apply_school_edition() {
    echo "--> Configuring AxisOS Work & School Profile..."
    cat << 'EOF' > "$EDITION_FILE"
EDITION_ID="education"
EDITION_NAME="AxisOS Work & School"
EDITION_DESC="Full productivity workstation for students, educators, and professionals."
VERSION="2.1"
EOF
    chmod 644 "$EDITION_FILE"

    # Productivity and educational environment configuration
    cat << 'EOF' > /etc/axisos-work-school.conf
# AxisOS Work & School Environment Settings
PRODUCTIVITY_PROFILE=standard
CONTENT_SHIELD_SAFE_SEARCH=recommended
OFFICE_SUITE=libreoffice
DEV_TOOLING=enabled
EOF
    chmod 644 /etc/axisos-work-school.conf

    notify "AxisOS Edition Changed" "Switched to AxisOS Work & School. Office suite, STEM developer tools, and productivity workspace configured."
}

# Check root privileges
if [ "${1:-}" != "--status" ] && [ "${1:-}" != "-s" ] && [ "${1:-}" != "--list" ] && [ $# -gt 0 ]; then
    if [ "$(id -u)" -ne 0 ]; then
        echo "[!] Administrative privileges required to switch system editions."
        exec pkexec "$0" "$@"
    fi
fi

TARGET_EDITION="${1:-}"

case "$TARGET_EDITION" in
    base|home|gaming)
        apply_base_edition
        ;;
    enterprise|workstation|corp)
        apply_enterprise_edition
        ;;
    school|education|work|edu)
        apply_school_edition
        ;;
    --status|-s|--list|list|status|"")
        show_status
        ;;
    *)
        echo "Error: Unknown edition '$TARGET_EDITION'"
        echo "Valid options: base, enterprise, school"
        exit 1
        ;;
esac

exit 0
