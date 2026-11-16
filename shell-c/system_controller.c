#define _GNU_SOURCE
#include "system_controller.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <gio/gio.h>

void app_entry_free(AppEntry *entry) {
    if (!entry) return;
    g_free(entry->name);
    g_free(entry->exec);
    g_free(entry->icon);
    g_free(entry->comment);
    g_free(entry->categories);
    g_free(entry->desktop_file);
    g_free(entry);
}

static const gchar introspection_xml[] =
    "<node>"
    "  <interface name='org.freedesktop.Notifications'>"
    "    <method name='Notify'>"
    "      <arg type='s' name='app_name' direction='in'/>"
    "      <arg type='u' name='replaces_id' direction='in'/>"
    "      <arg type='s' name='app_icon' direction='in'/>"
    "      <arg type='s' name='summary' direction='in'/>"
    "      <arg type='s' name='body' direction='in'/>"
    "      <arg type='as' name='actions' direction='in'/>"
    "      <arg type='a{sv}' name='hints' direction='in'/>"
    "      <arg type='i' name='expire_timeout' direction='in'/>"
    "      <arg type='u' name='id' direction='out'/>"
    "    </method>"
    "    <method name='CloseNotification'>"
    "      <arg type='u' name='id' direction='in'/>"
    "    </method>"
    "    <method name='GetCapabilities'>"
    "      <arg type='as' name='capabilities' direction='out'/>"
    "    </method>"
    "    <method name='GetServerInformation'>"
    "      <arg type='s' name='name' direction='out'/>"
    "      <arg type='s' name='vendor' direction='out'/>"
    "      <arg type='s' name='version' direction='out'/>"
    "      <arg type='s' name='spec_version' direction='out'/>"
    "    </method>"
    "  </interface>"
    "</node>";

static void handle_method_call(GDBusConnection *connection,
                               const gchar *sender,
                               const gchar *object_path,
                               const gchar *interface_name,
                               const gchar *method_name,
                               GVariant *parameters,
                               GDBusMethodInvocation *invocation,
                               gpointer user_data) {
    (void)connection; (void)sender; (void)object_path; (void)interface_name;
    SystemController *ctrl = (SystemController *)user_data;

    if (g_strcmp0(method_name, "Notify") == 0) {
        const gchar *app_name = NULL;
        guint32 replaces_id = 0;
        const gchar *app_icon = NULL;
        const gchar *summary = NULL;
        const gchar *body = NULL;
        GVariantIter *actions_iter = NULL;
        GVariantIter *hints_iter = NULL;
        gint32 timeout = -1;

        g_variant_get(parameters, "(&su&s&s&sas@a{sv}i)",
                      &app_name, &replaces_id, &app_icon, &summary, &body,
                      &actions_iter, &hints_iter, &timeout);

        uint32_t id = replaces_id ? replaces_id : ++ctrl->next_notification_id;

        if (ctrl->notify_cb) {
            ctrl->notify_cb(id, app_name, summary, body, app_icon, ctrl->notify_user_data);
        }

        if (actions_iter) g_variant_iter_free(actions_iter);
        if (hints_iter) g_variant_iter_free(hints_iter);

        g_dbus_method_invocation_return_value(invocation, g_variant_new("(u)", id));
    } else if (g_strcmp0(method_name, "GetCapabilities") == 0) {
        GVariantBuilder builder;
        g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
        g_variant_builder_add(&builder, "s", "body");
        g_variant_builder_add(&builder, "s", "actions");
        g_variant_builder_add(&builder, "s", "icon-static");
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(as)", &builder));
    } else if (g_strcmp0(method_name, "GetServerInformation") == 0) {
        g_dbus_method_invocation_return_value(
            invocation,
            g_variant_new("(ssss)", "AxisOS Notification Server", "AxisOS", "2.0", "1.2")
        );
    } else {
        g_dbus_method_invocation_return_value(invocation, NULL);
    }
}

static const GDBusInterfaceVTable interface_vtable = {
    handle_method_call,
    NULL,
    NULL,
    { 0 }
};

static void on_bus_acquired(GDBusConnection *connection, const gchar *name, gpointer user_data) {
    (void)name;
    SystemController *ctrl = (SystemController *)user_data;
    GError *error = NULL;
    GDBusNodeInfo *introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, &error);

    if (error) {
        g_error_free(error);
        return;
    }

    g_dbus_connection_register_object(
        connection,
        "/org/freedesktop/Notifications",
        introspection_data->interfaces[0],
        &interface_vtable,
        ctrl,
        NULL,
        &error
    );

    if (error) {
        g_error_free(error);
    }
    g_dbus_node_info_unref(introspection_data);
}

