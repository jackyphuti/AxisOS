#define _GNU_SOURCE
#include "ui_shell.h"
#include <cairo.h>
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline void add_class(GtkWidget *w, const char *cls) {
    if (w && cls) {
        gtk_style_context_add_class(gtk_widget_get_style_context(w), cls);
    }
}

static inline void remove_class(GtkWidget *w, const char *cls) {
    if (w && cls) {
        gtk_style_context_remove_class(gtk_widget_get_style_context(w), cls);
    }
}

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
    UiShell *shell = (UiShell *)user_data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    gboolean dark = TRUE;
    if (shell && shell->sys_ctrl) {
        dark = shell->sys_ctrl->metrics.is_dark_mode;
    }

    double r = 22.0;
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rounded_rect(cr, 0.5, 0.5, alloc.width - 1.0, alloc.height - 1.0, r);

    if (dark) {
        // Base fill rgba(20, 24, 38, 0.86)
        cairo_set_source_rgba(cr, 20.0 / 255.0, 24.0 / 255.0, 38.0 / 255.0, 0.86);
        cairo_fill_preserve(cr);

        // Outer border rgba(255, 255, 255, 0.08)
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.08);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        // 1px top specular arc/line rgba(255, 255, 255, 0.18)
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.18);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, r, 0.5);
        cairo_line_to(cr, alloc.width - r, 0.5);
        cairo_stroke(cr);
    } else {
        // Base fill rgba(244, 246, 251, 0.88)
        cairo_set_source_rgba(cr, 244.0 / 255.0, 246.0 / 255.0, 251.0 / 255.0, 0.88);
        cairo_fill_preserve(cr);

        // Outer border rgba(20, 40, 160, 0.12)
        cairo_set_source_rgba(cr, 20.0 / 255.0, 40.0 / 255.0, 160.0 / 255.0, 0.12);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);

        // 1px top specular highlight rgba(255, 255, 255, 0.75)
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.75);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, r, 0.5);
        cairo_line_to(cr, alloc.width - r, 0.5);
        cairo_stroke(cr);
    }
    cairo_restore(cr);

    return FALSE; // Let child widgets render
}

static gboolean on_draw_topbar(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)widget;
    (void)user_data;
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);
    cairo_restore(cr);
    return FALSE;
}

static gboolean on_draw_greeter_backdrop(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    (void)user_data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);

    double cx = alloc.width / 2.0;
    double cy = alloc.height / 2.0;
    double r = sqrt(cx * cx + cy * cy);
    if (r < 1.0) r = 1.0;

    cairo_pattern_t *pat = cairo_pattern_create_radial(cx, cy, 0, cx, cy, r);
    // Center: #1a2138 -> rgba(26, 33, 56)
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 26.0 / 255.0, 33.0 / 255.0, 56.0 / 255.0, 1.0);
    // Edges: #0f1422 -> rgba(15, 20, 34)
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 15.0 / 255.0, 20.0 / 255.0, 34.0 / 255.0, 1.0);

    cairo_set_source(cr, pat);
    cairo_paint(cr);
    cairo_pattern_destroy(pat);

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

static void update_qs_tiles(UiShell *shell) {
    if (!shell || !shell->sys_ctrl) return;
    SystemMetrics *m = &shell->sys_ctrl->metrics;

    if (shell->qs_tile_wifi && shell->qs_sub_wifi) {
        if (m->wifi_connected) {
            add_class(shell->qs_tile_wifi, "active");
            gtk_label_set_text(GTK_LABEL(shell->qs_sub_wifi), "Connected");
        } else {
            remove_class(shell->qs_tile_wifi, "active");
            gtk_label_set_text(GTK_LABEL(shell->qs_sub_wifi), "Off");
        }
    }

    if (shell->qs_tile_bt && shell->qs_sub_bt) {
        if (m->bluetooth_enabled) {
            add_class(shell->qs_tile_bt, "active");
            gtk_label_set_text(GTK_LABEL(shell->qs_sub_bt), "On");
        } else {
            remove_class(shell->qs_tile_bt, "active");
            gtk_label_set_text(GTK_LABEL(shell->qs_sub_bt), "Off");
        }
    }

    if (shell->qs_tile_dark && shell->qs_sub_dark) {
        if (m->is_dark_mode) {
            add_class(shell->qs_tile_dark, "active");
            gtk_label_set_text(GTK_LABEL(shell->qs_sub_dark), "Active");
        } else {
            remove_class(shell->qs_tile_dark, "active");
            gtk_label_set_text(GTK_LABEL(shell->qs_sub_dark), "Light");
        }
    }

    if (shell->qs_tile_game && shell->qs_sub_game) {
        if (shell->gameboost_active) {
            add_class(shell->qs_tile_game, "active");
            gtk_label_set_text(GTK_LABEL(shell->qs_sub_game), "Performance");
        } else {
            remove_class(shell->qs_tile_game, "active");
            gtk_label_set_text(GTK_LABEL(shell->qs_sub_game), "Balanced");
        }
    }
}

