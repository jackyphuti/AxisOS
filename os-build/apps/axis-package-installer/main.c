/*
 * AxisOS Package Installer & Universal App Store (Flatpak/Flathub + APT + .deb/.flatpakref)
 * 100% Pure C11 (-std=gnu11) with GTK 3
 */

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct {
    const char *id;         /* Flatpak App ID or APT package name */
    const char *name;
    const char *desc;
    const char *icon;
    const char *category;
    const char *source;     /* "Flatpak" or "APT" */
    gboolean installed;
} AppPackage;

static AppPackage CATALOG[] = {
    /* Browsers */
    {"org.mozilla.firefox",             "Firefox",             "Fast, private web browser from Mozilla",                  "firefox",               "Browsers",    "Flatpak", FALSE},
    {"com.brave.Browser",               "Brave Browser",       "Privacy-focused browser with built-in ad blocker",        "web-browser",           "Browsers",    "Flatpak", FALSE},
    {"com.google.Chrome",               "Google Chrome",       "Fast, secure web browser by Google",                      "web-browser",           "Browsers",    "Flatpak", FALSE},
    {"chromium",                        "Chromium",            "Open-source web browser project (native APT)",            "chromium",              "Browsers",    "APT",     FALSE},
    {"org.torproject.torbrowser-launcher", "Tor Browser",      "Private browsing over the Tor anonymity network",         "web-browser",           "Browsers",    "Flatpak", FALSE},

    /* Communication */
    {"com.discordapp.Discord",          "Discord",             "Voice, video, and text chat for gaming & communities",    "internet-chat",         "Social",      "Flatpak", FALSE},
    {"org.telegram.desktop",            "Telegram Desktop",    "Fast, cloud-based secure desktop messenger",              "internet-chat",         "Social",      "Flatpak", FALSE},
    {"com.rtosta.zapzap",               "WhatsApp (ZapZap)",   "Native Linux desktop client for WhatsApp Web",            "internet-chat",         "Social",      "Flatpak", FALSE},
    {"us.zoom.Zoom",                    "Zoom",                "HD video meetings, chat, and collaboration",              "camera-web",            "Social",      "Flatpak", FALSE},

    /* Gaming */
    {"com.valvesoftware.Steam",         "Steam",               "Valve's PC gaming store & Proton compatibility layer",    "applications-games",    "Gaming",      "Flatpak", FALSE},
    {"com.heroicgameslauncher.hgl",     "Heroic Games",        "Native launcher for Epic Games, GOG, and Amazon Games",   "applications-games",    "Gaming",      "Flatpak", FALSE},
    {"net.lutris.Lutris",               "Lutris",              "Open gaming platform for Linux, Wine & emulators",        "applications-games",    "Gaming",      "Flatpak", FALSE},
    {"com.usebottles.bottles",          "Bottles",             "Run Windows software and games in isolated prefixes",     "applications-games",    "Gaming",      "Flatpak", FALSE},
    {"net.davidotek.pupgui2",           "ProtonUp-Qt",         "Install GE-Proton and Wine-GE for Steam & Lutris",        "applications-games",    "Gaming",      "Flatpak", FALSE},
    {"org.libretro.RetroArch",          "RetroArch",           "All-in-one multi-system retro game emulator",             "applications-games",    "Gaming",      "Flatpak", FALSE},
    {"org.prismlauncher.PrismLauncher", "Prism Launcher",      "Custom open-source Minecraft launcher with mod support",  "applications-games",    "Gaming",      "Flatpak", FALSE},
    {"gamemode",                        "GameMode",            "On-demand CPU governor & kernel gaming optimizer",        "preferences-desktop-gaming", "Gaming", "APT",     FALSE},
    {"mangohud",                        "MangoHud",            "Vulkan/OpenGL real-time FPS, CPU & GPU HUD overlay",      "utilities-system-monitor",   "Gaming", "APT",     FALSE},

    /* Development */
    {"com.visualstudio.code",           "VS Code",             "Modern extensible code editor from Microsoft",            "applications-development", "Development", "Flatpak", FALSE},
    {"com.getpostman.Postman",          "Postman",             "API platform for building and testing REST/GraphQL APIs", "applications-development", "Development", "Flatpak", FALSE},
    {"io.github.shiftey.Desktop",       "GitHub Desktop",      "Visual Git & GitHub repository workflow client",          "applications-development", "Development", "Flatpak", FALSE},
    {"git",                             "Git",                 "Distributed version control system (native APT)",         "applications-development", "Development", "APT",     FALSE},
    {"build-essential",                 "Build Essential",     "GCC, G++, Make & C/C++ development toolchain",            "applications-development", "Development", "APT",     FALSE},
    {"nodejs",                          "Node.js & NPM",       "JavaScript runtime and package manager",                  "applications-development", "Development", "APT",     FALSE},
    {"neovim",                          "Neovim",              "Hyperextensible modern Vim-based text editor",            "utilities-terminal",       "Development", "APT",     FALSE},

    /* Media & Graphics */
    {"org.videolan.VLC",                "VLC Media Player",    "Plays virtually every audio and video format",            "applications-multimedia",  "Media",       "Flatpak", FALSE},
    {"com.obsproject.Studio",           "OBS Studio",          "Live streaming and high-FPS screen recording",            "applications-multimedia",  "Media",       "Flatpak", FALSE},
    {"com.spotify.Client",              "Spotify",             "Music and podcast streaming client",                      "applications-multimedia",  "Media",       "Flatpak", FALSE},
    {"org.gimp.GIMP",                   "GIMP",                "GNU Image Manipulation Program for photo editing",        "applications-graphics",    "Media",       "Flatpak", FALSE},
    {"org.blender.Blender",             "Blender",             "3D modeling, animation, VFX, and rendering suite",        "applications-graphics",    "Media",       "Flatpak", FALSE},
    {"org.kde.kdenlive",                "Kdenlive",            "Professional multi-track non-linear video editor",        "applications-multimedia",  "Media",       "Flatpak", FALSE},
    {"org.audacityteam.Audacity",       "Audacity",            "Multi-track audio editor and recorder",                   "applications-multimedia",  "Media",       "Flatpak", FALSE},
    {"org.inkscape.Inkscape",           "Inkscape",            "Professional SVG vector graphics editor",                 "applications-graphics",    "Media",       "Flatpak", FALSE},
    {"org.kde.krita",                   "Krita",               "Digital painting and illustration studio",                "applications-graphics",    "Media",       "Flatpak", FALSE},

    /* Office & Utilities */
    {"org.libreoffice.LibreOffice",     "LibreOffice",         "Full open-source office suite (Writer, Calc, Impress)",   "applications-office",      "Office",      "Flatpak", FALSE},
    {"org.onlyoffice.desktopeditors",   "ONLYOFFICE",          "MS Office-compatible document, sheet & slide editor",     "applications-office",      "Office",      "Flatpak", FALSE},
    {"com.bitwarden.desktop",           "Bitwarden",           "Open-source cross-platform password manager",             "dialog-password",          "System",      "Flatpak", FALSE},
    {"gparted",                         "GParted",             "Graphical disk & partition manager (native APT)",         "drive-harddisk",           "System",      "APT",     FALSE},
    {"btop",                            "btop++",              "Modern terminal resource monitor",                        "utilities-system-monitor", "System",      "APT",     FALSE},
    {"timeshift",                       "Timeshift",           "System restore snapshots (Btrfs / RSYNC)",                "system-software-update",   "System",      "APT",     FALSE},
    {NULL, NULL, NULL, NULL, NULL, NULL, FALSE}
};

