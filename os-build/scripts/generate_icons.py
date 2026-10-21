#!/usr/bin/env python3
"""
AxisOS High-Craft Icon Generator (One UI / Titanium Architecture)
Generates:
1. High-precision 2px stroke symbolic vector icons categorized semantically:
   - status/ (Network, Battery, Audio, Power states, Weather, Emblems)
   - places/ (Folder, Drives)
   - actions/ (Search, Close, Tabs, App Grid, Playback, Power Actions)
   - apps/ (Application launchers)
2. Premium 64x64 squircle desktop application icons for the dock and launcher
3. AxisHarmony/index.theme with explicit Inherits=Papirus,Adwaita,hicolor fallback
"""

import os
import shutil

STATUS_ICONS = {
    # Network states
    "network-wireless-signal-excellent-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M12 18.5a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3z" fill="currentColor"/>
  <path d="M8.5 16.2a5 5 0 0 1 7 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M5.5 13a9.2 9.2 0 0 1 13 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M2.5 9.8a13.5 13.5 0 0 1 19 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
</svg>""",

    "network-wireless-signal-good-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M12 18.5a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3z" fill="currentColor"/>
  <path d="M8.5 16.2a5 5 0 0 1 7 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M5.5 13a9.2 9.2 0 0 1 13 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M2.5 9.8a13.5 13.5 0 0 1 19 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.25"/>
</svg>""",

    "network-wireless-signal-ok-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M12 18.5a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3z" fill="currentColor"/>
  <path d="M8.5 16.2a5 5 0 0 1 7 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M5.5 13a9.2 9.2 0 0 1 13 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.25"/>
  <path d="M2.5 9.8a13.5 13.5 0 0 1 19 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.25"/>
</svg>""",

    "network-wireless-signal-weak-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M12 18.5a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3z" fill="currentColor"/>
  <path d="M8.5 16.2a5 5 0 0 1 7 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.25"/>
  <path d="M5.5 13a9.2 9.2 0 0 1 13 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.25"/>
  <path d="M2.5 9.8a13.5 13.5 0 0 1 19 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.25"/>
</svg>""",

    "network-wireless-signal-none-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M12 18.5a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3z" fill="currentColor"/>
  <path d="M8.5 16.2a5 5 0 0 1 7 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.2"/>
  <path d="M5.5 13a9.2 9.2 0 0 1 13 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.2"/>
  <path d="M2.5 9.8a13.5 13.5 0 0 1 19 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.2"/>
</svg>""",

    "network-wireless-offline-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M12 18.5a1.5 1.5 0 1 1 0 3 1.5 1.5 0 0 1 0-3z" fill="currentColor" opacity="0.3"/>
  <path d="M8.5 16.2a5 5 0 0 1 7 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.3"/>
  <path d="M5.5 13a9.2 9.2 0 0 1 13 0" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.3"/>
  <line x1="3" y1="3" x2="21" y2="21" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>""",

    "network-wired-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3" y="4" width="18" height="13" rx="3" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8"/>
  <path d="M7 17v3h10v-3" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>
  <line x1="12" y1="20" x2="12" y2="22" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <circle cx="12" cy="10.5" r="2" fill="currentColor"/>
</svg>""",

    "network-wired-disconnected-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3" y="4" width="18" height="13" rx="3" fill="none" stroke="currentColor" stroke-width="1.8" opacity="0.4"/>
  <path d="M7 17v3h10v-3" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round" opacity="0.4"/>
  <line x1="3" y1="3" x2="21" y2="21" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>""",

    # Battery levels
    "battery-level-100-charged-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3" y="6" width="16" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/>
  <path d="M21 10v4" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <rect x="5.5" y="8.5" width="11" height="7" rx="1.5" fill="currentColor"/>
</svg>""",

    "battery-level-80-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3" y="6" width="16" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/>
  <path d="M21 10v4" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <rect x="5.5" y="8.5" width="8.8" height="7" rx="1.5" fill="currentColor"/>
</svg>""",

    "battery-level-60-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3" y="6" width="16" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/>
  <path d="M21 10v4" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <rect x="5.5" y="8.5" width="6.6" height="7" rx="1.5" fill="currentColor"/>
</svg>""",

    "battery-level-40-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3" y="6" width="16" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/>
  <path d="M21 10v4" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <rect x="5.5" y="8.5" width="4.4" height="7" rx="1.5" fill="currentColor"/>
</svg>""",

    "battery-level-20-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3" y="6" width="16" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/>
  <path d="M21 10v4" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <rect x="5.5" y="8.5" width="2.4" height="7" rx="1" fill="currentColor"/>
</svg>""",

    "battery-level-0-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3" y="6" width="16" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/>
  <path d="M21 10v4" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="11" y1="9" x2="11" y2="15" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" opacity="0.5"/>
</svg>""",

    "battery-charging-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3" y="6" width="16" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8"/>
  <path d="M21 10v4" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <path d="M11.5 8L8.5 12.5H12L10.5 16L14.5 11.5H11L11.5 8Z" fill="currentColor"/>
</svg>""",

    "battery-missing-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3" y="6" width="16" height="12" rx="2.5" fill="none" stroke="currentColor" stroke-width="1.8" stroke-dasharray="2 2"/>
  <path d="M21 10v4" stroke="currentColor" stroke-width="2" stroke-linecap="round" opacity="0.5"/>
  <line x1="8" y1="12" x2="14" y2="12" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>""",

    # Audio Volume
    "audio-volume-high-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M4 9h4l5-4v14l-5-4H4a1 1 0 0 1-1-1v-4a1 1 0 0 1 1-1z" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round"/>
  <path d="M16 8.5a5 5 0 0 1 0 7" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M18.8 6a9 9 0 0 1 0 12" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
</svg>""",

    "audio-volume-medium-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M4 9h4l5-4v14l-5-4H4a1 1 0 0 1-1-1v-4a1 1 0 0 1 1-1z" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round"/>
  <path d="M16 8.5a5 5 0 0 1 0 7" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
</svg>""",

    "audio-volume-low-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M4 9h4l5-4v14l-5-4H4a1 1 0 0 1-1-1v-4a1 1 0 0 1 1-1z" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round"/>
  <path d="M16 10.5a2.5 2.5 0 0 1 0 3" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
</svg>""",

    "audio-volume-muted-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M4 9h4l5-4v14l-5-4H4a1 1 0 0 1-1-1v-4a1 1 0 0 1 1-1z" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round"/>
  <line x1="16" y1="10" x2="21" y2="15" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="21" y1="10" x2="16" y2="15" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>""",

    # System Power Actions in status
    "system-shutdown-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M18.36 6.64A9 9 0 1 1 5.64 6.64" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/>
  <line x1="12" y1="2" x2="12" y2="12" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"/>
</svg>""",

    "system-reboot-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M21 12a9 9 0 1 1-2.6-6.4L21 8" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M21 3v5h-5" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>""",

    "system-lock-screen-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="5" y="11" width="14" height="10" rx="3" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8"/>
  <path d="M8 11V7a4 4 0 0 1 8 0v4" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <circle cx="12" cy="15.5" r="1.5" fill="currentColor"/>
</svg>""",

    "system-suspend-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round"/>
</svg>""",

    # Weather & Emblems
    "emblem-ok-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <circle cx="12" cy="12" r="9" fill="currentColor" fill-opacity="0.18" stroke="currentColor" stroke-width="1.8"/>
  <path d="M8 12.2l2.8 2.8L16.2 9" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>""",

    "weather-clear-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <circle cx="12" cy="12" r="4.5" fill="currentColor" fill-opacity="0.25" stroke="currentColor" stroke-width="1.8"/>
  <line x1="12" y1="2" x2="12" y2="4.5" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="12" y1="19.5" x2="12" y2="22" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="2" y1="12" x2="4.5" y2="12" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="19.5" y1="12" x2="22" y2="12" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="4.9" y1="4.9" x2="6.7" y2="6.7" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="17.3" y1="17.3" x2="19.1" y2="19.1" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="4.9" y1="19.1" x2="6.7" y2="17.3" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="17.3" y1="6.7" x2="19.1" y2="4.9" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>""",

    "weather-clear-night-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M19 13.5A7.8 7.8 0 1 1 10.5 5a6.2 6.2 0 0 0 8.5 8.5z" fill="currentColor" fill-opacity="0.22" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round"/>
  <path d="M19 4l.6 1.2 1.2.6-1.2.6-.6 1.2-.6-1.2-1.2-.6 1.2-.6.6-1.2z" fill="currentColor"/>
</svg>""",
}

PLACES_ICONS = {
    "folder-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M2.5 7.5a2 2 0 0 1 2-2h4.5a2 2 0 0 1 1.4.6L12 7.5H19.5a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2h-15a2 2 0 0 1-2-2v-11z" fill="currentColor" fill-opacity="0.15" stroke="currentColor" stroke-width="1.7" stroke-linejoin="round"/>
  <path d="M2.5 11.5h19v5a2 2 0 0 1-2 2h-15a2 2 0 0 1-2-2v-5z" fill="currentColor" fill-opacity="0.32" stroke="currentColor" stroke-width="1.7" stroke-linejoin="round"/>
</svg>""",
}