static gboolean on_hide_launcher_timeout(gpointer data) {
    GtkWidget *w = GTK_WIDGET(data);
    if (w) gtk_widget_hide(w);
    return G_SOURCE_REMOVE;
}

static gboolean on_hide_qs_timeout(gpointer data) {
    GtkWidget *w = GTK_WIDGET(data);
    if (w) gtk_widget_hide(w);
    return G_SOURCE_REMOVE;
}

void ui_shell_toggle_launcher(UiShell *shell) {
    if (!shell || !shell->launcher_window) return;
    if (gtk_widget_get_visible(shell->launcher_window)) {
        if (shell->launcher_revealer) {
            gtk_revealer_set_reveal_child(GTK_REVEALER(shell->launcher_revealer), FALSE);
            g_timeout_add(190, on_hide_launcher_timeout, shell->launcher_window);
        } else {
            gtk_widget_hide(shell->launcher_window);
        }
    } else {
        if (shell->quicksettings_window) gtk_widget_hide(shell->quicksettings_window);
        gtk_entry_set_text(GTK_ENTRY(shell->launcher_search_entry), "");
        gtk_widget_show_all(shell->launcher_window);
        if (shell->launcher_revealer) {
            gtk_revealer_set_reveal_child(GTK_REVEALER(shell->launcher_revealer), TRUE);
        }
        gtk_widget_grab_focus(shell->launcher_search_entry);
    }
}

