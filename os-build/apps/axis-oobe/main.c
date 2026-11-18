/*
 * AxisOS Native Out-of-Box Experience (OOBE) First-Boot Wizard
 * Written in Pure C11 (-std=gnu11) using GTK 3
 * Features:
 *  - Hardware detection (CPU, RAM, GPUs)
 *  - Network connectivity enforcement
 *  - System onboarding and ready state
 */

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *lbl_net_status;
    GtkWidget *switch_wifi;
    GtkWidget *list_wifi;
    GtkWidget *btn_scan;
    GtkWidget *lbl_scan_status;
} AxisOobeApp;

static char *exec_cmd(const char *cmd) {
    if (!cmd) return g_strdup("");
    FILE *pipe = popen(cmd, "r");
    if (!pipe) return g_strdup("");

    GString *result = g_string_new("");
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        g_string_append(result, buffer);
    }
    pclose(pipe);
    return g_string_free(result, FALSE);
}

static const char *CSS_OOBE =
    "@define-color axis_base #141826;"
    "@define-color axis_surface #1F2231;"
    "@define-color axis_surface_raised #262A3D;"
    "@define-color axis_outline #2E3349;"
    "@define-color axis_accent #7D95FF;"
    "@define-color axis_accent_btn_text #0F1422;"
    "@define-color axis_text #E2E8F0;"
    "@define-color axis_text_muted #94A3B8;"
    "@define-color axis_danger #F87171;"
    "@define-color axis_success #34D399;"
    "window, dialog, headerbar, button, entry, switch {"
    "    background-image: none;"
    "    box-shadow: none;"
    "}"
    "window, dialog {"
    "    background-color: @axis_base;"
    "    color: @axis_text;"
    "    font-family: 'IBM Plex Sans', sans-serif;"
    "}"
    "headerbar {"
    "    background-color: @axis_surface;"
    "    color: @axis_text;"
    "    border-bottom: 1px solid @axis_outline;"
    "}"
    "entry {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 10px;"
    "    padding: 8px 12px;"
    "}"
    "entry:focus {"
    "    border-color: @axis_accent;"
    "}"
    ".card {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 16px;"
    "    padding: 20px;"
    "    margin: 8px 16px;"
    "}"
    ".btn-primary {"
    "    background: @axis_accent;"
    "    background-color: @axis_accent;"
    "    color: @axis_accent_btn_text;"
    "    font-weight: 700;"
    "    border-radius: 9999px;"
    "    border: 1px solid @axis_accent;"
    "    padding: 10px 24px;"
    "}"
    ".btn-primary:hover {"
    "    background-color: mix(@axis_accent, @axis_base, 0.12);"
    "}"
    ".btn-secondary {"
    "    background: @axis_surface;"
    "    background-color: @axis_surface;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 9999px;"
    "    padding: 8px 18px;"
    "    font-weight: 600;"
    "}"
    ".btn-secondary:hover {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_accent;"
    "}"
    ".badge-green {"
    "    background-color: @axis_success;"
    "    color: #0F1422;"
    "    font-weight: 700;"
    "    font-size: 11px;"
    "    padding: 2px 8px;"
    "    border-radius: 9999px;"
    "}"
    ".dim-label {"
    "    color: @axis_text_muted;"
    "    font-size: 12px;"
    "}"
    "listbox {"
    "    background-color: @axis_surface;"
    "    border-radius: 12px;"
    "}"
    "listbox row {"
    "    background-color: transparent;"
    "    color: @axis_text;"
    "    padding: 6px 10px;"
    "    border-bottom: 1px solid @axis_outline;"
    "    border-radius: 8px;"
    "}"
    "listbox row:hover {"
    "    background-color: @axis_surface_raised;"
    "}"
    "listbox row:selected {"
    "    background-color: @axis_accent;"
    "    color: @axis_accent_btn_text;"
    "}";