ACTIONS_ICONS = {
    "system-search-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <circle cx="10.5" cy="10.5" r="7" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M7.5 7.5a4.2 4.2 0 0 1 4.5-.8" fill="none" stroke="currentColor" stroke-width="1.4" stroke-linecap="round" opacity="0.75"/>
  <path d="M15.8 15.8L21 21" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"/>
</svg>""",

    "window-close-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <circle cx="12" cy="12" r="9" fill="currentColor" fill-opacity="0.14" stroke="currentColor" stroke-width="1.6"/>
  <line x1="9" y1="9" x2="15" y2="15" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="15" y1="9" x2="9" y2="15" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>""",

    "document-open-recent-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <circle cx="12" cy="12" r="9" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8"/>
  <polyline points="12 7 12 12 15.5 14" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>""",

    "tab-new-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3.5" y="3.5" width="17" height="17" rx="4" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8"/>
  <line x1="12" y1="8" x2="12" y2="16" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <line x1="8" y1="12" x2="16" y2="12" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>""",

    "view-app-grid-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3.5" y="3.5" width="6.5" height="6.5" rx="2.5" fill="currentColor"/>
  <rect x="14" y="3.5" width="6.5" height="6.5" rx="2.5" fill="currentColor" fill-opacity="0.45"/>
  <rect x="3.5" y="14" width="6.5" height="6.5" rx="2.5" fill="currentColor" fill-opacity="0.45"/>
  <rect x="14" y="14" width="6.5" height="6.5" rx="2.5" fill="currentColor"/>
</svg>""",

    "media-playback-start-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M9 16.5V6.5l10-2v10" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M9 10.5l10-2" fill="none" stroke="currentColor" stroke-width="1.8"/>
  <ellipse cx="6.5" cy="16.5" rx="3" ry="2.2" transform="rotate(-15 6.5 16.5)" fill="currentColor"/>
  <ellipse cx="16.5" cy="14.5" rx="3" ry="2.2" transform="rotate(-15 16.5 14.5)" fill="currentColor"/>
</svg>""",

    "system-shutdown-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M18.36 6.64A9 9 0 1 1 5.64 6.64" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"/>
  <line x1="12" y1="2" x2="12" y2="12" stroke="currentColor" stroke-width="2.4" stroke-linecap="round"/>
</svg>""",

    "system-reboot-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M21 12a9 9 0 1 1-2.6-6.4L21 8" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M21 3v5h-5" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>""",

    "system-lock-screen-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="5" y="11" width="14" height="10" rx="3" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8"/>
  <path d="M8 11V7a4 4 0 0 1 8 0v4" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <circle cx="12" cy="15.5" r="1.5" fill="currentColor"/>
</svg>""",

    "system-suspend-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round"/>
</svg>""",
}

