#define _GNU_SOURCE
#include "ui_shell.h"
#include <cairo.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void cairo_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2, 0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, M_PI / 2);
    cairo_arc(cr, x + r, y + h - r, r, M_PI / 2, M_PI);
    cairo_arc(cr, x + r, y + r, r, M_PI, 3 * M_PI / 2);
    cairo_close_path(cr);
}

static gboolean on_draw_glassmorphic(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)user_data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rounded_rect(cr, 0, 0, alloc.width, alloc.height, 16.0);
    // Dark glassmorphic background rgba(15, 23, 42, 0.88)
    cairo_set_source_rgba(cr, 15.0 / 255.0, 23.0 / 255.0, 42.0 / 255.0, 0.88);
    cairo_fill_preserve(cr);

    // Subtle border rgba(255, 255, 255, 0.12)
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.12);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);
    cairo_restore(cr);

    return FALSE; // Let child widgets render
}

static gboolean on_draw_topbar(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)user_data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 10.0 / 255.0, 15.0 / 255.0, 30.0 / 255.0, 0.85);
    cairo_paint(cr);

    // Bottom border line
    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.08);
    cairo_set_line_width(cr, 1.0);
    cairo_move_to(cr, 0, alloc.height - 0.5);
    cairo_line_to(cr, alloc.width, alloc.height - 0.5);
    cairo_stroke(cr);
    cairo_restore(cr);

    return FALSE;
}

static gboolean on_hide_osd(gpointer data) {
    UiShell *shell = (UiShell *)data;
    if (shell->osd_window) {
        gtk_widget_hide(shell->osd_window);
    }
    shell->osd_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

void ui_shell_show_osd(UiShell *shell, const char *icon_name, int percent) {
    if (!shell || !shell->osd_window) return;
    if (shell->osd_timeout_id) {
        g_source_remove(shell->osd_timeout_id);
    }

    gtk_image_set_from_icon_name(GTK_IMAGE(shell->osd_icon), icon_name, GTK_ICON_SIZE_DIALOG);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(shell->osd_progress), percent / 100.0);
    gtk_widget_show_all(shell->osd_window);

    shell->osd_timeout_id = g_timeout_add(1500, on_hide_osd, shell);
}

static gboolean on_hide_notification(gpointer data) {
    UiShell *shell = (UiShell *)data;
    if (shell->notification_window) {
        gtk_widget_hide(shell->notification_window);
    }
    shell->notif_timeout_id = 0;
    return G_SOURCE_REMOVE;
}

void ui_shell_show_notification(UiShell *shell, const char *app_name, const char *summary,
                                const char *body, const char *icon) {
    (void)app_name;
    if (!shell || !shell->notification_window) return;
    if (shell->notif_timeout_id) {
        g_source_remove(shell->notif_timeout_id);
    }

    const char *ic = (icon && *icon) ? icon : "dialog-information";
    gtk_image_set_from_icon_name(GTK_IMAGE(shell->notif_icon), ic, GTK_ICON_SIZE_DND);
    gtk_label_set_text(GTK_LABEL(shell->notif_title), summary ? summary : "Notification");
    gtk_label_set_text(GTK_LABEL(shell->notif_body), body ? body : "");

    gtk_widget_show_all(shell->notification_window);
    shell->notif_timeout_id = g_timeout_add_seconds(5, on_hide_notification, shell);
}

static void on_system_notification_incoming(uint32_t id, const char *app_name, const char *summary,
                                            const char *body, const char *icon, gpointer user_data) {
    (void)id;
    UiShell *shell = (UiShell *)user_data;
    ui_shell_show_notification(shell, app_name, summary, body, icon);
}

void ui_shell_toggle_launcher(UiShell *shell) {
    if (!shell || !shell->launcher_window) return;
    if (gtk_widget_get_visible(shell->launcher_window)) {
        gtk_widget_hide(shell->launcher_window);
    } else {
        if (shell->quicksettings_window) gtk_widget_hide(shell->quicksettings_window);
        gtk_entry_set_text(GTK_ENTRY(shell->launcher_search_entry), "");
        gtk_widget_show_all(shell->launcher_window);
        gtk_widget_grab_focus(shell->launcher_search_entry);
    }
}