/* Valid GTK 3 Dark CSS (Zero !important — 100% compliant) */
static const char *CSS_INSTALLER =
    "* { outline-style: none; font-family: 'IBM Plex Sans', sans-serif; }"
    "window, window.background, .background, box, paned, scrolledwindow, viewport {"
    "    background-color: #181C28;"
    "    background-image: none;"
    "    color: #F1F5F9;"
    "}"
    ".top-bar {"
    "    background-color: #121622;"
    "    border-bottom: 1px solid #262E45;"
    "    padding: 10px 14px;"
    "}"
    ".top-title {"
    "    font-size: 15px;"
    "    font-weight: 700;"
    "    color: #FFFFFF;"
    "}"
    "list, listbox {"
    "    background-color: #131722;"
    "    border-right: 1px solid #262E45;"
    "}"
    "list row {"
    "    background-color: transparent;"
    "    color: #CBD5E1;"
    "    border-radius: 6px;"
    "    margin: 2px 6px;"
    "}"
    "list row:selected {"
    "    background-color: #1F2A44;"
    "    color: #38BDF8;"
    "    font-weight: 700;"
    "}"
    ".app-card {"
    "    background-color: #1F2536;"
    "    border: 1px solid #2D3650;"
    "    border-radius: 10px;"
    "    padding: 12px;"
    "}"
    ".app-card:hover {"
    "    border-color: #38BDF8;"
    "}"
    ".app-title {"
    "    font-size: 13px;"
    "    font-weight: 700;"
    "    color: #FFFFFF;"
    "}"
    ".app-desc {"
    "    font-size: 11px;"
    "    color: #94A3B8;"
    "}"
    ".badge-flatpak {"
    "    background-color: #0E3A53;"
    "    color: #38BDF8;"
    "    border-radius: 4px;"
    "    padding: 2px 7px;"
    "    font-size: 10px;"
    "    font-weight: 700;"
    "}"
    ".badge-apt {"
    "    background-color: #27254D;"
    "    color: #A5B4FC;"
    "    border-radius: 4px;"
    "    padding: 2px 7px;"
    "    font-size: 10px;"
    "    font-weight: 700;"
    "}"
    "button {"
    "    background-color: #283149;"
    "    background-image: none;"
    "    color: #F1F5F9;"
    "    border: 1px solid #3A476A;"
    "    border-radius: 7px;"
    "    padding: 6px 14px;"
    "    font-size: 12px;"
    "    font-weight: 600;"
    "    box-shadow: none;"
    "}"
    "button label { color: #F1F5F9; }"
    "button:hover { background-color: #323E5C; border-color: #38BDF8; }"
    "button.btn-install {"
    "    background-color: #0284C7;"
    "    border-color: #38BDF8;"
    "    color: #FFFFFF;"
    "}"
    "button.btn-install label { color: #FFFFFF; }"
    "button.btn-install:hover { background-color: #0EA5E9; }"
    "button.btn-remove {"
    "    background-color: #4C1D24;"
    "    border-color: #F87171;"
    "    color: #FECACA;"
    "}"
    "button.btn-remove label { color: #FECACA; }"
    "button.btn-launch {"
    "    background-color: #14532D;"
    "    border-color: #34D399;"
    "    color: #D1FAE5;"
    "}"
    "button.btn-launch label { color: #D1FAE5; }"
    "entry {"
    "    background-color: #0F131E;"
    "    background-image: none;"
    "    color: #FFFFFF;"
    "    caret-color: #38BDF8;"
    "    border: 1px solid #334155;"
    "    border-radius: 7px;"
    "    padding: 6px 12px;"
    "}"
    "entry:focus { border-color: #38BDF8; }"
    "textview, textview text {"
    "    background-color: #0C101A;"
    "    color: #CBD5E1;"
    "    font-family: 'IBM Plex Mono', monospace;"
    "    font-size: 11px;"
    "}"
    "progressbar trough { background-color: #0F131E; border-radius: 4px; }"
    "progressbar progress { background-color: #38BDF8; border-radius: 4px; }"
    "expander title { color: #CBD5E1; font-weight: 600; }";