APPS_SYMBOLIC_ICONS = {
    "utilities-terminal-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="2.5" y="3.5" width="19" height="17" rx="3.5" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8"/>
  <line x1="2.5" y1="8" x2="21.5" y2="8" stroke="currentColor" stroke-width="1.2" opacity="0.6"/>
  <circle cx="5.5" cy="5.8" r="0.9" fill="currentColor"/>
  <circle cx="8" cy="5.8" r="0.9" fill="currentColor"/>
  <circle cx="10.5" cy="5.8" r="0.9" fill="currentColor"/>
  <path d="M6 11.5l3.5 2.5L6 16.5" fill="none" stroke="currentColor" stroke-width="1.9" stroke-linecap="round" stroke-linejoin="round"/>
  <line x1="12" y1="16.5" x2="17.5" y2="16.5" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
</svg>""",

    "system-file-manager-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M2.5 7.5a2 2 0 0 1 2-2h4.5a2 2 0 0 1 1.4.6L12 7.5H19.5a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2h-15a2 2 0 0 1-2-2v-11z" fill="currentColor" fill-opacity="0.15" stroke="currentColor" stroke-width="1.7" stroke-linejoin="round"/>
  <path d="M2.5 11.5h19v5a2 2 0 0 1-2 2h-15a2 2 0 0 1-2-2v-5z" fill="currentColor" fill-opacity="0.32" stroke="currentColor" stroke-width="1.7" stroke-linejoin="round"/>
  <line x1="8" y1="14" x2="16" y2="14" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" opacity="0.7"/>
</svg>""",

    "preferences-system-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M12 15a3 3 0 1 0 0-6 3 3 0 0 0 0 6z" fill="currentColor" fill-opacity="0.25" stroke="currentColor" stroke-width="1.8"/>
  <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1-2.83 2.83l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-4 0v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1 0-4h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 2.83-2.83l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 4 0v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 0 4h-.09a1.65 1.65 0 0 0-1.51 1z" fill="currentColor" fill-opacity="0.14" stroke="currentColor" stroke-width="1.6" stroke-linejoin="round"/>
</svg>""",

    "web-browser-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <circle cx="12" cy="12" r="9" fill="currentColor" fill-opacity="0.14" stroke="currentColor" stroke-width="1.8"/>
  <line x1="3" y1="12" x2="21" y2="12" stroke="currentColor" stroke-width="1.4" opacity="0.7"/>
  <ellipse cx="12" cy="12" rx="4.8" ry="9" fill="none" stroke="currentColor" stroke-width="1.4" opacity="0.85"/>
  <polygon points="12 4.5 14.5 11 12 10 9.5 11" fill="currentColor"/>
  <polygon points="12 19.5 9.5 13 12 14 14.5 13" fill="currentColor" fill-opacity="0.5"/>
</svg>""",

    "system-software-install-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="3.5" y="7" width="17" height="13.5" rx="3.5" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8"/>
  <path d="M8 7V5.5a4 4 0 0 1 8 0V7" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round"/>
  <path d="M12 10.5v5m0 0l-2.6-2.6m2.6 2.6l2.6-2.6" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/>
</svg>""",

    "text-editor-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M5 4a2 2 0 0 1 2-2h7l5 5v13a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V4z" fill="currentColor" fill-opacity="0.14" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round"/>
  <path d="M14 2v5.5a1.5 1.5 0 0 0 1.5 1.5H21" fill="none" stroke="currentColor" stroke-width="1.6" stroke-linejoin="round"/>
  <path d="M8.5 13.5l-2 2 2 2m7-4l2 2-2 2m-4.5 1.5l2-7" fill="none" stroke="currentColor" stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>
</svg>""",

    "camera-photo-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <rect x="2.5" y="6.5" width="19" height="14" rx="3.8" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8"/>
  <path d="M8 6.5V5a1.5 1.5 0 0 1 1.5-1.5h5A1.5 1.5 0 0 1 16 5v1.5" fill="currentColor" fill-opacity="0.25" stroke="currentColor" stroke-width="1.6"/>
  <circle cx="12" cy="13.5" r="4.2" fill="currentColor" fill-opacity="0.25" stroke="currentColor" stroke-width="1.8"/>
  <circle cx="12" cy="13.5" r="2" fill="currentColor"/>
  <circle cx="18" cy="9.5" r="1.1" fill="currentColor"/>
</svg>""",

    "applications-games-symbolic.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24">
  <path d="M6 5.5h12a4.5 4.5 0 0 1 4.3 5.3l-1.3 6.8a2.8 2.8 0 0 1-2.7 2.2h-.2a3 3 0 0 1-2.5-1.4l-1.5-2.8a2.5 2.5 0 0 0-2.1-1.3h-.1a2.5 2.5 0 0 0-2.1 1.3l-1.5 2.8A3 3 0 0 1 5.9 20h-.2a2.8 2.8 0 0 1-2.7-2.2L1.7 11A4.5 4.5 0 0 1 6 5.5z" fill="currentColor" fill-opacity="0.16" stroke="currentColor" stroke-width="1.8" stroke-linejoin="round"/>
  <path d="M7 9v4m-2-2h4" stroke="currentColor" stroke-width="2" stroke-linecap="round"/>
  <circle cx="17" cy="9.5" r="1.1" fill="currentColor"/>
  <circle cx="15" cy="11.5" r="1.1" fill="currentColor"/>
  <circle cx="19" cy="11.5" r="1.1" fill="currentColor"/>
  <circle cx="17" cy="13.5" r="1.1" fill="currentColor"/>
</svg>""",
}