void ui_shell_toggle_quicksettings(UiShell *shell) {
    if (!shell || !shell->quicksettings_window) return;
    if (gtk_widget_get_visible(shell->quicksettings_window)) {
        gtk_widget_hide(shell->quicksettings_window);
    } else {
        if (shell->launcher_window) gtk_widget_hide(shell->launcher_window);
        gtk_scale_button_set_value(GTK_SCALE_BUTTON(shell->qs_vol_scale), shell->sys_ctrl->metrics.volume);
        gtk_range_set_value(GTK_RANGE(shell->qs_bright_scale), shell->sys_ctrl->metrics.brightness);
        gtk_switch_set_active(GTK_SWITCH(shell->qs_wifi_switch), shell->sys_ctrl->metrics.wifi_connected);
        gtk_switch_set_active(GTK_SWITCH(shell->qs_bt_switch), shell->sys_ctrl->metrics.bluetooth_enabled);
        gtk_switch_set_active(GTK_SWITCH(shell->qs_dark_switch), shell->sys_ctrl->metrics.is_dark_mode);
        gtk_widget_show_all(shell->quicksettings_window);
    }
}

static void on_launcher_app_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *cmd = (const char *)user_data;
    UiShell *shell = (UiShell *)g_object_get_data(G_OBJECT(btn), "shell");
    if (cmd && shell) {
        system_controller_launch_app(shell->sys_ctrl, cmd);
        gtk_widget_hide(shell->launcher_window);
    }
}

static void filter_launcher_apps(UiShell *shell) {
    const gchar *query = gtk_entry_get_text(GTK_ENTRY(shell->launcher_search_entry));
    GList *children = gtk_container_get_children(GTK_CONTAINER(shell->launcher_flowbox));

    for (GList *l = children; l != NULL; l = l->next) {
        GtkWidget *child = GTK_WIDGET(l->data);
        const char *name = (const char *)g_object_get_data(G_OBJECT(child), "app_name");
        const char *cat = (const char *)g_object_get_data(G_OBJECT(child), "app_cat");

        gboolean match = TRUE;
        if (query && *query) {
            if (!name || !g_str_match_string(query, name, TRUE)) {
                match = FALSE;
            }
        }

        if (match && shell->launcher_active_filter && strcmp(shell->launcher_active_filter, "All") != 0) {
            if (!cat || !g_str_match_string(shell->launcher_active_filter, cat, TRUE)) {
                match = FALSE;
            }
        }

        gtk_widget_set_visible(child, match);
    }
    g_list_free(children);
}

static void on_launcher_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    (void)entry;
    UiShell *shell = (UiShell *)user_data;
    filter_launcher_apps(shell);
}

static void on_filter_btn_clicked(GtkButton *btn, gpointer user_data) {
    UiShell *shell = (UiShell *)user_data;
    const char *filter = gtk_button_get_label(btn);
    g_free(shell->launcher_active_filter);
    shell->launcher_active_filter = g_strdup(filter);
    filter_launcher_apps(shell);
}