typedef struct {
    GtkWidget *window;
    GtkWidget *flowbox;
    GtkWidget *search_entry;
    GtkWidget *sidebar_list;
    GtkWidget *progress_bar;
    GtkWidget *log_view;
    GtkWidget *log_expander;
    GtkTextBuffer *log_buffer;
    GtkWidget *status_label;
    GtkWidget *install_card;
    GtkWidget *card_title;
    GtkWidget *card_desc;
    GtkWidget *card_action_btn;

    char *active_category;
    char *target_file;
    GHashTable *installed_set; /* pkg_id or flatpak_id -> GINT_TO_POINTER(1) */
    GPid child_pid;
} PkgInstallerApp;

static void refresh_installed_set(PkgInstallerApp *app) {
    if (!app->installed_set) {
        app->installed_set = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    } else {
        g_hash_table_remove_all(app->installed_set);
    }

    /* 1. Scan installed APT packages via dpkg-query */
    FILE *fp = popen("dpkg-query -W -f='${Status}\t${Package}\n' 2>/dev/null", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "install ok installed\t", 21) == 0) {
                char *pkg = g_strstrip(line + 21);
                if (pkg[0]) g_hash_table_insert(app->installed_set, g_strdup(pkg), GINT_TO_POINTER(1));
            }
        }
        pclose(fp);
    }

    /* 2. Scan installed Flatpak apps (both --system and --user) */
    fp = popen("flatpak list --app --columns=application 2>/dev/null", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            char *id = g_strstrip(line);
            if (id[0]) g_hash_table_insert(app->installed_set, g_strdup(id), GINT_TO_POINTER(1));
        }
        pclose(fp);
    }

    for (int i = 0; CATALOG[i].id != NULL; ++i) {
        CATALOG[i].installed = g_hash_table_contains(app->installed_set, CATALOG[i].id);
    }
}

static void append_log(PkgInstallerApp *app, const char *text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app->log_buffer, &end);
    gtk_text_buffer_insert(app->log_buffer, &end, text, -1);
    GtkTextMark *mark = gtk_text_buffer_get_insert(app->log_buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->log_view), mark, 0.0, FALSE, 0.0, 1.0);
}

static void rebuild_catalog_cards(PkgInstallerApp *app);

static gboolean on_read_child_output(GIOChannel *channel, GIOCondition cond, gpointer user_data) {
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    if (cond & G_IO_IN) {
        gchar buf[1024];
        gsize bytes_read = 0;
        GError *err = NULL;
        GIOStatus st = g_io_channel_read_chars(channel, buf, sizeof(buf) - 1, &bytes_read, &err);
        if (st == G_IO_STATUS_NORMAL && bytes_read > 0) {
            buf[bytes_read] = '\0';
            append_log(app, buf);
            gtk_progress_bar_pulse(GTK_PROGRESS_BAR(app->progress_bar));
        }
        if (err) g_error_free(err);
    }
    if (cond & (G_IO_HUP | G_IO_ERR)) return G_SOURCE_REMOVE;
    return G_SOURCE_CONTINUE;
}

