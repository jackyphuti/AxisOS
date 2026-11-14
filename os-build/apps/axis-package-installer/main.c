#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    const char *id;
    const char *name;
    const char *desc;
    const char *icon;
    const char *category;
    gboolean installed;
} AppPackage;

static AppPackage CATALOG[] = {
    {"firefox", "Firefox", "Fast, private web browser from Mozilla", "firefox", "Browsers", FALSE},
    {"chromium", "Chromium", "Open-source web browser engine behind Chrome", "chromium", "Browsers", FALSE},
    {"brave-browser", "Brave", "Privacy-focused browser with ad-blocking built in", "brave-browser", "Browsers", FALSE},
    {"torbrowser-launcher", "Tor Browser", "Anonymity and censorship evasion network browser", "tor-browser", "Browsers", FALSE},

    {"steam", "Steam", "Valve's premier digital gaming distribution platform", "steam", "Gaming", FALSE},
    {"lutris", "Lutris", "Open gaming platform for Windows, Epic, and GOG titles", "lutris", "Gaming", FALSE},
    {"heroic", "Heroic Games", "Native launcher for Epic Games and GOG", "heroic", "Gaming", FALSE},
    {"gamemode", "GameMode", "Daemon optimizing system performance on demand", "preferences-system", "Gaming", FALSE},
    {"mangohud", "MangoHud", "Vulkan and OpenGL overlay for monitoring FPS and temps", "utilities-system-monitor", "Gaming", FALSE},
    {"wine", "Wine", "Compatibility layer capable of running Windows apps", "wine", "Gaming", FALSE},

    {"code", "VS Code", "Extensible code editor from Microsoft", "visual-studio-code", "Development", FALSE},
    {"git", "Git", "Fast, distributed version control system", "git", "Development", FALSE},
    {"build-essential", "Build Essential", "GCC, Make, and standard development tools", "applications-development", "Development", FALSE},
    {"nodejs", "Node.js & NPM", "Asynchronous event-driven JavaScript runtime", "nodejs", "Development", FALSE},
    {"clang", "LLVM Clang", "C/C++ compiler toolchain", "applications-development", "Development", FALSE},

    {"vlc", "VLC Media Player", "Versatile multimedia player for virtually all audio/video formats", "vlc", "Media", FALSE},
    {"obs-studio", "OBS Studio", "Free and open source software for video recording and live streaming", "obs", "Media", FALSE},
    {"audacity", "Audacity", "Multi-track audio editor and recorder", "audacity", "Media", FALSE},
    {"kdenlive", "Kdenlive", "Powerful non-linear video editing suite", "kdenlive", "Media", FALSE},
    {"gimp", "GIMP", "GNU Image Manipulation Program for photo retouching", "gimp", "Media", FALSE},

    {"gparted", "GParted", "Graphical partition editor for disks and drives", "gparted", "System", FALSE},
    {"btop", "btop++", "Resource monitor with responsive terminal UI", "utilities-system-monitor", "System", FALSE},
    {"neofetch", "Neofetch", "CLI system information tool", "utilities-terminal", "System", FALSE},
    {"timeshift", "Timeshift", "System restore tool using snapshots", "system-software-update", "System", FALSE},

    {"libreoffice", "LibreOffice Suite", "Comprehensive open-source office productivity suite", "libreoffice-main", "Office", FALSE},
    {NULL, NULL, NULL, NULL, NULL, FALSE}
};

typedef struct {
    GtkWidget *window;
    GtkWidget *flowbox;
    GtkWidget *search_entry;
    GtkWidget *sidebar_list;
    GtkWidget *progress_bar;
    GtkWidget *log_view;
    GtkTextBuffer *log_buffer;
    GtkWidget *status_label;
    GtkWidget *install_card;
    GtkWidget *card_title;
    GtkWidget *card_desc;
    GtkWidget *card_action_btn;

    char *active_category;
    char *target_file;
    GPid child_pid;
    gint child_stdout_fd;
} PkgInstallerApp;

static void check_installed_status(void) {
    for (int i = 0; CATALOG[i].id != NULL; ++i) {
        char path[256];
        snprintf(path, sizeof(path), "/usr/bin/%s", CATALOG[i].id);
        CATALOG[i].installed = (access(path, X_OK) == 0);
    }
}

