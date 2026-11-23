// =============================================================================
// AxisOS Obsidian Signature KDE Plasma 5.27 Desktop Layout
// Dual-Zone Architecture:
//   1. Floating Top Command-HUD Bar (AxisOS Launcher, App Menu, Workspaces,
//      Centered Date/Time Pill, Live CPU/RAM Telemetry, System Tray, Search)
//   2. Centered Floating Glass App Dock (Auto-tucks behind maximized windows,
//      pinned AxisOS core apps + Trash)
// =============================================================================

var screenGeo = screenGeometry(0);

// -----------------------------------------------------------------------------
// 1. TOP FLOATING COMMAND HUD BAR
// -----------------------------------------------------------------------------
var topBar = new Panel;
topBar.location = "top";
topBar.height = 34;
topBar.floating = true;
topBar.hiding = "none";

// Left: AxisOS Launcher
var kickoff = topBar.addWidget("org.kde.plasma.kickoff");
kickoff.currentConfigGroup = ["Shortcuts"];
kickoff.writeConfig("global", "Alt+F1");
kickoff.currentConfigGroup = ["General"];
kickoff.writeConfig("icon", "axisos");
kickoff.writeConfig("favoritesPortedToKAstats", "true");
kickoff.writeConfig("showActionButtonCaptions", "false");
kickoff.writeConfig("alphaSort", "false");

// Left: Global Application Menu (HUD integration)
var appmenu = topBar.addWidget("org.kde.plasma.appmenu");
appmenu.currentConfigGroup = ["Appearance"];
appmenu.writeConfig("compactView", "false");

// Left: Compact Workspace Pills
var pager = topBar.addWidget("org.kde.plasma.pager");
pager.currentConfigGroup = ["General"];
pager.writeConfig("showWindowOutlines", "true");
pager.writeConfig("showWindowIcons", "true");
pager.writeConfig("displayedText", "None");

// Flexible Left Spacer -> centers clock perfectly
topBar.addWidget("org.kde.plasma.panelspacer");

// Center: Crisp AxisOS Date & Time Pill
var clock = topBar.addWidget("org.kde.plasma.digitalclock");
clock.currentConfigGroup = ["Appearance"];
clock.writeConfig("showDate", "true");
clock.writeConfig("dateDisplayFormat", "BesideTime");
clock.writeConfig("dateFormat", "custom");
clock.writeConfig("customDateFormat", "ddd d MMM  •");
clock.writeConfig("showSeconds", "false");
clock.writeConfig("use24hFormat", "2");
clock.writeConfig("autoFontAndSize", "false");
clock.writeConfig("fontFamily", "IBM Plex Sans");
clock.writeConfig("boldText", "true");
clock.writeConfig("fontSize", "10");

// Flexible Right Spacer
topBar.addWidget("org.kde.plasma.panelspacer");

// Right: Live Compact Hardware Telemetry (CPU & RAM gauges in top HUD)
var cpuMon = topBar.addWidget("org.kde.plasma.systemmonitor.cpu");
cpuMon.currentConfigGroup = ["Appearance"];
cpuMon.writeConfig("title", "CPU");
cpuMon.writeConfig("chartFace", "org.kde.ksysguard.piechart");

var memMon = topBar.addWidget("org.kde.plasma.systemmonitor.memory");
memMon.currentConfigGroup = ["Appearance"];
memMon.writeConfig("title", "RAM");
memMon.writeConfig("chartFace", "org.kde.ksysguard.piechart");

// Right: Media Controls + System Tray + Spotlight Search + Lock/Power
topBar.addWidget("org.kde.plasma.mediacontroller");
topBar.addWidget("org.kde.plasma.systemtray");
topBar.addWidget("org.kde.milou");

var lockLogout = topBar.addWidget("org.kde.plasma.lock_logout");
lockLogout.currentConfigGroup = ["General"];
lockLogout.writeConfig("show_lockScreen", "true");
lockLogout.writeConfig("show_requestShutDown", "true");

// -----------------------------------------------------------------------------
// 2. BOTTOM CENTERED FLOATING GLASS CAPSULE DOCK
// -----------------------------------------------------------------------------
var dock = new Panel;
dock.location = "bottom";
dock.height = 54;
dock.floating = true;
dock.alignment = "center";
dock.hiding = "windowscover";
dock.minimumLength = 340;
dock.maximumLength = Math.max(640, Math.floor(screenGeo.width * 0.72));

var icontasks = dock.addWidget("org.kde.plasma.icontasks");
icontasks.currentConfigGroup = ["General"];
icontasks.writeConfig("launchers", [
    "applications:axis-welcome.desktop",
    "applications:org.kde.dolphin.desktop",
    "applications:chromium.desktop",
    "applications:axis-terminal.desktop",
    "applications:axis-task-manager.desktop",
    "applications:nobara-welcome.desktop",
    "applications:systemsettings.desktop",
    "applications:install-axisos.desktop"
].join(","));
icontasks.writeConfig("groupingStrategy", "1");
icontasks.writeConfig("middleClickAction", "2");
icontasks.writeConfig("indicateAudioStreams", "true");
icontasks.writeConfig("iconSpacing", "2");
icontasks.writeConfig("maxStripes", "1");
icontasks.writeConfig("showOnlyCurrentDesktop", "false");

dock.addWidget("org.kde.plasma.marginsseparator");
dock.addWidget("org.kde.plasma.trash");

// -----------------------------------------------------------------------------
// 3. DESKTOP WALLPAPER & CONTAINMENT
// -----------------------------------------------------------------------------
var desktopsArray = desktopsForActivity(currentActivity());
for (var j = 0; j < desktopsArray.length; j++) {
    desktopsArray[j].wallpaperPlugin = "org.kde.image";
    desktopsArray[j].currentConfigGroup = ["Wallpaper", "org.kde.image", "General"];
    desktopsArray[j].writeConfig("Image", "file:///usr/share/backgrounds/axisos/08-axisos-obsidian-wave.jpg");
    desktopsArray[j].writeConfig("FillMode", "2");
}