void ui_shell_toggle_quicksettings(UiShell *shell) {
    if (!shell || !shell->quicksettings_window) return;
    if (gtk_widget_get_visible(shell->quicksettings_window)) {
        if (shell->qs_revealer) {
            gtk_revealer_set_reveal_child(GTK_REVEALER(shell->qs_revealer), FALSE);
            g_timeout_add(190, on_hide_qs_timeout, shell->quicksettings_window);
        } else {
            gtk_widget_hide(shell->quicksettings_window);
        }
    } else {
        if (shell->launcher_window) gtk_widget_hide(shell->launcher_window);
        if (shell->qs_vol_scale) {
            gtk_range_set_value(GTK_RANGE(shell->qs_vol_scale), shell->sys_ctrl->metrics.volume);
        }
        if (shell->qs_bright_scale) {
            gtk_range_set_value(GTK_RANGE(shell->qs_bright_scale), shell->sys_ctrl->metrics.brightness);
        }
        update_qs_tiles(shell);
        gtk_widget_show_all(shell->quicksettings_window);
        if (shell->qs_revealer) {
            gtk_revealer_set_reveal_child(GTK_REVEALER(shell->qs_revealer), TRUE);
        }
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

    if (shell->launcher_filter_box) {
        GList *children = gtk_container_get_children(GTK_CONTAINER(shell->launcher_filter_box));
        for (GList *l = children; l != NULL; l = l->next) {
            remove_class(GTK_WIDGET(l->data), "active");
        }
        g_list_free(children);
    }
    add_class(GTK_WIDGET(btn), "active");

    filter_launcher_apps(shell);
}

static void on_workspace_clicked(GtkButton *btn, gpointer user_data) {
    UiShell *shell = (UiShell *)user_data;
    for (int i = 0; i < 4; i++) {
        if (shell->workspace_btns[i]) {
            remove_class(shell->workspace_btns[i], "active");
        }
    }
    add_class(GTK_WIDGET(btn), "active");
}

static gboolean update_clock_and_status(gpointer user_data) {
    UiShell *shell = (UiShell *)user_data;
    if (!shell) return G_SOURCE_REMOVE;

    GDateTime *now = g_date_time_new_now_local();
    if (now) {
        gchar *time_str = g_date_time_format(now, "%Y-%m-%d  %H:%M");
        if (shell->topbar_clock_label) {
            gtk_label_set_text(GTK_LABEL(shell->topbar_clock_label), time_str);
        }
        g_free(time_str);

        if (shell->greeter_clock_label) {
            gchar *gtime = g_date_time_format(now, "%H:%M");
            gtk_label_set_text(GTK_LABEL(shell->greeter_clock_label), gtime);
            g_free(gtime);
        }
        if (shell->greeter_date_label) {
            gchar *gdate = g_date_time_format(now, "%A, %B %d");
            gtk_label_set_text(GTK_LABEL(shell->greeter_date_label), gdate);
            g_free(gdate);
        }

        g_date_time_unref(now);
    }

    if (shell->sys_ctrl) {
        SystemMetrics *m = &shell->sys_ctrl->metrics;
        char buf[64];

        if (shell->topbar_cpu_label) {
            snprintf(buf, sizeof(buf), "%d%%", m->cpu_load_percent);
            gtk_label_set_text(GTK_LABEL(shell->topbar_cpu_label), buf);
        }
        if (shell->topbar_ram_label) {
            snprintf(buf, sizeof(buf), "%d%%", m->ram_usage_percent);
            gtk_label_set_text(GTK_LABEL(shell->topbar_ram_label), buf);
        }
        if (shell->topbar_bat_label) {
            snprintf(buf, sizeof(buf), "%d%%%s", m->battery_percent, m->battery_charging ? " ⚡" : "");
            gtk_label_set_text(GTK_LABEL(shell->topbar_bat_label), buf);
        }
        if (shell->topbar_vol_label) {
            snprintf(buf, sizeof(buf), "%d%%", m->volume);
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

static GtkWidget *create_qs_tile(const char *icon_name, const char *title, const char *sub_init,
                                 GtkWidget **out_icon, GtkWidget **out_sub) {
    GtkWidget *btn = gtk_button_new();
    add_class(btn, "qs-tile");

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_BUTTON);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *title_lbl = gtk_label_new(title);
    add_class(title_lbl, "title-4");
    gtk_widget_set_halign(title_lbl, GTK_ALIGN_START);
    GtkWidget *sub_lbl = gtk_label_new(sub_init);
    add_class(sub_lbl, "caption");
    gtk_widget_set_halign(sub_lbl, GTK_ALIGN_START);

    gtk_box_pack_start(GTK_BOX(vbox), title_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), sub_lbl, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0);
    gtk_container_add(GTK_CONTAINER(btn), hbox);

    if (out_icon) *out_icon = icon;
    if (out_sub) *out_sub = sub_lbl;
    return btn;
}

static void on_qs_tile_wifi_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    UiShell *shell = (UiShell *)user_data;
    if (!shell || !shell->sys_ctrl) return;
    bool new_state = !shell->sys_ctrl->metrics.wifi_connected;
    system_controller_toggle_wifi(shell->sys_ctrl, new_state);
    shell->sys_ctrl->metrics.wifi_connected = new_state;
    update_qs_tiles(shell);
}

static void on_qs_tile_bt_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    UiShell *shell = (UiShell *)user_data;
    if (!shell || !shell->sys_ctrl) return;
    bool new_state = !shell->sys_ctrl->metrics.bluetooth_enabled;
    system_controller_toggle_bluetooth(shell->sys_ctrl, new_state);
    shell->sys_ctrl->metrics.bluetooth_enabled = new_state;
    update_qs_tiles(shell);
}

static void on_qs_tile_dark_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    UiShell *shell = (UiShell *)user_data;
    if (!shell || !shell->sys_ctrl) return;
    bool new_dark = !shell->sys_ctrl->metrics.is_dark_mode;
    shell->sys_ctrl->metrics.is_dark_mode = new_dark;
    system_controller_set_dark_mode(shell->sys_ctrl, new_dark);
    ui_shell_apply_theme(new_dark);
    update_qs_tiles(shell);

    if (shell->topbar_window) gtk_widget_queue_draw(shell->topbar_window);
    if (shell->dock_window) gtk_widget_queue_draw(shell->dock_window);
    if (shell->launcher_window) gtk_widget_queue_draw(shell->launcher_window);
    if (shell->quicksettings_window) gtk_widget_queue_draw(shell->quicksettings_window);
    if (shell->osd_window) gtk_widget_queue_draw(shell->osd_window);
    if (shell->notification_window) gtk_widget_queue_draw(shell->notification_window);
    if (shell->greeter_window) gtk_widget_queue_draw(shell->greeter_window);
}

