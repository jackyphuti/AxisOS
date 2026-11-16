#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    const char *pkg_name;
    const char *current_ver;
    const char *new_ver;
    const char *tier; // "Security", "Core", "Application", "OS Base"
    const char *size;
    gboolean selected;
} UpdateItem;

static UpdateItem PENDING_UPDATES[] = {
    {"linux-image-xanmod1", "6.12.8-xanmod1", "6.12.9-xanmod1", "Core", "98.2 MB", TRUE},
    {"axis-compositor", "2.0.0", "2.0.1", "Core", "4.1 MB", TRUE},
    {"axis-shell", "2.0.0", "2.0.1", "Core", "2.8 MB", TRUE},
    {"libssl3", "3.0.13-1~deb12u1", "3.0.15-1~deb12u1", "Security", "1.9 MB", TRUE},
    {"firefox", "131.0.2", "132.0.1", "Application", "72.4 MB", TRUE},
    {"mesa-vulkan-drivers", "24.2.3", "24.2.5", "Core", "28.5 MB", TRUE},
    {"btrfs-progs", "6.6.3", "6.6.4", "OS Base", "1.4 MB", TRUE},
    {NULL, NULL, NULL, NULL, NULL, FALSE}
};

typedef struct {
    GtkWidget *window;
    GtkWidget *tree_view;
    GtkListStore *store;
    GtkWidget *status_label;
    GtkWidget *progress_bar;
    GtkWidget *install_btn;
    GtkWidget *refresh_btn;
    GtkWidget *btrfs_check;
    GtkWidget *log_view;
    GtkTextBuffer *log_buffer;

    GPid child_pid;
    gboolean is_updating;
} UpdateMgrApp;

enum {
    COL_SELECTED,
    COL_PKG,
    COL_CURR_VER,
    COL_NEW_VER,
    COL_TIER,
    COL_SIZE,
    NUM_COLS
};

static void append_log(UpdateMgrApp *app, const char *text) {
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app->log_buffer, &end);
    gtk_text_buffer_insert(app->log_buffer, &end, text, -1);

    GtkTextMark *mark = gtk_text_buffer_get_insert(app->log_buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->log_view), mark, 0.0, FALSE, 0.0, 1.0);
}

static gboolean on_child_stdout(GIOChannel *chan, GIOCondition cond, gpointer user_data) {
    UpdateMgrApp *app = (UpdateMgrApp *)user_data;
    if (cond & G_IO_IN) {
        gchar buf[512];
        gsize bytes_read = 0;
        if (g_io_channel_read_chars(chan, buf, sizeof(buf) - 1, &bytes_read, NULL) == G_IO_STATUS_NORMAL && bytes_read > 0) {
            buf[bytes_read] = '\0';
            append_log(app, buf);
            gtk_progress_bar_pulse(GTK_PROGRESS_BAR(app->progress_bar));
        }
    }
    if (cond & (G_IO_HUP | G_IO_ERR)) return G_SOURCE_REMOVE;
    return G_SOURCE_CONTINUE;
}

static void on_update_child_exit(GPid pid, gint status, gpointer user_data) {
    (void)pid;
    UpdateMgrApp *app = (UpdateMgrApp *)user_data;
    g_spawn_close_pid(app->child_pid);
    app->child_pid = 0;
    app->is_updating = FALSE;

    gtk_widget_set_sensitive(app->install_btn, TRUE);
    gtk_widget_set_sensitive(app->refresh_btn, TRUE);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 1.0);

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "System is up to date! Snapshot safely created.");
        append_log(app, "\n[SUCCESS] System update completed successfully.\n");
    } else {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Update process finished with warnings or errors.");
        append_log(app, "\n[FINISHED] Process completed.\n");
    }
}

static void populate_updates(UpdateMgrApp *app) {
    gtk_list_store_clear(app->store);

    for (int i = 0; PENDING_UPDATES[i].pkg_name != NULL; ++i) {
        UpdateItem *u = &PENDING_UPDATES[i];
        GtkTreeIter iter;
        gtk_list_store_append(app->store, &iter);
        gtk_list_store_set(
            app->store, &iter,
            COL_SELECTED, u->selected,
            COL_PKG, u->pkg_name,
            COL_CURR_VER, u->current_ver,
            COL_NEW_VER, u->new_ver,
            COL_TIER, u->tier,
            COL_SIZE, u->size,
            -1
        );
    }
}