static void load_axis_theme(void) {
    GdkScreen *screen = gdk_screen_get_default();
    if (!screen) return;

    gboolean dark_mode = TRUE;
    const char *home = g_get_home_dir();
    if (home) {
        char conf_path[512];
        snprintf(conf_path, sizeof(conf_path), "%s/.config/axisos/theme.conf", home);
        FILE *f = fopen(conf_path, "r");
        if (f) {
            char buf[128];
            if (fgets(buf, sizeof(buf), f)) {
                if (strstr(buf, "light") || strstr(buf, "Light")) {
                    dark_mode = FALSE;
                }
            }
            fclose(f);
        }
    }

    const char *dark_paths[] = {
        "data/theme/axis-dark.css",
        "/usr/share/axisos/theme/axis-dark.css",
        "/usr/local/share/axisos/theme/axis-dark.css",
        NULL
    };
    const char *light_paths[] = {
        "data/theme/axis-light.css",
        "/usr/share/axisos/theme/axis-light.css",
        "/usr/local/share/axisos/theme/axis-light.css",
        NULL
    };

    const char **paths = dark_mode ? dark_paths : light_paths;
    GtkCssProvider *provider = gtk_css_provider_new();
    for (int i = 0; paths[i] != NULL; ++i) {
        if (access(paths[i], R_OK) == 0) {
            if (gtk_css_provider_load_from_path(provider, paths[i], NULL)) {
                gtk_style_context_add_provider_for_screen(
                    screen,
                    GTK_STYLE_PROVIDER(provider),
                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
                );
                break;
            }
        }
    }
    g_object_unref(provider);

    GtkCssProvider *extra_prov = gtk_css_provider_new();
    gtk_css_provider_load_from_data(extra_prov, CSS_OOBE, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(extra_prov),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1
    );
    g_object_unref(extra_prov);
}

static char *check_network(void) {
    char *ping = exec_cmd("ping -c 1 -W 2 1.1.1.1 2>/dev/null && echo 'CONNECTED' || echo 'OFFLINE'");
    char *res;
    if (strstr(ping, "CONNECTED") != NULL) {
        res = g_strdup("✓ Connected to the Internet. Online services, updates, and Steam are accessible.");
    } else {
        res = g_strdup("⚠ No active internet connection detected. You can configure Wi-Fi in Settings or proceed offline.");
    }
    g_free(ping);
    return res;
}

static char **parse_nmcli_terse_line(const char *line, int expected_fields) {
    char **fields = g_new0(char *, expected_fields + 1);
    int field_idx = 0;
    GString *cur = g_string_new("");
    for (int i = 0; line[i] != '\0' && field_idx < expected_fields; i++) {
        if (line[i] == '\\' && line[i+1] == ':') {
            g_string_append_c(cur, ':');
            i++;
        } else if (line[i] == '\\' && line[i+1] == '\\') {
            g_string_append_c(cur, '\\');
            i++;
        } else if (line[i] == ':') {
            fields[field_idx++] = g_string_free(cur, FALSE);
            cur = g_string_new("");
        } else {
            g_string_append_c(cur, line[i]);
        }
    }
    if (field_idx < expected_fields) {
        fields[field_idx++] = g_string_free(cur, FALSE);
    } else {
        g_string_free(cur, TRUE);
    }
    return fields;
}

static const char *get_wifi_icon_name(int signal) {
    if (signal >= 75) return "network-wireless-signal-excellent-symbolic";
    if (signal >= 50) return "network-wireless-signal-good-symbolic";
    if (signal >= 25) return "network-wireless-signal-ok-symbolic";
    if (signal > 0) return "network-wireless-signal-weak-symbolic";
    return "network-wireless-signal-none-symbolic";
}

static void on_show_pass_toggled(GtkToggleButton *tb, gpointer user_data) {
    GtkEntry *entry = GTK_ENTRY(user_data);
    gtk_entry_set_visibility(entry, gtk_toggle_button_get_active(tb));
}

static void refresh_wifi_list(AxisOobeApp *app);