static void on_child_watch(GPid pid, gint status, gpointer user_data) {
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    (void)pid;
    g_spawn_close_pid(app->child_pid);
    app->child_pid = 0;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 1.0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Completed successfully!");
        append_log(app, "\n[SUCCESS] Operation finished cleanly.\n");
    } else {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Operation finished with errors (expand log below).");
        append_log(app, "\n[ERROR] Command exited with a non-zero status.\n");
    }
    refresh_installed_set(app);
    rebuild_catalog_cards(app);
}

static void run_shell_job(PkgInstallerApp *app, const char *display_title, const char *shell_cmd) {
    if (app->child_pid > 0) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Another installation is already running — please wait.");
        return;
    }

    gtk_text_buffer_set_text(app->log_buffer, "", -1);
    gtk_expander_set_expanded(GTK_EXPANDER(app->log_expander), TRUE);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 0.1);

    char start_msg[1024];
    snprintf(start_msg, sizeof(start_msg), ">>> %s\n>>> Command: %s\n\n", display_title, shell_cmd);
    append_log(app, start_msg);
    gtk_label_set_text(GTK_LABEL(app->status_label), display_title);

    char wrapped[2048];
    snprintf(wrapped, sizeof(wrapped), "( %s ) 2>&1", shell_cmd);
    gchar *argv[] = {"/bin/bash", "-c", wrapped, NULL};

    GError *err = NULL;
    gint out_fd = -1;
    if (g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
                                 NULL, NULL, &app->child_pid, NULL, &out_fd, NULL, &err)) {
        GIOChannel *chan = g_io_channel_unix_new(out_fd);
        g_io_channel_set_encoding(chan, NULL, NULL);
        g_io_channel_set_flags(chan, G_IO_FLAG_NONBLOCK, NULL);
        g_io_add_watch(chan, G_IO_IN | G_IO_HUP | G_IO_ERR, on_read_child_output, app);
        g_io_channel_unref(chan);
        g_child_watch_add(app->child_pid, on_child_watch, app);
    } else if (err) {
        append_log(app, err->message);
        g_error_free(err);
    }
}

static void install_software(PkgInstallerApp *app, const char *id, const char *source, gboolean is_remove) {
    char cmd[1024];
    char title[256];
    char *quoted = g_shell_quote(id);

    if (g_str_has_suffix(id, ".deb")) {
        snprintf(title, sizeof(title), "Installing local .deb package: %s", id);
        snprintf(cmd, sizeof(cmd), "pkexec env DEBIAN_FRONTEND=noninteractive apt-get update && pkexec env DEBIAN_FRONTEND=noninteractive apt-get install -y --allow-downgrades %s", quoted);
    } else if (g_str_has_suffix(id, ".flatpakref") || g_str_has_suffix(id, ".flatpak")) {
        snprintf(title, sizeof(title), "Installing Flatpak bundle: %s", id);
        snprintf(cmd, sizeof(cmd), "flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo && flatpak install --user -y --noninteractive %s", quoted);
    } else if (g_str_has_suffix(id, ".axis")) {
        snprintf(title, sizeof(title), "Installing Axis package: %s", id);
        snprintf(cmd, sizeof(cmd), "pkexec /usr/local/bin/axis install %s", quoted);
    } else if (source && strcmp(source, "Flatpak") == 0) {
        if (is_remove) {
            snprintf(title, sizeof(title), "Removing Flatpak: %s", id);
            snprintf(cmd, sizeof(cmd), "flatpak uninstall --user -y --noninteractive %s 2>/dev/null || pkexec flatpak uninstall --system -y --noninteractive %s", quoted, quoted);
        } else {
            snprintf(title, sizeof(title), "Installing %s from Flathub (Flatpak)...", id);
            snprintf(cmd, sizeof(cmd),
                     "if ! command -v flatpak >/dev/null 2>&1; then pkexec env DEBIAN_FRONTEND=noninteractive apt-get update && pkexec env DEBIAN_FRONTEND=noninteractive apt-get install -y flatpak; fi && "
                     "flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo && "
                     "flatpak install --user -y --noninteractive flathub %s", quoted);
        }
    } else {
        if (is_remove) {
            snprintf(title, sizeof(title), "Removing APT package: %s", id);
            snprintf(cmd, sizeof(cmd), "pkexec env DEBIAN_FRONTEND=noninteractive apt-get remove -y %s", quoted);
        } else {
            snprintf(title, sizeof(title), "Installing %s via APT...", id);
            snprintf(cmd, sizeof(cmd), "pkexec env DEBIAN_FRONTEND=noninteractive apt-get update && pkexec env DEBIAN_FRONTEND=noninteractive apt-get install -y %s", quoted);
        }
    }

    g_free(quoted);
    run_shell_job(app, title, cmd);
}

