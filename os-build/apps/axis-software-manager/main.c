#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    const char *id;
    const char *name;
    const char *category;
    const char *source;
    const char *desc;
    const char *icon;
    gboolean installed;
} SoftItem;

static SoftItem SOFTWARE_LIST[] = {
    {"firefox", "Mozilla Firefox", "Internet", "APT", "Fast and privacy-first web browser", "firefox", FALSE},
    {"chromium", "Chromium Web Browser", "Internet", "APT", "Open-source browser project", "chromium", FALSE},
    {"thunderbird", "Mozilla Thunderbird", "Internet", "APT", "Full-featured email and news client", "thunderbird", FALSE},
    {"discord", "Discord", "Internet", "Flatpak", "Voice, video and text chat platform", "discord", FALSE},

    {"steam", "Steam", "Games", "APT", "Digital distribution gaming platform", "steam", FALSE},
    {"lutris", "Lutris", "Games", "APT", "Open gaming platform for Linux", "lutris", FALSE},
    {"heroic", "Heroic Games Launcher", "Games", "Flatpak", "Epic and GOG launcher", "heroic", FALSE},
    {"retroarch", "RetroArch", "Games", "Flatpak", "Multi-system emulator frontend", "retroarch", FALSE},

    {"code", "Visual Studio Code", "Development", "APT", "Code editing redefined", "visual-studio-code", FALSE},
    {"git", "Git VCS", "Development", "APT", "Fast distributed version control", "git", FALSE},
    {"neovim", "Neovim", "Development", "APT", "Vim-fork focused on extensibility", "nvim", FALSE},
    {"sublime-text", "Sublime Text", "Development", "APT", "Sophisticated text editor for code", "sublime-text", FALSE},

    {"gimp", "GIMP", "Graphics", "APT", "GNU Image Manipulation Program", "gimp", FALSE},
    {"inkscape", "Inkscape", "Graphics", "APT", "Professional vector graphics editor", "inkscape", FALSE},
    {"blender", "Blender", "Graphics", "APT", "3D creation suite", "blender", FALSE},
    {"krita", "Krita", "Graphics", "APT", "Digital painting software", "krita", FALSE},

    {"vlc", "VLC Media Player", "Sound & Video", "APT", "Universal media player", "vlc", FALSE},
    {"obs-studio", "OBS Studio", "Sound & Video", "APT", "Live streaming and video recording", "obs", FALSE},
    {"audacity", "Audacity", "Sound & Video", "APT", "Audio recording and editing", "audacity", FALSE},
    {"spotify-client", "Spotify", "Sound & Video", "APT", "Digital music streaming service", "spotify-client", FALSE},

    {"libreoffice", "LibreOffice Suite", "Office", "APT", "Full-featured office suite", "libreoffice-main", FALSE},
    {"evince", "Document Viewer", "Office", "APT", "View PDF and PostScript documents", "evince", FALSE},

    {"gparted", "GParted", "Administration", "APT", "Partition devices and filesystems", "gparted", FALSE},
    {"timeshift", "Timeshift", "Administration", "APT", "System restore tool using snapshots", "system-software-update", FALSE},
    {"htop", "htop", "Administration", "APT", "Interactive process viewer", "utilities-terminal", FALSE},
    {NULL, NULL, NULL, NULL, NULL, NULL, FALSE}
};

typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *store;
    GtkWidget *search_entry;
    GtkWidget *category_combo;
    GtkWidget *status_label;
    GtkWidget *progress_bar;
    GtkWidget *action_btn;
    GtkWidget *log_view;
    GtkTextBuffer *log_buffer;

    GPid child_pid;
    char *selected_pkg;
    char *selected_source;
    gboolean selected_installed;
} SoftMgrApp;

enum {
    COL_ICON,
    COL_NAME,
    COL_CAT,
    COL_SRC,
    COL_STATUS,
    COL_ID,
    COL_DESC,
    COL_INSTALLED,
    NUM_COLS
};

static void check_installed_packages(void) {
    for (int i = 0; SOFTWARE_LIST[i].id != NULL; ++i) {
        char bin_path[256];
        snprintf(bin_path, sizeof(bin_path), "/usr/bin/%s", SOFTWARE_LIST[i].id);
        SOFTWARE_LIST[i].installed = (access(bin_path, X_OK) == 0);
    }
}

