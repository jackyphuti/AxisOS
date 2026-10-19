#!/bin/bash
# ==============================================================================
# AxisOS Enterprise Domain Join Utility
# Joins an Active Directory or FreeIPA / LDAP corporate network with SSSD.
# Author: Jacky Mpoka <jackympoka22@gmail.com>
# ==============================================================================

set -euo pipefail

if [ "$(id -u)" -ne 0 ]; then
    echo "[!] Root privileges required to configure corporate domain join."
    exec pkexec "$0" "$@"
fi

DOMAIN="${1:-}"
ADMIN_USER="${2:-}"

echo "=================================================="
echo "      AxisOS Enterprise Domain Join Wizard        "
echo "=================================================="

if [ -z "$DOMAIN" ]; then
    read -rp "Enter Active Directory / LDAP Domain (e.g. corp.company.com): " DOMAIN
fi

if [ -z "$DOMAIN" ]; then
    echo "Error: Domain cannot be empty."
    exit 1
fi

if [ -z "$ADMIN_USER" ]; then
    read -rp "Enter Domain Administrator Username [Administrator]: " ADMIN_USER
    ADMIN_USER="${ADMIN_USER:-Administrator}"
fi

echo "--> Discovering domain: $DOMAIN ..."
if command -v realm >/dev/null 2>&1; then
    realm discover "$DOMAIN" || true
    echo "--> Joining realm $DOMAIN with user $ADMIN_USER ..."
    realm join --user="$ADMIN_USER" "$DOMAIN"
    echo "--> Configuring SSSD PAM authorization..."
    if command -v pam-auth-update >/dev/null 2>&1; then
        pam-auth-update --enable mkhomedir || true
    fi
    echo "[SUCCESS] Successfully joined domain $DOMAIN!"
else
    echo "--> Installing realmd, sssd, and adcli packages..."
    DEBIAN_FRONTEND=noninteractive apt-get update -qq && apt-get install -y realmd sssd adcli samba-common-bin libnss-sss libpam-sss
    realm join --user="$ADMIN_USER" "$DOMAIN"
    pam-auth-update --enable mkhomedir || true
    echo "[SUCCESS] Successfully joined domain $DOMAIN!"
fi

exit 0