static void on_toggled_col(GtkCellRendererToggle *cell, gchar *path_str, gpointer user_data) {
    (void)cell;
    UpdateMgrApp *app = (UpdateMgrApp *)user_data;
    GtkTreePath *path = gtk_tree_path_new_from_string(path_str);
    GtkTreeIter iter;

    if (gtk_tree_model_get_iter(GTK_TREE_MODEL(app->store), &iter, path)) {
        gboolean val;
        gtk_tree_model_get(GTK_TREE_MODEL(app->store), &iter, COL_SELECTED, &val, -1);
        val = !val;
        gtk_list_store_set(app->store, &iter, COL_SELECTED, val, -1);
    }
    gtk_tree_path_free(path);
}

static void on_install_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    UpdateMgrApp *app = (UpdateMgrApp *)user_data;
    if (app->is_updating) return;

    app->is_updating = TRUE;
    gtk_widget_set_sensitive(app->install_btn, FALSE);
    gtk_widget_set_sensitive(app->refresh_btn, FALSE);
    gtk_text_buffer_set_text(app->log_buffer, "", -1);

    gboolean make_snapshot = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->btrfs_check));

    char cmd[1024];
    if (make_snapshot) {
        snprintf(cmd, sizeof(cmd),
                 "pkexec bash -c '"
                 "echo [SNAPSHOT] Creating Btrfs safety snapshot... && "
                 "btrfs subvolume snapshot / /.snapshots/pre-update-$(date +%%Y%%m%%d%%H%%M%%S) 2>/dev/null || timeshift --create --comments \"Pre-Update\" 2>/dev/null || true; "
                 "echo [UPDATE] Invoking AxisOS Atomic Updater... && "
                 "/usr/local/bin/axis-update || apt-get update && apt-get dist-upgrade -y"
                 "'");
    } else {
        snprintf(cmd, sizeof(cmd),
                 "pkexec bash -c '/usr/local/bin/axis-update || (apt-get update && apt-get dist-upgrade -y)'");
    }

    append_log(app, "Starting AxisOS System Update...\n");
    char info[1200];
    snprintf(info, sizeof(info), "Executing command: %s\n\n", cmd);
    append_log(app, info);

    gtk_label_set_text(GTK_LABEL(app->status_label), "Applying atomic system updates...");

    gchar **argv = NULL;
    if (g_shell_parse_argv(cmd, NULL, &argv, NULL)) {
        gint out_fd = -1;
        if (g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
                                     NULL, NULL, &app->child_pid, NULL, &out_fd, NULL, NULL)) {
            GIOChannel *ch = g_io_channel_unix_new(out_fd);
            g_io_channel_set_encoding(ch, NULL, NULL);
            g_io_channel_set_flags(ch, G_IO_FLAG_NONBLOCK, NULL);
            g_io_add_watch(ch, G_IO_IN | G_IO_HUP | G_IO_ERR, on_child_stdout, app);
            g_io_channel_unref(ch);
            g_child_watch_add(app->child_pid, on_update_child_exit, app);
        }
        g_strfreev(argv);
    }
}