static gboolean update_clock_and_status(gpointer user_data) {
    UiShell *shell = (UiShell *)user_data;
    if (!shell) return G_SOURCE_REMOVE;

    GDateTime *now = g_date_time_new_now_local();
    if (now) {
        gchar *time_str = g_date_time_format(now, "%a %b %d  %H:%M:%S");
        if (shell->topbar_clock_label) {
            gtk_label_set_text(GTK_LABEL(shell->topbar_clock_label), time_str);
        }
        g_free(time_str);
        g_date_time_unref(now);
    }

    if (shell->sys_ctrl) {
        SystemMetrics *m = &shell->sys_ctrl->metrics;
        char buf[64];

        if (shell->topbar_cpu_label) {
            snprintf(buf, sizeof(buf), "CPU %d%%", m->cpu_load_percent);
            gtk_label_set_text(GTK_LABEL(shell->topbar_cpu_label), buf);
        }
        if (shell->topbar_ram_label) {
            snprintf(buf, sizeof(buf), "RAM %d%%", m->ram_usage_percent);
            gtk_label_set_text(GTK_LABEL(shell->topbar_ram_label), buf);
        }
        if (shell->topbar_bat_label) {
            snprintf(buf, sizeof(buf), "%d%%%s", m->battery_percent, m->battery_charging ? " ⚡" : "");
            gtk_label_set_text(GTK_LABEL(shell->topbar_bat_label), buf);
        }
        if (shell->topbar_vol_label) {
            snprintf(buf, sizeof(buf), "VOL %d%%", m->volume);
            gtk_label_set_text(GTK_LABEL(shell->topbar_vol_label), buf);
        }
        if (shell->topbar_net_label) {
            gtk_label_set_text(GTK_LABEL(shell->topbar_net_label), m->wifi_connected ? (m->wifi_ssid ? m->wifi_ssid : "Online") : "Offline");
        }
    }

    return G_SOURCE_CONTINUE;
}

static void on_auth_status(AuthResult result, const char *message, gpointer user_data) {
    UiShell *shell = (UiShell *)user_data;
    if (!shell || !shell->greeter_status_label) return;

    gtk_spinner_stop(GTK_SPINNER(shell->greeter_spinner));
    gtk_label_set_text(GTK_LABEL(shell->greeter_status_label), message ? message : "");

    if (result == AUTH_RESULT_SUCCESS) {
        ui_shell_unlock(shell);
    } else {
        gtk_entry_set_text(GTK_ENTRY(shell->greeter_pass_entry), "");
    }
}

static void on_greeter_submit(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    UiShell *shell = (UiShell *)user_data;
    const char *pass = gtk_entry_get_text(GTK_ENTRY(shell->greeter_pass_entry));
    if (pass && *pass) {
        gtk_spinner_start(GTK_SPINNER(shell->greeter_spinner));
        gtk_label_set_text(GTK_LABEL(shell->greeter_status_label), "Verifying credentials...");
        auth_manager_authenticate(shell->auth_mgr, pass);
    }
}

void ui_shell_lock(UiShell *shell) {
    if (!shell) return;
    auth_manager_lock(shell->auth_mgr);
    gtk_entry_set_text(GTK_ENTRY(shell->greeter_pass_entry), "");
    gtk_label_set_text(GTK_LABEL(shell->greeter_status_label), "Enter password to unlock");
    gtk_widget_show_all(shell->greeter_window);
    gtk_widget_grab_focus(shell->greeter_pass_entry);
}

void ui_shell_unlock(UiShell *shell) {
    if (!shell) return;
    auth_manager_unlock(shell->auth_mgr);
    if (!shell->is_greeter_mode) {
        gtk_widget_hide(shell->greeter_window);
    } else {
        // Greeter mode: launch desktop session or exit
        g_print("[AxisOS Shell] Greeter authenticated successfully. Launching user session...\n");
        g_spawn_command_line_async("axisos-session", NULL);
        gtk_main_quit();
    }
}

static void setup_layer_window(GtkWidget *win, GtkLayerShellLayer layer, const char *name) {
    gtk_widget_set_name(win, name);
    gtk_widget_set_app_paintable(win, TRUE);

    GdkScreen *screen = gtk_widget_get_screen(win);
    GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
    if (visual) {
        gtk_widget_set_visual(win, visual);
    }

    if (gtk_layer_is_supported()) {
        gtk_layer_init_for_window(GTK_WINDOW(win));
        gtk_layer_set_layer(GTK_WINDOW(win), layer);
        gtk_layer_set_namespace(GTK_WINDOW(win), name);
    }
}