static gboolean on_poll_telemetry_tick(gpointer user_data) {
    SystemController *ctrl = (SystemController *)user_data;
    system_controller_poll_telemetry(ctrl);
    return G_SOURCE_CONTINUE;
}

static void read_cpu_stats(SystemController *ctrl) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return;

    unsigned long long user = 0, nice = 0, system = 0, idle = 0, iowait = 0, irq = 0, softirq = 0, steal = 0;
    if (fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) == 8) {
        unsigned long long current_idle = idle + iowait;
        unsigned long long current_total = user + nice + system + current_idle + irq + softirq + steal;

        if (ctrl->prev_total > 0 && current_total > ctrl->prev_total) {
            unsigned long long total_diff = current_total - ctrl->prev_total;
            unsigned long long idle_diff = current_idle - ctrl->prev_idle;
            int load = (int)(((total_diff - idle_diff) * 100ULL) / total_diff);
            if (load < 0) load = 0;
            if (load > 100) load = 100;
            ctrl->metrics.cpu_load_percent = load;
        }
        ctrl->prev_idle = current_idle;
        ctrl->prev_total = current_total;
    }
    fclose(f);
}

static void read_mem_stats(SystemController *ctrl) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;

    char line[256];
    uint64_t mem_total_kb = 0;
    uint64_t mem_avail_kb = 0;

    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line + 9, "%lu", &mem_total_kb);
        } else if (strncmp(line, "MemAvailable:", 13) == 0) {
            sscanf(line + 13, "%lu", &mem_avail_kb);
        }
    }
    fclose(f);

    if (mem_total_kb > 0) {
        ctrl->metrics.ram_total_mb = mem_total_kb / 1024;
        uint64_t used_kb = (mem_total_kb > mem_avail_kb) ? (mem_total_kb - mem_avail_kb) : 0;
        ctrl->metrics.ram_used_mb = used_kb / 1024;
        ctrl->metrics.ram_usage_percent = (int)((used_kb * 100ULL) / mem_total_kb);
    }
}