static gboolean on_delayed_wifi_refresh(gpointer user_data) {
    AxisOobeApp *app = (AxisOobeApp *)user_data;
    if (app && app->lbl_scan_status) {
        gtk_label_set_text(GTK_LABEL(app->lbl_scan_status), "");
    }
    if (app && app->lbl_net_status) {
        char *status = check_network();
        gtk_label_set_text(GTK_LABEL(app->lbl_net_status), status);
        g_free(status);
    }
    refresh_wifi_list(app);
    return G_SOURCE_REMOVE;
}

static void trigger_wifi_rescan(AxisOobeApp *app) {
    if (app->lbl_scan_status) {
        gtk_label_set_text(GTK_LABEL(app->lbl_scan_status), "Scanning...");
    }
    g_spawn_command_line_async("nmcli radio wifi on && nmcli device wifi rescan", NULL);
    g_timeout_add(1500, (GSourceFunc)on_delayed_wifi_refresh, app);
}

static void on_wifi_row_activated(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)box;
    AxisOobeApp *app = (AxisOobeApp *)user_data;
    if (!row) return;

    const char *ssid = (const char *)g_object_get_data(G_OBJECT(row), "ssid");
    const char *sec = (const char *)g_object_get_data(G_OBJECT(row), "security");
    gboolean in_use = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(row), "in_use"));

    if (!ssid || ssid[0] == '\0') return;
    if (in_use) return;

    gboolean is_secured = (sec && sec[0] != '\0' && g_strcmp0(sec, "--") != 0);

    if (!is_secured) {
        if (app->lbl_scan_status) {
            gtk_label_set_text(GTK_LABEL(app->lbl_scan_status), "Connecting...");
        }
        char *q_ssid = g_shell_quote(ssid);
        char *cmd = g_strdup_printf("nmcli device wifi connect %s", q_ssid);
        g_spawn_command_line_async(cmd, NULL);
        g_free(cmd);
        g_free(q_ssid);
        g_timeout_add(2500, (GSourceFunc)on_delayed_wifi_refresh, app);
    } else {
        GtkWidget *dialog = gtk_dialog_new_with_buttons(
            "Connect to Wi-Fi",
            GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Connect", GTK_RESPONSE_ACCEPT,
            NULL
        );
        gtk_window_set_default_size(GTK_WINDOW(dialog), 380, 160);

        GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
        gtk_container_set_border_width(GTK_CONTAINER(content), 16);
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

        char *prompt_text = g_strdup_printf("Enter password for \"%s\":", ssid);
        GtkWidget *lbl = gtk_label_new(prompt_text);
        g_free(prompt_text);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_box_pack_start(GTK_BOX(vbox), lbl, FALSE, FALSE, 0);

        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
        gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

        GtkWidget *chk_show = gtk_check_button_new_with_label("Show Password");
        g_signal_connect(chk_show, "toggled", G_CALLBACK(on_show_pass_toggled), entry);
        gtk_box_pack_start(GTK_BOX(vbox), chk_show, FALSE, FALSE, 0);

        gtk_container_add(GTK_CONTAINER(content), vbox);
        gtk_widget_show_all(dialog);

        gint res = gtk_dialog_run(GTK_DIALOG(dialog));
        if (res == GTK_RESPONSE_ACCEPT) {
            const char *pass = gtk_entry_get_text(GTK_ENTRY(entry));
            if (app->lbl_scan_status) {
                gtk_label_set_text(GTK_LABEL(app->lbl_scan_status), "Connecting...");
            }
            char *q_ssid = g_shell_quote(ssid);
            char *q_pass = g_shell_quote(pass ? pass : "");
            char *cmd = g_strdup_printf("nmcli device wifi connect %s password %s", q_ssid, q_pass);
            g_spawn_command_line_async(cmd, NULL);
            g_free(cmd);
            g_free(q_ssid);
            g_free(q_pass);
            g_timeout_add(2500, (GSourceFunc)on_delayed_wifi_refresh, app);
        }
        gtk_widget_destroy(dialog);
    }
}