# Rich 64x64 Desktop Application Icons (for apps/ category)
APP_ICONS = {
    "utilities-terminal.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">
  <defs>
    <linearGradient id="term_bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#242830"/>
      <stop offset="100%" stop-color="#0E1013"/>
    </linearGradient>
    <linearGradient id="term_rim" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#4B5563"/>
      <stop offset="100%" stop-color="#1F2937"/>
    </linearGradient>
  </defs>
  <rect x="4" y="4" width="56" height="56" rx="14" fill="url(#term_bg)" stroke="url(#term_rim)" stroke-width="1.5"/>
  <line x1="4" y1="18" x2="60" y2="18" stroke="#374151" stroke-width="1"/>
  <circle cx="12" cy="11" r="2.2" fill="#EF4444"/>
  <circle cx="18" cy="11" r="2.2" fill="#F59E0B"/>
  <circle cx="24" cy="11" r="2.2" fill="#10B981"/>
  <path d="M14 28l9 7-9 7" fill="none" stroke="#F9FAFB" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>
  <line x1="28" y1="42" x2="44" y2="42" stroke="#F9FAFB" stroke-width="3" stroke-linecap="round"/>
</svg>""",

    "system-file-manager.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">
  <defs>
    <linearGradient id="files_bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#2E333D"/>
      <stop offset="100%" stop-color="#16181D"/>
    </linearGradient>
  </defs>
  <rect x="4" y="4" width="56" height="56" rx="14" fill="url(#files_bg)" stroke="#4B5563" stroke-width="1.5"/>
  <path d="M12 20a3 3 0 0 1 3-3h11a3 3 0 0 1 2.2 1L31 21h18a3 3 0 0 1 3 3v20a3 3 0 0 1-3 3H15a3 3 0 0 1-3-3V20z" fill="#374151" stroke="#9CA3AF" stroke-width="1.5"/>
  <path d="M12 27h40v17a3 3 0 0 1-3 3H15a3 3 0 0 1-3-3V27z" fill="#4B5563" stroke="#D1D5DB" stroke-width="1.5"/>
  <line x1="22" y1="35" x2="42" y2="35" stroke="#FFFFFF" stroke-width="2.5" stroke-linecap="round"/>
</svg>""",

    "web-browser.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">
  <defs>
    <linearGradient id="globe_bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#272B33"/>
      <stop offset="100%" stop-color="#111317"/>
    </linearGradient>
  </defs>
  <rect x="4" y="4" width="56" height="56" rx="14" fill="url(#globe_bg)" stroke="#4B5563" stroke-width="1.5"/>
  <circle cx="32" cy="32" r="18" fill="#1F2937" stroke="#9CA3AF" stroke-width="2"/>
  <line x1="14" y1="32" x2="50" y2="32" stroke="#9CA3AF" stroke-width="1.5"/>
  <ellipse cx="32" cy="32" rx="9" ry="18" fill="none" stroke="#9CA3AF" stroke-width="1.5"/>
  <polygon points="32 18 37 30 32 28 27 30" fill="#FFFFFF"/>
  <polygon points="32 46 27 34 32 36 37 34" fill="#6B7280"/>
</svg>""",

    "system-software-install.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">
  <defs>
    <linearGradient id="soft_bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#282C36"/>
      <stop offset="100%" stop-color="#121418"/>
    </linearGradient>
  </defs>
  <rect x="4" y="4" width="56" height="56" rx="14" fill="url(#soft_bg)" stroke="#4B5563" stroke-width="1.5"/>
  <rect x="14" y="21" width="36" height="29" rx="6" fill="#1F2937" stroke="#9CA3AF" stroke-width="2"/>
  <path d="M24 21v-3a8 8 0 0 1 16 0v3" fill="none" stroke="#9CA3AF" stroke-width="2.5" stroke-linecap="round"/>
  <path d="M32 28v13m0 0l-5-5m5 5l5-5" fill="none" stroke="#FFFFFF" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>
</svg>""",

    "preferences-system.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">
  <defs>
    <linearGradient id="gear_bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#2A2E38"/>
      <stop offset="100%" stop-color="#131519"/>
    </linearGradient>
  </defs>
  <rect x="4" y="4" width="56" height="56" rx="14" fill="url(#gear_bg)" stroke="#4B5563" stroke-width="1.5"/>
  <circle cx="32" cy="32" r="6" fill="#111827" stroke="#FFFFFF" stroke-width="2.5"/>
  <path d="M48 32a16 16 0 0 0-.4-3.5l4-3.1-3.6-6.2-4.8 1.6a16 16 0 0 0-3-1.8L39 14h-7.2l-1.2 4.9a16 16 0 0 0-3 1.8l-4.8-1.6-3.6 6.2 4 3.1a16 16 0 0 0 0 7.2l-4 3.1 3.6 6.2 4.8-1.6a16 16 0 0 0 3 1.8L31.8 50h7.2l1.2-4.9a16 16 0 0 0 3-1.8l4.8 1.6 3.6-6.2-4-3.1a16 16 0 0 0 .4-3.6z" fill="none" stroke="#E5E7EB" stroke-width="2.5" stroke-linejoin="round"/>
</svg>""",

    "emblem-ok.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">
  <defs>
    <linearGradient id="ok_bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#252932"/>
      <stop offset="100%" stop-color="#101215"/>
    </linearGradient>
  </defs>
  <rect x="4" y="4" width="56" height="56" rx="14" fill="url(#ok_bg)" stroke="#4B5563" stroke-width="1.5"/>
  <circle cx="32" cy="32" r="18" fill="#1F2937" stroke="#D1D5DB" stroke-width="2"/>
  <path d="M23 32.5l6.5 6.5 12-13" fill="none" stroke="#FFFFFF" stroke-width="3.5" stroke-linecap="round" stroke-linejoin="round"/>
</svg>""",

    "text-editor.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">
  <defs>
    <linearGradient id="doc_bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#262A34"/>
      <stop offset="100%" stop-color="#121317"/>
    </linearGradient>
  </defs>
  <rect x="4" y="4" width="56" height="56" rx="14" fill="url(#doc_bg)" stroke="#4B5563" stroke-width="1.5"/>
  <path d="M18 15a3 3 0 0 1 3-3h15l10 10v27a3 3 0 0 1-3 3H21a3 3 0 0 1-3-3V15z" fill="#1F2937" stroke="#9CA3AF" stroke-width="2"/>
  <path d="M36 12v7a3 3 0 0 0 3 3h7" fill="none" stroke="#9CA3AF" stroke-width="2"/>
  <path d="M25 33l-4 4 4 4m14-8l4 4-4 4m-9 3l4-14" fill="none" stroke="#FFFFFF" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round"/>
</svg>""",

    "camera-photo.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">
  <defs>
    <linearGradient id="cam_bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#282C36"/>
      <stop offset="100%" stop-color="#111317"/>
    </linearGradient>
  </defs>
  <rect x="4" y="4" width="56" height="56" rx="14" fill="url(#cam_bg)" stroke="#4B5563" stroke-width="1.5"/>
  <rect x="12" y="21" width="40" height="29" rx="6" fill="#1F2937" stroke="#9CA3AF" stroke-width="2"/>
  <path d="M23 21v-3a2 2 0 0 1 2-2h14a2 2 0 0 1 2 2v3" fill="#374151" stroke="#9CA3AF" stroke-width="2"/>
  <circle cx="32" cy="35.5" r="8.5" fill="#111827" stroke="#D1D5DB" stroke-width="2.5"/>
  <circle cx="32" cy="35.5" r="4" fill="#FFFFFF"/>
  <circle cx="44" cy="27" r="2" fill="#E5E7EB"/>
</svg>""",

    "media-playback-start.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">
  <defs>
    <linearGradient id="mus_bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#282C36"/>
      <stop offset="100%" stop-color="#111317"/>
    </linearGradient>
  </defs>
  <rect x="4" y="4" width="56" height="56" rx="14" fill="url(#mus_bg)" stroke="#4B5563" stroke-width="1.5"/>
  <circle cx="32" cy="32" r="18" fill="#1F2937" stroke="#9CA3AF" stroke-width="2"/>
  <path d="M26 40V22l16-4v17" fill="none" stroke="#FFFFFF" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>
  <path d="M26 28l16-4" fill="none" stroke="#FFFFFF" stroke-width="3"/>
  <circle cx="22" cy="40" r="4" fill="#FFFFFF"/>
  <circle cx="38" cy="36" r="4" fill="#FFFFFF"/>
</svg>""",

    "applications-games.svg": """<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 64 64" width="64" height="64">
  <defs>
    <linearGradient id="game_bg" x1="0" y1="0" x2="0" y2="1">
      <stop offset="0%" stop-color="#282C36"/>
      <stop offset="100%" stop-color="#111317"/>
    </linearGradient>
  </defs>
  <rect x="4" y="4" width="56" height="56" rx="14" fill="url(#game_bg)" stroke="#4B5563" stroke-width="1.5"/>
  <path d="M16 19h32a10 10 0 0 1 9.8 12.1l-2.6 13.8a6 6 0 0 1-5.8 4.9h-.4a6 6 0 0 1-5.3-3.2L44 40.5a5 5 0 0 0-4.3-2.5h-15.4a5 5 0 0 0-4.3 2.5l-3.7 6.1A6 6 0 0 1 11.3 50h-.5a6 6 0 0 1-5.8-4.9L2.4 31.1A10 10 0 0 1 16 19z" fill="#1F2937" stroke="#9CA3AF" stroke-width="2.5" stroke-linejoin="round"/>
  <path d="M18 26v8m-4-4h8" stroke="#FFFFFF" stroke-width="3" stroke-linecap="round"/>
  <circle cx="44" cy="27" r="2.2" fill="#FFFFFF"/>
  <circle cx="40" cy="31" r="2.2" fill="#9CA3AF"/>
  <circle cx="48" cy="31" r="2.2" fill="#9CA3AF"/>
  <circle cx="44" cy="35" r="2.2" fill="#FFFFFF"/>
</svg>"""
}

INDEX_THEME_CONTENT = """[Icon Theme]
Name=AxisHarmony
Comment=Samsung One UI & HarmonyOS-inspired 2px stroke symbolic icon theme for AxisOS
Inherits=Papirus,Adwaita,hicolor
Directories=scalable/apps,scalable/actions,scalable/categories,scalable/places,scalable/status

[scalable/apps]
Size=24
MinSize=16
MaxSize=512
Type=Scalable

[scalable/actions]
Size=24
MinSize=16
MaxSize=512
Type=Scalable

[scalable/categories]
Size=24
MinSize=16
MaxSize=512
Type=Scalable

[scalable/places]
Size=24
MinSize=16
MaxSize=512
Type=Scalable

[scalable/status]
Size=24
MinSize=16
MaxSize=512
Type=Scalable
"""

def generate_all_icons():
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    
    target_bases = [
        os.path.join(repo_root, "data", "icons"),
        os.path.join(repo_root, "os-build", "data", "icons")
    ]

    for base_dir in target_bases:
        if not os.path.exists(base_dir):
            continue

        harmony_dir = os.path.join(base_dir, "AxisHarmony")
        os.makedirs(harmony_dir, exist_ok=True)

        # 1. Clean old duplicated subdirectories
        for cat in ["apps", "actions", "places", "status", "categories"]:
            cat_dir = os.path.join(harmony_dir, "scalable", cat)
            if os.path.exists(cat_dir):
                shutil.rmtree(cat_dir)
            os.makedirs(cat_dir, exist_ok=True)

        # 2. Clean loose SVGs in the parent icons base directory
        for item in os.listdir(base_dir):
            item_p = os.path.join(base_dir, item)
            if os.path.isfile(item_p) and item.endswith(".svg"):
                os.remove(item_p)
            # Remove legacy redundant flat dirs (e.g. data/icons/apps) outside AxisHarmony
            if os.path.isdir(item_p) and item in ["apps", "actions", "places", "status", "categories"]:
                shutil.rmtree(item_p)

        # 3. Write semantic category files into AxisHarmony/scalable/
        # Status icons
        status_target = os.path.join(harmony_dir, "scalable", "status")
        for name, svg in STATUS_ICONS.items():
            with open(os.path.join(status_target, name), "w", encoding="utf-8") as f:
                f.write(svg)

        # Places icons
        places_target = os.path.join(harmony_dir, "scalable", "places")
        for name, svg in PLACES_ICONS.items():
            with open(os.path.join(places_target, name), "w", encoding="utf-8") as f:
                f.write(svg)

        # Actions icons
        actions_target = os.path.join(harmony_dir, "scalable", "actions")
        for name, svg in ACTIONS_ICONS.items():
            with open(os.path.join(actions_target, name), "w", encoding="utf-8") as f:
                f.write(svg)

        # Apps icons (both symbolic and 64x64)
        apps_target = os.path.join(harmony_dir, "scalable", "apps")
        for name, svg in APPS_SYMBOLIC_ICONS.items():
            with open(os.path.join(apps_target, name), "w", encoding="utf-8") as f:
                f.write(svg)
        for name, svg in APP_ICONS.items():
            with open(os.path.join(apps_target, name), "w", encoding="utf-8") as f:
                f.write(svg)

        # 4. Write index.theme with explicit Inherits=Papirus,Adwaita,hicolor
        with open(os.path.join(harmony_dir, "index.theme"), "w", encoding="utf-8") as f:
            f.write(INDEX_THEME_CONTENT)
        # Also maintain top-level index.theme in base_dir
        with open(os.path.join(base_dir, "index.theme"), "w", encoding="utf-8") as f:
            f.write(INDEX_THEME_CONTENT)

    print("Generated semantic AxisHarmony icons:")
    print(f"  - status: {len(STATUS_ICONS)} icons")
    print(f"  - places: {len(PLACES_ICONS)} icons")
    print(f"  - actions: {len(ACTIONS_ICONS)} icons")
    print(f"  - apps: {len(APPS_SYMBOLIC_ICONS) + len(APP_ICONS)} icons")
    print("  - Inherits: Papirus,Adwaita,hicolor")

if __name__ == "__main__":
    generate_all_icons()