static void read_battery_stats(SystemController *ctrl) {
    // Check UPower or /sys/class/power_supply
    DIR *dir = opendir("/sys/class/power_supply");
    if (!dir) {
        ctrl->metrics.has_battery = false;
        return;
    }

    struct dirent *ent;
    bool found_bat = false;
    while ((ent = readdir(dir)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        if (strncmp(ent->d_name, "BAT", 3) == 0) {
            char cap_path[512];
            snprintf(cap_path, sizeof(cap_path), "/sys/class/power_supply/%s/capacity", ent->d_name);
            FILE *f = fopen(cap_path, "r");
            if (f) {
                int cap = 100;
                if (fscanf(f, "%d", &cap) == 1) {
                    ctrl->metrics.battery_percent = cap;
                    found_bat = true;
                }
                fclose(f);
            }

            char status_path[512];
            snprintf(status_path, sizeof(status_path), "/sys/class/power_supply/%s/status", ent->d_name);
            f = fopen(status_path, "r");
            if (f) {
                char status_str[32] = {0};
                if (fgets(status_str, sizeof(status_str), f)) {
                    ctrl->metrics.battery_charging = (strncmp(status_str, "Charging", 8) == 0);
                }
                fclose(f);
            }
            break;
        }
    }
    closedir(dir);
    ctrl->metrics.has_battery = found_bat;
    if (!found_bat) {
        ctrl->metrics.battery_percent = 100;
        ctrl->metrics.battery_charging = false;
    }
}

static void read_network_stats(SystemController *ctrl) {
    // Read active connection via NetworkManager D-Bus or sysfs
    if (ctrl->system_bus) {
        GError *error = NULL;
        GDBusProxy *proxy = g_dbus_proxy_new_sync(
            ctrl->system_bus,
            G_DBUS_PROXY_FLAGS_NONE,
            NULL,
            "org.freedesktop.NetworkManager",
            "/org/freedesktop/NetworkManager",
            "org.freedesktop.NetworkManager",
            NULL,
            &error
        );

        if (proxy) {
            GVariant *primary_conn = g_dbus_proxy_get_cached_property(proxy, "PrimaryConnection");
            if (primary_conn) {
                const gchar *path = g_variant_get_string(primary_conn, NULL);
                if (path && strlen(path) > 1 && strcmp(path, "/") != 0) {
                    ctrl->metrics.wifi_connected = true;
                }
                g_variant_unref(primary_conn);
            }
            g_object_unref(proxy);
        } else if (error) {
            g_error_free(error);
        }
    }

    // Fallback: check /sys/class/net
    if (!ctrl->metrics.wifi_connected) {
        DIR *d = opendir("/sys/class/net");
        if (d) {
            struct dirent *ent;
            while ((ent = readdir(d)) != NULL) {
                if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 || strcmp(ent->d_name, "lo") == 0)
                    continue;
                char oper_path[512];
                snprintf(oper_path, sizeof(oper_path), "/sys/class/net/%s/operstate", ent->d_name);
                FILE *f = fopen(oper_path, "r");
                if (f) {
                    char st[32] = {0};
                    if (fgets(st, sizeof(st), f)) {
                        if (strncmp(st, "up", 2) == 0) {
                            ctrl->metrics.wifi_connected = true;
                            if (!ctrl->metrics.wifi_ssid || strcmp(ctrl->metrics.wifi_ssid, "Offline") == 0) {
                                g_free(ctrl->metrics.wifi_ssid);
                                ctrl->metrics.wifi_ssid = g_strdup(ent->d_name);
                            }
                            ctrl->metrics.wifi_strength = 90;
                            fclose(f);
                            break;
                        }
                    }
                    fclose(f);
                }
            }
            closedir(d);
        }
    }

    // Read active SSID if connected
    char *ssid_out = NULL;
    if (g_spawn_command_line_sync("nmcli -t -f ACTIVE,SSID dev wifi 2>/dev/null | grep '^yes:' | cut -d: -f2", &ssid_out, NULL, NULL, NULL) && ssid_out) {
        g_strstrip(ssid_out);
        if (ssid_out[0] != '\0') {
            g_free(ctrl->metrics.wifi_ssid);
            ctrl->metrics.wifi_ssid = g_strdup(ssid_out);
            ctrl->metrics.wifi_connected = true;
        }
        g_free(ssid_out);
    }
}

static void read_cpu_temp(SystemController *ctrl) {
    ctrl->metrics.cpu_temp_c = 42;
    DIR *d = opendir("/sys/class/thermal");
    if (d) {
        struct dirent *ent;
        while ((ent = readdir(d)) != NULL) {
            if (strncmp(ent->d_name, "thermal_zone", 12) == 0) {
                char temp_path[512];
                snprintf(temp_path, sizeof(temp_path), "/sys/class/thermal/%s/temp", ent->d_name);
                FILE *f = fopen(temp_path, "r");
                if (f) {
                    long milli = 0;
                    if (fscanf(f, "%ld", &milli) == 1 && milli > 0) {
                        int c = (int)(milli / 1000);
                        if (c >= 10 && c <= 115) {
                            ctrl->metrics.cpu_temp_c = c;
                            fclose(f);
                            closedir(d);
                            return;
                        }
                    }
                    fclose(f);
                }
            }
        }
        closedir(d);
    }
}

static void read_audio_stats(SystemController *ctrl) {
    char *out = NULL;
    if (g_spawn_command_line_sync("wpctl get-volume @DEFAULT_AUDIO_SINK@", &out, NULL, NULL, NULL) && out && strstr(out, "Volume:")) {
        char *p = strstr(out, "Volume:");
        if (p) {
            float vol = 0.0f;
            if (sscanf(p + 7, "%f", &vol) == 1) {
                ctrl->metrics.volume = (int)(vol * 100.0f + 0.5f);
                if (ctrl->metrics.volume > 100) ctrl->metrics.volume = 100;
                if (ctrl->metrics.volume < 0) ctrl->metrics.volume = 0;
            }
            ctrl->metrics.is_muted = (strstr(out, "[MUTED]") != NULL);
        }
        g_free(out);
        return;
    }
    g_free(out);
    out = NULL;

    if (g_spawn_command_line_sync("pactl get-sink-volume @DEFAULT_SINK@", &out, NULL, NULL, NULL) && out) {
        char *pct = strchr(out, '%');
        if (pct) {
            char *start = pct - 1;
            while (start > out && g_ascii_isdigit(*(start - 1))) {
                start--;
            }
            int v = atoi(start);
            if (v >= 0 && v <= 150) {
                ctrl->metrics.volume = v > 100 ? 100 : v;
            }
        }
        g_free(out);
        out = NULL;
    }
    if (g_spawn_command_line_sync("pactl get-sink-mute @DEFAULT_SINK@", &out, NULL, NULL, NULL) && out) {
        if (strstr(out, "yes") || strstr(out, "Mute: yes")) {
            ctrl->metrics.is_muted = true;
        } else if (strstr(out, "no") || strstr(out, "Mute: no")) {
            ctrl->metrics.is_muted = false;
        }
        g_free(out);
        out = NULL;
    }
}