static void on_wifi_switch_active_changed(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    AxisOobeApp *app = (AxisOobeApp *)user_data;
    gboolean active = gtk_switch_get_active(sw);
    char *cmd = g_strdup_printf("nmcli radio wifi %s", active ? "on" : "off");
    g_spawn_command_line_async(cmd, NULL);
    g_free(cmd);
    if (active) {
        trigger_wifi_rescan(app);
    } else {
        GList *children = gtk_container_get_children(GTK_CONTAINER(app->list_wifi));
        for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(children);
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new("Wi-Fi is turned off.");
        gtk_container_add(GTK_CONTAINER(row), lbl);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_wifi), row, -1);
        gtk_widget_show_all(app->list_wifi);
    }
}

static void on_scan_clicked_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisOobeApp *app = (AxisOobeApp *)user_data;
    trigger_wifi_rescan(app);
}

static void refresh_wifi_list(AxisOobeApp *app) {
    if (!app || !app->list_wifi) return;

    GList *children = gtk_container_get_children(GTK_CONTAINER(app->list_wifi));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    char *out = exec_cmd("nmcli -t -f IN-USE,SSID,SIGNAL,SECURITY device wifi list 2>/dev/null || true");
    gboolean found = FALSE;
    char **lines = g_strsplit(out, "\n", -1);
    GHashTable *seen_ssids = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    for (int i = 0; lines[i] != NULL; i++) {
        if (lines[i][0] == '\0') continue;
        char **parts = parse_nmcli_terse_line(lines[i], 4);
        if (parts && parts[0] && parts[1] && parts[1][0] != '\0' && g_strcmp0(parts[1], "--") != 0) {
            const char *ssid = parts[1];
            gboolean in_use = (g_strcmp0(parts[0], "*") == 0);

            if (!in_use && g_hash_table_contains(seen_ssids, ssid)) {
                g_strfreev(parts);
                continue;
            }
            g_hash_table_add(seen_ssids, g_strdup(ssid));

            const char *signal_str = parts[2] ? parts[2] : "0";
            int signal = atoi(signal_str);
            const char *sec = parts[3] ? parts[3] : "";
            gboolean is_secured = (sec[0] != '\0' && g_strcmp0(sec, "--") != 0);

            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
            gtk_widget_set_margin_start(hbox, 12);
            gtk_widget_set_margin_end(hbox, 12);
            gtk_widget_set_margin_top(hbox, 6);
            gtk_widget_set_margin_bottom(hbox, 6);

            GtkWidget *icon = gtk_image_new_from_icon_name(get_wifi_icon_name(signal), GTK_ICON_SIZE_BUTTON);
            gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);

            GtkWidget *lbl_ssid = gtk_label_new(ssid);
            gtk_label_set_xalign(GTK_LABEL(lbl_ssid), 0.0);
            gtk_box_pack_start(GTK_BOX(hbox), lbl_ssid, TRUE, TRUE, 0);

            if (in_use) {
                GtkWidget *badge = gtk_label_new("Connected");
                gtk_style_context_add_class(gtk_widget_get_style_context(badge), "badge-green");
                gtk_box_pack_start(GTK_BOX(hbox), badge, FALSE, FALSE, 4);
            }

            if (is_secured) {
                GtkWidget *lock_icon = gtk_image_new_from_icon_name("network-wireless-encrypted-symbolic", GTK_ICON_SIZE_MENU);
                gtk_box_pack_start(GTK_BOX(hbox), lock_icon, FALSE, FALSE, 2);
            }

            char *sig_lbl_str = g_strdup_printf("%d%%  %s", signal, is_secured ? sec : "Open");
            GtkWidget *lbl_sig = gtk_label_new(sig_lbl_str);
            g_free(sig_lbl_str);
            gtk_style_context_add_class(gtk_widget_get_style_context(lbl_sig), "dim-label");
            gtk_box_pack_end(GTK_BOX(hbox), lbl_sig, FALSE, FALSE, 0);

            gtk_container_add(GTK_CONTAINER(row), hbox);
            g_object_set_data_full(G_OBJECT(row), "ssid", g_strdup(ssid), g_free);
            g_object_set_data_full(G_OBJECT(row), "security", g_strdup(sec), g_free);
            g_object_set_data(G_OBJECT(row), "in_use", GINT_TO_POINTER(in_use));

            gtk_list_box_insert(GTK_LIST_BOX(app->list_wifi), row, -1);
            found = TRUE;
        }
        g_strfreev(parts);
    }
    g_hash_table_destroy(seen_ssids);
    g_strfreev(lines);
    g_free(out);

    if (!found) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new("No Wi-Fi access points found. Ensure Wi-Fi is enabled and click Scan.");
        gtk_widget_set_margin_top(lbl, 12);
        gtk_widget_set_margin_bottom(lbl, 12);
        gtk_container_add(GTK_CONTAINER(row), lbl);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_wifi), row, -1);
    }
    gtk_widget_show_all(app->list_wifi);
}