static void on_card_install_btn(GtkButton *btn, gpointer user_data) {
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    const char *id = (const char *)g_object_get_data(G_OBJECT(btn), "pkg_id");
    const char *src = (const char *)g_object_get_data(G_OBJECT(btn), "pkg_src");
    gboolean remove = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "pkg_remove"));
    if (id) install_software(app, id, src, remove);
}

static void on_card_launch_btn(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    const char *id = (const char *)g_object_get_data(G_OBJECT(btn), "pkg_id");
    const char *src = (const char *)g_object_get_data(G_OBJECT(btn), "pkg_src");
    if (!id) return;
    char cmd[512];
    if (src && strcmp(src, "Flatpak") == 0) {
        snprintf(cmd, sizeof(cmd), "flatpak run %s", id);
    } else {
        snprintf(cmd, sizeof(cmd), "%s", id);
    }
    g_spawn_command_line_async(cmd, NULL);
}

static GtkWidget *create_app_card(PkgInstallerApp *app, const char *id, const char *name,
                                  const char *desc, const char *icon_name, const char *source,
                                  gboolean installed) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "app-card");
    gtk_widget_set_size_request(card, 250, 145);

    GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name && icon_name[0] ? icon_name : "package-x-generic", GTK_ICON_SIZE_DND);
    GtkWidget *lbl_name = gtk_label_new(name);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_name), "app-title");
    gtk_widget_set_halign(lbl_name, GTK_ALIGN_START);
    gtk_label_set_ellipsize(GTK_LABEL(lbl_name), PANGO_ELLIPSIZE_END);

    GtkWidget *badge = gtk_label_new(source);
    gtk_style_context_add_class(gtk_widget_get_style_context(badge),
                                strcmp(source, "Flatpak") == 0 ? "badge-flatpak" : "badge-apt");

    gtk_box_pack_start(GTK_BOX(hdr), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hdr), lbl_name, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(hdr), badge, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), hdr, FALSE, FALSE, 0);

    GtkWidget *lbl_desc = gtk_label_new(desc && desc[0] ? desc : id);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_desc), "app-desc");
    gtk_label_set_line_wrap(GTK_LABEL(lbl_desc), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(lbl_desc), 30);
    gtk_label_set_lines(GTK_LABEL(lbl_desc), 2);
    gtk_label_set_ellipsize(GTK_LABEL(lbl_desc), PANGO_ELLIPSIZE_END);
    gtk_widget_set_halign(lbl_desc, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(card), lbl_desc, TRUE, TRUE, 2);

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    if (installed) {
        GtkWidget *btn_launch = gtk_button_new_with_label("Launch");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_launch), "btn-launch");
        g_object_set_data_full(G_OBJECT(btn_launch), "pkg_id", g_strdup(id), g_free);
        g_object_set_data_full(G_OBJECT(btn_launch), "pkg_src", g_strdup(source), g_free);
        g_signal_connect(btn_launch, "clicked", G_CALLBACK(on_card_launch_btn), app);
        gtk_box_pack_start(GTK_BOX(btn_row), btn_launch, TRUE, TRUE, 0);

        GtkWidget *btn_rem = gtk_button_new_with_label("Remove");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_rem), "btn-remove");
        g_object_set_data_full(G_OBJECT(btn_rem), "pkg_id", g_strdup(id), g_free);
        g_object_set_data_full(G_OBJECT(btn_rem), "pkg_src", g_strdup(source), g_free);
        g_object_set_data(G_OBJECT(btn_rem), "pkg_remove", GINT_TO_POINTER(1));
        g_signal_connect(btn_rem, "clicked", G_CALLBACK(on_card_install_btn), app);
        gtk_box_pack_start(GTK_BOX(btn_row), btn_rem, TRUE, TRUE, 0);
    } else {
        GtkWidget *btn_inst = gtk_button_new_with_label("Install");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_inst), "btn-install");
        g_object_set_data_full(G_OBJECT(btn_inst), "pkg_id", g_strdup(id), g_free);
        g_object_set_data_full(G_OBJECT(btn_inst), "pkg_src", g_strdup(source), g_free);
        g_object_set_data(G_OBJECT(btn_inst), "pkg_remove", GINT_TO_POINTER(0));
        g_signal_connect(btn_inst, "clicked", G_CALLBACK(on_card_install_btn), app);
        gtk_box_pack_start(GTK_BOX(btn_row), btn_inst, TRUE, TRUE, 0);
    }
    gtk_box_pack_end(GTK_BOX(card), btn_row, FALSE, FALSE, 0);
    return card;
}

