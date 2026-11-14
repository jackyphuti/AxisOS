#ifndef AXIS_SYSTEM_CONTROLLER_H
#define AXIS_SYSTEM_CONTROLLER_H

#define _GNU_SOURCE
#include <glib.h>
#include <gio/gio.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *name;
    char *exec;
    char *icon;
    char *comment;
    char *categories;
    char *desktop_file;
} AppEntry;

typedef struct {
    int cpu_load_percent;
    int ram_usage_percent;
    uint64_t ram_total_mb;
    uint64_t ram_used_mb;
    int zram_saved_mb;
    int battery_percent;
    bool battery_charging;
    bool has_battery;
    bool wifi_connected;
    char *wifi_ssid;
    int wifi_strength;
    bool bluetooth_enabled;
    int bluetooth_connected_devices;
    int volume;
    bool is_muted;
    int brightness;
    bool is_dark_mode;
    char kernel_version[128];
    char hostname[128];
} SystemMetrics;

typedef void (*NotificationCallback)(uint32_t id, const char *app_name, const char *summary,
                                      const char *body, const char *icon, gpointer user_data);

typedef struct {
    GDBusConnection *system_bus;
    GDBusConnection *session_bus;
    guint notification_owner_id;
    uint32_t next_notification_id;

    SystemMetrics metrics;

    unsigned long long prev_idle;
    unsigned long long prev_total;

    GPtrArray *installed_apps; // Array of AppEntry*
    NotificationCallback notify_cb;
    gpointer notify_user_data;

    guint poll_timer_id;
} SystemController;

SystemController *system_controller_create(void);
void system_controller_destroy(SystemController *ctrl);

void system_controller_set_notification_callback(SystemController *ctrl,
                                                 NotificationCallback cb,
                                                 gpointer user_data);

void system_controller_poll_telemetry(SystemController *ctrl);
void system_controller_scan_apps(SystemController *ctrl);
GPtrArray *system_controller_get_installed_apps(SystemController *ctrl);
bool system_controller_launch_app(SystemController *ctrl, const char *command);

void system_controller_set_volume(SystemController *ctrl, int volume);
void system_controller_set_muted(SystemController *ctrl, bool muted);
void system_controller_set_brightness(SystemController *ctrl, int brightness);
void system_controller_toggle_wifi(SystemController *ctrl, bool enabled);
void system_controller_toggle_bluetooth(SystemController *ctrl, bool enabled);
void system_controller_set_dark_mode(SystemController *ctrl, bool dark);

void system_controller_power_off(SystemController *ctrl);
void system_controller_reboot(SystemController *ctrl);
void system_controller_suspend(SystemController *ctrl);

void app_entry_free(AppEntry *entry);

#ifdef __cplusplus
}
#endif

#endif /* AXIS_SYSTEM_CONTROLLER_H */