UiShell *ui_shell_create(SystemController *sys_ctrl, AuthManager *auth_mgr, gboolean greeter_mode) {
    UiShell *shell = g_new0(UiShell, 1);
    shell->sys_ctrl = sys_ctrl;
    shell->auth_mgr = auth_mgr;
    shell->is_greeter_mode = greeter_mode;
    shell->launcher_active_filter = g_strdup("All");

    if (auth_mgr) {
        auth_manager_set_callback(auth_mgr, on_auth_status, shell);
    }
    if (sys_ctrl) {
        system_controller_set_notification_callback(sys_ctrl, on_system_notification_incoming, shell);
    }

    // ==========================================
    // 1. TOP BAR (LiquidTopBar)
    // ==========================================
    shell->topbar_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_layer_window(shell->topbar_window, GTK_LAYER_SHELL_LAYER_TOP, "LiquidTopBar");

    if (gtk_layer_is_supported()) {
        gtk_layer_set_anchor(GTK_WINDOW(shell->topbar_window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->topbar_window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->topbar_window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_exclusive_zone(GTK_WINDOW(shell->topbar_window), 36);
    }
    gtk_widget_set_size_request(shell->topbar_window, -1, 36);
    g_signal_connect(shell->topbar_window, "draw", G_CALLBACK(on_draw_topbar), shell);

    GtkWidget *topbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(topbar_box), 4);
    gtk_container_add(GTK_CONTAINER(shell->topbar_window), topbar_box);

    // Left: Launcher button & Workspaces
    GtkWidget *launcher_btn = gtk_button_new_from_icon_name("view-app-grid-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(launcher_btn, "Applications");
    g_signal_connect_swapped(launcher_btn, "clicked", G_CALLBACK(ui_shell_toggle_launcher), shell);
    gtk_box_pack_start(GTK_BOX(topbar_box), launcher_btn, FALSE, FALSE, 4);

    for (int i = 1; i <= 4; ++i) {
        char ws_name[4];
        snprintf(ws_name, sizeof(ws_name), "%d", i);
        GtkWidget *ws_btn = gtk_button_new_with_label(ws_name);
        gtk_widget_set_size_request(ws_btn, 28, 28);
        gtk_box_pack_start(GTK_BOX(topbar_box), ws_btn, FALSE, FALSE, 2);
    }

    // Center: Clock
    shell->topbar_clock_label = gtk_label_new("Loading time...");
    gtk_widget_set_hexpand(shell->topbar_clock_label, TRUE);
    gtk_label_set_justify(GTK_LABEL(shell->topbar_clock_label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(topbar_box), shell->topbar_clock_label, TRUE, TRUE, 0);

    // Right: Telemetry & Controls
    shell->topbar_cpu_label = gtk_label_new("CPU 0%");
    gtk_box_pack_start(GTK_BOX(topbar_box), shell->topbar_cpu_label, FALSE, FALSE, 6);

    shell->topbar_ram_label = gtk_label_new("RAM 0%");
    gtk_box_pack_start(GTK_BOX(topbar_box), shell->topbar_ram_label, FALSE, FALSE, 6);

    shell->topbar_net_label = gtk_label_new("Online");
    gtk_box_pack_start(GTK_BOX(topbar_box), shell->topbar_net_label, FALSE, FALSE, 6);

    shell->topbar_vol_label = gtk_label_new("VOL 70%");
    gtk_box_pack_start(GTK_BOX(topbar_box), shell->topbar_vol_label, FALSE, FALSE, 6);

    shell->topbar_bat_label = gtk_label_new("100%");
    gtk_box_pack_start(GTK_BOX(topbar_box), shell->topbar_bat_label, FALSE, FALSE, 6);

    GtkWidget *qs_btn = gtk_button_new_from_icon_name("open-menu-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(qs_btn, "Quick Settings");
    g_signal_connect_swapped(qs_btn, "clicked", G_CALLBACK(ui_shell_toggle_quicksettings), shell);
    gtk_box_pack_start(GTK_BOX(topbar_box), qs_btn, FALSE, FALSE, 4);

    // ==========================================
    // 2. DOCK (LiquidDock)
    // ==========================================
    shell->dock_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_layer_window(shell->dock_window, GTK_LAYER_SHELL_LAYER_TOP, "LiquidDock");

    if (gtk_layer_is_supported()) {
        gtk_layer_set_anchor(GTK_WINDOW(shell->dock_window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(shell->dock_window), GTK_LAYER_SHELL_EDGE_BOTTOM, 8);
        gtk_layer_set_exclusive_zone(GTK_WINDOW(shell->dock_window), 64);
    }
    g_signal_connect(shell->dock_window, "draw", G_CALLBACK(on_draw_glassmorphic), shell);

    GtkWidget *dock_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(dock_box), 8);
    gtk_container_add(GTK_CONTAINER(shell->dock_window), dock_box);

    const struct {
        const char *icon;
        const char *name;
        const char *exec;
    } pinned_apps[] = {
        {"utilities-terminal", "Terminal", "axis-terminal"},
        {"system-file-manager", "Files", "nautilus || thunar"},
        {"web-browser", "Browser", "firefox || chromium"},
        {"preferences-system", "Settings", "axis-settings"},
        {"utilities-system-monitor", "Task Manager", "axis-task-manager"},
        {"system-software-install", "Software", "axis-software-manager"},
        {"system-software-update", "Updater", "axis-update-manager"},
        {NULL, NULL, NULL}
    };

    for (int i = 0; pinned_apps[i].name != NULL; ++i) {
        GtkWidget *btn = gtk_button_new();
        GtkWidget *img = gtk_image_new_from_icon_name(pinned_apps[i].icon, GTK_ICON_SIZE_DND);
        gtk_button_set_image(GTK_BUTTON(btn), img);
        gtk_widget_set_tooltip_text(btn, pinned_apps[i].name);
        g_object_set_data(G_OBJECT(btn), "shell", shell);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_launcher_app_clicked), (gpointer)pinned_apps[i].exec);
        gtk_box_pack_start(GTK_BOX(dock_box), btn, FALSE, FALSE, 2);
    }

    // ==========================================
    // 3. APP LAUNCHER (LiquidAppLauncher)
    // ==========================================
    shell->launcher_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_layer_window(shell->launcher_window, GTK_LAYER_SHELL_LAYER_OVERLAY, "LiquidAppLauncher");

    if (gtk_layer_is_supported()) {
        gtk_layer_set_anchor(GTK_WINDOW(shell->launcher_window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(shell->launcher_window), GTK_LAYER_SHELL_EDGE_TOP, 44);
        gtk_layer_set_keyboard_mode(GTK_WINDOW(shell->launcher_window), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
    }
    gtk_window_set_default_size(GTK_WINDOW(shell->launcher_window), 640, 500);
    g_signal_connect(shell->launcher_window, "draw", G_CALLBACK(on_draw_glassmorphic), shell);

    GtkWidget *launcher_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(launcher_card), 16);
    gtk_container_add(GTK_CONTAINER(shell->launcher_window), launcher_card);

    shell->launcher_search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(shell->launcher_search_entry), "Search applications, utilities, commands...");
    g_signal_connect(shell->launcher_search_entry, "search-changed", G_CALLBACK(on_launcher_search_changed), shell);
    gtk_box_pack_start(GTK_BOX(launcher_card), shell->launcher_search_entry, FALSE, FALSE, 4);

    // Filter pills
    GtkWidget *filter_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    const char *cats[] = {"All", "System", "Utility", "Development", "Network", "AudioVideo", NULL};
    for (int i = 0; cats[i] != NULL; ++i) {
        GtkWidget *cat_btn = gtk_button_new_with_label(cats[i]);
        g_signal_connect(cat_btn, "clicked", G_CALLBACK(on_filter_btn_clicked), shell);
        gtk_box_pack_start(GTK_BOX(filter_box), cat_btn, FALSE, FALSE, 2);
    }
    gtk_box_pack_start(GTK_BOX(launcher_card), filter_box, FALSE, FALSE, 4);

    // Scrolled apps flowbox
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(launcher_card), scroll, TRUE, TRUE, 4);

    shell->launcher_flowbox = gtk_flow_box_new();
    gtk_widget_set_valign(shell->launcher_flowbox, GTK_ALIGN_START);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(shell->launcher_flowbox), 5);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(shell->launcher_flowbox), GTK_SELECTION_NONE);
    gtk_container_add(GTK_CONTAINER(scroll), shell->launcher_flowbox);

    if (sys_ctrl && sys_ctrl->installed_apps) {
        for (guint i = 0; i < sys_ctrl->installed_apps->len; ++i) {
            AppEntry *app = g_ptr_array_index(sys_ctrl->installed_apps, i);
            GtkWidget *app_btn = gtk_button_new();
            gtk_widget_set_size_request(app_btn, 100, 90);

            GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
            GtkWidget *img = gtk_image_new_from_icon_name(app->icon, GTK_ICON_SIZE_DND);
            GtkWidget *lbl = gtk_label_new(app->name);
            gtk_label_set_ellipsize(GTK_LABEL(lbl), PANGO_ELLIPSIZE_END);
            gtk_label_set_max_width_chars(GTK_LABEL(lbl), 12);

            gtk_box_pack_start(GTK_BOX(box), img, TRUE, TRUE, 2);
            gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 2);
            gtk_container_add(GTK_CONTAINER(app_btn), box);

            g_object_set_data(G_OBJECT(app_btn), "shell", shell);
            g_object_set_data_full(G_OBJECT(app_btn), "app_name", g_strdup(app->name), g_free);
            g_object_set_data_full(G_OBJECT(app_btn), "app_cat", g_strdup(app->categories), g_free);
            g_signal_connect(app_btn, "clicked", G_CALLBACK(on_launcher_app_clicked), (gpointer)app->exec);

            gtk_container_add(GTK_CONTAINER(shell->launcher_flowbox), app_btn);
        }
    }

    // ==========================================
    // 4. QUICK SETTINGS (QuickSettingsPanel)
    // ==========================================
    shell->quicksettings_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_layer_window(shell->quicksettings_window, GTK_LAYER_SHELL_LAYER_OVERLAY, "QuickSettingsPanel");

    if (gtk_layer_is_supported()) {
        gtk_layer_set_anchor(GTK_WINDOW(shell->quicksettings_window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->quicksettings_window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(shell->quicksettings_window), GTK_LAYER_SHELL_EDGE_TOP, 44);
        gtk_layer_set_margin(GTK_WINDOW(shell->quicksettings_window), GTK_LAYER_SHELL_EDGE_RIGHT, 12);
    }
    gtk_widget_set_size_request(shell->quicksettings_window, 340, -1);
    g_signal_connect(shell->quicksettings_window, "draw", G_CALLBACK(on_draw_glassmorphic), shell);

    GtkWidget *qs_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(qs_card), 16);
    gtk_container_add(GTK_CONTAINER(shell->quicksettings_window), qs_card);

    // Title / user header
    GtkWidget *user_hdr = gtk_label_new(auth_mgr ? auth_mgr->display_name : "AxisOS User");
    gtk_widget_set_halign(user_hdr, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(qs_card), user_hdr, FALSE, FALSE, 0);

    // Sliders: Volume & Brightness
    GtkWidget *vol_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *vol_lbl = gtk_label_new("Volume");
    shell->qs_vol_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_widget_set_hexpand(shell->qs_vol_scale, TRUE);
    gtk_box_pack_start(GTK_BOX(vol_row), vol_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vol_row), shell->qs_vol_scale, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(qs_card), vol_row, FALSE, FALSE, 2);

    GtkWidget *bright_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *bright_lbl = gtk_label_new("Brightness");
    shell->qs_bright_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 5, 100, 1);
    gtk_widget_set_hexpand(shell->qs_bright_scale, TRUE);
    gtk_box_pack_start(GTK_BOX(bright_row), bright_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bright_row), shell->qs_bright_scale, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(qs_card), bright_row, FALSE, FALSE, 2);

    // Switches: Wi-Fi, BT, Dark Mode
    GtkWidget *switches_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(switches_grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(switches_grid), 12);

    GtkWidget *wifi_lbl = gtk_label_new("Wi-Fi");
    gtk_widget_set_halign(wifi_lbl, GTK_ALIGN_START);
    shell->qs_wifi_switch = gtk_switch_new();
    gtk_grid_attach(GTK_GRID(switches_grid), wifi_lbl, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(switches_grid), shell->qs_wifi_switch, 1, 0, 1, 1);

    GtkWidget *bt_lbl = gtk_label_new("Bluetooth");
    gtk_widget_set_halign(bt_lbl, GTK_ALIGN_START);
    shell->qs_bt_switch = gtk_switch_new();
    gtk_grid_attach(GTK_GRID(switches_grid), bt_lbl, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(switches_grid), shell->qs_bt_switch, 1, 1, 1, 1);

    GtkWidget *dark_lbl = gtk_label_new("Dark Mode");
    gtk_widget_set_halign(dark_lbl, GTK_ALIGN_START);
    shell->qs_dark_switch = gtk_switch_new();
    gtk_grid_attach(GTK_GRID(switches_grid), dark_lbl, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(switches_grid), shell->qs_dark_switch, 1, 2, 1, 1);

    gtk_box_pack_start(GTK_BOX(qs_card), switches_grid, FALSE, FALSE, 4);

    // Power buttons
    GtkWidget *pwr_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *lock_btn = gtk_button_new_with_label("Lock");
    g_signal_connect_swapped(lock_btn, "clicked", G_CALLBACK(ui_shell_lock), shell);

    GtkWidget *reboot_btn = gtk_button_new_with_label("Reboot");
    g_signal_connect_swapped(reboot_btn, "clicked", G_CALLBACK(system_controller_reboot), sys_ctrl);

    GtkWidget *power_btn = gtk_button_new_with_label("Power Off");
    g_signal_connect_swapped(power_btn, "clicked", G_CALLBACK(system_controller_power_off), sys_ctrl);

    gtk_box_pack_start(GTK_BOX(pwr_box), lock_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pwr_box), reboot_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pwr_box), power_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(qs_card), pwr_box, FALSE, FALSE, 4);

    // ==========================================
    // 5. OSD OVERLAY (OSDOverlay)
    // ==========================================
    shell->osd_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_layer_window(shell->osd_window, GTK_LAYER_SHELL_LAYER_OVERLAY, "OSDOverlay");
    gtk_widget_set_size_request(shell->osd_window, 220, 90);
    g_signal_connect(shell->osd_window, "draw", G_CALLBACK(on_draw_glassmorphic), shell);

    GtkWidget *osd_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(osd_box), 16);
    gtk_container_add(GTK_CONTAINER(shell->osd_window), osd_box);

    shell->osd_icon = gtk_image_new_from_icon_name("audio-volume-high-symbolic", GTK_ICON_SIZE_DIALOG);
    shell->osd_progress = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(osd_box), shell->osd_icon, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(osd_box), shell->osd_progress, FALSE, FALSE, 4);

    // ==========================================
    // 6. NOTIFICATION POPUP (NotificationPopup)
    // ==========================================
    shell->notification_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_layer_window(shell->notification_window, GTK_LAYER_SHELL_LAYER_OVERLAY, "NotificationPopup");

    if (gtk_layer_is_supported()) {
        gtk_layer_set_anchor(GTK_WINDOW(shell->notification_window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->notification_window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(shell->notification_window), GTK_LAYER_SHELL_EDGE_TOP, 44);
        gtk_layer_set_margin(GTK_WINDOW(shell->notification_window), GTK_LAYER_SHELL_EDGE_RIGHT, 16);
    }
    gtk_widget_set_size_request(shell->notification_window, 360, -1);
    g_signal_connect(shell->notification_window, "draw", G_CALLBACK(on_draw_glassmorphic), shell);

    GtkWidget *notif_card = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(notif_card), 14);
    gtk_container_add(GTK_CONTAINER(shell->notification_window), notif_card);

    shell->notif_icon = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_DND);
    gtk_box_pack_start(GTK_BOX(notif_card), shell->notif_icon, FALSE, FALSE, 0);

    GtkWidget *text_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    shell->notif_title = gtk_label_new("Notification");
    gtk_widget_set_halign(shell->notif_title, GTK_ALIGN_START);
    shell->notif_body = gtk_label_new("");
    gtk_label_set_line_wrap(GTK_LABEL(shell->notif_body), TRUE);
    gtk_widget_set_halign(shell->notif_body, GTK_ALIGN_START);

    gtk_box_pack_start(GTK_BOX(text_box), shell->notif_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(text_box), shell->notif_body, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(notif_card), text_box, TRUE, TRUE, 0);

    // ==========================================
    // 7. LOGIN GREETER / LOCK SCREEN (LoginScreen)
    // ==========================================
    shell->greeter_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_layer_window(shell->greeter_window, GTK_LAYER_SHELL_LAYER_OVERLAY, "LoginScreen");

    if (gtk_layer_is_supported()) {
        gtk_layer_set_anchor(GTK_WINDOW(shell->greeter_window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->greeter_window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->greeter_window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->greeter_window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_keyboard_mode(GTK_WINDOW(shell->greeter_window), GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
    }
    gtk_window_set_default_size(GTK_WINDOW(shell->greeter_window), 1024, 768);

    GtkWidget *greeter_center = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_widget_set_halign(greeter_center, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(greeter_center, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(shell->greeter_window), greeter_center);

    GtkWidget *avatar = gtk_image_new_from_icon_name("avatar-default", GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(greeter_center), avatar, FALSE, FALSE, 4);

    GtkWidget *user_name_lbl = gtk_label_new(auth_mgr ? auth_mgr->display_name : "AxisOS");
    gtk_box_pack_start(GTK_BOX(greeter_center), user_name_lbl, FALSE, FALSE, 4);

    shell->greeter_pass_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(shell->greeter_pass_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(shell->greeter_pass_entry), "Password");
    g_signal_connect(shell->greeter_pass_entry, "activate", G_CALLBACK(on_greeter_submit), shell);
    gtk_box_pack_start(GTK_BOX(greeter_center), shell->greeter_pass_entry, FALSE, FALSE, 4);

    GtkWidget *submit_btn = gtk_button_new_with_label("Unlock");
    g_signal_connect(submit_btn, "clicked", G_CALLBACK(on_greeter_submit), shell);
    gtk_box_pack_start(GTK_BOX(greeter_center), submit_btn, FALSE, FALSE, 4);

    shell->greeter_spinner = gtk_spinner_new();
    gtk_box_pack_start(GTK_BOX(greeter_center), shell->greeter_spinner, FALSE, FALSE, 2);

    shell->greeter_status_label = gtk_label_new(auth_mgr && auth_mgr->has_fingerprint ?
                                                "Scan fingerprint or enter password" : "");
    gtk_box_pack_start(GTK_BOX(greeter_center), shell->greeter_status_label, FALSE, FALSE, 4);

    // Initial display states
    if (shell->is_greeter_mode) {
        gtk_widget_show_all(shell->greeter_window);
        auth_manager_lock(auth_mgr);
    } else {
        gtk_widget_show_all(shell->topbar_window);
        gtk_widget_show_all(shell->dock_window);
    }

    shell->clock_timer_id = g_timeout_add_seconds(1, update_clock_and_status, shell);
    update_clock_and_status(shell);

    return shell;
}

void ui_shell_destroy(UiShell *shell) {
    if (!shell) return;
    if (shell->clock_timer_id) g_source_remove(shell->clock_timer_id);
    if (shell->osd_timeout_id) g_source_remove(shell->osd_timeout_id);
    if (shell->notif_timeout_id) g_source_remove(shell->notif_timeout_id);
    g_free(shell->launcher_active_filter);
    g_free(shell);
}