static void append_mgr_log(SoftMgrApp *app, const char *msg) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app->log_buffer, &end);
    gtk_text_buffer_insert(app->log_buffer, &end, msg, -1);

    GtkTextMark *mark = gtk_text_buffer_get_insert(app->log_buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->log_view), mark, 0.0, FALSE, 0.0, 1.0);
}

static gboolean on_mgr_read_output(GIOChannel *chan, GIOCondition cond, gpointer user_data) {
    SoftMgrApp *app = (SoftMgrApp *)user_data;
    if (cond & G_IO_IN) {
        gchar buf[512];
        gsize bytes_read = 0;
        if (g_io_channel_read_chars(chan, buf, sizeof(buf) - 1, &bytes_read, NULL) == G_IO_STATUS_NORMAL && bytes_read > 0) {
            buf[bytes_read] = '\0';
            append_mgr_log(app, buf);
            gtk_progress_bar_pulse(GTK_PROGRESS_BAR(app->progress_bar));
        }
    }
    if (cond & (G_IO_HUP | G_IO_ERR)) return G_SOURCE_REMOVE;
    return G_SOURCE_CONTINUE;
}

static void populate_store(SoftMgrApp *app);

static void on_mgr_child_exit(GPid pid, gint status, gpointer user_data) {
    (void)pid;
    SoftMgrApp *app = (SoftMgrApp *)user_data;
    g_spawn_close_pid(app->child_pid);
    app->child_pid = 0;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 1.0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Operation completed successfully.");
        append_mgr_log(app, "\n[SUCCESS] Operation completed.\n");
    } else {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Operation failed or was cancelled.");
        append_mgr_log(app, "\n[ERROR] Operation failed.\n");
    }
    check_installed_packages();
    populate_store(app);
}

static void populate_store(SoftMgrApp *app) {
    gtk_list_store_clear(app->store);
    const gchar *filter_cat = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->category_combo));
    const gchar *search = gtk_entry_get_text(GTK_ENTRY(app->search_entry));

    for (int i = 0; SOFTWARE_LIST[i].id != NULL; ++i) {
        SoftItem *it = &SOFTWARE_LIST[i];

        if (filter_cat && strcmp(filter_cat, "All Categories") != 0 && strcmp(filter_cat, it->category) != 0) {
            continue;
        }

        if (search && *search) {
            if (!g_str_match_string(search, it->name, TRUE) && !g_str_match_string(search, it->desc, TRUE)) {
                continue;
            }
        }

        GtkTreeIter iter;
        gtk_list_store_append(app->store, &iter);
        gtk_list_store_set(
            app->store, &iter,
            COL_ICON, it->icon,
            COL_NAME, it->name,
            COL_CAT, it->category,
            COL_SRC, it->source,
            COL_STATUS, it->installed ? "Installed" : "Available",
            COL_ID, it->id,
            COL_DESC, it->desc,
            COL_INSTALLED, it->installed,
            -1
        );
    }
}

static void on_selection_changed(GtkTreeSelection *sel, gpointer user_data) {
    SoftMgrApp *app = (SoftMgrApp *)user_data;
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        gchar *id = NULL;
        gchar *src = NULL;
        gboolean inst = FALSE;

        gtk_tree_model_get(model, &iter, COL_ID, &id, COL_SRC, &src, COL_INSTALLED, &inst, -1);

        g_free(app->selected_pkg);
        g_free(app->selected_source);
        app->selected_pkg = id;
        app->selected_source = src;
        app->selected_installed = inst;

        gtk_widget_set_sensitive(app->action_btn, TRUE);
        gtk_button_set_label(GTK_BUTTON(app->action_btn), inst ? "Remove" : "Install");
    } else {
        gtk_widget_set_sensitive(app->action_btn, FALSE);
    }
}