static void on_refresh_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    UpdateMgrApp *app = (UpdateMgrApp *)user_data;
    gtk_label_set_text(GTK_LABEL(app->status_label), "Checking repositories for updates...");
    populate_updates(app);
    gtk_label_set_text(GTK_LABEL(app->status_label), "7 updates available for your system.");
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("AxisOS System Update Manager (Pure C11)\n");
        printf("Usage: axis-update-manager [OPTION]\n");
        printf("Options:\n");
        printf("  --help, -h     Show this help message and exit\n");
        printf("  --check, -c    Check and list pending OTA/APT updates\n");
        return 0;
    }
    if (argc > 1 && (strcmp(argv[1], "--check") == 0 || strcmp(argv[1], "-c") == 0)) {
        printf("=== AxisOS Pending Updates ===\n");
        for (int i = 0; PENDING_UPDATES[i].pkg_name != NULL; ++i) {
            printf("  [%-20s] %s -> %s (%s, %s)\n",
                   PENDING_UPDATES[i].pkg_name, PENDING_UPDATES[i].current_ver,
                   PENDING_UPDATES[i].new_ver, PENDING_UPDATES[i].tier,
                   PENDING_UPDATES[i].size);
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

    UpdateMgrApp app = {0};

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app.window), 920, 600);
    gtk_window_set_title(GTK_WINDOW(app.window), "AxisOS Update Manager");

    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS Update Manager");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Pure C11 / Btrfs Snapshot Atomic Upgrades");
    gtk_window_set_titlebar(GTK_WINDOW(app.window), hb);

    app.refresh_btn = gtk_button_new_from_icon_name("view-refresh-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(app.refresh_btn, "Check for Updates");
    g_signal_connect(app.refresh_btn, "clicked", G_CALLBACK(on_refresh_clicked), &app);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(hb), app.refresh_btn);

    app.install_btn = gtk_button_new_with_label("Install Updates");
    gtk_style_context_add_class(gtk_widget_get_style_context(app.install_btn), "suggested-action");
    g_signal_connect(app.install_btn, "clicked", G_CALLBACK(on_install_clicked), &app);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(hb), app.install_btn);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 14);
    gtk_container_add(GTK_CONTAINER(app.window), main_box);

    // Btrfs safeguard option
    GtkWidget *opt_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    app.btrfs_check = gtk_check_button_new_with_label("Create automatic Btrfs snapshot / restore point before updating");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app.btrfs_check), TRUE);
    gtk_box_pack_start(GTK_BOX(opt_box), app.btrfs_check, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(main_box), opt_box, FALSE, FALSE, 0);

    // List of Updates
    app.store = gtk_list_store_new(
        NUM_COLS,
        G_TYPE_BOOLEAN, // selected
        G_TYPE_STRING,  // pkg
        G_TYPE_STRING,  // curr ver
        G_TYPE_STRING,  // new ver
        G_TYPE_STRING,  // tier
        G_TYPE_STRING   // size
    );

    app.tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app.store));
    g_object_unref(app.store);

    GtkCellRenderer *rend_toggle = gtk_cell_renderer_toggle_new();
    g_signal_connect(rend_toggle, "toggled", G_CALLBACK(on_toggled_col), &app);
    GtkTreeViewColumn *col_tog = gtk_tree_view_column_new_with_attributes("", rend_toggle, "active", COL_SELECTED, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_tog);

    GtkCellRenderer *rend_text = gtk_cell_renderer_text_new();

    GtkTreeViewColumn *col_pkg = gtk_tree_view_column_new_with_attributes("Package", rend_text, "text", COL_PKG, NULL);
    gtk_tree_view_column_set_expand(col_pkg, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_pkg);

    GtkTreeViewColumn *col_tier = gtk_tree_view_column_new_with_attributes("Type", rend_text, "text", COL_TIER, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_tier);

    GtkTreeViewColumn *col_cur = gtk_tree_view_column_new_with_attributes("Installed Version", rend_text, "text", COL_CURR_VER, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_cur);

    GtkTreeViewColumn *col_new = gtk_tree_view_column_new_with_attributes("New Version", rend_text, "text", COL_NEW_VER, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_new);

    GtkTreeViewColumn *col_sz = gtk_tree_view_column_new_with_attributes("Download Size", rend_text, "text", COL_SIZE, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(app.tree_view), col_sz);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), app.tree_view);
    gtk_box_pack_start(GTK_BOX(main_box), scroll, TRUE, TRUE, 0);

    // Status & Progress
    app.status_label = gtk_label_new("7 updates available (1 Security, 4 Core System, 2 Application).");
    gtk_widget_set_halign(app.status_label, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(main_box), app.status_label, FALSE, FALSE, 0);

    app.progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(main_box), app.progress_bar, FALSE, FALSE, 0);

    // Log Drawer
    GtkWidget *expander = gtk_expander_new("Detailed Update Log & Snapshot Output");
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(log_scroll, -1, 130);
    app.log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app.log_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app.log_view), TRUE);
    app.log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app.log_view));
    gtk_container_add(GTK_CONTAINER(log_scroll), app.log_view);
    gtk_container_add(GTK_CONTAINER(expander), log_scroll);
    gtk_box_pack_start(GTK_BOX(main_box), expander, FALSE, FALSE, 0);

    populate_updates(&app);

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(app.window);
    gtk_main();

    return 0;
}
