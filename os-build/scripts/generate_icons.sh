#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="${WORKSPACE_DIR:-$(cd "${SCRIPT_DIR}/../.." && pwd)}"
THEME_DIR="${REPO_ROOT}/data/theme/AxisHarmony"

echo "=== Generating AxisHarmony Pure Vector Icon Theme ==="
mkdir -p "${THEME_DIR}/status" "${THEME_DIR}/places" "${THEME_DIR}/actions" "${THEME_DIR}/apps"

cat << 'EOF' > "${THEME_DIR}/index.theme"
[Icon Theme]
Name=AxisHarmony
Comment=AxisOS Titanium Symbolic & Glassmorphic Icon Theme
Inherits=Papirus,Adwaita,hicolor
Directories=status,places,actions,apps

[status]
Size=24
Type=Fixed

[places]
Size=48
Type=Fixed

[actions]
Size=24
Type=Fixed

[apps]
Size=64
Type=Fixed
EOF

# Status wireless
cat << 'EOF' > "${THEME_DIR}/status/network-wireless-signal-excellent-symbolic.svg"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M12 18.5a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3z" fill="currentColor"/>
  <path d="M8.5 16.2a5 5 0 0 1 7 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M5.5 13a9.2 9.2 0 0 1 13 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M2.5 9.8a13.5 13.5 0 0 1 19 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
</svg>
EOF

cat << 'EOF' > "${THEME_DIR}/status/network-wireless-signal-good-symbolic.svg"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M12 18.5a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3z" fill="currentColor"/>
  <path d="M8.5 16.2a5 5 0 0 1 7 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M5.5 13a9.2 9.2 0 0 1 13 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M2.5 9.8a13.5 13.5 0 0 1 19 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.25"/>
</svg>
EOF

# Battery
cat << 'EOF' > "${THEME_DIR}/status/battery-full-symbolic.svg"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="2" y="6" width="17" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/>
  <path d="M21 10v4" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <rect x="4.5" y="8.5" width="12" height="7" rx="1.5" fill="currentColor"/>
</svg>
EOF

# Audio volume
cat << 'EOF' > "${THEME_DIR}/status/audio-volume-high-symbolic.svg"
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M4 10v4h3l5 4V6L7 10H4z" fill="currentColor"/>
  <path d="M15 9a4 4 0 0 1 0 6" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M17.5 6.5a7.5 7.5 0 0 1 0 11" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
</svg>
EOF

echo "[SUCCESS] AxisHarmony icon theme generated."