static void on_goto_net_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisOobeApp *app = (AxisOobeApp *)user_data;
    gtk_stack_set_visible_child_name(GTK_STACK(app->stack), "step_net");
    trigger_wifi_rescan(app);
}

static void on_goto_finish_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisOobeApp *app = (AxisOobeApp *)user_data;
    gtk_stack_set_visible_child_name(GTK_STACK(app->stack), "step_finish");
}

static void on_open_settings_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    int ret = system("axis-settings network &");
    (void)ret;
}

static void on_check_net_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisOobeApp *app = (AxisOobeApp *)user_data;
    char *status = check_network();
    gtk_label_set_text(GTK_LABEL(app->lbl_net_status), status);
    g_free(status);
}

static void mark_oobe_complete(void) {
    gchar *axis_cfg_dir = g_build_filename(g_get_user_config_dir(), "axisos", NULL);
    gchar *oobe_done = g_build_filename(axis_cfg_dir, "oobe-done", NULL);
    gchar *welcome_dis = g_build_filename(axis_cfg_dir, "welcome-disabled", NULL);
    gchar *autostart_welcome = g_build_filename(g_get_user_config_dir(), "autostart", "axis-welcome.desktop", NULL);
    gchar *autostart_oobe = g_build_filename(g_get_user_config_dir(), "autostart", "axis-oobe-firstboot.desktop", NULL);
    g_mkdir_with_parents(axis_cfg_dir, 0755);
    g_file_set_contents(oobe_done, "1\n", -1, NULL);
    g_file_set_contents(welcome_dis, "1\n", -1, NULL);
    unlink(autostart_welcome);
    unlink(autostart_oobe);
    g_free(axis_cfg_dir);
    g_free(oobe_done);
    g_free(welcome_dis);
    g_free(autostart_welcome);
    g_free(autostart_oobe);
    unlink("/var/lib/axisos/first-boot-pending");
    g_spawn_command_line_async("sudo -n rm -f /var/lib/axisos/first-boot-pending", NULL);
}

static void on_finish_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisOobeApp *app = (AxisOobeApp *)user_data;
    mark_oobe_complete();
    gtk_widget_destroy(app->window);
}

