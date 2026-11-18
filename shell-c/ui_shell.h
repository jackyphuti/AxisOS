#ifndef AXIS_UI_SHELL_H
#define AXIS_UI_SHELL_H

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <gtk-layer-shell/gtk-layer-shell.h>
#include "system_controller.h"
#include "auth_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    SystemController *sys_ctrl;
    AuthManager *auth_mgr;

    // Windows
    GtkWidget *topbar_window;
    GtkWidget *dock_window;
    GtkWidget *launcher_window;
    GtkWidget *quicksettings_window;
    GtkWidget *games_window;
    GtkWidget *osd_window;
    GtkWidget *notification_window;
    GtkWidget *greeter_window;

    // Top Bar Widgets
    GtkWidget *topbar_clock_label;
    GtkWidget *topbar_cpu_label;
    GtkWidget *topbar_ram_label;
    GtkWidget *topbar_bat_label;
    GtkWidget *topbar_net_label;
    GtkWidget *topbar_vol_label;
    GtkWidget *topbar_net_icon;
    GtkWidget *topbar_vol_icon;
    GtkWidget *topbar_bat_icon;
    GtkWidget *topbar_cpu_icon;
    GtkWidget *workspace_btns[4];

    // Launcher Widgets
    GtkWidget *launcher_revealer;
    GtkWidget *launcher_search_entry;
    GtkWidget *launcher_filter_box;
    GtkWidget *launcher_flowbox;
    char *launcher_active_filter;

    // Games Panel Widgets
    GtkWidget *games_revealer;
    GtkWidget *games_search_entry;
    GtkWidget *games_flowbox;

    // Quick Settings Widgets
    GtkWidget *qs_revealer;
    GtkWidget *qs_vol_scale;
    GtkWidget *qs_bright_scale;
    GtkWidget *qs_wifi_switch;
    GtkWidget *qs_bt_switch;
    GtkWidget *qs_dark_switch;
    GtkWidget *qs_tile_wifi;
    GtkWidget *qs_sub_wifi;
    GtkWidget *qs_tile_bt;
    GtkWidget *qs_sub_bt;
    GtkWidget *qs_tile_dark;
    GtkWidget *qs_sub_dark;
    GtkWidget *qs_tile_game;
    GtkWidget *qs_sub_game;
    GtkWidget *qs_lbl_vol_val;
    GtkWidget *qs_lbl_bright_val;
    GtkWidget *qs_btn_vol_mute;
    GtkWidget *qs_lbl_battery;
    GtkWidget *qs_lbl_telemetry;
    GtkWidget *qs_power_revealer;
    gboolean gameboost_active;
    int power_mode;

    // OSD Widgets
    GtkWidget *osd_icon;
    GtkWidget *osd_progress;
    guint osd_timeout_id;

    // Notification Widgets
    GtkWidget *notif_icon;
    GtkWidget *notif_title;
    GtkWidget *notif_body;
    guint notif_timeout_id;

    // Greeter Widgets
    GtkWidget *greeter_clock_label;
    GtkWidget *greeter_date_label;
    GtkWidget *greeter_pass_entry;
    GtkWidget *greeter_status_label;
    GtkWidget *greeter_spinner;

    guint clock_timer_id;
    gboolean is_greeter_mode;
} UiShell;

UiShell *ui_shell_create(SystemController *sys_ctrl, AuthManager *auth_mgr, gboolean greeter_mode);
void ui_shell_destroy(UiShell *shell);

void ui_shell_apply_theme(gboolean dark_mode);

void ui_shell_toggle_launcher(UiShell *shell);
void ui_shell_toggle_quicksettings(UiShell *shell);
void ui_shell_toggle_games(UiShell *shell);
void ui_shell_show_osd(UiShell *shell, const char *icon_name, int percent);
void ui_shell_show_notification(UiShell *shell, const char *app_name, const char *summary,
                                const char *body, const char *icon);

void ui_shell_lock(UiShell *shell);
void ui_shell_unlock(UiShell *shell);

#ifdef __cplusplus
}
#endif

#endif /* AXIS_UI_SHELL_H */