static void on_qs_tile_game_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    UiShell *shell = (UiShell *)user_data;
    if (!shell) return;
    shell->gameboost_active = !shell->gameboost_active;
    update_qs_tiles(shell);
}

static void on_qs_vol_changed(GtkRange *range, gpointer user_data) {
    UiShell *shell = (UiShell *)user_data;
    if (!shell || !shell->sys_ctrl) return;
    int val = (int)gtk_range_get_value(range);
    system_controller_set_volume(shell->sys_ctrl, val);
    ui_shell_show_osd(shell, val == 0 ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic", val);
}

static void on_qs_bright_changed(GtkRange *range, gpointer user_data) {
    UiShell *shell = (UiShell *)user_data;
    if (!shell || !shell->sys_ctrl) return;
    int val = (int)gtk_range_get_value(range);
    system_controller_set_brightness(shell->sys_ctrl, val);
    ui_shell_show_osd(shell, "display-brightness-symbolic", val);
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
    // 1. TOP BAR (LiquidTopBar - 3 Islands)
    // ==========================================
    shell->topbar_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_layer_window(shell->topbar_window, GTK_LAYER_SHELL_LAYER_TOP, "LiquidTopBar");

    if (gtk_layer_is_supported()) {
        gtk_layer_set_anchor(GTK_WINDOW(shell->topbar_window), GTK_LAYER_SHELL_EDGE_LEFT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->topbar_window), GTK_LAYER_SHELL_EDGE_RIGHT, TRUE);
        gtk_layer_set_anchor(GTK_WINDOW(shell->topbar_window), GTK_LAYER_SHELL_EDGE_TOP, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(shell->topbar_window), GTK_LAYER_SHELL_EDGE_TOP, 6);
        gtk_layer_set_margin(GTK_WINDOW(shell->topbar_window), GTK_LAYER_SHELL_EDGE_LEFT, 12);
        gtk_layer_set_margin(GTK_WINDOW(shell->topbar_window), GTK_LAYER_SHELL_EDGE_RIGHT, 12);
        gtk_layer_set_exclusive_zone(GTK_WINDOW(shell->topbar_window), 44);
    }
    gtk_widget_set_size_request(shell->topbar_window, -1, 36);
    g_signal_connect(shell->topbar_window, "draw", G_CALLBACK(on_draw_topbar), shell);

    GtkWidget *topbar_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(shell->topbar_window), topbar_box);

    // Left Island: App Launcher button + 4 workspace buttons
    GtkWidget *left_island = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    add_class(left_island, "topbar-pill");

    GtkWidget *launcher_btn = gtk_button_new_from_icon_name("view-app-grid-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(launcher_btn, "Applications");
    add_class(launcher_btn, "workspace-pill");
    g_signal_connect_swapped(launcher_btn, "clicked", G_CALLBACK(ui_shell_toggle_launcher), shell);
    gtk_box_pack_start(GTK_BOX(left_island), launcher_btn, FALSE, FALSE, 2);

    for (int i = 0; i < 4; ++i) {
        char ws_name[4];
        snprintf(ws_name, sizeof(ws_name), "%d", i + 1);
        GtkWidget *ws_btn = gtk_button_new_with_label(ws_name);
        add_class(ws_btn, "workspace-pill");
        if (i == 0) {
            add_class(ws_btn, "active");
        }
        shell->workspace_btns[i] = ws_btn;
        g_signal_connect(ws_btn, "clicked", G_CALLBACK(on_workspace_clicked), shell);
        gtk_box_pack_start(GTK_BOX(left_island), ws_btn, FALSE, FALSE, 2);
    }
    gtk_box_pack_start(GTK_BOX(topbar_box), left_island, FALSE, FALSE, 0);

    // Center Island: Centered clock label
    GtkWidget *center_island = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    add_class(center_island, "topbar-pill");
    shell->topbar_clock_label = gtk_label_new("Loading...");
    add_class(shell->topbar_clock_label, "title-4");
    gtk_label_set_single_line_mode(GTK_LABEL(shell->topbar_clock_label), TRUE);
    gtk_box_pack_start(GTK_BOX(center_island), shell->topbar_clock_label, TRUE, TRUE, 6);
    gtk_box_set_center_widget(GTK_BOX(topbar_box), center_island);

    // Right Island: Icon + compact value pairs + Quick Settings trigger
    GtkWidget *right_island = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    add_class(right_island, "topbar-pill");

    // CPU
    shell->topbar_cpu_icon = gtk_image_new_from_icon_name("utilities-system-monitor-symbolic", GTK_ICON_SIZE_MENU);
    shell->topbar_cpu_label = gtk_label_new("0%");
    add_class(shell->topbar_cpu_label, "caption");
    gtk_label_set_single_line_mode(GTK_LABEL(shell->topbar_cpu_label), TRUE);
    gtk_box_pack_start(GTK_BOX(right_island), shell->topbar_cpu_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_island), shell->topbar_cpu_label, FALSE, FALSE, 2);

    // Network
    shell->topbar_net_icon = gtk_image_new_from_icon_name("network-wireless-signal-excellent-symbolic", GTK_ICON_SIZE_MENU);
    shell->topbar_net_label = gtk_label_new("Online");
    add_class(shell->topbar_net_label, "caption");
    gtk_label_set_single_line_mode(GTK_LABEL(shell->topbar_net_label), TRUE);
    gtk_box_pack_start(GTK_BOX(right_island), shell->topbar_net_icon, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(right_island), shell->topbar_net_label, FALSE, FALSE, 2);

    // Audio
    shell->topbar_vol_icon = gtk_image_new_from_icon_name("audio-volume-high-symbolic", GTK_ICON_SIZE_MENU);
    shell->topbar_vol_label = gtk_label_new("70%");
    add_class(shell->topbar_vol_label, "caption");
    gtk_label_set_single_line_mode(GTK_LABEL(shell->topbar_vol_label), TRUE);
    gtk_box_pack_start(GTK_BOX(right_island), shell->topbar_vol_icon, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(right_island), shell->topbar_vol_label, FALSE, FALSE, 2);

    // Battery
    shell->topbar_bat_icon = gtk_image_new_from_icon_name("battery-good-symbolic", GTK_ICON_SIZE_MENU);
    shell->topbar_bat_label = gtk_label_new("100%");
    add_class(shell->topbar_bat_label, "caption");
    gtk_label_set_single_line_mode(GTK_LABEL(shell->topbar_bat_label), TRUE);
    gtk_box_pack_start(GTK_BOX(right_island), shell->topbar_bat_icon, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(right_island), shell->topbar_bat_label, FALSE, FALSE, 2);

    // Quick Settings trigger button
    GtkWidget *qs_btn = gtk_button_new_from_icon_name("open-menu-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_widget_set_tooltip_text(qs_btn, "Quick Settings");
    add_class(qs_btn, "workspace-pill");
    g_signal_connect_swapped(qs_btn, "clicked", G_CALLBACK(ui_shell_toggle_quicksettings), shell);
    gtk_box_pack_start(GTK_BOX(right_island), qs_btn, FALSE, FALSE, 4);

    gtk_box_pack_end(GTK_BOX(topbar_box), right_island, FALSE, FALSE, 0);

    // ==========================================
    // 2. DOCK (LiquidDock)
    // ==========================================
    shell->dock_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    setup_layer_window(shell->dock_window, GTK_LAYER_SHELL_LAYER_TOP, "LiquidDock");

    if (gtk_layer_is_supported()) {
        gtk_layer_set_anchor(GTK_WINDOW(shell->dock_window), GTK_LAYER_SHELL_EDGE_BOTTOM, TRUE);
        gtk_layer_set_margin(GTK_WINDOW(shell->dock_window), GTK_LAYER_SHELL_EDGE_BOTTOM, 10);
        gtk_layer_set_exclusive_zone(GTK_WINDOW(shell->dock_window), 72);
    }
    g_signal_connect(shell->dock_window, "draw", G_CALLBACK(on_draw_glassmorphic), shell);

    GtkWidget *dock_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    add_class(dock_box, "dock-container");
    gtk_container_set_border_width(GTK_CONTAINER(dock_box), 4);
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
        add_class(btn, "dock-tile");
        GtkWidget *img = gtk_image_new_from_icon_name(pinned_apps[i].icon, GTK_ICON_SIZE_INVALID);
        gtk_image_set_pixel_size(GTK_IMAGE(img), 48);
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
        gtk_layer_set_margin(GTK_WINDOW(shell->launcher_window), GTK_LAYER_SHELL_EDGE_TOP, 50);
        gtk_layer_set_keyboard_mode(GTK_WINDOW(shell->launcher_window), GTK_LAYER_SHELL_KEYBOARD_MODE_ON_DEMAND);
    }
    gtk_window_set_default_size(GTK_WINDOW(shell->launcher_window), 680, 520);
    g_signal_connect(shell->launcher_window, "draw", G_CALLBACK(on_draw_glassmorphic), shell);

    shell->launcher_revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(shell->launcher_revealer), GTK_REVEALER_TRANSITION_TYPE_CROSSFADE);
    gtk_revealer_set_transition_duration(GTK_REVEALER(shell->launcher_revealer), 180);
    gtk_container_add(GTK_CONTAINER(shell->launcher_window), shell->launcher_revealer);

    GtkWidget *launcher_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(launcher_card), 18);
    gtk_container_add(GTK_CONTAINER(shell->launcher_revealer), launcher_card);

    shell->launcher_search_entry = gtk_search_entry_new();
    add_class(shell->launcher_search_entry, "launcher-search");
    gtk_entry_set_placeholder_text(GTK_ENTRY(shell->launcher_search_entry), "Search applications, utilities, commands...");
    g_signal_connect(shell->launcher_search_entry, "search-changed", G_CALLBACK(on_launcher_search_changed), shell);
    gtk_box_pack_start(GTK_BOX(launcher_card), shell->launcher_search_entry, FALSE, FALSE, 4);

    // Filter pills
    shell->launcher_filter_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    const char *cats[] = {"All", "System", "Utility", "Development", "Network", "AudioVideo", NULL};
    for (int i = 0; cats[i] != NULL; ++i) {
        GtkWidget *cat_btn = gtk_button_new_with_label(cats[i]);
        add_class(cat_btn, "filter-pill");
        if (strcmp(cats[i], "All") == 0) {
            add_class(cat_btn, "active");
        }
        g_signal_connect(cat_btn, "clicked", G_CALLBACK(on_filter_btn_clicked), shell);
        gtk_box_pack_start(GTK_BOX(shell->launcher_filter_box), cat_btn, FALSE, FALSE, 2);
    }
    gtk_box_pack_start(GTK_BOX(launcher_card), shell->launcher_filter_box, FALSE, FALSE, 4);

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
            add_class(app_btn, "app-tile");
            gtk_widget_set_size_request(app_btn, 112, 98);

            GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
            GtkWidget *img = gtk_image_new_from_icon_name(app->icon, GTK_ICON_SIZE_DND);
            GtkWidget *lbl = gtk_label_new(app->name);
            add_class(lbl, "caption");
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
        gtk_layer_set_margin(GTK_WINDOW(shell->quicksettings_window), GTK_LAYER_SHELL_EDGE_TOP, 50);
        gtk_layer_set_margin(GTK_WINDOW(shell->quicksettings_window), GTK_LAYER_SHELL_EDGE_RIGHT, 12);
    }
    gtk_widget_set_size_request(shell->quicksettings_window, 360, -1);
    g_signal_connect(shell->quicksettings_window, "draw", G_CALLBACK(on_draw_glassmorphic), shell);

    shell->qs_revealer = gtk_revealer_new();
    gtk_revealer_set_transition_type(GTK_REVEALER(shell->qs_revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
    gtk_revealer_set_transition_duration(GTK_REVEALER(shell->qs_revealer), 180);
    gtk_container_add(GTK_CONTAINER(shell->quicksettings_window), shell->qs_revealer);

    GtkWidget *qs_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(qs_card), 18);
    gtk_container_add(GTK_CONTAINER(shell->qs_revealer), qs_card);

    // Title / user header
    GtkWidget *user_hdr = gtk_label_new(auth_mgr ? auth_mgr->display_name : "AxisOS User");
    add_class(user_hdr, "title-3");
    gtk_widget_set_halign(user_hdr, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(qs_card), user_hdr, FALSE, FALSE, 0);

    // Modern 2x2 Tile Grid
    GtkWidget *qs_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(qs_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(qs_grid), 10);
    gtk_grid_set_row_homogeneous(GTK_GRID(qs_grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(qs_grid), TRUE);

    shell->qs_tile_wifi = create_qs_tile("network-wireless-signal-excellent-symbolic", "Wi-Fi", "Off", NULL, &shell->qs_sub_wifi);
    g_signal_connect(shell->qs_tile_wifi, "clicked", G_CALLBACK(on_qs_tile_wifi_clicked), shell);
    gtk_grid_attach(GTK_GRID(qs_grid), shell->qs_tile_wifi, 0, 0, 1, 1);

    shell->qs_tile_bt = create_qs_tile("bluetooth-active-symbolic", "Bluetooth", "Off", NULL, &shell->qs_sub_bt);
    g_signal_connect(shell->qs_tile_bt, "clicked", G_CALLBACK(on_qs_tile_bt_clicked), shell);
    gtk_grid_attach(GTK_GRID(qs_grid), shell->qs_tile_bt, 1, 0, 1, 1);

    shell->qs_tile_dark = create_qs_tile("weather-clear-night-symbolic", "Dark Theme", "Active", NULL, &shell->qs_sub_dark);
    g_signal_connect(shell->qs_tile_dark, "clicked", G_CALLBACK(on_qs_tile_dark_clicked), shell);
    gtk_grid_attach(GTK_GRID(qs_grid), shell->qs_tile_dark, 0, 1, 1, 1);

    shell->qs_tile_game = create_qs_tile("applications-games-symbolic", "GameBoost", "Balanced", NULL, &shell->qs_sub_game);
    g_signal_connect(shell->qs_tile_game, "clicked", G_CALLBACK(on_qs_tile_game_clicked), shell);
    gtk_grid_attach(GTK_GRID(qs_grid), shell->qs_tile_game, 1, 1, 1, 1);

    gtk_box_pack_start(GTK_BOX(qs_card), qs_grid, FALSE, FALSE, 4);

    // Sliders: Volume & Brightness with icons beside them
    GtkWidget *vol_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *vol_icon = gtk_image_new_from_icon_name("audio-volume-high-symbolic", GTK_ICON_SIZE_BUTTON);
    shell->qs_vol_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(shell->qs_vol_scale), FALSE);
    gtk_widget_set_hexpand(shell->qs_vol_scale, TRUE);
    g_signal_connect(shell->qs_vol_scale, "value-changed", G_CALLBACK(on_qs_vol_changed), shell);
    gtk_box_pack_start(GTK_BOX(vol_row), vol_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vol_row), shell->qs_vol_scale, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(qs_card), vol_row, FALSE, FALSE, 2);

    GtkWidget *bright_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *bright_icon = gtk_image_new_from_icon_name("display-brightness-symbolic", GTK_ICON_SIZE_BUTTON);
    shell->qs_bright_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 5, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(shell->qs_bright_scale), FALSE);
    gtk_widget_set_hexpand(shell->qs_bright_scale, TRUE);
    g_signal_connect(shell->qs_bright_scale, "value-changed", G_CALLBACK(on_qs_bright_changed), shell);
    gtk_box_pack_start(GTK_BOX(bright_row), bright_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bright_row), shell->qs_bright_scale, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(qs_card), bright_row, FALSE, FALSE, 2);

    // Power buttons with icons and secondary-button class
    GtkWidget *pwr_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);

    GtkWidget *lock_btn = gtk_button_new_from_icon_name("system-lock-screen-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_label(GTK_BUTTON(lock_btn), " Lock");
    gtk_button_set_always_show_image(GTK_BUTTON(lock_btn), TRUE);
    add_class(lock_btn, "secondary-button");
    g_signal_connect_swapped(lock_btn, "clicked", G_CALLBACK(ui_shell_lock), shell);

    GtkWidget *settings_btn = gtk_button_new_from_icon_name("preferences-system-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_label(GTK_BUTTON(settings_btn), " Settings");
    gtk_button_set_always_show_image(GTK_BUTTON(settings_btn), TRUE);
    add_class(settings_btn, "secondary-button");
    g_signal_connect(settings_btn, "clicked", G_CALLBACK(on_launcher_app_clicked), (gpointer)"axis-settings");

    GtkWidget *power_btn = gtk_button_new_from_icon_name("system-shutdown-symbolic", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_label(GTK_BUTTON(power_btn), " Power");
    gtk_button_set_always_show_image(GTK_BUTTON(power_btn), TRUE);
    add_class(power_btn, "secondary-button");
    g_signal_connect_swapped(power_btn, "clicked", G_CALLBACK(system_controller_power_off), sys_ctrl);

    gtk_box_pack_start(GTK_BOX(pwr_box), lock_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pwr_box), settings_btn, TRUE, TRUE, 0);
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
        gtk_layer_set_margin(GTK_WINDOW(shell->notification_window), GTK_LAYER_SHELL_EDGE_TOP, 50);
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
    add_class(shell->notif_title, "title-4");
    gtk_widget_set_halign(shell->notif_title, GTK_ALIGN_START);
    shell->notif_body = gtk_label_new("");
    add_class(shell->notif_body, "caption");
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
    g_signal_connect(shell->greeter_window, "draw", G_CALLBACK(on_draw_greeter_backdrop), shell);

    GtkWidget *greeter_center = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(greeter_center, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(greeter_center, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(shell->greeter_window), greeter_center);

    // Large Live Clock & Date above authentication card
    GtkWidget *clock_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_set_halign(clock_box, GTK_ALIGN_CENTER);

    shell->greeter_clock_label = gtk_label_new("12:00");
    add_class(shell->greeter_clock_label, "greeter-clock");
    gtk_box_pack_start(GTK_BOX(clock_box), shell->greeter_clock_label, FALSE, FALSE, 0);

    shell->greeter_date_label = gtk_label_new("Monday, January 01");
    add_class(shell->greeter_date_label, "greeter-date");
    gtk_box_pack_start(GTK_BOX(clock_box), shell->greeter_date_label, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(greeter_center), clock_box, FALSE, FALSE, 8);

    // Centered Frosted Authentication Card
    GtkWidget *greeter_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    add_class(greeter_card, "greeter-card");
    gtk_widget_set_size_request(greeter_card, 340, -1);
    gtk_box_pack_start(GTK_BOX(greeter_center), greeter_card, FALSE, FALSE, 0);

    GtkWidget *avatar = gtk_image_new_from_icon_name("avatar-default", GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(greeter_card), avatar, FALSE, FALSE, 4);

    GtkWidget *user_name_lbl = gtk_label_new(auth_mgr ? auth_mgr->display_name : "AxisOS User");
    add_class(user_name_lbl, "title-3");
    gtk_box_pack_start(GTK_BOX(greeter_card), user_name_lbl, FALSE, FALSE, 4);

    shell->greeter_pass_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(shell->greeter_pass_entry), FALSE);
    gtk_entry_set_placeholder_text(GTK_ENTRY(shell->greeter_pass_entry), "Password");
    g_signal_connect(shell->greeter_pass_entry, "activate", G_CALLBACK(on_greeter_submit), shell);
    gtk_box_pack_start(GTK_BOX(greeter_card), shell->greeter_pass_entry, FALSE, FALSE, 4);

    GtkWidget *submit_btn = gtk_button_new_with_label("Unlock");
    add_class(submit_btn, "suggested-action");
    g_signal_connect(submit_btn, "clicked", G_CALLBACK(on_greeter_submit), shell);
    gtk_box_pack_start(GTK_BOX(greeter_card), submit_btn, FALSE, FALSE, 4);

    shell->greeter_spinner = gtk_spinner_new();
    gtk_box_pack_start(GTK_BOX(greeter_card), shell->greeter_spinner, FALSE, FALSE, 2);

    shell->greeter_status_label = gtk_label_new(auth_mgr && auth_mgr->has_fingerprint ?
                                                "Scan fingerprint or enter password" : "");
    add_class(shell->greeter_status_label, "caption");
    gtk_box_pack_start(GTK_BOX(greeter_card), shell->greeter_status_label, FALSE, FALSE, 4);

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
    update_qs_tiles(shell);

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