int system_controller_read_brightness(void) {
    GDir *dir = g_dir_open("/sys/class/backlight", 0, NULL);
    if (dir) {
        const char *entry;
        while ((entry = g_dir_read_name(dir)) != NULL) {
            char b_path[256], m_path[256];
            snprintf(b_path, sizeof(b_path), "/sys/class/backlight/%s/brightness", entry);
            snprintf(m_path, sizeof(m_path), "/sys/class/backlight/%s/max_brightness", entry);
            char *b_str = NULL, *m_str = NULL;
            if (g_file_get_contents(b_path, &b_str, NULL, NULL) &&
                g_file_get_contents(m_path, &m_str, NULL, NULL)) {
                long b_val = strtol(b_str, NULL, 10);
                long m_val = strtol(m_str, NULL, 10);
                g_free(b_str);
                g_free(m_str);
                g_dir_close(dir);
                if (m_val > 0) {
                    int pct = (int)((b_val * 100) / m_val);
                    if (pct < 1) pct = 1;
                    if (pct > 100) pct = 100;
                    return pct;
                }
            }
            g_free(b_str);
            g_free(m_str);
        }
        g_dir_close(dir);
    }

    char *out = NULL;
    if (g_spawn_command_line_sync("brightnessctl -m", &out, NULL, NULL, NULL) && out) {
        char **lines = g_strsplit(out, "\n", -1);
        for (int i = 0; lines[i]; i++) {
            if (lines[i][0] == '\0') continue;
            char **cols = g_strsplit(lines[i], ",", -1);
            int col_count = g_strv_length(cols);
            if (col_count >= 4) {
                int pct = atoi(cols[3]);
                if (pct > 0) {
                    g_strfreev(cols);
                    g_strfreev(lines);
                    g_free(out);
                    return pct;
                }
            }
            g_strfreev(cols);
        }
        g_strfreev(lines);
        g_free(out);
    }
    return 100;
}

SystemController *system_controller_create(void) {
    SystemController *ctrl = g_new0(SystemController, 1);
    ctrl->next_notification_id = 100;
    ctrl->installed_apps = g_ptr_array_new_with_free_func((GDestroyNotify)app_entry_free);

    ctrl->metrics.volume = 70;
    ctrl->metrics.is_muted = false;
    ctrl->metrics.brightness = system_controller_read_brightness();
    read_audio_stats(ctrl);
    read_cpu_temp(ctrl);
    ctrl->metrics.is_dark_mode = true;
    ctrl->metrics.wifi_ssid = g_strdup("Connected");
    ctrl->metrics.wifi_strength = 85;
    ctrl->metrics.wifi_connected = false;
    ctrl->metrics.bluetooth_enabled = true;
    ctrl->metrics.bluetooth_connected_devices = 0;
    ctrl->metrics.battery_percent = 100;
    ctrl->metrics.has_battery = true;

    struct utsname u;
    if (uname(&u) == 0) {
        snprintf(ctrl->metrics.kernel_version, sizeof(ctrl->metrics.kernel_version), "Linux %s", u.release);
        snprintf(ctrl->metrics.hostname, sizeof(ctrl->metrics.hostname), "%s", u.nodename);
    } else {
        g_strlcpy(ctrl->metrics.kernel_version, "Linux 6.x", sizeof(ctrl->metrics.kernel_version));
        g_strlcpy(ctrl->metrics.hostname, "axisos", sizeof(ctrl->metrics.hostname));
    }

    GError *error = NULL;
    ctrl->system_bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (error) {
        g_error_free(error);
        error = NULL;
    }

    ctrl->session_bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (error) {
        g_error_free(error);
        error = NULL;
    }

    if (ctrl->session_bus) {
        ctrl->notification_owner_id = g_bus_own_name(
            G_BUS_TYPE_SESSION,
            "org.freedesktop.Notifications",
            G_BUS_NAME_OWNER_FLAGS_REPLACE,
            on_bus_acquired,
            NULL,
            NULL,
            ctrl,
            NULL
        );
    }

    system_controller_poll_telemetry(ctrl);
    system_controller_scan_apps(ctrl);

    ctrl->poll_timer_id = g_timeout_add_seconds(2, on_poll_telemetry_tick, ctrl);

    return ctrl;
}

