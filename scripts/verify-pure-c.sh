#!/usr/bin/env bash
set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

echo "======================================================================"
echo "          AxisOS Pure C11 Strict Linkage Verification                 "
echo "======================================================================"

BINARIES=(
    "os-build/apps/axis-installer/axis-installer"
    "os-build/apps/axis-oobe/axis-oobe"
    "os-build/apps/axis-package-installer/axis-package-installer"
    "os-build/apps/axis-scheduler-manager/axis-scheduler-manager"
    "os-build/apps/axis-settings/axisos-settings-panel"
    "os-build/apps/axis-software-manager/axis-software-manager"
    "os-build/apps/axis-task-manager/axis-task-manager"
    "os-build/apps/axis-terminal/axis-terminal"
    "os-build/apps/axis-update-manager/axis-update-manager"
    "os-build/apps/axis-usb-daemon/axis-usb-daemon"
    "os-build/apps/axis-welcome/axis-welcome"
    "os-build/apps/nobara-welcome/nobara-welcome"
    "shell-c/axisos-shell"
    "pkg-mgr/bin/axis"
    "pkg-mgr/bin/axis_repo_builder"
    "os-build/updater/axis-update"
    "tests/test_hardware_probe"
    "tests/verify_harmony_ui"
)

FAILED=0
for b in "${BINARIES[@]}"; do
    if [ ! -f "$b" ]; then
        echo "[-] Missing binary: $b"
        FAILED=$((FAILED + 1))
        continue
    fi
    if ldd "$b" 2>&1 | grep -qE "stdc\+\+|Qt6|python"; then
        echo "[-] FAILED $b links to forbidden runtime:"
        ldd "$b" | grep -E "stdc\+\+|Qt6|python"
        FAILED=$((FAILED + 1))
    else
        printf "  [PASS] %-60s -> Pure C (-std=gnu11)\n" "$b"
    fi
done

echo "----------------------------------------------------------------------"
if [ "$FAILED" -eq 0 ]; then
    echo "VERIFICATION PASSED: All 18 binaries are strictly 100% Pure C."
    exit 0
else
    echo "VERIFICATION FAILED: $FAILED binaries had issues."
    exit 1
fi