static void build_welcome_step(AxisOobeApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

    GtkWidget *lbl_t = gtk_label_new("Welcome to your newly installed AxisOS PC");
    gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(lbl_t), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_pack_start(GTK_BOX(card), lbl_t, FALSE, FALSE, 0);

    GtkWidget *lbl_d = gtk_label_new("Let's finish getting your hardware and network configured so you can start gaming and coding right away.");
    gtk_label_set_xalign(GTK_LABEL(lbl_d), 0.0);
    gtk_box_pack_start(GTK_BOX(card), lbl_d, FALSE, FALSE, 0);

    // Hardware details
    char *cpu = exec_cmd("grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 || echo 'x86_64 CPU'");
    char *mem = exec_cmd("awk '/^MemTotal:/ {printf \"%.1f GB\", $2/1048576}' /proc/meminfo 2>/dev/null || free -h | awk '/^Mem:/ {print $2}'");
    char *gpu = exec_cmd("lspci 2>/dev/null | grep -iE 'vga|3d|display' | sed 's/.*: //g' | head -n1");
    g_strstrip(cpu);
    g_strstrip(mem);
    g_strstrip(gpu);
    if (!gpu[0]) {
        g_free(gpu);
        gpu = g_strdup("Wayland DRM / KMS Graphics");
    }

    char *hw_str = g_strdup_printf("Hardware Profile:\n• Processor: %s\n• Memory: %s RAM\n• Graphics: %s\n• Compositor: Wayland Native (wlroots)", cpu, mem, gpu);
    GtkWidget *lbl_hw = gtk_label_new(hw_str);
    gtk_label_set_xalign(GTK_LABEL(lbl_hw), 0.0);
    gtk_box_pack_start(GTK_BOX(card), lbl_hw, FALSE, FALSE, 12);

    g_free(cpu);
    g_free(mem);
    g_free(gpu);
    g_free(hw_str);

    gtk_box_pack_start(GTK_BOX(vbox), card, TRUE, TRUE, 0);

    GtkWidget *btn_next = gtk_button_new_with_label("Next: Network Setup →");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_next), "btn-primary");
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_goto_net_cb), app);
    gtk_box_pack_end(GTK_BOX(vbox), btn_next, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(app->stack), vbox, "step_welcome");
}

static void build_network_step(AxisOobeApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

    GtkWidget *lbl_t = gtk_label_new("Internet Connectivity");
    gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(lbl_t), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_pack_start(GTK_BOX(card), lbl_t, FALSE, FALSE, 0);

    char *net_status = check_network();
    app->lbl_net_status = gtk_label_new(net_status);
    g_free(net_status);
    gtk_label_set_xalign(GTK_LABEL(app->lbl_net_status), 0.0);
    gtk_box_pack_start(GTK_BOX(card), app->lbl_net_status, FALSE, FALSE, 4);

    // Wi-Fi Header: Switch + Scan Button + Status
    GtkWidget *wifi_hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *wifi_icon = gtk_image_new_from_icon_name("network-wireless-symbolic", GTK_ICON_SIZE_BUTTON);
    GtkWidget *lbl_wifi = gtk_label_new("Wi-Fi Networks");
    PangoAttrList *attrs_w = pango_attr_list_new();
    pango_attr_list_insert(attrs_w, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(lbl_wifi), attrs_w);
    pango_attr_list_unref(attrs_w);

    app->switch_wifi = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_wifi), TRUE);
    g_signal_connect(app->switch_wifi, "notify::active", G_CALLBACK(on_wifi_switch_active_changed), app);

    app->btn_scan = gtk_button_new_with_label("Scan");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->btn_scan), "btn-secondary");
    g_signal_connect(app->btn_scan, "clicked", G_CALLBACK(on_scan_clicked_cb), app);

    app->lbl_scan_status = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_scan_status), "dim-label");

    gtk_box_pack_start(GTK_BOX(wifi_hdr), wifi_icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(wifi_hdr), lbl_wifi, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(wifi_hdr), app->switch_wifi, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(wifi_hdr), app->btn_scan, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(wifi_hdr), app->lbl_scan_status, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(card), wifi_hdr, FALSE, FALSE, 4);

    // Scrollable Wi-Fi List
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 200);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    app->list_wifi = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->list_wifi), GTK_SELECTION_SINGLE);
    g_signal_connect(app->list_wifi, "row-activated", G_CALLBACK(on_wifi_row_activated), app);
    gtk_container_add(GTK_CONTAINER(scroll), app->list_wifi);
    gtk_box_pack_start(GTK_BOX(card), scroll, TRUE, TRUE, 4);

    // Secondary buttons: Open Network Settings & Check Connection
    GtkWidget *hbox_b = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_settings = gtk_button_new_with_label("Open Network Settings");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_settings), "btn-secondary");
    g_signal_connect(btn_settings, "clicked", G_CALLBACK(on_open_settings_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_b), btn_settings, FALSE, FALSE, 0);

    GtkWidget *btn_check = gtk_button_new_with_label("Check Connection");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_check), "btn-secondary");
    g_signal_connect(btn_check, "clicked", G_CALLBACK(on_check_net_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_b), btn_check, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(card), hbox_b, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), card, TRUE, TRUE, 0);

    GtkWidget *btn_next = gtk_button_new_with_label("Next: Complete Setup →");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_next), "btn-primary");
    g_signal_connect(btn_next, "clicked", G_CALLBACK(on_goto_finish_cb), app);
    gtk_box_pack_end(GTK_BOX(vbox), btn_next, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(app->stack), vbox, "step_net");
}