void system_controller_destroy(SystemController *ctrl) {
    if (!ctrl) return;
    if (ctrl->poll_timer_id) {
        g_source_remove(ctrl->poll_timer_id);
    }
    if (ctrl->notification_owner_id) {
        g_bus_unown_name(ctrl->notification_owner_id);
    }
    if (ctrl->system_bus) {
        g_object_unref(ctrl->system_bus);
    }
    if (ctrl->session_bus) {
        g_object_unref(ctrl->session_bus);
    }
    if (ctrl->installed_apps) {
        g_ptr_array_unref(ctrl->installed_apps);
    }
    g_free(ctrl->metrics.wifi_ssid);
    g_free(ctrl);
}

void system_controller_set_notification_callback(SystemController *ctrl,
                                                 NotificationCallback cb,
                                                 gpointer user_data) {
    if (!ctrl) return;
    ctrl->notify_cb = cb;
    ctrl->notify_user_data = user_data;
}

void system_controller_poll_telemetry(SystemController *ctrl) {
    if (!ctrl) return;
    read_cpu_stats(ctrl);
    read_mem_stats(ctrl);
    read_battery_stats(ctrl);
    read_network_stats(ctrl);
    read_cpu_temp(ctrl);
    read_audio_stats(ctrl);
    ctrl->metrics.brightness = system_controller_read_brightness();
}

static void scan_directory_for_apps(SystemController *ctrl, const char *dir_path) {
    GDir *dir = g_dir_open(dir_path, 0, NULL);
    if (!dir) return;

    const gchar *filename;
    while ((filename = g_dir_read_name(dir)) != NULL) {
        if (!g_str_has_suffix(filename, ".desktop")) continue;

        gchar *full_path = g_build_filename(dir_path, filename, NULL);
        GKeyFile *kf = g_key_file_new();

        if (g_key_file_load_from_file(kf, full_path, G_KEY_FILE_NONE, NULL)) {
            gboolean no_display = g_key_file_get_boolean(kf, "Desktop Entry", "NoDisplay", NULL);
            if (!no_display) {
                gchar *name = g_key_file_get_locale_string(kf, "Desktop Entry", "Name", NULL, NULL);
                gchar *exec = g_key_file_get_string(kf, "Desktop Entry", "Exec", NULL);
                gchar *icon = g_key_file_get_string(kf, "Desktop Entry", "Icon", NULL);
                gchar *comment = g_key_file_get_locale_string(kf, "Desktop Entry", "Comment", NULL, NULL);
                gchar *cat = g_key_file_get_string(kf, "Desktop Entry", "Categories", NULL);

                if (name && exec) {
                    // Check if duplicate
                    bool exists = false;
                    for (guint i = 0; i < ctrl->installed_apps->len; ++i) {
                        AppEntry *e = g_ptr_array_index(ctrl->installed_apps, i);
                        if (g_strcmp0(e->name, name) == 0) {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists) {
                        AppEntry *entry = g_new0(AppEntry, 1);
                        entry->name = name;
                        // Strip field codes (%u, %f, etc) from exec
                        gchar **tokens = g_strsplit(exec, " %", 2);
                        entry->exec = g_strdup(tokens[0]);
                        g_strfreev(tokens);
                        g_free(exec);

                        entry->icon = icon ? icon : g_strdup("application-x-executable");
                        entry->comment = comment ? comment : g_strdup("");
                        entry->categories = cat ? cat : g_strdup("Utility");
                        entry->desktop_file = g_strdup(full_path);
                        g_ptr_array_add(ctrl->installed_apps, entry);
                    } else {
                        g_free(name);
                        g_free(exec);
                        g_free(icon);
                        g_free(comment);
                        g_free(cat);
                    }
                } else {
                    g_free(name);
                    g_free(exec);
                    g_free(icon);
                    g_free(comment);
                    g_free(cat);
                }
            }
        }
        g_key_file_free(kf);
        g_free(full_path);
    }
    g_dir_close(dir);
}

static gint app_sort_func(gconstpointer a, gconstpointer b) {
    const AppEntry *ea = *(const AppEntry **)a;
    const AppEntry *eb = *(const AppEntry **)b;
    return g_utf8_collate(ea->name, eb->name);
}

void system_controller_scan_apps(SystemController *ctrl) {
    if (!ctrl) return;
    g_ptr_array_set_size(ctrl->installed_apps, 0);

    const char *data_dirs[] = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        NULL
    };

    for (int i = 0; data_dirs[i] != NULL; ++i) {
        scan_directory_for_apps(ctrl, data_dirs[i]);
    }

    gchar *user_apps = g_build_filename(g_get_user_data_dir(), "applications", NULL);
    scan_directory_for_apps(ctrl, user_apps);
    g_free(user_apps);

    g_ptr_array_sort(ctrl->installed_apps, app_sort_func);
}