static void on_action_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    SoftMgrApp *app = (SoftMgrApp *)user_data;
    if (!app->selected_pkg) return;

    char cmd[512];
    if (app->selected_installed) {
        // Remove
        if (strcmp(app->selected_source, "Flatpak") == 0) {
            snprintf(cmd, sizeof(cmd), "pkexec flatpak uninstall -y '%s'", app->selected_pkg);
        } else {
            snprintf(cmd, sizeof(cmd), "pkexec apt-get remove -y '%s'", app->selected_pkg);
        }
    } else {
        // Install
        if (strcmp(app->selected_source, "Flatpak") == 0) {
            snprintf(cmd, sizeof(cmd), "pkexec flatpak install -y flathub '%s'", app->selected_pkg);
        } else {
            snprintf(cmd, sizeof(cmd), "pkexec apt-get install -y '%s' || pkexec axis install '%s'", app->selected_pkg, app->selected_pkg);
        }
    }

    gtk_text_buffer_set_text(app->log_buffer, "", -1);
    char start_msg[1024];
    snprintf(start_msg, sizeof(start_msg), "Executing: %s\n\n", cmd);
    append_mgr_log(app, start_msg);

    char st[128];
    snprintf(st, sizeof(st), "%s %s...", app->selected_installed ? "Removing" : "Installing", app->selected_pkg);
    gtk_label_set_text(GTK_LABEL(app->status_label), st);

    gchar **argv = NULL;
    if (g_shell_parse_argv(cmd, NULL, &argv, NULL)) {
        gint out_fd = -1;
        if (g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
                                     NULL, NULL, &app->child_pid, NULL, &out_fd, NULL, NULL)) {
            GIOChannel *ch = g_io_channel_unix_new(out_fd);
            g_io_channel_set_encoding(ch, NULL, NULL);
            g_io_channel_set_flags(ch, G_IO_FLAG_NONBLOCK, NULL);
            g_io_add_watch(ch, G_IO_IN | G_IO_HUP | G_IO_ERR, on_mgr_read_output, app);
            g_io_channel_unref(ch);
            g_child_watch_add(app->child_pid, on_mgr_child_exit, app);
        }
        g_strfreev(argv);
    }
}

static void on_filter_changed(GtkWidget *w, gpointer user_data) {
    (void)w;
    SoftMgrApp *app = (SoftMgrApp *)user_data;
    populate_store(app);
}