static void append_log(PkgInstallerApp *app, const char *text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app->log_buffer, &end);
    gtk_text_buffer_insert(app->log_buffer, &end, text, -1);

    GtkTextMark *mark = gtk_text_buffer_get_insert(app->log_buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->log_view), mark, 0.0, FALSE, 0.0, 1.0);
}

static gboolean on_read_child_output(GIOChannel *channel, GIOCondition cond, gpointer user_data) {
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    if (cond & G_IO_IN) {
        gchar buf[512];
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

    if (cond & (G_IO_HUP | G_IO_ERR)) {
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static void on_child_watch(GPid pid, gint status, gpointer user_data) {
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    (void)pid;
    g_spawn_close_pid(app->child_pid);
    app->child_pid = 0;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 1.0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Installation completed successfully!");
        append_log(app, "\n[SUCCESS] Package installation complete.\n");
    } else {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Installation encountered an error.");
        append_log(app, "\n[ERROR] Package installation returned an error.\n");
    }

    check_installed_status();
}

static void run_install_command(PkgInstallerApp *app, const char *pkg_or_file) {
    char cmd[512];
    if (g_str_has_suffix(pkg_or_file, ".deb")) {
        snprintf(cmd, sizeof(cmd), "pkexec apt-get install -y --allow-downgrades '%s'", pkg_or_file);
    } else if (g_str_has_suffix(pkg_or_file, ".axis")) {
        snprintf(cmd, sizeof(cmd), "pkexec axis install '%s'", pkg_or_file);
    } else {
        snprintf(cmd, sizeof(cmd), "pkexec apt-get install -y '%s' || pkexec axis install '%s'", pkg_or_file, pkg_or_file);
    }

    gtk_text_buffer_set_text(app->log_buffer, "", -1);
    char start_msg[1024];
    snprintf(start_msg, sizeof(start_msg), "Starting installation of: %s\nExecuting: %s\n\n", pkg_or_file, cmd);
    append_log(app, start_msg);
    gtk_label_set_text(GTK_LABEL(app->status_label), "Installing package...");

    gchar **argv = NULL;
    GError *err = NULL;
    if (!g_shell_parse_argv(cmd, NULL, &argv, &err)) {
        if (err) {
            append_log(app, err->message);
            g_error_free(err);
        }
        return;
    }

    gint out_fd = -1;
    if (g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
                                 NULL, NULL, &app->child_pid, NULL, &out_fd, NULL, &err)) {
        GIOChannel *chan = g_io_channel_unix_new(out_fd);
        g_io_channel_set_encoding(chan, NULL, NULL);
        g_io_channel_set_flags(chan, G_IO_FLAG_NONBLOCK, NULL);
        g_io_add_watch(chan, G_IO_IN | G_IO_HUP | G_IO_ERR, on_read_child_output, app);
        g_io_channel_unref(chan);

        g_child_watch_add(app->child_pid, on_child_watch, app);
    } else {
        if (err) {
            append_log(app, err->message);
            g_error_free(err);
        }
    }
    g_strfreev(argv);
}

static void on_install_pkg_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    PkgInstallerApp *app = (PkgInstallerApp *)g_object_get_data(G_OBJECT(btn), "app");
    const char *id = (const char *)user_data;
    if (app && id) {
        run_install_command(app, id);
    }
}

static void filter_catalog(PkgInstallerApp *app) {
    const gchar *q = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->flowbox));

    for (GList *l = children; l != NULL; l = l->next) {
        GtkWidget *child = GTK_WIDGET(l->data);
        const char *name = (const char *)g_object_get_data(G_OBJECT(child), "name");
        const char *desc = (const char *)g_object_get_data(G_OBJECT(child), "desc");
        const char *cat = (const char *)g_object_get_data(G_OBJECT(child), "cat");

        gboolean match = TRUE;
        if (q && *q) {
            if (!g_str_match_string(q, name, TRUE) && !g_str_match_string(q, desc, TRUE)) {
                match = FALSE;
            }
        }

        if (match && app->active_category && strcmp(app->active_category, "All") != 0) {
            if (strcmp(cat, app->active_category) != 0) {
                match = FALSE;
            }
        }

        gtk_widget_set_visible(child, match);
    }
    g_list_free(children);
}