GPtrArray *system_controller_get_installed_apps(SystemController *ctrl) {
    return ctrl ? ctrl->installed_apps : NULL;
}

bool system_controller_launch_app(SystemController *ctrl, const char *command) {
    (void)ctrl;
    if (!command || !*command) return false;
    char *argv[] = { "/bin/sh", "-c", (char *)command, NULL };
    GError *error = NULL;
    gboolean ret = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, &error);
    if (error) {
        g_error_free(error);
        return false;
    }
    return ret;
}

void system_controller_set_volume(SystemController *ctrl, int volume) {
    if (!ctrl) return;
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    ctrl->metrics.volume = volume;

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "pactl set-sink-volume @DEFAULT_SINK@ %d%% 2>/dev/null || wpctl set-volume @DEFAULT_AUDIO_SINK@ %.2f 2>/dev/null",
             volume, volume / 100.0);
    g_spawn_command_line_async(cmd, NULL);
}

void system_controller_set_muted(SystemController *ctrl, bool muted) {
    if (!ctrl) return;
    ctrl->metrics.is_muted = muted;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "pactl set-sink-mute @DEFAULT_SINK@ %s 2>/dev/null || wpctl set-mute @DEFAULT_AUDIO_SINK@ %s 2>/dev/null",
             muted ? "1" : "0", muted ? "1" : "0");
    g_spawn_command_line_async(cmd, NULL);
}

void system_controller_set_brightness(SystemController *ctrl, int brightness) {
    if (!ctrl) return;
    if (brightness < 5) brightness = 5;
    if (brightness > 100) brightness = 100;
    ctrl->metrics.brightness = brightness;

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "brightnessctl set %d%% 2>/dev/null", brightness);
    g_spawn_command_line_async(cmd, NULL);
}

void system_controller_toggle_wifi(SystemController *ctrl, bool enabled) {
    if (!ctrl) return;
    ctrl->metrics.wifi_connected = enabled;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "nmcli radio wifi %s 2>/dev/null", enabled ? "on" : "off");
    g_spawn_command_line_async(cmd, NULL);
}

void system_controller_toggle_bluetooth(SystemController *ctrl, bool enabled) {
    if (!ctrl) return;
    ctrl->metrics.bluetooth_enabled = enabled;
    char cmd[128];
    snprintf(cmd, sizeof(cmd), "bluetoothctl power %s 2>/dev/null", enabled ? "on" : "off");
    g_spawn_command_line_async(cmd, NULL);
}

void system_controller_set_dark_mode(SystemController *ctrl, bool dark) {
    if (!ctrl) return;
    ctrl->metrics.is_dark_mode = dark;
}

void system_controller_power_off(SystemController *ctrl) {
    (void)ctrl;
    g_spawn_command_line_async("systemctl poweroff 2>/dev/null || loginctl poweroff 2>/dev/null", NULL);
}

void system_controller_reboot(SystemController *ctrl) {
    (void)ctrl;
    g_spawn_command_line_async("systemctl reboot 2>/dev/null || loginctl reboot 2>/dev/null", NULL);
}

void system_controller_suspend(SystemController *ctrl) {
    (void)ctrl;
    g_spawn_command_line_async("systemctl suspend 2>/dev/null || loginctl suspend 2>/dev/null", NULL);
}