static void clear_flowbox(PkgInstallerApp *app) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->flowbox));
    for (GList *l = children; l != NULL; l = l->next) {
        gtk_widget_destroy(GTK_WIDGET(l->data));
    }
    g_list_free(children);
}

static void rebuild_catalog_cards(PkgInstallerApp *app) {
    clear_flowbox(app);
    const gchar *q = gtk_entry_get_text(GTK_ENTRY(app->search_entry));

    for (int i = 0; CATALOG[i].id != NULL; ++i) {
        AppPackage *pkg = &CATALOG[i];
        if (app->active_category && strcmp(app->active_category, "All") != 0 &&
            strcmp(app->active_category, "Installed") != 0 &&
            strcmp(pkg->category, app->active_category) != 0) {
            continue;
        }
        if (app->active_category && strcmp(app->active_category, "Installed") == 0 && !pkg->installed) {
            continue;
        }
        if (q && *q) {
            if (!g_str_match_string(q, pkg->name, TRUE) &&
                !g_str_match_string(q, pkg->desc, TRUE) &&
                !g_str_match_string(q, pkg->id, TRUE)) {
                continue;
            }
        }
        GtkWidget *card = create_app_card(app, pkg->id, pkg->name, pkg->desc, pkg->icon, pkg->source, pkg->installed);
        gtk_container_add(GTK_CONTAINER(app->flowbox), card);
    }
    gtk_widget_show_all(app->flowbox);
}

/* Live Universal Search across ALL of Flathub (Flatpak) + Debian APT */
static void on_live_search_clicked(GtkWidget *w, gpointer user_data) {
    (void)w;
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    const gchar *q = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
    if (!q || strlen(q) < 2) {
        rebuild_catalog_cards(app);
        return;
    }

    clear_flowbox(app);
    gtk_label_set_text(GTK_LABEL(app->status_label), "Searching Flathub (Flatpak) & Debian APT repositories...");
    while (gtk_events_pending()) gtk_main_iteration();

    char *quoted = g_shell_quote(q);
    int found = 0;

    /* 1. Search Flathub live */
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo 2>/dev/null; "
             "flatpak search --columns=name,application,description %s 2>/dev/null | head -n 30", quoted);
    FILE *fp = popen(cmd, "r");
    if (fp) {
        char line[1024];
        while (fgets(line, sizeof(line), fp)) {
            g_strstrip(line);
            if (!line[0] || strncmp(line, "No matches", 10) == 0) continue;
            gchar **parts = g_strsplit(line, "\t", 3);
            if (parts[0] && parts[1] && parts[1][0]) {
                const char *fname = parts[0];
                const char *fid = parts[1];
                const char *fdesc = parts[2] ? parts[2] : "Flathub Flatpak application";
                gboolean inst = app->installed_set ? g_hash_table_contains(app->installed_set, fid) : FALSE;
                GtkWidget *card = create_app_card(app, fid, fname, fdesc, "package-x-generic", "Flatpak", inst);
                gtk_container_add(GTK_CONTAINER(app->flowbox), card);
                found++;
            }
            g_strfreev(parts);
        }
        pclose(fp);
    }

    /* 2. Search APT live */
    snprintf(cmd, sizeof(cmd), "apt-cache search %s 2>/dev/null | head -n 20", quoted);
    fp = popen(cmd, "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            g_strstrip(line);
            char *sep = strstr(line, " - ");
            if (sep) {
                *sep = '\0';
                const char *pkg = line;
                const char *desc = sep + 3;
                gboolean inst = app->installed_set ? g_hash_table_contains(app->installed_set, pkg) : FALSE;
                GtkWidget *card = create_app_card(app, pkg, pkg, desc, "system-software-install", "APT", inst);
                gtk_container_add(GTK_CONTAINER(app->flowbox), card);
                found++;
            }
        }
        pclose(fp);
    }
    g_free(quoted);

    gtk_widget_show_all(app->flowbox);
    char msg[128];
    snprintf(msg, sizeof(msg), "Found %d matching packages across Flathub (Flatpak) & APT.", found);
    gtk_label_set_text(GTK_LABEL(app->status_label), msg);
}

static void on_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    (void)entry;
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    rebuild_catalog_cards(app);
}

static void on_category_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)box;
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    if (!row) return;
    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
    g_free(app->active_category);
    app->active_category = g_strdup(gtk_label_get_text(GTK_LABEL(label)));
    rebuild_catalog_cards(app);
}