static void on_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    (void)entry;
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    filter_catalog(app);
}

static void on_category_selected(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)box;
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    if (!row) return;

    GtkWidget *label = gtk_bin_get_child(GTK_BIN(row));
    const char *cat = gtk_label_get_text(GTK_LABEL(label));
    g_free(app->active_category);
    app->active_category = g_strdup(cat);
    filter_catalog(app);
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
    gtk_file_filter_set_name(filter, "Package Files (*.deb, *.axis)");
    gtk_file_filter_add_pattern(filter, "*.deb");
    gtk_file_filter_add_pattern(filter, "*.axis");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        if (filename) {
            g_free(app->target_file);
            app->target_file = filename;

            gtk_label_set_text(GTK_LABEL(app->card_title), g_path_get_basename(filename));
            gtk_label_set_text(GTK_LABEL(app->card_desc), filename);
            gtk_widget_show_all(app->install_card);
        }
    }
    gtk_widget_destroy(dialog);
}

static void on_card_action_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    PkgInstallerApp *app = (PkgInstallerApp *)user_data;
    if (app->target_file) {
        run_install_command(app, app->target_file);
    }
}

static void populate_catalog(PkgInstallerApp *app) {
    for (int i = 0; CATALOG[i].id != NULL; ++i) {
        AppPackage *pkg = &CATALOG[i];

        GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_widget_set_size_request(card, 220, 140);
        gtk_container_set_border_width(GTK_CONTAINER(card), 10);

        GtkWidget *header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        GtkWidget *icon = gtk_image_new_from_icon_name(pkg->icon, GTK_ICON_SIZE_DND);
        GtkWidget *name = gtk_label_new(pkg->name);
        gtk_widget_set_halign(name, GTK_ALIGN_START);
        gtk_label_set_ellipsize(GTK_LABEL(name), PANGO_ELLIPSIZE_END);

        gtk_box_pack_start(GTK_BOX(header), icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(header), name, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(card), header, FALSE, FALSE, 0);

        GtkWidget *desc = gtk_label_new(pkg->desc);
        gtk_label_set_line_wrap(GTK_LABEL(desc), TRUE);
        gtk_label_set_max_width_chars(GTK_LABEL(desc), 26);
        gtk_widget_set_halign(desc, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(card), desc, TRUE, TRUE, 2);

        GtkWidget *btn = gtk_button_new_with_label(pkg->installed ? "Installed" : "Install");
        if (pkg->installed) {
            gtk_widget_set_sensitive(btn, FALSE);
        }
        g_object_set_data(G_OBJECT(btn), "app", app);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_install_pkg_clicked), (gpointer)pkg->id);
        gtk_box_pack_end(GTK_BOX(card), btn, FALSE, FALSE, 0);

        GtkWidget *frame = gtk_frame_new(NULL);
        gtk_container_add(GTK_CONTAINER(frame), card);

        g_object_set_data_full(G_OBJECT(frame), "name", g_strdup(pkg->name), g_free);
        g_object_set_data_full(G_OBJECT(frame), "desc", g_strdup(pkg->desc), g_free);
        g_object_set_data_full(G_OBJECT(frame), "cat", g_strdup(pkg->category), g_free);

        gtk_container_add(GTK_CONTAINER(app->flowbox), frame);
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);
    check_installed_status();

    PkgInstallerApp app = {0};
    app.active_category = g_strdup("All");

    if (argc > 1 && argv[1][0] != '-') {
        app.target_file = g_strdup(argv[1]);
    }

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app.window), 980, 640);
    gtk_window_set_title(GTK_WINDOW(app.window), "AxisOS Package Installer");

    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS Package Installer");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Pure C11 / GTK 3");
    gtk_window_set_titlebar(GTK_WINDOW(app.window), hb);

    GtkWidget *open_btn = gtk_button_new_with_label("Open Package...");
    g_signal_connect(open_btn, "clicked", G_CALLBACK(on_open_file_clicked), &app);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(hb), open_btn);

    app.search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app.search_entry), "Search packages...");
    g_signal_connect(app.search_entry, "search-changed", G_CALLBACK(on_search_changed), &app);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(hb), app.search_entry);

    GtkWidget *main_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(main_paned), 180);
    gtk_container_add(GTK_CONTAINER(app.window), main_paned);

    // Sidebar categories
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new(NULL, NULL);
    app.sidebar_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app.sidebar_list), GTK_SELECTION_SINGLE);
    g_signal_connect(app.sidebar_list, "row-selected", G_CALLBACK(on_category_selected), &app);

    const char *cats[] = {"All", "Browsers", "Gaming", "Development", "Media", "System", "Office", NULL};
    for (int i = 0; cats[i] != NULL; ++i) {
        GtkWidget *row_lbl = gtk_label_new(cats[i]);
        gtk_widget_set_halign(row_lbl, GTK_ALIGN_START);
        gtk_widget_set_margin_start(row_lbl, 12);
        gtk_widget_set_margin_top(row_lbl, 8);
        gtk_widget_set_margin_bottom(row_lbl, 8);
        gtk_list_box_insert(GTK_LIST_BOX(app.sidebar_list), row_lbl, -1);
    }
    gtk_container_add(GTK_CONTAINER(sidebar_scroll), app.sidebar_list);
    gtk_paned_pack1(GTK_PANED(main_paned), sidebar_scroll, FALSE, FALSE);

    // Right content area
    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(content_box), 12);
    gtk_paned_pack2(GTK_PANED(main_paned), content_box, TRUE, FALSE);

    // Target file card
    app.install_card = gtk_frame_new("Selected Package File");
    GtkWidget *card_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(card_box), 10);

    GtkWidget *c_info = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    app.card_title = gtk_label_new(app.target_file ? g_path_get_basename(app.target_file) : "");
    gtk_widget_set_halign(app.card_title, GTK_ALIGN_START);
    app.card_desc = gtk_label_new(app.target_file ? app.target_file : "");
    gtk_widget_set_halign(app.card_desc, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(c_info), app.card_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(c_info), app.card_desc, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_box), c_info, TRUE, TRUE, 0);

    app.card_action_btn = gtk_button_new_with_label("Install File");
    g_signal_connect(app.card_action_btn, "clicked", G_CALLBACK(on_card_action_clicked), &app);
    gtk_box_pack_end(GTK_BOX(card_box), app.card_action_btn, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(app.install_card), card_box);
    gtk_box_pack_start(GTK_BOX(content_box), app.install_card, FALSE, FALSE, 4);

    if (!app.target_file) {
        gtk_widget_set_no_show_all(app.install_card, TRUE);
        gtk_widget_hide(app.install_card);
    }

    // Catalog flowbox
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    app.flowbox = gtk_flow_box_new();
    gtk_widget_set_valign(app.flowbox, GTK_ALIGN_START);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(app.flowbox), 3);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(app.flowbox), GTK_SELECTION_NONE);
    gtk_container_add(GTK_CONTAINER(scroll), app.flowbox);
    populate_catalog(&app);
    gtk_box_pack_start(GTK_BOX(content_box), scroll, TRUE, TRUE, 4);

    // Progress bar & status
    app.status_label = gtk_label_new("Ready");
    gtk_widget_set_halign(app.status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(content_box), app.status_label, FALSE, FALSE, 2);

    app.progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(content_box), app.progress_bar, FALSE, FALSE, 2);

    // Log pane
    GtkWidget *log_expander = gtk_expander_new("Installation Details / Terminal Output");
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(log_scroll, -1, 140);
    app.log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.log_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app.log_view), TRUE);
    app.log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app.log_view));
    gtk_container_add(GTK_CONTAINER(log_scroll), app.log_view);
    gtk_container_add(GTK_CONTAINER(log_expander), log_scroll);
    gtk_box_pack_start(GTK_BOX(content_box), log_expander, FALSE, FALSE, 4);

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(app.window);
    gtk_main();

    g_free(app.active_category);
    g_free(app.target_file);
    return 0;
}