static void on_refresh_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    SoftMgrApp *app = (SoftMgrApp *)user_data;
    gtk_label_set_text(GTK_LABEL(app->status_label), "Refreshing package lists...");
    check_installed_packages();
    populate_store(app);
    gtk_label_set_text(GTK_LABEL(app->status_label), "Package lists up to date.");
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("AxisOS Software Manager (Pure C11)\n");
        printf("Usage: axis-software-manager [OPTION]\n");
        printf("Options:\n");
        printf("  --help, -h    Show this help message and exit\n");
        printf("  --list, -l    List featured applications and their status\n");
        return 0;
    }
    check_installed_packages();
    if (argc > 1 && (strcmp(argv[1], "--list") == 0 || strcmp(argv[1], "-l") == 0)) {
        printf("=== AxisOS Curated Software Catalog ===\n");
        for (int i = 0; SOFTWARE_LIST[i].id != NULL; ++i) {
            printf("  [%-18s] %-26s (%s - %s) [%s]\n",
                   SOFTWARE_LIST[i].id, SOFTWARE_LIST[i].name,
                   SOFTWARE_LIST[i].category, SOFTWARE_LIST[i].source,
                   SOFTWARE_LIST[i].installed ? "Installed" : "Available");
        }
        return 0;
    }
    gtk_init(&argc, &argv);

    SoftMgrApp app = {0};

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app.window), 960, 620);
    gtk_window_set_title(GTK_WINDOW(app.window), "AxisOS Software Manager");

    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "Software Manager");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Pure C11 / Multi-Source APT & Flatpak");
    gtk_window_set_titlebar(GTK_WINDOW(app.window), hb);

    GtkWidget *ref_btn = gtk_button_new_from_icon_name("view-refresh-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(ref_btn, "Refresh Catalogs");
    g_signal_connect(ref_btn, "clicked", G_CALLBACK(on_refresh_clicked), &app);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(hb), ref_btn);

    app.action_btn = gtk_button_new_with_label("Install");
    gtk_widget_set_sensitive(app.action_btn, FALSE);
    g_signal_connect(app.action_btn, "clicked", G_CALLBACK(on_action_clicked), &app);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(hb), app.action_btn);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 12);
    gtk_container_add(GTK_CONTAINER(app.window), main_box);

    // Filters row
    GtkWidget *filter_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    app.category_combo = gtk_combo_box_text_new();
    const char *cats[] = {"All Categories", "Internet", "Games", "Development", "Graphics", "Sound & Video", "Office", "Administration", NULL};
    for (int i = 0; cats[i] != NULL; ++i) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app.category_combo), cats[i]);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(app.category_combo), 0);
    g_signal_connect(app.category_combo, "changed", G_CALLBACK(on_filter_changed), &app);
    gtk_box_pack_start(GTK_BOX(filter_row), app.category_combo, FALSE, FALSE, 0);

    app.search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app.search_entry), "Search applications...");
    g_signal_connect(app.search_entry, "search-changed", G_CALLBACK(on_filter_changed), &app);
    gtk_box_pack_start(GTK_BOX(filter_row), app.search_entry, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(main_box), filter_row, FALSE, FALSE, 0);

    // TreeView Table
    app.store = gtk_list_store_new(
        NUM_COLS,
        G_TYPE_STRING, // icon
        G_TYPE_STRING, // name
        G_TYPE_STRING, // cat
        G_TYPE_STRING, // src
        G_TYPE_STRING, // status
        G_TYPE_STRING, // id
        G_TYPE_STRING, // desc
        G_TYPE_BOOLEAN // installed
    );

    app.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store));
    g_object_unref(app.store);

    GtkCellRenderer *rend_icon = gtk_cell_renderer_pixbuf_new();
    GtkCellRenderer *rend_text = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *col_app = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col_app, "Application");
    gtk_tree_view_column_pack_start(col_app, rend_icon, FALSE);
    gtk_tree_view_column_add_attribute(col_app, rend_icon, "icon-name", COL_ICON);
    gtk_tree_view_column_pack_start(col_app, rend_text, TRUE);
    gtk_tree_view_column_add_attribute(col_app, rend_text, "text", COL_NAME);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_app);

    GtkTreeViewColumn *col_desc = gtk_tree_view_column_new_with_attributes("Description", rend_text, "text", COL_DESC, NULL);
    gtk_tree_view_column_set_expand(col_desc, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_desc);

    GtkTreeViewColumn *col_cat = gtk_tree_view_column_new_with_attributes("Category", rend_text, "text", COL_CAT, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_cat);

    GtkTreeViewColumn *col_src = gtk_tree_view_column_new_with_attributes("Source", rend_text, "text", COL_SRC, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_src);

    GtkTreeViewColumn *col_status = gtk_tree_view_column_new_with_attributes("Status", rend_text, "text", COL_STATUS, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_status);

    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.tree_view));
    gtk_tree_selection_set_mode(sel, GTK_SELECTION_SINGLE);
    g_signal_connect(sel, "changed", G_CALLBACK(on_selection_changed), &app);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), app.tree_view);
    gtk_box_pack_start(GTK_BOX(main_box), scroll, TRUE, TRUE, 0);

    // Progress bar & status
    app.status_label = gtk_label_new("Ready");
    gtk_widget_set_halign(app.status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(main_box), app.status_label, FALSE, FALSE, 0);

    app.progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(main_box), app.progress_bar, FALSE, FALSE, 0);

    // Terminal log drawer
    GtkWidget *expander = gtk_expander_new("Operation Log");
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(log_scroll, -1, 120);
    app.log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.log_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app.log_view), TRUE);
    app.log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app.log_view));
    gtk_container_add(GTK_CONTAINER(log_scroll), app.log_view);
    gtk_container_add(GTK_CONTAINER(expander), log_scroll);
    gtk_box_pack_start(GTK_BOX(main_box), expander, FALSE, FALSE, 0);

    populate_store(&app);

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(app.window);
    gtk_main();

    g_free(app.selected_pkg);
    g_free(app.selected_source);
    return 0;
}