static void on_open_file_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open Package File",
        GTK_WINDOW(app->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL
    );

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "All Supported Packages (*.deb, *.flatpakref, *.flatpak, *.axis)");
    gtk_file_filter_add_pattern(filter, "*.deb");
    gtk_file_filter_add_pattern(filter, "*.flatpakref");
    gtk_file_filter_add_pattern(filter, "*.flatpak");
    gtk_file_filter_add_pattern(filter, "*.axis");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            g_free(app->target_file);
            app->target_file = filename;
            char *base = g_path_get_basename(filename);
            gtk_label_set_text(GTK_LABEL(app->card_title), base);
            g_free(base);
            gtk_label_set_text(GTK_LABEL(app->card_desc), filename);
            gtk_widget_set_no_show_all(app->install_card, FALSE);
            gtk_widget_show_all(app->install_card);
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_card_action_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    if (app->target_file) {
        install_software(app, app->target_file, "Local", FALSE);
    }
}

static void load_axis_theme(void) {
    GdkScreen *screen = gdk_screen_get_default();
    if (!screen) return;
    GtkCssProvider *provider = gtk_css_provider_new();
    GError *err = NULL;
    if (!gtk_css_provider_load_from_data(provider, CSS_INSTALLER, -1, &err)) {
        if (err) {
            fprintf(stderr, "[axis-package-installer] CSS error: %s\n", err->message);
            g_error_free(err);
        }
    }
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("AxisOS Universal Package Installer (Flatpak/Flathub + APT, Pure C11)\n");
        printf("Usage: axis-package-installer [PACKAGE_FILE | --catalog | --verify-css | --help]\n");
        return 0;
    }
    if (argc > 1 && strcmp(argv[1], "--verify-css") == 0) {
        GtkCssProvider *p = gtk_css_provider_new();
        GError *err = NULL;
        if (!gtk_css_provider_load_from_data(p, CSS_INSTALLER, -1, &err)) {
            fprintf(stderr, "CSS_FAIL: %s\n", err ? err->message : "unknown");
            return 1;
        }
        printf("INSTALLER_CSS_OK: %zu bytes parsed with 0 errors\n", strlen(CSS_INSTALLER));
        g_object_unref(p);
        return 0;
    }

    PkgInstallerApp app = {0};
    refresh_installed_set(&app);
    if (argc > 1 && (strcmp(argv[1], "--catalog") == 0 || strcmp(argv[1], "-c") == 0)) {
        printf("=== AxisOS Universal Software Catalog (Flathub + APT) ===\n");
        for (int i = 0; CATALOG[i].id != NULL; ++i) {
            printf("  [%-7s] %-22s %-34s [%s]\n",
                   CATALOG[i].source, CATALOG[i].name, CATALOG[i].id,
                   CATALOG[i].installed ? "Installed" : "Available");
        }
        return 0;
    }

    gtk_init(&argc, &argv);
    GtkSettings *default_settings = gtk_settings_get_default();
    if (default_settings) {
        g_object_set(default_settings,
                     "gtk-application-prefer-dark-theme", TRUE,
                     "gtk-theme-name", "Adwaita-dark",
                     "gtk-icon-theme-name", "Papirus-Dark",
                     NULL);
    }
    load_axis_theme();

    app.active_category = g_strdup("All");
    if (argc > 1 && argv[1][0] != '-') {
        app.target_file = g_strdup(argv[1]);
    }

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app.window), 1040, 660);
    gtk_window_set_position(GTK_WINDOW(app.window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(app.window), "AxisOS Package Installer (Flathub + APT)");
    gtk_window_set_icon_name(GTK_WINDOW(app.window), "system-software-install");

    GtkWidget *root_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app.window), root_vbox);

    /* Sleek Top Command Bar */
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_style_context_add_class(gtk_widget_get_style_context(top_bar), "top-bar");

    GtkWidget *top_icon = gtk_image_new_from_icon_name("system-software-install", GTK_ICON_SIZE_LARGE_TOOLBAR);
    GtkWidget *top_title = gtk_label_new("AxisOS App Store & Package Installer");
    gtk_style_context_add_class(gtk_widget_get_style_context(top_title), "top-title");
    gtk_box_pack_start(GTK_BOX(top_bar), top_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(top_bar), top_title, FALSE, FALSE, 0);

    app.search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app.search_entry), "Search catalog or press Enter to search ALL of Flathub & APT...");
    g_signal_connect(app.search_entry, "search-changed", G_CALLBACK(on_search_changed), &app);
    g_signal_connect(app.search_entry, "activate", G_CALLBACK(on_live_search_clicked), &app);
    gtk_box_pack_start(GTK_BOX(top_bar), app.search_entry, TRUE, TRUE, 4);

    GtkWidget *btn_flathub = gtk_button_new_with_label("Search Flathub & APT");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_flathub), "btn-install");
    g_signal_connect(btn_flathub, "clicked", G_CALLBACK(on_live_search_clicked), &app);
    gtk_box_pack_start(GTK_BOX(top_bar), btn_flathub, FALSE, FALSE, 0);

    GtkWidget *open_btn = gtk_button_new_with_label("Open File (.deb/.flatpakref)");
    g_signal_connect(open_btn, "clicked", G_CALLBACK(on_open_file_clicked), &app);
    gtk_box_pack_end(GTK_BOX(top_bar), open_btn, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root_vbox), top_bar, FALSE, FALSE, 0);

    /* Main Split Area */
    GtkWidget *main_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(main_paned), 175);
    gtk_box_pack_start(GTK_BOX(root_vbox), main_paned, TRUE, TRUE, 0);

    /* Left Category Sidebar */
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new(NULL, NULL);
    app.sidebar_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app.sidebar_list), GTK_SELECTION_SINGLE);
    g_signal_connect(app.sidebar_list, "row-selected", G_CALLBACK(on_category_selected), &app);

    const char *cats[] = {"All", "Browsers", "Social", "Gaming", "Development", "Media", "Office", "System", "Installed", NULL};
    for (int i = 0; cats[i] != NULL; ++i) {
        GtkWidget *row_lbl = gtk_label_new(cats[i]);
        gtk_widget_set_halign(row_lbl, GTK_ALIGN_START);
        gtk_widget_set_margin_start(row_lbl, 12);
        gtk_widget_set_margin_top(row_lbl, 9);
        gtk_widget_set_margin_bottom(row_lbl, 9);
        gtk_list_box_insert(GTK_LIST_BOX(app.sidebar_list), row_lbl, -1);
    }
    gtk_container_add(GTK_CONTAINER(sidebar_scroll), app.sidebar_list);
    gtk_paned_pack1(GTK_PANED(main_paned), sidebar_scroll, FALSE, FALSE);

    /* Right Content Area */
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(content_box), 12);
    gtk_paned_pack2(GTK_PANED(main_paned), content_box, TRUE, FALSE);

    /* Local Package File Card */
    app.install_card = gtk_frame_new("Selected Package File");
    GtkWidget *card_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(card_box), 10);

    GtkWidget *c_info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    char *init_base = app.target_file ? g_path_get_basename(app.target_file) : g_strdup("");
    app.card_title = gtk_label_new(init_base);
    g_free(init_base);
    gtk_widget_set_halign(app.card_title, GTK_ALIGN_START);
    app.card_desc = gtk_label_new(app.target_file ? app.target_file : "");
    gtk_widget_set_halign(app.card_desc, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(c_info), app.card_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(c_info), app.card_desc, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_box), c_info, TRUE, TRUE, 0);

    app.card_action_btn = gtk_button_new_with_label("Install Package File");
    gtk_style_context_add_class(gtk_widget_get_style_context(app.card_action_btn), "btn-install");
    g_signal_connect(app.card_action_btn, "clicked", G_CALLBACK(on_card_action_clicked), &app);
    gtk_box_pack_end(GTK_BOX(card_box), app.card_action_btn, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(app.install_card), card_box);
    gtk_box_pack_start(GTK_BOX(content_box), app.install_card, FALSE, FALSE, 4);

    if (!app.target_file) {
        gtk_widget_set_no_show_all(app.install_card, TRUE);
        gtk_widget_hide(app.install_card);
    }

    /* App Grid */
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    app.flowbox = gtk_flow_box_new();
    gtk_widget_set_valign(app.flowbox, GTK_ALIGN_START);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(app.flowbox), 10);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(app.flowbox), 10);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(app.flowbox), 3);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(app.flowbox), GTK_SELECTION_NONE);
    gtk_container_add(GTK_CONTAINER(scroll), app.flowbox);
    rebuild_catalog_cards(&app);
    gtk_box_pack_start(GTK_BOX(content_box), scroll, TRUE, TRUE, 4);

    /* Status & Live Log */
    app.status_label = gtk_label_new("Ready — Flatpak (Flathub) & Debian APT enabled. Type any app name and press Enter to search all software.");
    gtk_widget_set_halign(app.status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content_box), app.status_label, FALSE, FALSE, 2);

    app.progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(content_box), app.progress_bar, FALSE, FALSE, 2);

    app.log_expander = gtk_expander_new("Installation Output Log");
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(log_scroll, -1, 140);
    app.log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.log_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app.log_view), TRUE);
    app.log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app.log_view));
    gtk_container_add(GTK_CONTAINER(log_scroll), app.log_view);
    gtk_container_add(GTK_CONTAINER(app.log_expander), log_scroll);
    gtk_box_pack_start(GTK_BOX(content_box), app.log_expander, FALSE, FALSE, 4);

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(app.window);
    gtk_main();

    g_free(app.active_category);
    g_free(app.target_file);
    if (app.installed_set) g_hash_table_destroy(app.installed_set);
    return 0;
}