static void build_finish_step(AxisOobeApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

    GtkWidget *lbl_t = gtk_label_new("You're all set!");
    gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(lbl_t), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_pack_start(GTK_BOX(card), lbl_t, FALSE, FALSE, 0);

    GtkWidget *lbl_d = gtk_label_new("AxisOS is ready for use.\nEnjoy high FPS gaming on Steam, fast development in VS Code, and a sleek Wayland desktop.");
    gtk_label_set_xalign(GTK_LABEL(lbl_d), 0.0);
    gtk_box_pack_start(GTK_BOX(card), lbl_d, FALSE, FALSE, 8);

    gtk_box_pack_start(GTK_BOX(vbox), card, TRUE, TRUE, 0);

    GtkWidget *btn_done = gtk_button_new_with_label("Start Using AxisOS");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_done), "btn-primary");
    g_signal_connect(btn_done, "clicked", G_CALLBACK(on_finish_cb), app);
    gtk_box_pack_end(GTK_BOX(vbox), btn_done, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(app->stack), vbox, "step_finish");
}

static void on_oobe_window_destroy(GtkWidget *w, gpointer user_data) {
    (void)w;
    (void)user_data;
    mark_oobe_complete();
    gtk_main_quit();
}

static AxisOobeApp *axis_oobe_app_create(void) {
    AxisOobeApp *app = g_new0(AxisOobeApp, 1);
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Welcome to AxisOS Setup");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 780, 520);
    gtk_window_set_position(GTK_WINDOW(app->window), GTK_WIN_POS_CENTER);

    // Apply system theme
    load_axis_theme();

    // Headerbar
    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS Out-of-Box Experience");
    gtk_window_set_titlebar(GTK_WINDOW(app->window), hb);

    app->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(app->stack), 150);
    gtk_container_add(GTK_CONTAINER(app->window), app->stack);

    build_welcome_step(app);
    build_network_step(app);
    build_finish_step(app);

    g_signal_connect(app->window, "destroy", G_CALLBACK(on_oobe_window_destroy), NULL);
    return app;
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("AxisOS First-Boot Out-of-Box Experience (OOBE) Wizard (Pure C11)\n");
        printf("Usage: axis-oobe [OPTION]\n");
        printf("Options:\n");
        printf("  --help, -h       Show this help message and exit\n");
        printf("  --status, -s     Check if first boot setup is pending\n");
        return 0;
    }
    if (argc > 1 && (strcmp(argv[1], "--status") == 0 || strcmp(argv[1], "-s") == 0)) {
        gboolean pending = (access("/var/lib/axisos/first-boot-pending", F_OK) == 0);
        printf("AxisOS OOBE Status: %s\n", pending ? "Setup Pending (first boot)" : "Setup Completed");
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
    AxisOobeApp *app = axis_oobe_app_create();
    gtk_widget_show_all(app->window);
    gtk_main();
    g_free(app);
    return 0;
}
