// AxisOS High-Performance & Liquid Nodes Optimizations
// Hardware Acceleration & Wayland
pref("widget.use-xdg-desktop-portal.mime-handler", 1);
pref("widget.use-xdg-desktop-portal.file-picker", 1);
pref("widget.dmabuf.force-enabled", true);
pref("gfx.webrender.all", true);
pref("gfx.webrender.compositor", true);
pref("media.ffmpeg.vaapi.enabled", true);
pref("media.rdd-ffmpeg.enabled", true);
pref("media.hardware-video-decoding.force-enabled", true);

// Fast Memory & zRAM Caching
pref("browser.cache.disk.enable", false);
pref("browser.cache.memory.enable", true);
pref("browser.cache.memory.capacity", 524288); // 512MB RAM cache
pref("browser.sessionstore.interval", 60000); // Reduce disk writes

// UI & Liquid Nodes Dark Theme
pref("ui.systemUsesDarkTheme", 1);
pref("browser.theme.dark-private-windows", true);
pref("toolkit.legacyUserProfileCustomizations.stylesheets", true);
pref("svg.context-properties.content.enabled", true);

// Privacy & Low-Latency
pref("datareporting.policy.dataSubmissionEnabled", false);
pref("datareporting.healthreport.uploadEnabled", false);
pref("toolkit.telemetry.unified", false);
pref("toolkit.telemetry.enabled", false);
pref("experiments.activeExperiment", false);
pref("experiments.supported", false);
pref("network.dns.disablePrefetch", true);
pref("network.prefetch-next", false);
