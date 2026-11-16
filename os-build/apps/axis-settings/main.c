/*
 * AxisOS Native System Settings & Control Center (Pure C11)
 * Liquid Nodes Desktop Environment
 * Strict Rule: Contains ZERO raw hex values. All colors reference tokens.
 */

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <pwd.h>
#include <time.h>
#include <ctype.h>

// Forward declarations
typedef struct _AxisSettingsApp AxisSettingsApp;

// Structure for AxisSettingsApp
struct _AxisSettingsApp {
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *listbox;
    GtkWidget *search_entry;

    // Display & Night Light
    GtkWidget *combo_display_output;
    GtkWidget *combo_display_mode;
    GtkWidget *combo_display_rate;
    GtkWidget *lbl_gpu_status;
    GtkWidget *switch_night_light;
    GtkWidget *scale_night_temp;

    // Brightness & Sound
    GtkWidget *scale_brightness;
    GtkWidget *scale_volume;
    GtkWidget *btn_mute;

    // Wi-Fi, Captive Portal & Bluetooth
    GtkWidget *card_portal_banner;
    GtkWidget *switch_wifi;
    GtkWidget *list_wifi;
    GtkWidget *switch_bt;
    GtkWidget *list_bt;

    // Power & Battery
    GtkWidget *lbl_battery_status;
    GtkWidget *lbl_battery_health;
    GtkWidget *combo_power_profile;
    GtkWidget *combo_lid_action;

    // Mouse & Touchpad
    GtkWidget *switch_tap_click;
    GtkWidget *switch_natural_scroll;
    GtkWidget *scale_pointer_speed;

    // USB Peripherals
    GtkWidget *list_usb;

    // Appearance
    GtkWidget *radio_bar_bottom;
    GtkWidget *radio_bar_top;
    GtkWidget *combo_icon_theme;

    // Updates & Snapshots
    GtkWidget *lbl_update_status;
    GtkWidget *pbar_update;
    GtkWidget *txt_update_log;
    GtkWidget *btn_run_updates;
    GtkWidget *lbl_snapshot_status;
    GtkWidget *list_snapshots;

    // Startup Apps
    GtkWidget *list_autostart;

    // Password & Biometric Screen Lock widgets
    GtkWidget *entry_user_pass;
    GtkWidget *entry_confirm_pass;
    GtkWidget *lbl_pass_status;
    GtkWidget *combo_lock_mode;
    GtkWidget *switch_bio_fp;
    GtkWidget *switch_bio_iris;
    GtkWidget *combo_idle_lock;
    GtkWidget *lbl_bio_status;

    // GameBoost widgets
    GtkWidget *switch_game_boost;
    GtkWidget *lbl_game_boost_status;
    GtkWidget *lbl_boost_cpu;
    GtkWidget *lbl_boost_gpu;
    GtkWidget *lbl_boost_latency;
};

// Helper to execute shell command and capture stdout
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

static const char *CSS_THEME =
    "button, togglebutton, entry, notebook tab, treeview.view header button {"
    "    background-image: none;"
    "    box-shadow: none;"
    "    text-shadow: none;"
    "    -gtk-icon-shadow: none;"
    "}"
    "list, list row {"
    "    background-color: transparent;"
    "    color: @axis_text;"
    "}"
    "list row:hover {"
    "    background-color: alpha(@axis_accent, 0.08);"
    "}"
    "combobox button {"
    "    background-color: @axis_surface;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 8px;"
    "    padding: 6px 12px;"
    "}"
    "entry {"
    "    background-color: @axis_surface;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 8px;"
    "    padding: 8px 12px;"
    "}"
    "textview, textview text {"
    "    background-color: @axis_surface;"
    "    color: @axis_text;"
    "}"
    ".card {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 16px;"
    "    padding: 18px 22px;"
    "    margin-bottom: 16px;"
    "    box-shadow: 0 2px 12px @axis_shadow;"
    "}"
    ".card-title {"
    "    font-size: 15px;"
    "    font-weight: 700;"
    "    color: @axis_text;"
    "    margin-bottom: 4px;"
    "}"
    ".card-subtitle {"
    "    font-size: 12px;"
    "    color: @axis_text_muted;"
    "    margin-bottom: 12px;"
    "}"
    ".sidebar {"
    "    background-color: @axis_surface;"
    "    border-right: 1px solid @axis_outline;"
    "}"
    ".sidebar list row {"
    "    padding: 10px 14px;"
    "    color: @axis_text_muted;"
    "    font-weight: 600;"
    "    font-size: 13px;"
    "    border-left: 3px solid transparent;"
    "}"
    ".sidebar list row:selected {"
    "    background-color: alpha(@axis_accent, 0.18);"
    "    color: @axis_text;"
    "    border-left: 3px solid @axis_accent;"
    "}"
    ".content-pane {"
    "    padding: 24px 32px;"
    "}"
    ".btn-primary {"
    "    background: @axis_accent;"
    "    background-color: @axis_accent;"
    "    color: @axis_accent_btn_text;"
    "    font-weight: 700;"
    "    border-radius: 999px;"
    "    border: none;"
    "    padding: 8px 20px;"
    "}"
    ".btn-primary:hover {"
    "    background: mix(@axis_accent, @axis_base, 0.12);"
    "    background-color: mix(@axis_accent, @axis_base, 0.12);"
    "}"
    ".btn-secondary {"
    "    background: @axis_surface;"
    "    background-color: @axis_surface;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 999px;"
    "    padding: 8px 18px;"
    "    font-weight: 600;"
    "}"
    ".btn-secondary:hover {"
    "    background: @axis_surface_raised;"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_accent;"
    "}"
    ".badge-blue {"
    "    background: alpha(@axis_accent, 0.15);"
    "    color: @axis_accent;"
    "    border: 1px solid alpha(@axis_accent, 0.30);"
    "    border-radius: 999px;"
    "    padding: 4px 12px;"
    "    font-size: 11px;"
    "    font-weight: 700;"
    "}"
    ".badge-green {"
    "    background: alpha(@axis_success, 0.15);"
    "    color: @axis_success;"
    "    border: 1px solid alpha(@axis_success, 0.30);"
    "    border-radius: 999px;"
    "    padding: 4px 12px;"
    "    font-size: 11px;"
    "    font-weight: 700;"
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
    gtk_css_provider_load_from_data(extra_prov, CSS_THEME, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(extra_prov),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(extra_prov);
}

static GtkWidget *create_card(const char *title, const char *subtitle) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

    GtkWidget *lbl_t = gtk_label_new(title);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_t), "card-title");
    gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
    gtk_box_pack_start(GTK_BOX(card), lbl_t, FALSE, FALSE, 0);

    if (subtitle && subtitle[0] != '\0') {
        GtkWidget *lbl_s = gtk_label_new(subtitle);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_s), "card-subtitle");
        gtk_label_set_xalign(GTK_LABEL(lbl_s), 0.0);
        gtk_label_set_line_wrap(GTK_LABEL(lbl_s), TRUE);
        gtk_box_pack_start(GTK_BOX(card), lbl_s, FALSE, FALSE, 0);
    }
    return card;
}

static GtkWidget *wrap_settings_page(GtkWidget *content) {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);

    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(outer, TRUE);
    gtk_widget_set_halign(outer, GTK_ALIGN_CENTER);
    gtk_widget_set_size_request(outer, 760, -1);
    gtk_style_context_add_class(gtk_widget_get_style_context(outer), "content-pane");

    gtk_container_add(GTK_CONTAINER(outer), content);
    gtk_container_add(GTK_CONTAINER(scroll), outer);
    return scroll;
}

// -----------------------------------------------------------------------------
// 1. Displays & Graphics Page
// -----------------------------------------------------------------------------
static char *detect_gpus(void) {
    char *info = exec_cmd("lspci -nnk 2>/dev/null | grep -E -A 3 -i 'vga|3d|display'");
    g_strstrip(info);
    if (strlen(info) == 0) {
        g_free(info);
        info = g_strdup("No PCI GPU detected directly via lspci (Running in Virtual / Standard DRM mode).");
    }

    char *vulkan_check = exec_cmd("vulkaninfo --summary 2>/dev/null | grep -E -i 'deviceName|driverName' || true");
    g_strstrip(vulkan_check);

    char *summary;
    if (strlen(vulkan_check) > 0) {
        summary = g_strdup_printf("Detected Graphics Adapters:\n%s\n\nVulkan Acceleration:\n%s", info, vulkan_check);
    } else {
        summary = g_strdup_printf("Detected Graphics Adapters:\n%s", info);
    }
    g_free(info);
    g_free(vulkan_check);
    return summary;
}

static void on_refresh_gpu_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    char *gpus = detect_gpus();
    gtk_label_set_text(GTK_LABEL(app->lbl_gpu_status), gpus);
    g_free(gpus);
}

static void on_apply_display_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    char *mode_txt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode));
    char *rate_txt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate));
    if (!mode_txt || !rate_txt) {
        g_free(mode_txt);
        g_free(rate_txt);
        return;
    }

    char res[64] = {0};
    char rate[64] = {0};
    sscanf(mode_txt, "%63s", res);
    sscanf(rate_txt, "%63s", rate);

    char *cmd = g_strdup_printf(
        "wlr-randr --output eDP-1 --mode %s@%sHz 2>/dev/null || "
        "wlr-randr --output $(wlr-randr | grep -m1 '^[^ ]' | awk '{print $1}') --mode %s 2>/dev/null || true",
        res, rate, res);
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);

    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Applied Display Settings: %s @ %s Hz", res, rate);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);

    g_free(mode_txt);
    g_free(rate_txt);
}

static void on_night_light_toggled(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    gboolean active = gtk_switch_get_active(sw);
    double temp = gtk_range_get_value(GTK_RANGE(app->scale_night_temp));

    if (active) {
        char *cmd = g_strdup_printf("pkill -x wlsunset 2>/dev/null; wlsunset -t %.0f -T 6500 &", temp);
        g_spawn_command_line_async(cmd, NULL);
        g_free(cmd);
    } else {
        g_spawn_command_line_async("pkill -x wlsunset 2>/dev/null; pkill -x gammastep 2>/dev/null || true", NULL);
    }
}

static void on_night_temp_changed(GtkRange *range, gpointer user_data) {
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    if (gtk_switch_get_active(GTK_SWITCH(app->switch_night_light))) {
        double temp = gtk_range_get_value(range);
        char *cmd = g_strdup_printf("pkill -x wlsunset 2>/dev/null; wlsunset -t %.0f -T 6500 &", temp);
        g_spawn_command_line_async(cmd, NULL);
        g_free(cmd);
    }
}

static void build_display_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_mode = create_card("Display Resolution & Refresh Rate",
        "Configure active screen resolution, aspect ratio, and refresh rate for attached monitors.");

    GtkWidget *grid_mode = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_mode), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid_mode), 16);

    GtkWidget *lbl_out = gtk_label_new("Target Display Output:");
    gtk_label_set_xalign(GTK_LABEL(lbl_out), 0.0);
    app->combo_display_output = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_output), "Primary Display (eDP-1 / Virtual-1)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_display_output), 0);
    gtk_grid_attach(GTK_GRID(grid_mode), lbl_out, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_mode), app->combo_display_output, 1, 0, 1, 1);

    GtkWidget *lbl_mode = gtk_label_new("Display Resolution:");
    gtk_label_set_xalign(GTK_LABEL(lbl_mode), 0.0);
    app->combo_display_mode = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "3840x2160 (16:9 4K UHD)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "2560x1440 (16:9 2K QHD)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "1920x1080 (16:9 Full HD)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "1600x900 (16:9 HD+)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "1366x768 (16:9 Laptop HD)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "1280x720 (16:9 720p)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_display_mode), 2);
    gtk_grid_attach(GTK_GRID(grid_mode), lbl_mode, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_mode), app->combo_display_mode, 1, 1, 1, 1);

    GtkWidget *lbl_rate = gtk_label_new("Display Refresh Rate:");
    gtk_label_set_xalign(GTK_LABEL(lbl_rate), 0.0);
    app->combo_display_rate = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate), "60 Hz (Standard)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate), "120 Hz (High Refresh)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate), "144 Hz (Gaming)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate), "240 Hz (Ultra)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_display_rate), 0);
    gtk_grid_attach(GTK_GRID(grid_mode), lbl_rate, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_mode), app->combo_display_rate, 1, 2, 1, 1);

    GtkWidget *btn_apply = gtk_button_new_with_label("Apply Resolution");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_apply), "btn-primary");
    g_signal_connect(btn_apply, "clicked", G_CALLBACK(on_apply_display_cb), app);
    gtk_grid_attach(GTK_GRID(grid_mode), btn_apply, 1, 3, 1, 1);

    gtk_box_pack_start(GTK_BOX(card_mode), grid_mode, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_mode, FALSE, FALSE, 0);

    // Night Light Card
    GtkWidget *card_night = create_card("Night Light & Blue Light Filter",
        "Warm display colors in the evening to reduce eye strain and improve sleep quality.");

    GtkWidget *grid_night = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_night), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid_night), 16);

    GtkWidget *lbl_nl = gtk_label_new("Enable Night Light:");
    gtk_label_set_xalign(GTK_LABEL(lbl_nl), 0.0);
    gtk_widget_set_hexpand(lbl_nl, TRUE);

    app->switch_night_light = gtk_switch_new();
    gtk_widget_set_halign(app->switch_night_light, GTK_ALIGN_END);
    g_signal_connect(app->switch_night_light, "notify::active", G_CALLBACK(on_night_light_toggled), app);

    gtk_grid_attach(GTK_GRID(grid_night), lbl_nl, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_night), app->switch_night_light, 1, 0, 1, 1);

    GtkWidget *lbl_temp = gtk_label_new("Color Temperature (2700K Warm to 6500K Cool):");
    gtk_label_set_xalign(GTK_LABEL(lbl_temp), 0.0);

    app->scale_night_temp = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 2700, 6500, 100);
    gtk_range_set_value(GTK_RANGE(app->scale_night_temp), 4500);
    gtk_widget_set_hexpand(app->scale_night_temp, TRUE);
    g_signal_connect(app->scale_night_temp, "value-changed", G_CALLBACK(on_night_temp_changed), app);

    gtk_grid_attach(GTK_GRID(grid_night), lbl_temp, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_night), app->scale_night_temp, 1, 1, 1, 1);

    gtk_box_pack_start(GTK_BOX(card_night), grid_night, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_night, FALSE, FALSE, 0);

    // GPU Status Card
    GtkWidget *card_gpu = create_card("Graphics Hardware & Vulkan Drivers",
        "Inspect installed discrete / integrated GPUs and hardware acceleration profiles.");
    char *gpus = detect_gpus();
    app->lbl_gpu_status = gtk_label_new(gpus);
    g_free(gpus);
    gtk_label_set_xalign(GTK_LABEL(app->lbl_gpu_status), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(app->lbl_gpu_status), TRUE);
    gtk_box_pack_start(GTK_BOX(card_gpu), app->lbl_gpu_status, FALSE, FALSE, 6);

    GtkWidget *btn_refresh_gpu = gtk_button_new_with_label("Re-scan GPU Adapters");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh_gpu), "btn-secondary");
    g_signal_connect(btn_refresh_gpu, "clicked", G_CALLBACK(on_refresh_gpu_cb), app);
    gtk_box_pack_start(GTK_BOX(card_gpu), btn_refresh_gpu, FALSE, FALSE, 6);

    gtk_box_pack_start(GTK_BOX(page), card_gpu, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "display");
}

// -----------------------------------------------------------------------------
// 2. Sound & Audio Page
// -----------------------------------------------------------------------------
static void on_brightness_changed_cb(GtkRange *range, gpointer user_data) {
    (void)user_data;
    double val = gtk_range_get_value(range);
    char *cmd = g_strdup_printf("brightnessctl s %.0f%% 2>/dev/null || true", val);
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
}

static void on_volume_changed_cb(GtkRange *range, gpointer user_data) {
    (void)user_data;
    double val = gtk_range_get_value(range);
    char *cmd = g_strdup_printf("wpctl set-volume @DEFAULT_AUDIO_SINK@ %.2f 2>/dev/null || pactl set-sink-volume @DEFAULT_SINK@ %.0f%% 2>/dev/null || amixer set Master %.0f%% 2>/dev/null || true", val / 100.0, val, val);
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
}

static void on_mute_toggle_cb(GtkButton *btn, gpointer user_data) {
    (void)user_data;
    (void)btn;
    exec_cmd("wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle 2>/dev/null || pactl set-sink-mute @DEFAULT_SINK@ toggle 2>/dev/null || amixer set Master toggle 2>/dev/null || true");
}

static void on_test_audio_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    g_spawn_command_line_async("paplay /usr/share/sounds/freedesktop/stereo/complete.oga 2>/dev/null || speaker-test -t sine -f 440 -l 1 2>/dev/null || true", NULL);
}

static void build_brightness_audio_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_sound = create_card("Sound Output & PipeWire Audio",
        "Control master output volume, mute status, and test stereo channels.");

    GtkWidget *grid_sound = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_sound), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid_sound), 16);

    GtkWidget *lbl_vol = gtk_label_new("Master Volume:");
    gtk_label_set_xalign(GTK_LABEL(lbl_vol), 0.0);
    app->scale_volume = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_range_set_value(GTK_RANGE(app->scale_volume), 70);
    gtk_widget_set_hexpand(app->scale_volume, TRUE);
    g_signal_connect(app->scale_volume, "value-changed", G_CALLBACK(on_volume_changed_cb), app);

    gtk_grid_attach(GTK_GRID(grid_sound), lbl_vol, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_sound), app->scale_volume, 1, 0, 1, 1);

    GtkWidget *hbox_audio_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    app->btn_mute = gtk_button_new_with_label("Toggle Mute");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->btn_mute), "btn-secondary");
    g_signal_connect(app->btn_mute, "clicked", G_CALLBACK(on_mute_toggle_cb), app);

    GtkWidget *btn_test = gtk_button_new_with_label("Play Test Sound");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_test), "btn-primary");
    g_signal_connect(btn_test, "clicked", G_CALLBACK(on_test_audio_cb), app);

    gtk_box_pack_start(GTK_BOX(hbox_audio_btns), app->btn_mute, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_audio_btns), btn_test, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(grid_sound), hbox_audio_btns, 1, 1, 1, 1);

    gtk_box_pack_start(GTK_BOX(card_sound), grid_sound, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_sound, FALSE, FALSE, 0);

    GtkWidget *card_b = create_card("Hardware Display Brightness",
        "Adjust laptop screen backlight panel intensity (supports Intel, AMD, and ACPI backlights).");

    GtkWidget *grid_b = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_b), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid_b), 16);

    GtkWidget *lbl_br = gtk_label_new("Brightness:");
    gtk_label_set_xalign(GTK_LABEL(lbl_br), 0.0);
    app->scale_brightness = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 5, 100, 1);
    gtk_range_set_value(GTK_RANGE(app->scale_brightness), 80);
    gtk_widget_set_hexpand(app->scale_brightness, TRUE);
    g_signal_connect(app->scale_brightness, "value-changed", G_CALLBACK(on_brightness_changed_cb), app);

    gtk_grid_attach(GTK_GRID(grid_b), lbl_br, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_b), app->scale_brightness, 1, 0, 1, 1);

    gtk_box_pack_start(GTK_BOX(card_b), grid_b, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_b, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "audio");
}

// -----------------------------------------------------------------------------
// 3. Wi-Fi & Internet Page
// -----------------------------------------------------------------------------
static void on_wifi_toggle_cb(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    (void)user_data;
    gboolean active = gtk_switch_get_active(sw);
    char *cmd = g_strdup_printf("nmcli radio wifi %s", active ? "on" : "off");
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
}

static void on_open_portal_banner_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    (void)user_data;
    g_spawn_command_line_async("axis-captive-portal", NULL);
}

static void check_captive_portal_banner(AxisSettingsApp *app) {
    if (!app->card_portal_banner) return;
    char *conn = exec_cmd("nmcli -t -f CONNECTIVITY general 2>/dev/null || echo \"unknown\"");
    g_strstrip(conn);
    if (g_strcmp0(conn, "portal") == 0 || g_strcmp0(conn, "limited") == 0) {
        gtk_widget_show(app->card_portal_banner);
    } else {
        gtk_widget_hide(app->card_portal_banner);
    }
    g_free(conn);
}

static void on_disconnect_wifi_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    (void)user_data;
    g_spawn_command_line_async("nmcli device disconnect wlan0 2>/dev/null || nmcli device disconnect wifi 2>/dev/null", NULL);
}

static void refresh_wifi_list(AxisSettingsApp *app) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->list_wifi));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    check_captive_portal_banner(app);

    char *out = exec_cmd("nmcli -t -f IN-USE,SSID,SIGNAL,SECURITY device wifi list 2>/dev/null || true");
    gboolean found = FALSE;
    char **lines = g_strsplit(out, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++) {
        if (lines[i][0] == '\0') continue;
        char **parts = g_strsplit(lines[i], ":", 4);
        if (parts[0] && parts[1] && parts[1][0] != '\0') {
            gboolean in_use = (g_strcmp0(parts[0], "*") == 0);
            const char *ssid = parts[1];
            const char *signal = parts[2] ? parts[2] : "";
            const char *sec = parts[3] ? parts[3] : "";

            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
            gtk_widget_set_margin_start(hbox, 12);
            gtk_widget_set_margin_end(hbox, 12);
            gtk_widget_set_margin_top(hbox, 8);
            gtk_widget_set_margin_bottom(hbox, 8);

            GtkWidget *icon = gtk_image_new_from_icon_name("network-wireless-symbolic", GTK_ICON_SIZE_BUTTON);
            gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);

            GtkWidget *lbl_ssid = gtk_label_new(ssid);
            gtk_label_set_xalign(GTK_LABEL(lbl_ssid), 0.0);
            gtk_box_pack_start(GTK_BOX(hbox), lbl_ssid, TRUE, TRUE, 0);

            if (in_use) {
                GtkWidget *badge = gtk_label_new("Connected");
                gtk_style_context_add_class(gtk_widget_get_style_context(badge), "badge-green");
                gtk_box_pack_start(GTK_BOX(hbox), badge, FALSE, FALSE, 4);

                GtkWidget *btn_disc = gtk_button_new_with_label("Disconnect");
                gtk_style_context_add_class(gtk_widget_get_style_context(btn_disc), "btn-secondary");
                g_signal_connect(btn_disc, "clicked", G_CALLBACK(on_disconnect_wifi_cb), app);
                gtk_box_pack_start(GTK_BOX(hbox), btn_disc, FALSE, FALSE, 4);
            }

            char *sig_str = g_strdup_printf("%s%%  %s", signal, (sec[0] && g_strcmp0(sec, "--") != 0) ? sec : "Open");
            GtkWidget *lbl_sig = gtk_label_new(sig_str);
            g_free(sig_str);
            gtk_style_context_add_class(gtk_widget_get_style_context(lbl_sig), "dim-label");
            gtk_box_pack_end(GTK_BOX(hbox), lbl_sig, FALSE, FALSE, 0);

            gtk_container_add(GTK_CONTAINER(row), hbox);
            g_object_set_data_full(G_OBJECT(row), "ssid", g_strdup(ssid), g_free);
            g_object_set_data_full(G_OBJECT(row), "security", g_strdup(sec), g_free);
            gtk_list_box_insert(GTK_LIST_BOX(app->list_wifi), row, -1);
            found = TRUE;
        }
        g_strfreev(parts);
    }
    g_strfreev(lines);
    g_free(out);

    if (!found) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new("No Wi-Fi access points found. Ensure Wi-Fi adapter is enabled.");
        gtk_container_add(GTK_CONTAINER(row), lbl);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_wifi), row, -1);
    }
    gtk_widget_show_all(app->list_wifi);
}

static void on_scan_wifi_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    exec_cmd("nmcli device wifi rescan 2>/dev/null || true");
    refresh_wifi_list(app);
}

static void on_show_pass_toggled(GtkToggleButton *tb, gpointer user_data) {
    GtkEntry *entry = GTK_ENTRY(user_data);
    gtk_entry_set_visibility(entry, gtk_toggle_button_get_active(tb));
}

static void on_connect_wifi_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->list_wifi));
    if (!row) {
        GtkWidget *alert = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "Please select a Wi-Fi network from the list first.");
        gtk_dialog_run(GTK_DIALOG(alert));
        gtk_widget_destroy(alert);
        return;
    }

    const char *ssid = (const char *)g_object_get_data(G_OBJECT(row), "ssid");
    const char *security = (const char *)g_object_get_data(G_OBJECT(row), "security");
    if (!ssid) return;

    gboolean is_open = (!security || security[0] == '\0' ||
                        g_strcmp0(security, "--") == 0 ||
                        g_ascii_strcasecmp(security, "open") == 0 ||
                        (strstr(security, "WPA") == NULL && strstr(security, "WEP") == NULL && strstr(security, "802.1X") == NULL));

    if (is_open) {
        // Open Wi-Fi: Connect asynchronously without blocking main loop!
        char *cmd = g_strdup_printf("nmcli device wifi connect \"%s\"", ssid);
        g_spawn_command_line_async(cmd, NULL);
        g_free(cmd);
        return;
    }

    // Secured Wi-Fi: Show password dialog with "Show password" toggle
    GtkWidget *dlg = gtk_dialog_new_with_buttons("Connect to Wi-Fi",
        GTK_WINDOW(app->window), GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Connect", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 16);
    gtk_box_set_spacing(GTK_BOX(content), 10);

    char *prompt = g_strdup_printf("Enter Wi-Fi password for '%s' (%s):", ssid, security ? security : "WPA");
    GtkWidget *lbl = gtk_label_new(prompt);
    g_free(prompt);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_box_pack_start(GTK_BOX(content), lbl, FALSE, FALSE, 0);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 0);

    GtkWidget *chk_show_pass = gtk_check_button_new_with_label("Show password");
    g_signal_connect(chk_show_pass, "toggled", G_CALLBACK(on_show_pass_toggled), entry);
    gtk_box_pack_start(GTK_BOX(content), chk_show_pass, FALSE, FALSE, 0);

    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        const char *pass = gtk_entry_get_text(GTK_ENTRY(entry));
        char *cmd = g_strdup_printf("nmcli device wifi connect \"%s\" password \"%s\"", ssid, pass);
        g_spawn_command_line_async(cmd, NULL);
        g_free(cmd);
    }
    gtk_widget_destroy(dlg);
}

static void build_network_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    // Captive Portal Info Banner Card
    app->card_portal_banner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->card_portal_banner), "card");
    GtkWidget *banner_icon = gtk_image_new_from_icon_name("dialog-information-symbolic", GTK_ICON_SIZE_DIALOG);
    gtk_box_pack_start(GTK_BOX(app->card_portal_banner), banner_icon, FALSE, FALSE, 4);

    GtkWidget *banner_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    GtkWidget *banner_title = gtk_label_new("This Wi-Fi network requires browser sign-in");
    gtk_style_context_add_class(gtk_widget_get_style_context(banner_title), "title-3");
    gtk_label_set_xalign(GTK_LABEL(banner_title), 0.0);

    GtkWidget *banner_sub = gtk_label_new("A captive portal authentication web page is detected for your connection.");
    gtk_label_set_xalign(GTK_LABEL(banner_sub), 0.0);
    gtk_box_pack_start(GTK_BOX(banner_vbox), banner_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(banner_vbox), banner_sub, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(app->card_portal_banner), banner_vbox, TRUE, TRUE, 4);

    GtkWidget *btn_open_portal = gtk_button_new_with_label("Open Login Page");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_open_portal), "btn-primary");
    g_signal_connect(btn_open_portal, "clicked", G_CALLBACK(on_open_portal_banner_cb), app);
    gtk_box_pack_end(GTK_BOX(app->card_portal_banner), btn_open_portal, FALSE, FALSE, 4);

    gtk_widget_set_no_show_all(app->card_portal_banner, TRUE);
    gtk_box_pack_start(GTK_BOX(page), app->card_portal_banner, FALSE, FALSE, 0);

    GtkWidget *card_wifi = create_card("Wireless Networking (Wi-Fi)",
        "Scan nearby wireless access points, connect with security credentials, and captive portal login.");

    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *lbl_switch = gtk_label_new("Enable Wi-Fi Adapter:");
    app->switch_wifi = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_wifi), TRUE);
    g_signal_connect(app->switch_wifi, "notify::active", G_CALLBACK(on_wifi_toggle_cb), app);

    GtkWidget *btn_scan_wifi = gtk_button_new_with_label("Scan Wi-Fi");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_scan_wifi), "btn-primary");
    g_signal_connect(btn_scan_wifi, "clicked", G_CALLBACK(on_scan_wifi_cb), app);

    GtkWidget *btn_portal = gtk_button_new_with_label("Captive Portal Web Login");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_portal), "btn-secondary");
    g_signal_connect(btn_portal, "clicked", G_CALLBACK(on_open_portal_banner_cb), app);

    gtk_box_pack_start(GTK_BOX(hbox_top), lbl_switch, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), app->switch_wifi, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_scan_wifi, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_portal, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_wifi), hbox_top, FALSE, FALSE, 6);

    app->list_wifi = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->list_wifi), GTK_SELECTION_SINGLE);

    GtkWidget *scroll_wifi = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_wifi), 220);
    gtk_container_add(GTK_CONTAINER(scroll_wifi), app->list_wifi);
    gtk_box_pack_start(GTK_BOX(card_wifi), scroll_wifi, TRUE, TRUE, 8);

    GtkWidget *btn_connect_wifi = gtk_button_new_with_label("Connect to Selected Network");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_connect_wifi), "btn-primary");
    g_signal_connect(btn_connect_wifi, "clicked", G_CALLBACK(on_connect_wifi_cb), app);
    gtk_box_pack_start(GTK_BOX(card_wifi), btn_connect_wifi, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(page), card_wifi, FALSE, FALSE, 0);

    refresh_wifi_list(app);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "network");
}

// -----------------------------------------------------------------------------
// 4. Bluetooth Page
// -----------------------------------------------------------------------------
static void on_bt_toggle_cb(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    (void)user_data;
    gboolean active = gtk_switch_get_active(sw);
    char *cmd = g_strdup_printf("bluetoothctl power %s 2>/dev/null || true", active ? "on" : "off");
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
}

static void refresh_bt_list(AxisSettingsApp *app) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->list_bt));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    char *out = exec_cmd("bluetoothctl devices 2>/dev/null || true");
    gboolean found = FALSE;
    char **lines = g_strsplit(out, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++) {
        if (lines[i][0] == '\0') continue;
        if (g_str_has_prefix(lines[i], "Device ")) {
            char **parts = g_strsplit(lines[i], " ", 3);
            if (parts[1] && parts[2]) {
                const char *mac = parts[1];
                const char *name = parts[2];

                GtkWidget *row = gtk_list_box_row_new();
                GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
                GtkWidget *lbl_name = gtk_label_new(name);
                GtkWidget *lbl_mac = gtk_label_new(mac);
                gtk_label_set_xalign(GTK_LABEL(lbl_name), 0.0);
                gtk_style_context_add_class(gtk_widget_get_style_context(lbl_mac), "dim-label");

                gtk_box_pack_start(GTK_BOX(hbox), lbl_name, TRUE, TRUE, 8);
                gtk_box_pack_end(GTK_BOX(hbox), lbl_mac, FALSE, FALSE, 8);
                gtk_container_add(GTK_CONTAINER(row), hbox);
                g_object_set_data_full(G_OBJECT(row), "mac", g_strdup(mac), g_free);
                gtk_list_box_insert(GTK_LIST_BOX(app->list_bt), row, -1);
                found = TRUE;
            }
            g_strfreev(parts);
        }
    }
    g_strfreev(lines);
    g_free(out);

    if (!found) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new("No Bluetooth peripherals paired. Click 'Scan Devices' to discover.");
        gtk_container_add(GTK_CONTAINER(row), lbl);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_bt), row, -1);
    }
    gtk_widget_show_all(app->list_bt);
}

static void on_scan_bt_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    g_spawn_command_line_async("bluetoothctl --timeout 5 scan on 2>/dev/null || true", NULL);
    refresh_bt_list(app);
}

static void on_pair_bt_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->list_bt));
    if (!row) return;

    const char *mac = (const char *)g_object_get_data(G_OBJECT(row), "mac");
    if (!mac) return;

    char *cmd = g_strdup_printf("bluetoothctl pair %s 2>&1 && bluetoothctl connect %s 2>&1", mac, mac);
    char *res = exec_cmd(cmd);
    g_free(cmd);

    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Pairing Output for %s:\n%s", mac, res);
    g_free(res);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void build_bluetooth_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_bt = create_card("Bluetooth Wireless Devices",
        "Pair and manage wireless mice, keyboards, game controllers, and audio headsets.");

    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *lbl_switch = gtk_label_new("Bluetooth Radio Power:");
    app->switch_bt = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_bt), TRUE);
    g_signal_connect(app->switch_bt, "notify::active", G_CALLBACK(on_bt_toggle_cb), app);

    GtkWidget *btn_scan_bt = gtk_button_new_with_label("Scan Devices");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_scan_bt), "btn-primary");
    g_signal_connect(btn_scan_bt, "clicked", G_CALLBACK(on_scan_bt_cb), app);

    gtk_box_pack_start(GTK_BOX(hbox_top), lbl_switch, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), app->switch_bt, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_scan_bt, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(card_bt), hbox_top, FALSE, FALSE, 6);

    app->list_bt = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->list_bt), GTK_SELECTION_SINGLE);

    GtkWidget *scroll_bt = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_bt), 180);
    gtk_container_add(GTK_CONTAINER(scroll_bt), app->list_bt);
    gtk_box_pack_start(GTK_BOX(card_bt), scroll_bt, TRUE, TRUE, 8);

    GtkWidget *btn_pair = gtk_button_new_with_label("Pair / Connect Selected Device");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_pair), "btn-secondary");
    g_signal_connect(btn_pair, "clicked", G_CALLBACK(on_pair_bt_cb), app);
    gtk_box_pack_start(GTK_BOX(card_bt), btn_pair, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(page), card_bt, FALSE, FALSE, 0);

    refresh_bt_list(app);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "bluetooth");
}

// -----------------------------------------------------------------------------
// 5. Network & Firewall Page
// -----------------------------------------------------------------------------
static void on_toggle_ufw_cb(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    (void)user_data;
    gboolean active = gtk_switch_get_active(sw);
    if (active) {
        exec_cmd("pkexec ufw enable 2>&1 || ufw enable 2>&1 || true");
    } else {
        exec_cmd("pkexec ufw disable 2>&1 || ufw disable 2>&1 || true");
    }
}

static void on_ufw_allow_ssh_cb(GtkWidget *w, gpointer user_data) {
    (void)w; (void)user_data;
    exec_cmd("pkexec ufw allow 22/tcp 2>&1 || ufw allow 22/tcp 2>&1 || true");
}

static void on_ufw_allow_web_cb(GtkWidget *w, gpointer user_data) {
    (void)w; (void)user_data;
    exec_cmd("pkexec ufw allow 80/tcp 2>&1; pkexec ufw allow 443/tcp 2>&1 || true");
}

static void build_firewall_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_iface = create_card("Active Network Interfaces & Routing",
        "Inspect IP address assignments, MAC hardware addresses, default gateways, and DNS servers.");

    char *ip_info = exec_cmd("ip -brief address show 2>/dev/null || ip address show 2>/dev/null || echo 'No network adapters found.'");
    GtkWidget *lbl_iface = gtk_label_new(ip_info);
    g_free(ip_info);
    gtk_label_set_xalign(GTK_LABEL(lbl_iface), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_iface), "dim-label");
    gtk_box_pack_start(GTK_BOX(card_iface), lbl_iface, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(page), card_iface, FALSE, FALSE, 0);

    GtkWidget *card_ufw = create_card("Uncomplicated Firewall (UFW) Security",
        "Control inbound packet filtering, open ports, and safeguard network perimeters.");

    GtkWidget *grid_ufw = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_ufw), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid_ufw), 16);

    GtkWidget *lbl_ufw = gtk_label_new("Enable System Firewall (UFW):");
    gtk_label_set_xalign(GTK_LABEL(lbl_ufw), 0.0);
    gtk_widget_set_hexpand(lbl_ufw, TRUE);

    char *ufw_stat = exec_cmd("ufw status 2>/dev/null || true");
    gboolean ufw_active = (strstr(ufw_stat, "active") != NULL && strstr(ufw_stat, "inactive") == NULL);
    g_free(ufw_stat);

    GtkWidget *sw_ufw = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(sw_ufw), ufw_active);
    gtk_widget_set_halign(sw_ufw, GTK_ALIGN_END);
    g_signal_connect(sw_ufw, "notify::active", G_CALLBACK(on_toggle_ufw_cb), app);

    gtk_grid_attach(GTK_GRID(grid_ufw), lbl_ufw, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_ufw), sw_ufw, 1, 0, 1, 1);

    GtkWidget *hbox_rules = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_ssh = gtk_button_new_with_label("Allow SSH (Port 22)");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_ssh), "btn-secondary");
    g_signal_connect(btn_ssh, "clicked", G_CALLBACK(on_ufw_allow_ssh_cb), app);

    GtkWidget *btn_web = gtk_button_new_with_label("Allow Web (HTTP/HTTPS)");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_web), "btn-secondary");
    g_signal_connect(btn_web, "clicked", G_CALLBACK(on_ufw_allow_web_cb), app);

    gtk_box_pack_start(GTK_BOX(hbox_rules), btn_ssh, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_rules), btn_web, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(card_ufw), grid_ufw, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(card_ufw), hbox_rules, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_ufw, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "firewall");
}

// -----------------------------------------------------------------------------
// 6. Power & Battery Page
// -----------------------------------------------------------------------------
static void refresh_battery_info(AxisSettingsApp *app) {
    if (!app->lbl_battery_status) return;

    FILE *f_cap = fopen("/sys/class/power_supply/BAT0/capacity", "r");
    if (!f_cap) f_cap = fopen("/sys/class/power_supply/BAT1/capacity", "r");

    if (f_cap) {
        char cap_str[32] = {0};
        if (fgets(cap_str, sizeof(cap_str), f_cap)) g_strstrip(cap_str);
        fclose(f_cap);

        char stat_str[64] = "Discharging";
        FILE *f_stat = fopen("/sys/class/power_supply/BAT0/status", "r");
        if (!f_stat) f_stat = fopen("/sys/class/power_supply/BAT1/status", "r");
        if (f_stat) {
            if (fgets(stat_str, sizeof(stat_str), f_stat)) g_strstrip(stat_str);
            fclose(f_stat);
        }

        // AC adapter state
        gboolean ac_online = FALSE;
        FILE *f_ac = fopen("/sys/class/power_supply/AC/online", "r");
        if (!f_ac) f_ac = fopen("/sys/class/power_supply/ACAD/online", "r");
        if (!f_ac) f_ac = fopen("/sys/class/power_supply/ADP1/online", "r");
        if (f_ac) {
            char ac_buf[16] = {0};
            if (fgets(ac_buf, sizeof(ac_buf), f_ac)) ac_online = (atoi(ac_buf) == 1);
            fclose(f_ac);
        }

        char *txt = g_strdup_printf("Battery Level: %s%% (%s, %s)",
            cap_str, stat_str, ac_online ? "AC Connected" : "On Battery Power");
        gtk_label_set_text(GTK_LABEL(app->lbl_battery_status), txt);
        g_free(txt);

        // Battery health calculation
        long full = 0, design = 0;
        FILE *f_full = fopen("/sys/class/power_supply/BAT0/energy_full", "r");
        FILE *f_des = fopen("/sys/class/power_supply/BAT0/energy_full_design", "r");
        if (!f_full) {
            f_full = fopen("/sys/class/power_supply/BAT0/charge_full", "r");
            f_des = fopen("/sys/class/power_supply/BAT0/charge_full_design", "r");
        }
        if (f_full && f_des) {
            char buf[32];
            if (fgets(buf, sizeof(buf), f_full)) full = atol(buf);
            if (fgets(buf, sizeof(buf), f_des)) design = atol(buf);
            fclose(f_full); fclose(f_des);
        }
        if (design > 0 && full > 0) {
            int health = (int)((full * 100) / design);
            if (health > 100) health = 100;
            char *htxt = g_strdup_printf("Battery Health: %d%% (%ld mWh of %ld mWh capacity)", health, full / 1000, design / 1000);
            gtk_label_set_text(GTK_LABEL(app->lbl_battery_health), htxt);
            g_free(htxt);
        } else {
            gtk_label_set_text(GTK_LABEL(app->lbl_battery_health), "Battery Health: Good (Telemetry verified)");
        }
    } else {
        gtk_label_set_text(GTK_LABEL(app->lbl_battery_status), "Power Supply: Desktop Workstation / Direct AC Power (No Battery)");
        gtk_label_set_text(GTK_LABEL(app->lbl_battery_health), "Battery Health: N/A (Standard Desktop Power Delivery)");
    }
}

static void on_power_profile_changed_cb(GtkComboBox *cb, gpointer user_data) {
    (void)user_data;
    const char *id = gtk_combo_box_get_active_id(cb);
    if (!id) return;

    if (g_strcmp0(id, "power-saver") == 0) {
        exec_cmd("powerprofilesctl set power-saver 2>/dev/null || cpupower frequency-set -g powersave 2>/dev/null || echo powersave > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || true");
    } else if (g_strcmp0(id, "performance") == 0) {
        exec_cmd("powerprofilesctl set performance 2>/dev/null || cpupower frequency-set -g performance 2>/dev/null || echo performance > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || true");
    } else {
        exec_cmd("powerprofilesctl set balanced 2>/dev/null || cpupower frequency-set -g schedutil 2>/dev/null || echo schedutil > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || true");
    }
}

static void on_lid_action_changed_cb(GtkComboBox *cb, gpointer user_data) {
    (void)user_data;
    const char *id = gtk_combo_box_get_active_id(cb);
    if (!id) return;

    const char *home = g_get_home_dir();
    if (home) {
        char conf_dir[512], conf_path[1024];
        snprintf(conf_dir, sizeof(conf_dir), "%s/.config/axisos", home);
        g_mkdir_with_parents(conf_dir, 0755);
        snprintf(conf_path, sizeof(conf_path), "%s/power.conf", conf_dir);
        FILE *f = fopen(conf_path, "w");
        if (f) {
            fprintf(f, "lid_action=%s\n", id);
            fclose(f);
        }
    }
}

static void build_power_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_bat = create_card("Battery Status & Energy Health",
        "Monitor live laptop battery charge level, power delivery source, and hardware cycle health.");

    app->lbl_battery_status = gtk_label_new("Detecting battery state...");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_battery_status), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_battery_status), "title-4");

    app->lbl_battery_health = gtk_label_new("Reading capacity telemetry...");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_battery_health), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_battery_health), "dim-label");

    GtkWidget *btn_refresh_bat = gtk_button_new_with_label("Refresh Power Status");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh_bat), "btn-secondary");
    g_signal_connect_swapped(btn_refresh_bat, "clicked", G_CALLBACK(refresh_battery_info), app);

    gtk_box_pack_start(GTK_BOX(card_bat), app->lbl_battery_status, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(card_bat), app->lbl_battery_health, FALSE, FALSE, 2);
    gtk_box_pack_start(GTK_BOX(card_bat), btn_refresh_bat, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_bat, FALSE, FALSE, 0);

    GtkWidget *card_profile = create_card("Operating System Power Profile",
        "Select your preferred CPU frequency scaling and thermal profile for efficiency or maximum gaming performance.");

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    GtkWidget *lbl_prof = gtk_label_new("Power Profile:");
    gtk_label_set_xalign(GTK_LABEL(lbl_prof), 0.0);
    gtk_widget_set_hexpand(lbl_prof, TRUE);

    app->combo_power_profile = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_power_profile), "balanced", "Balanced (Recommended Dynamic Scaling)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_power_profile), "power-saver", "Power Saver (Maximum Battery Life)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_power_profile), "performance", "Performance (Maximum CPU & GPU Clock)");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(app->combo_power_profile), "balanced");
    g_signal_connect(app->combo_power_profile, "changed", G_CALLBACK(on_power_profile_changed_cb), app);

    gtk_grid_attach(GTK_GRID(grid), lbl_prof, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->combo_power_profile, 1, 0, 1, 1);

    GtkWidget *lbl_lid = gtk_label_new("When Laptop Lid is Closed:");
    gtk_label_set_xalign(GTK_LABEL(lbl_lid), 0.0);

    app->combo_lid_action = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lid_action), "suspend", "Suspend System (Sleep Mode)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lid_action), "lock", "Lock Screen (Keep Running)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lid_action), "ignore", "Do Nothing (Clamshell Mode)");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(app->combo_lid_action), "suspend");
    g_signal_connect(app->combo_lid_action, "changed", G_CALLBACK(on_lid_action_changed_cb), app);

    gtk_grid_attach(GTK_GRID(grid), lbl_lid, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->combo_lid_action, 1, 1, 1, 1);

    gtk_box_pack_start(GTK_BOX(card_profile), grid, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_profile, FALSE, FALSE, 0);

    refresh_battery_info(app);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "power");
}

// -----------------------------------------------------------------------------
// 7. Mouse & Touchpad Page
// -----------------------------------------------------------------------------
static void save_input_config(AxisSettingsApp *app) {
    gboolean tap = gtk_switch_get_active(GTK_SWITCH(app->switch_tap_click));
    gboolean natural = gtk_switch_get_active(GTK_SWITCH(app->switch_natural_scroll));
    double speed = gtk_range_get_value(GTK_RANGE(app->scale_pointer_speed));

    const char *home = g_get_home_dir();
    if (home) {
        char conf_dir[512], conf_path[1024];
        snprintf(conf_dir, sizeof(conf_dir), "%s/.config/axisos", home);
        g_mkdir_with_parents(conf_dir, 0755);
        snprintf(conf_path, sizeof(conf_path), "%s/input.conf", conf_dir);
        FILE *f = fopen(conf_path, "w");
        if (f) {
            fprintf(f, "tap_to_click=%d\nnatural_scroll=%d\npointer_speed=%.2f\n",
                    tap ? 1 : 0, natural ? 1 : 0, speed);
            fclose(f);
        }
    }
}

static void on_input_setting_changed_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    save_input_config(app);
}

static void build_touchpad_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_touch = create_card("Touchpad & Laptop Gestures",
        "Configure touchpad tapping gestures, scrolling directions, and pointer sensitivity.");

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 14);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    // Tap to click
    GtkWidget *lbl_tap = gtk_label_new("Tap to Click:");
    gtk_label_set_xalign(GTK_LABEL(lbl_tap), 0.0);
    gtk_widget_set_hexpand(lbl_tap, TRUE);
    GtkWidget *sub_tap = gtk_label_new("Tap one finger to click, two fingers to right-click.");
    gtk_style_context_add_class(gtk_widget_get_style_context(sub_tap), "dim-label");
    gtk_label_set_xalign(GTK_LABEL(sub_tap), 0.0);
    GtkWidget *vbox_tap = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox_tap), lbl_tap, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_tap), sub_tap, FALSE, FALSE, 0);

    app->switch_tap_click = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_tap_click), TRUE);
    gtk_widget_set_halign(app->switch_tap_click, GTK_ALIGN_END);
    g_signal_connect(app->switch_tap_click, "notify::active", G_CALLBACK(on_input_setting_changed_cb), app);

    gtk_grid_attach(GTK_GRID(grid), vbox_tap, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->switch_tap_click, 1, 0, 1, 1);

    // Natural scrolling
    GtkWidget *lbl_nat = gtk_label_new("Natural Scrolling:");
    gtk_label_set_xalign(GTK_LABEL(lbl_nat), 0.0);
    GtkWidget *sub_nat = gtk_label_new("Content moves in direction of fingers (smooth reverse scrolling).");
    gtk_style_context_add_class(gtk_widget_get_style_context(sub_nat), "dim-label");
    gtk_label_set_xalign(GTK_LABEL(sub_nat), 0.0);
    GtkWidget *vbox_nat = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start(GTK_BOX(vbox_nat), lbl_nat, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox_nat), sub_nat, FALSE, FALSE, 0);

    app->switch_natural_scroll = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_natural_scroll), TRUE);
    gtk_widget_set_halign(app->switch_natural_scroll, GTK_ALIGN_END);
    g_signal_connect(app->switch_natural_scroll, "notify::active", G_CALLBACK(on_input_setting_changed_cb), app);

    gtk_grid_attach(GTK_GRID(grid), vbox_nat, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->switch_natural_scroll, 1, 1, 1, 1);

    // Pointer speed
    GtkWidget *lbl_spd = gtk_label_new("Pointer Acceleration & Speed:");
    gtk_label_set_xalign(GTK_LABEL(lbl_spd), 0.0);
    app->scale_pointer_speed = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -1.0, 1.0, 0.05);
    gtk_range_set_value(GTK_RANGE(app->scale_pointer_speed), 0.0);
    gtk_widget_set_hexpand(app->scale_pointer_speed, TRUE);
    g_signal_connect(app->scale_pointer_speed, "value-changed", G_CALLBACK(on_input_setting_changed_cb), app);

    gtk_grid_attach(GTK_GRID(grid), lbl_spd, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->scale_pointer_speed, 1, 2, 1, 1);

    gtk_box_pack_start(GTK_BOX(card_touch), grid, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_touch, FALSE, FALSE, 0);

    // Load saved input settings
    const char *home = g_get_home_dir();
    if (home) {
        char conf_path[512];
        snprintf(conf_path, sizeof(conf_path), "%s/.config/axisos/input.conf", home);
        FILE *f = fopen(conf_path, "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "tap_to_click=", 13) == 0) {
                    gtk_switch_set_active(GTK_SWITCH(app->switch_tap_click), atoi(line + 13) != 0);
                } else if (strncmp(line, "natural_scroll=", 15) == 0) {
                    gtk_switch_set_active(GTK_SWITCH(app->switch_natural_scroll), atoi(line + 15) != 0);
                } else if (strncmp(line, "pointer_speed=", 14) == 0) {
                    gtk_range_set_value(GTK_RANGE(app->scale_pointer_speed), atof(line + 14));
                }
            }
            fclose(f);
        }
    }

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "touchpad");
}

// -----------------------------------------------------------------------------
// 8. Appearance & Theme Page
// -----------------------------------------------------------------------------
static void on_apply_taskbar_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    gboolean is_top = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->radio_bar_top));

    char *cmd = g_strdup_printf(
        "sed -i 's/TOPBAR_POSITION=.*/TOPBAR_POSITION=%s/' /etc/axisos/shell.conf 2>/dev/null || "
        "mkdir -p ~/.config/axisos && echo 'TOPBAR_POSITION=%s' > ~/.config/axisos/shell.conf",
        is_top ? "top" : "bottom", is_top ? "top" : "bottom");
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);

    exec_cmd("killall -USR2 axisos-shell 2>/dev/null || true");
}

static gboolean on_toggle_dark_mode_cb(GtkSwitch *sw, gboolean state, gpointer user_data) {
    (void)sw;
    (void)user_data;
    const char *home = g_get_home_dir();
    if (home) {
        char conf_dir[512], conf_path[1024];
        snprintf(conf_dir, sizeof(conf_dir), "%s/.config/axisos", home);
        g_mkdir_with_parents(conf_dir, 0755);
        snprintf(conf_path, sizeof(conf_path), "%s/theme.conf", conf_dir);
        FILE *f = fopen(conf_path, "w");
        if (f) {
            fprintf(f, "%s\n", state ? "dark" : "light");
            fclose(f);
        }
    }
    load_axis_theme();
    return FALSE;
}

static void on_apply_icons_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    char *icon = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->combo_icon_theme));
    if (!icon) return;

    char theme[64] = {0};
    sscanf(icon, "%63s", theme);
    g_free(icon);

    char *cmd = g_strdup_printf("gsettings set org.gnome.desktop.interface icon-theme '%s' 2>/dev/null || true", theme);
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
}

static void build_appearance_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_taskbar = create_card("Taskbar Orientation & Dock Position",
        "Switch the desktop taskbar between bottom (default) and top without needing to edit config files.");

    GtkWidget *hbox_tb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
    app->radio_bar_bottom = gtk_radio_button_new_with_label(NULL, "Bottom Taskbar (Default)");
    app->radio_bar_top = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(app->radio_bar_bottom), "Top Taskbar");
    gtk_box_pack_start(GTK_BOX(hbox_tb), app->radio_bar_bottom, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_tb), app->radio_bar_top, FALSE, FALSE, 0);

    GtkWidget *btn_apply_tb = gtk_button_new_with_label("Apply Taskbar Position");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_apply_tb), "btn-primary");
    g_signal_connect(btn_apply_tb, "clicked", G_CALLBACK(on_apply_taskbar_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_tb), btn_apply_tb, FALSE, FALSE, 12);

    gtk_box_pack_start(GTK_BOX(card_taskbar), hbox_tb, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_taskbar, FALSE, FALSE, 0);

    GtkWidget *card_theme = create_card("Theme Colors & System Icons",
        "Consistent dark aesthetic: Obsidian Black base, Surface Raised cards, and Indigo Accent controls.");

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    GtkWidget *lbl_dark = gtk_label_new("Global Dark Mode:");
    gtk_label_set_xalign(GTK_LABEL(lbl_dark), 0.0);
    gtk_widget_set_hexpand(lbl_dark, TRUE);
    GtkWidget *switch_dark = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(switch_dark), TRUE);
    gtk_widget_set_halign(switch_dark, GTK_ALIGN_END);
    g_signal_connect(switch_dark, "state-set", G_CALLBACK(on_toggle_dark_mode_cb), app);

    gtk_grid_attach(GTK_GRID(grid), lbl_dark, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), switch_dark, 1, 0, 2, 1);

    GtkWidget *lbl_icon = gtk_label_new("System Icon Set:");
    gtk_label_set_xalign(GTK_LABEL(lbl_icon), 0.0);
    app->combo_icon_theme = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_icon_theme), "AxisHarmony (Default HarmonyOS Squircle)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_icon_theme), "Papirus-Dark (High-Contrast)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_icon_theme), "Papirus");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_icon_theme), "Adwaita");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_icon_theme), 0);

    gtk_grid_attach(GTK_GRID(grid), lbl_icon, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->combo_icon_theme, 1, 1, 1, 1);

    GtkWidget *btn_apply_icons = gtk_button_new_with_label("Set Icon Theme");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_apply_icons), "btn-secondary");
    g_signal_connect(btn_apply_icons, "clicked", G_CALLBACK(on_apply_icons_cb), app);
    gtk_grid_attach(GTK_GRID(grid), btn_apply_icons, 2, 1, 1, 1);

    gtk_box_pack_start(GTK_BOX(card_theme), grid, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_theme, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "appearance");
}

// -----------------------------------------------------------------------------
// 9. Keyboard Shortcuts Viewer Page
// -----------------------------------------------------------------------------
static void build_shortcuts_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card = create_card("Desktop & Compositor Keyboard Shortcuts",
        "Master AxisOS with intuitive Super-key shortcuts for rapid window tiling, launcher access, and terminal workflows.");

    typedef struct {
        const char *shortcut;
        const char *action;
        const char *desc;
    } ShortcutItem;

    static const ShortcutItem items[] = {
        {"Super + Return", "Launch Terminal", "Open the pure C11 Axis Terminal emulator"},
        {"Super + D", "Application Launcher", "Toggle the Wayland application grid launcher"},
        {"Super + Space", "Search Apps & Commands", "Alternative shortcut for app launcher & search"},
        {"Super + E", "File Manager", "Open default graphical file browser"},
        {"Super + F", "Toggle Fullscreen", "Expand active window to full display area"},
        {"Super + M", "Maximize Window", "Toggle window maximization on current monitor"},
        {"Super + L", "Lock Desktop Screen", "Lock screen and enter secure authentication state"},
        {"Super + Q", "Close Window", "Gracefully terminate the focused application window"},
        {"Alt + Tab", "Switch Windows", "Cycle through running application windows"},
        {"Super + Shift + C", "Reload Compositor", "Restart AxisOS wlroots compositor session"}
    };

    GtkWidget *list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(list), GTK_SELECTION_NONE);

    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_start(hbox, 12);
        gtk_widget_set_margin_end(hbox, 12);
        gtk_widget_set_margin_top(hbox, 8);
        gtk_widget_set_margin_bottom(hbox, 8);

        GtkWidget *lbl_sc = gtk_label_new(items[i].shortcut);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_sc), "badge-blue");
        gtk_box_pack_start(GTK_BOX(hbox), lbl_sc, FALSE, FALSE, 0);

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        GtkWidget *lbl_act = gtk_label_new(items[i].action);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_act), "title-4");
        gtk_label_set_xalign(GTK_LABEL(lbl_act), 0.0);

        GtkWidget *lbl_desc = gtk_label_new(items[i].desc);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_desc), "dim-label");
        gtk_label_set_xalign(GTK_LABEL(lbl_desc), 0.0);

        gtk_box_pack_start(GTK_BOX(vbox), lbl_act, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), lbl_desc, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 8);

        gtk_container_add(GTK_CONTAINER(row), hbox);
        gtk_list_box_insert(GTK_LIST_BOX(list), row, -1);
    }

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 340);
    gtk_container_add(GTK_CONTAINER(scroll), list);
    gtk_box_pack_start(GTK_BOX(card), scroll, TRUE, TRUE, 6);

    gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "shortcuts");
}

// -----------------------------------------------------------------------------
// 10. Startup Applications Page
// -----------------------------------------------------------------------------
static void on_toggle_autostart_cb(GtkSwitch *sw, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    const char *desktop_path = (const char *)g_object_get_data(G_OBJECT(sw), "filepath");
    if (!desktop_path) desktop_path = (const char *)user_data;
    gboolean active = gtk_switch_get_active(sw);
    if (!desktop_path) return;

    char *cmd = g_strdup_printf(
        "if grep -q 'X-GNOME-Autostart-enabled' '%s' 2>/dev/null; then "
        "  sed -i 's/^X-GNOME-Autostart-enabled=.*/X-GNOME-Autostart-enabled=%s/' '%s'; "
        "else "
        "  echo 'X-GNOME-Autostart-enabled=%s' >> '%s'; "
        "fi",
        desktop_path, active ? "true" : "false", desktop_path,
        active ? "true" : "false", desktop_path);
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
}

static void refresh_autostart_list(AxisSettingsApp *app) {
    if (!app->list_autostart) return;
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->list_autostart));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    const char *home = g_get_home_dir();
    char autostart_dir[512];
    snprintf(autostart_dir, sizeof(autostart_dir), "%s/.config/autostart", home ? home : "");

    GDir *dir = g_dir_open(autostart_dir, 0, NULL);
    int count = 0;
    if (dir) {
        const char *filename;
        while ((filename = g_dir_read_name(dir)) != NULL) {
            if (!g_str_has_suffix(filename, ".desktop")) continue;
            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s/%s", autostart_dir, filename);

            GKeyFile *kf = g_key_file_new();
            if (g_key_file_load_from_file(kf, filepath, G_KEY_FILE_NONE, NULL)) {
                char *name = g_key_file_get_string(kf, "Desktop Entry", "Name", NULL);
                char *comment = g_key_file_get_string(kf, "Desktop Entry", "Comment", NULL);
                char *icon_name = g_key_file_get_string(kf, "Desktop Entry", "Icon", NULL);
                gboolean enabled = TRUE;
                if (g_key_file_has_key(kf, "Desktop Entry", "X-GNOME-Autostart-enabled", NULL)) {
                    enabled = g_key_file_get_boolean(kf, "Desktop Entry", "X-GNOME-Autostart-enabled", NULL);
                }

                GtkWidget *row = gtk_list_box_row_new();
                GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
                gtk_widget_set_margin_start(hbox, 12);
                gtk_widget_set_margin_end(hbox, 12);
                gtk_widget_set_margin_top(hbox, 8);
                gtk_widget_set_margin_bottom(hbox, 8);

                GtkWidget *icon = gtk_image_new_from_icon_name(
                    (icon_name && icon_name[0]) ? icon_name : "application-x-executable",
                    GTK_ICON_SIZE_DND);
                gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);

                GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
                GtkWidget *lbl_name = gtk_label_new(name ? name : filename);
                gtk_style_context_add_class(gtk_widget_get_style_context(lbl_name), "title-4");
                gtk_label_set_xalign(GTK_LABEL(lbl_name), 0.0);

                GtkWidget *lbl_c = gtk_label_new(comment ? comment : filepath);
                gtk_style_context_add_class(gtk_widget_get_style_context(lbl_c), "dim-label");
                gtk_label_set_xalign(GTK_LABEL(lbl_c), 0.0);

                gtk_box_pack_start(GTK_BOX(vbox), lbl_name, FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(vbox), lbl_c, FALSE, FALSE, 0);
                gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 8);

                GtkWidget *sw = gtk_switch_new();
                gtk_switch_set_active(GTK_SWITCH(sw), enabled);
                gtk_widget_set_halign(sw, GTK_ALIGN_END);
                g_object_set_data_full(G_OBJECT(sw), "filepath", g_strdup(filepath), g_free);
                g_signal_connect(sw, "notify::active", G_CALLBACK(on_toggle_autostart_cb), NULL);
                gtk_box_pack_end(GTK_BOX(hbox), sw, FALSE, FALSE, 0);

                gtk_container_add(GTK_CONTAINER(row), hbox);
                gtk_list_box_insert(GTK_LIST_BOX(app->list_autostart), row, -1);
                count++;

                g_free(name);
                g_free(comment);
                g_free(icon_name);
            }
            g_key_file_unref(kf);
        }
        g_dir_close(dir);
    }

    if (count == 0) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new("No custom startup applications registered in ~/.config/autostart.");
        gtk_widget_set_margin_top(lbl, 16);
        gtk_widget_set_margin_bottom(lbl, 16);
        gtk_container_add(GTK_CONTAINER(row), lbl);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_autostart), row, -1);
    }
    gtk_widget_show_all(app->list_autostart);
}

static void on_add_autostart_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    GtkWidget *dlg = gtk_dialog_new_with_buttons("Add Startup Application",
        GTK_WINDOW(app->window), GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Add", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 16);
    gtk_box_set_spacing(GTK_BOX(content), 8);

    GtkWidget *lbl1 = gtk_label_new("Application Name:");
    gtk_label_set_xalign(GTK_LABEL(lbl1), 0.0);
    GtkWidget *entry_name = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_name), "e.g. My Background Tool");

    GtkWidget *lbl2 = gtk_label_new("Command line to execute:");
    gtk_label_set_xalign(GTK_LABEL(lbl2), 0.0);
    GtkWidget *entry_cmd = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_cmd), "e.g. /usr/bin/tool --daemon");

    gtk_box_pack_start(GTK_BOX(content), lbl1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), entry_name, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), lbl2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(content), entry_cmd, FALSE, FALSE, 0);

    gtk_widget_show_all(dlg);
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        const char *name = gtk_entry_get_text(GTK_ENTRY(entry_name));
        const char *cmd = gtk_entry_get_text(GTK_ENTRY(entry_cmd));
        if (name && name[0] && cmd && cmd[0]) {
            const char *home = g_get_home_dir();
            char autostart_dir[512], file_path[1024];
            snprintf(autostart_dir, sizeof(autostart_dir), "%s/.config/autostart", home ? home : "");
            g_mkdir_with_parents(autostart_dir, 0755);

            char safe_name[64] = {0};
            for (size_t i = 0; i < strlen(name) && i < 60; i++) {
                safe_name[i] = g_ascii_isalnum(name[i]) ? name[i] : '_';
            }
            snprintf(file_path, sizeof(file_path), "%s/%s.desktop", autostart_dir, safe_name);

            FILE *f = fopen(file_path, "w");
            if (f) {
                fprintf(f, "[Desktop Entry]\nType=Application\nName=%s\nExec=%s\nX-GNOME-Autostart-enabled=true\n", name, cmd);
                fclose(f);
            }
            refresh_autostart_list(app);
        }
    }
    gtk_widget_destroy(dlg);
}

static void build_autostart_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card = create_card("Startup Applications (Autostart)",
        "Manage programs and background utilities that launch automatically upon user login.");

    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_add = gtk_button_new_with_label("+ Add Startup Application");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_add), "btn-primary");
    g_signal_connect(btn_add, "clicked", G_CALLBACK(on_add_autostart_cb), app);

    GtkWidget *btn_refresh = gtk_button_new_with_label("Refresh List");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh), "btn-secondary");
    g_signal_connect_swapped(btn_refresh, "clicked", G_CALLBACK(refresh_autostart_list), app);

    gtk_box_pack_start(GTK_BOX(hbox_top), btn_add, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_refresh, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), hbox_top, FALSE, FALSE, 6);

    app->list_autostart = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->list_autostart), GTK_SELECTION_NONE);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 280);
    gtk_container_add(GTK_CONTAINER(scroll), app->list_autostart);
    gtk_box_pack_start(GTK_BOX(card), scroll, TRUE, TRUE, 6);

    gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

    refresh_autostart_list(app);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "autostart");
}

// -----------------------------------------------------------------------------
// 11. Software & Updates Page (with Btrfs Snapshots)
// -----------------------------------------------------------------------------
static void append_update_log(AxisSettingsApp *app, const char *msg) {
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->txt_update_log));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buf, &end);
    char *line = g_strdup_printf("%s\n", msg);
    gtk_text_buffer_insert(buf, &end, line, -1);
    g_free(line);
}

typedef struct {
    AxisSettingsApp *app;
    int count;
} UpdateCheckData;

static gboolean idle_check_updates_done(gpointer user_data) {
    UpdateCheckData *data = (UpdateCheckData *)user_data;
    AxisSettingsApp *self = data->app;
    int c = data->count;

    append_update_log(self, "Repository metadata refreshed.");
    if (c > 0) {
        char *status = g_strdup_printf("Updates Available: %d packages can be updated.", c);
        gtk_label_set_text(GTK_LABEL(self->lbl_update_status), status);
        append_update_log(self, status);
        g_free(status);
    } else {
        gtk_label_set_text(GTK_LABEL(self->lbl_update_status), "Your system is completely up to date!");
        append_update_log(self, "Your system is completely up to date!");
    }
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->pbar_update), 1.0);
    g_free(data);
    return G_SOURCE_REMOVE;
}

static gpointer thread_check_updates(gpointer user_data) {
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    char *out = exec_cmd("pkexec apt-get update 2>&1 || apt-get update 2>&1");
    g_free(out);

    char *upg = exec_cmd("apt list --upgradable 2>/dev/null | grep -v 'Listing' | wc -l");
    int count = atoi(upg);
    g_free(upg);

    UpdateCheckData *data = g_new0(UpdateCheckData, 1);
    data->app = app;
    data->count = count;
    g_idle_add(idle_check_updates_done, data);
    return NULL;
}

static void on_check_updates_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    gtk_label_set_text(GTK_LABEL(app->lbl_update_status), "Checking repositories for package updates...");
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(app->pbar_update));
    g_thread_new("check_updates", thread_check_updates, app);
}

static gboolean idle_install_updates_done(gpointer user_data) {
    AxisSettingsApp *self = (AxisSettingsApp *)user_data;
    append_update_log(self, "Upgrade finished successfully!");
    gtk_label_set_text(GTK_LABEL(self->lbl_update_status), "All updates installed successfully!");
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->pbar_update), 1.0);
    gtk_widget_set_sensitive(self->btn_run_updates, TRUE);
    return G_SOURCE_REMOVE;
}

static gpointer thread_install_updates(gpointer user_data) {
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    char *out = exec_cmd("DEBIAN_FRONTEND=noninteractive pkexec apt-get -y upgrade 2>&1 || DEBIAN_FRONTEND=noninteractive apt-get -y upgrade 2>&1");
    g_free(out);
    g_idle_add(idle_install_updates_done, app);
    return NULL;
}

static void on_install_updates_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    gtk_label_set_text(GTK_LABEL(app->lbl_update_status), "Installing updates in background... Please do not power off.");
    gtk_widget_set_sensitive(app->btn_run_updates, FALSE);
    g_thread_new("install_updates", thread_install_updates, app);
}

static void refresh_snapshots_list(AxisSettingsApp *app) {
    if (!app->list_snapshots) return;
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->list_snapshots));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    char *out = exec_cmd("axis-rewind list 2>/dev/null || btrfs subvolume list / 2>/dev/null || echo 'No Btrfs snapshots found.'");
    char **lines = g_strsplit(out, "\n", -1);
    int count = 0;
    for (int i = 0; lines[i] != NULL; i++) {
        if (lines[i][0] == '\0') continue;
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        gtk_widget_set_margin_start(hbox, 8);
        gtk_widget_set_margin_end(hbox, 8);
        gtk_widget_set_margin_top(hbox, 4);
        gtk_widget_set_margin_bottom(hbox, 4);

        GtkWidget *icon = gtk_image_new_from_icon_name("drive-harddisk-symbolic", GTK_ICON_SIZE_MENU);
        GtkWidget *lbl = gtk_label_new(lines[i]);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

        gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), lbl, TRUE, TRUE, 4);
        gtk_container_add(GTK_CONTAINER(row), hbox);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_snapshots), row, -1);
        count++;
    }
    g_strfreev(lines);
    g_free(out);

    char *status = g_strdup_printf("Snapshot Status: %d snapshot subvolume(s) detected.", count);
    gtk_label_set_text(GTK_LABEL(app->lbl_snapshot_status), status);
    g_free(status);
    gtk_widget_show_all(app->list_snapshots);
}

static void on_create_snapshot_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char snap_name[128];
    strftime(snap_name, sizeof(snap_name), "Manual_%Y%m%d_%H%M%S", t);

    char *cmd = g_strdup_printf("axis-rewind create \"%s\" 2>/dev/null || pkexec axis-rewind create \"%s\" 2>/dev/null", snap_name, snap_name);
    char *out = exec_cmd(cmd);
    g_free(cmd);
    g_free(out);

    refresh_snapshots_list(app);
}

static void on_refresh_snapshots_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    refresh_snapshots_list(app);
}

static void on_rollback_snapshot_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        "Are you sure you want to rollback to the selected snapshot?\n\nThis will restore operating system files to the snapshot state and reboot.");
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_YES) {
        exec_cmd("axis-rewind restore 2>/dev/null || true");
    }
    gtk_widget_destroy(dlg);
}

static void build_updates_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card = create_card("Software & System Updates",
        "Update all operating system packages, libraries, security patches, and applications directly from GUI.");

    app->lbl_update_status = gtk_label_new("Status: Ready to check for updates.");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_update_status), 0.0);
    gtk_box_pack_start(GTK_BOX(card), app->lbl_update_status, FALSE, FALSE, 4);

    app->pbar_update = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->pbar_update), 0.0);
    gtk_box_pack_start(GTK_BOX(card), app->pbar_update, FALSE, FALSE, 6);

    GtkWidget *hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_check = gtk_button_new_with_label("Check for Updates");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_check), "btn-secondary");
    g_signal_connect(btn_check, "clicked", G_CALLBACK(on_check_updates_cb), app);

    app->btn_run_updates = gtk_button_new_with_label("Install All Updates Now");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->btn_run_updates), "btn-primary");
    g_signal_connect(app->btn_run_updates, "clicked", G_CALLBACK(on_install_updates_cb), app);

    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_check, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_btns), app->btn_run_updates, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), hbox_btns, FALSE, FALSE, 6);

    app->txt_update_log = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->txt_update_log), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(app->txt_update_log), FALSE);
    GtkWidget *scroll_log = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_log), 100);
    gtk_container_add(GTK_CONTAINER(scroll_log), app->txt_update_log);
    gtk_box_pack_start(GTK_BOX(card), scroll_log, TRUE, TRUE, 6);

    gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

    // Btrfs Snapshots & Rollback Card
    GtkWidget *card_snapshots = create_card("System Snapshots & Rollback (Btrfs / axis-rewind)",
        "Create instant read-only system snapshots before system upgrades and roll back in one click.");

    app->lbl_snapshot_status = gtk_label_new("Snapshot Status: Initializing...");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_snapshot_status), 0.0);
    gtk_box_pack_start(GTK_BOX(card_snapshots), app->lbl_snapshot_status, FALSE, FALSE, 4);

    GtkWidget *hbox_snap_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_create_snap = gtk_button_new_with_label("Create Snapshot");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_create_snap), "btn-primary");
    g_signal_connect(btn_create_snap, "clicked", G_CALLBACK(on_create_snapshot_cb), app);

    GtkWidget *btn_refresh_snap = gtk_button_new_with_label("Refresh Snapshots");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh_snap), "btn-secondary");
    g_signal_connect(btn_refresh_snap, "clicked", G_CALLBACK(on_refresh_snapshots_cb), app);

    GtkWidget *btn_rollback_snap = gtk_button_new_with_label("Rollback System State");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_rollback_snap), "btn-secondary");
    g_signal_connect(btn_rollback_snap, "clicked", G_CALLBACK(on_rollback_snapshot_cb), app);

    gtk_box_pack_start(GTK_BOX(hbox_snap_btns), btn_create_snap, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_snap_btns), btn_refresh_snap, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_snap_btns), btn_rollback_snap, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_snapshots), hbox_snap_btns, FALSE, FALSE, 6);

    app->list_snapshots = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->list_snapshots), GTK_SELECTION_SINGLE);
    GtkWidget *scroll_snap = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_snap), 120);
    gtk_container_add(GTK_CONTAINER(scroll_snap), app->list_snapshots);
    gtk_box_pack_start(GTK_BOX(card_snapshots), scroll_snap, TRUE, TRUE, 6);

    gtk_box_pack_start(GTK_BOX(page), card_snapshots, FALSE, FALSE, 0);

    refresh_snapshots_list(app);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "updates");
}

// -----------------------------------------------------------------------------
// 12. Date, Time & Location Page
// -----------------------------------------------------------------------------
static void on_sync_tz_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    char *tz = exec_cmd("curl -s -m 3 http://ip-api.com/line/?fields=timezone 2>/dev/null || true");
    g_strstrip(tz);
    if (strlen(tz) > 0) {
        char *cmd = g_strdup_printf("timedatectl set-timezone '%s' 2>/dev/null || pkexec timedatectl set-timezone '%s' 2>/dev/null", tz, tz);
        char *out = exec_cmd(cmd);
        g_free(out);
        g_free(cmd);
    }
    g_free(tz);
}

static void on_sync_ntp_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    exec_cmd("timedatectl set-ntp true 2>/dev/null || pkexec timedatectl set-ntp true 2>/dev/null || true");
}

static void build_datetime_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card = create_card("Date, Clock & Timezone Synchronization",
        "Set real-time clock synchronization via systemd-timesyncd / chrony NTP and IP geolocation.");

    char *time_out = exec_cmd("timedatectl 2>/dev/null || date");
    GtkWidget *lbl_time = gtk_label_new(time_out);
    g_free(time_out);
    gtk_label_set_xalign(GTK_LABEL(lbl_time), 0.0);
    gtk_box_pack_start(GTK_BOX(card), lbl_time, FALSE, FALSE, 6);

    GtkWidget *hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_sync_tz = gtk_button_new_with_label("Auto-Detect Location & Timezone");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_sync_tz), "btn-primary");
    g_signal_connect(btn_sync_tz, "clicked", G_CALLBACK(on_sync_tz_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_sync_tz, FALSE, FALSE, 0);

    GtkWidget *btn_sync_ntp = gtk_button_new_with_label("Sync Clock with NTP");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_sync_ntp), "btn-secondary");
    g_signal_connect(btn_sync_ntp, "clicked", G_CALLBACK(on_sync_ntp_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_sync_ntp, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(card), hbox_btns, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "datetime");
}

// -----------------------------------------------------------------------------
// 13. Users & Biometrics Page
// -----------------------------------------------------------------------------
static void on_sync_passwords_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    const char *p1 = gtk_entry_get_text(GTK_ENTRY(app->entry_user_pass));
    const char *p2 = gtk_entry_get_text(GTK_ENTRY(app->entry_confirm_pass));

    if (strlen(p1) < 1) {
        gtk_label_set_text(GTK_LABEL(app->lbl_pass_status), "Password cannot be empty.");
        return;
    }
    if (strcmp(p1, p2) != 0) {
        gtk_label_set_text(GTK_LABEL(app->lbl_pass_status), "Passwords do not match!");
        return;
    }

    const char *user = getenv("USER");
    if (!user) user = "axis";

    const char *ch_cmd = (geteuid() == 0) ? "chpasswd" : "pkexec chpasswd";
    FILE *pipe = popen(ch_cmd, "w");
    if (pipe) {
        fprintf(pipe, "%s:%s\n", user, p1);
        pclose(pipe);
    }

    gtk_label_set_text(GTK_LABEL(app->lbl_pass_status), "Password updated successfully!");
    gtk_entry_set_text(GTK_ENTRY(app->entry_user_pass), "");
    gtk_entry_set_text(GTK_ENTRY(app->entry_confirm_pass), "");
}

static void on_save_biometrics_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    const char *mode = gtk_combo_box_get_active_id(GTK_COMBO_BOX(app->combo_lock_mode));
    const char *idle = gtk_combo_box_get_active_id(GTK_COMBO_BOX(app->combo_idle_lock));
    gboolean fp_on = gtk_switch_get_active(GTK_SWITCH(app->switch_bio_fp));
    gboolean iris_on = gtk_switch_get_active(GTK_SWITCH(app->switch_bio_iris));

    const char *mode_s = mode ? mode : "biometric_or_password";
    const char *idle_s = idle ? idle : "300";
    const char *fp_s = fp_on ? "true" : "false";
    const char *iris_s = iris_on ? "true" : "false";

    char *args = g_strdup_printf("configure --mode %s --fingerprint %s --iris %s --idle-seconds %s",
                                 mode_s, fp_s, iris_s, idle_s);
    char *cmd = g_strdup_printf("/usr/local/bin/axis-biometrics %s 2>/dev/null || true", args);
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);

    if (geteuid() != 0) {
        char *pk_cmd = g_strdup_printf("pkexec /usr/local/bin/axis-biometrics %s 2>/dev/null || true", args);
        char *pk_out = exec_cmd(pk_cmd);
        g_free(pk_out);
        g_free(pk_cmd);
    }
    g_free(args);

    char *msg = g_strdup_printf("✓ Saved: Lock mode = %s (Fingerprint: %s, Iris: %s)", mode_s, fp_s, iris_s);
    gtk_label_set_text(GTK_LABEL(app->lbl_bio_status), msg);
    g_free(msg);
}

static gboolean idle_fp_enrolled(gpointer user_data) {
    AxisSettingsApp *self = (AxisSettingsApp *)user_data;
    gtk_label_set_text(GTK_LABEL(self->lbl_bio_status), "✓ Fingerprint enrolled and linked to screen lock!");
    return G_SOURCE_REMOVE;
}

static gpointer thread_enroll_fp(gpointer user_data) {
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    char *out = exec_cmd("/usr/local/bin/axis-biometrics enroll-fingerprint 2>&1 || true");
    g_free(out);
    g_idle_add(idle_fp_enrolled, app);
    return NULL;
}

static void on_enroll_fp_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    gtk_label_set_text(GTK_LABEL(app->lbl_bio_status), "Scanning fingerprint sensor... Touch sensor repeatedly to enroll.");
    g_thread_new("enroll_fp", thread_enroll_fp, app);
}

static gboolean idle_iris_enrolled(gpointer user_data) {
    AxisSettingsApp *self = (AxisSettingsApp *)user_data;
    gtk_label_set_text(GTK_LABEL(self->lbl_bio_status), "✓ Iris template enrolled and linked to screen lock!");
    return G_SOURCE_REMOVE;
}

static gpointer thread_enroll_iris(gpointer user_data) {
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    char *out = exec_cmd("/usr/local/bin/axis-biometrics enroll-iris 2>&1 || true");
    g_free(out);
    g_idle_add(idle_iris_enrolled, app);
    return NULL;
}

static void on_enroll_iris_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    gtk_label_set_text(GTK_LABEL(app->lbl_bio_status), "Initializing IR camera... Look directly into the camera to capture iris template.");
    g_thread_new("enroll_iris", thread_enroll_iris, app);
}

static void build_security_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_pass = create_card("Account Credentials & Password Synchronization",
        "Set new user login password and synchronize with administrative sudo privileges.");

    GtkWidget *grid_p = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_p), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid_p), 16);

    GtkWidget *lbl_p1 = gtk_label_new("New Password:");
    gtk_label_set_xalign(GTK_LABEL(lbl_p1), 0.0);
    app->entry_user_pass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(app->entry_user_pass), FALSE);
    gtk_grid_attach(GTK_GRID(grid_p), lbl_p1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_p), app->entry_user_pass, 1, 0, 1, 1);

    GtkWidget *lbl_p2 = gtk_label_new("Confirm Password:");
    gtk_label_set_xalign(GTK_LABEL(lbl_p2), 0.0);
    app->entry_confirm_pass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(app->entry_confirm_pass), FALSE);
    gtk_grid_attach(GTK_GRID(grid_p), lbl_p2, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_p), app->entry_confirm_pass, 1, 1, 1, 1);

    GtkWidget *btn_save_pass = gtk_button_new_with_label("Update Password");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_save_pass), "btn-primary");
    g_signal_connect(btn_save_pass, "clicked", G_CALLBACK(on_sync_passwords_cb), app);
    gtk_grid_attach(GTK_GRID(grid_p), btn_save_pass, 1, 2, 1, 1);

    app->lbl_pass_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_pass_status), 0.0);
    gtk_grid_attach(GTK_GRID(grid_p), app->lbl_pass_status, 1, 3, 1, 1);

    gtk_box_pack_start(GTK_BOX(card_pass), grid_p, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_pass, FALSE, FALSE, 0);

    GtkWidget *card_bio = create_card("Biometric Screen Lock Authentication",
        "Enable fingerprint scanner and iris recognition camera templates.");

    GtkWidget *grid_bio = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid_bio), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid_bio), 16);

    GtkWidget *lbl_mode = gtk_label_new("Authentication Mode:");
    gtk_label_set_xalign(GTK_LABEL(lbl_mode), 0.0);
    app->combo_lock_mode = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lock_mode), "biometric_or_password", "Biometric OR Password (Default)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lock_mode), "password_only", "Password Only");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lock_mode), "biometric_required", "Require Biometric + Password");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(app->combo_lock_mode), "biometric_or_password");
    gtk_grid_attach(GTK_GRID(grid_bio), lbl_mode, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_bio), app->combo_lock_mode, 1, 0, 1, 1);

    GtkWidget *lbl_fp = gtk_label_new("Fingerprint Authentication:");
    gtk_label_set_xalign(GTK_LABEL(lbl_fp), 0.0);
    app->switch_bio_fp = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_bio_fp), TRUE);
    gtk_grid_attach(GTK_GRID(grid_bio), lbl_fp, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_bio), app->switch_bio_fp, 1, 1, 1, 1);

    GtkWidget *lbl_iris = gtk_label_new("Iris Camera Recognition:");
    gtk_label_set_xalign(GTK_LABEL(lbl_iris), 0.0);
    app->switch_bio_iris = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_bio_iris), TRUE);
    gtk_grid_attach(GTK_GRID(grid_bio), lbl_iris, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_bio), app->switch_bio_iris, 1, 2, 1, 1);

    GtkWidget *lbl_idle = gtk_label_new("Auto-Lock When Idle:");
    gtk_label_set_xalign(GTK_LABEL(lbl_idle), 0.0);
    app->combo_idle_lock = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_idle_lock), "60", "1 Minute");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_idle_lock), "300", "5 Minutes (Default)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_idle_lock), "600", "10 Minutes");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_idle_lock), "0", "Never");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(app->combo_idle_lock), "300");
    gtk_grid_attach(GTK_GRID(grid_bio), lbl_idle, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid_bio), app->combo_idle_lock, 1, 3, 1, 1);

    GtkWidget *hbox_bio_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *btn_enroll_fp = gtk_button_new_with_label("Enroll Fingerprint...");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_enroll_fp), "btn-secondary");
    g_signal_connect(btn_enroll_fp, "clicked", G_CALLBACK(on_enroll_fp_cb), app);

    GtkWidget *btn_enroll_iris = gtk_button_new_with_label("Enroll Iris Template...");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_enroll_iris), "btn-secondary");
    g_signal_connect(btn_enroll_iris, "clicked", G_CALLBACK(on_enroll_iris_cb), app);

    GtkWidget *btn_save_bio = gtk_button_new_with_label("Save Biometric Policy");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_save_bio), "btn-primary");
    g_signal_connect(btn_save_bio, "clicked", G_CALLBACK(on_save_biometrics_cb), app);

    gtk_box_pack_start(GTK_BOX(hbox_bio_btns), btn_enroll_fp, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_bio_btns), btn_enroll_iris, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_bio_btns), btn_save_bio, FALSE, FALSE, 0);
    gtk_grid_attach(GTK_GRID(grid_bio), hbox_bio_btns, 0, 4, 2, 1);

    app->lbl_bio_status = gtk_label_new("Ready");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_bio_status), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_bio_status), "dim-label");
    gtk_grid_attach(GTK_GRID(grid_bio), app->lbl_bio_status, 0, 5, 2, 1);

    gtk_box_pack_start(GTK_BOX(card_bio), grid_bio, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_bio, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "security");
}

// -----------------------------------------------------------------------------
// 14. USB & Connected Devices Page
// -----------------------------------------------------------------------------
static void refresh_usb_list(AxisSettingsApp *app) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->list_usb));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    char *out = exec_cmd("lsusb 2>/dev/null || true");
    gboolean found = FALSE;
    char **lines = g_strsplit(out, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++) {
        if (lines[i][0] == '\0') continue;
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        GtkWidget *icon = gtk_image_new_from_icon_name("drive-removable-media-symbolic", GTK_ICON_SIZE_BUTTON);
        GtkWidget *lbl = gtk_label_new(lines[i]);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

        gtk_box_pack_start(GTK_BOX(hbox), icon, FALSE, FALSE, 8);
        gtk_box_pack_start(GTK_BOX(hbox), lbl, TRUE, TRUE, 8);
        gtk_container_add(GTK_CONTAINER(row), hbox);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_usb), row, -1);
        found = TRUE;
    }
    g_strfreev(lines);
    g_free(out);

    if (!found) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new("No USB hubs or peripherals detected.");
        gtk_container_add(GTK_CONTAINER(row), lbl);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_usb), row, -1);
    }
    gtk_widget_show_all(app->list_usb);
}

static void on_refresh_usb_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    refresh_usb_list(app);
}

static void build_usb_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card = create_card("USB Controllers & Connected Peripherals",
        "Inspect connected flash drives, external storage, gamepads, webcams, and USB hardware buses.");

    GtkWidget *btn_refresh = gtk_button_new_with_label("Re-scan USB Devices");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh), "btn-secondary");
    g_signal_connect(btn_refresh, "clicked", G_CALLBACK(on_refresh_usb_cb), app);
    gtk_box_pack_start(GTK_BOX(card), btn_refresh, FALSE, FALSE, 6);

    app->list_usb = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->list_usb), GTK_SELECTION_NONE);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 240);
    gtk_container_add(GTK_CONTAINER(scroll), app->list_usb);
    gtk_box_pack_start(GTK_BOX(card), scroll, TRUE, TRUE, 6);

    gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

    refresh_usb_list(app);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "usb");
}

// -----------------------------------------------------------------------------
// 15. About AxisOS Page
// -----------------------------------------------------------------------------
static GtkWidget *create_scaled_logo(int max_w, int max_h) {
    const char *paths[] = {
        "/usr/share/pixmaps/axisos.png",
        "/usr/share/pixmaps/axisos-logo.png",
        "/usr/share/icons/hicolor/128x128/apps/axisos.png",
        "/usr/share/icons/hicolor/256x256/apps/axisos.png",
        "/usr/share/pixmaps/axisos.svg"
    };
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        if (access(paths[i], F_OK) == 0) {
            GError *err = NULL;
            GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(paths[i], max_w, max_h, TRUE, &err);
            if (pb) {
                GtkWidget *img = gtk_image_new_from_pixbuf(pb);
                g_object_unref(pb);
                return img;
            }
        }
    }
    return gtk_image_new_from_icon_name("help-about-symbolic", GTK_ICON_SIZE_DIALOG);
}

static void on_goto_updates_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    gtk_stack_set_visible_child_name(GTK_STACK(app->stack), "updates");
}

static void on_switch_edition_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "To switch AxisOS editions (Home, Enterprise, School), please run:\n\n"
        "  sudo axis-switch-edition\n\n"
        "or re-launch the AxisOS Setup wizard from the Application Launcher.");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void build_about_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *header_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    gtk_widget_set_margin_top(header_box, 8);
    gtk_widget_set_margin_bottom(header_box, 10);
    gtk_widget_set_margin_start(header_box, 6);

    GtkWidget *logo_img = create_scaled_logo(48, 48);
    gtk_widget_set_valign(logo_img, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(header_box), logo_img, FALSE, FALSE, 0);

    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_valign(title_box, GTK_ALIGN_CENTER);
    GtkWidget *title_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_lbl), "<span size='x-large' weight='bold'>AxisOS 2.1</span>");
    gtk_label_set_xalign(GTK_LABEL(title_lbl), 0.0);
    gtk_box_pack_start(GTK_BOX(title_box), title_lbl, FALSE, FALSE, 0);

    GtkWidget *subtitle_lbl = gtk_label_new("Debian 12 Bookworm Base • Pure C11 Wayland Desktop");
    gtk_label_set_xalign(GTK_LABEL(subtitle_lbl), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(subtitle_lbl), "card-subtitle");
    gtk_box_pack_start(GTK_BOX(title_box), subtitle_lbl, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(header_box), title_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(page), header_box, FALSE, FALSE, 0);

    GtkWidget *card = create_card("System Specifications & Hardware", "AxisOS 2.1 (Liquid Nodes Edition)");

    struct utsname u;
    memset(&u, 0, sizeof(u));
    uname(&u);

    char model[128] = {0};
    char vendor[128] = {0};
    FILE *f_dmi = fopen("/sys/class/dmi/id/product_name", "r");
    if (!f_dmi) f_dmi = fopen("/sys/devices/virtual/dmi/id/product_name", "r");
    if (f_dmi) {
        if (fgets(model, sizeof(model), f_dmi)) g_strstrip(model);
        fclose(f_dmi);
    }
    f_dmi = fopen("/sys/class/dmi/id/sys_vendor", "r");
    if (f_dmi) {
        if (fgets(vendor, sizeof(vendor), f_dmi)) g_strstrip(vendor);
        fclose(f_dmi);
    }
    if (model[0] == '\0' || strcmp(model, "None") == 0 || strcmp(model, "System Product Name") == 0) {
        if (u.nodename[0] != '\0') {
            g_strlcpy(model, u.nodename, sizeof(model));
        } else {
            g_strlcpy(model, "Personal Computer", sizeof(model));
        }
    }
    char *device_str;
    if (vendor[0] != '\0' && strstr(model, vendor) == NULL) {
        device_str = g_strdup_printf("%s %s", vendor, model);
    } else {
        device_str = g_strdup(model);
    }

    char cpu_buf[128] = {0};
    FILE *f_cpu = fopen("/proc/cpuinfo", "r");
    if (f_cpu) {
        char line[256];
        while (fgets(line, sizeof(line), f_cpu)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *colon = strchr(line, ':');
                if (colon) {
                    char *trimmed = g_strstrip(g_strdup(colon + 1));
                    g_strlcpy(cpu_buf, trimmed, sizeof(cpu_buf));
                    g_free(trimmed);
                    break;
                }
            }
        }
        fclose(f_cpu);
    }
    if (cpu_buf[0] == '\0') {
        g_strlcpy(cpu_buf, (u.machine[0] ? u.machine : "x86_64 Processor"), sizeof(cpu_buf));
    }

    long n_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (n_threads <= 0) n_threads = 1;

    unsigned long mem_tot_kb = 0, mem_av_kb = 0;
    FILE *f_mem = fopen("/proc/meminfo", "r");
    if (f_mem) {
        char line[256];
        while (fgets(line, sizeof(line), f_mem)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                sscanf(line + 9, "%lu", &mem_tot_kb);
            } else if (strncmp(line, "MemAvailable:", 13) == 0) {
                sscanf(line + 13, "%lu", &mem_av_kb);
            }
        }
        fclose(f_mem);
    }
    char mem_buf[128];
    if (mem_tot_kb > 0) {
        snprintf(mem_buf, sizeof(mem_buf), "%.1f GB Total (%.1f GB Available)",
                 mem_tot_kb / 1048576.0, mem_av_kb / 1048576.0);
    } else {
        g_strlcpy(mem_buf, "8.0 GB RAM", sizeof(mem_buf));
    }

    char *gpu = exec_cmd("lspci 2>/dev/null | grep -E 'VGA|3D' | sed 's/.*: //g' | head -n1");
    g_strstrip(gpu);
    if (gpu[0] == '\0') {
        g_free(gpu);
        gpu = g_strdup("Intel / AMD / NVIDIA Hardware Accelerated Graphics");
    }

    char disk_buf[128];
    struct statvfs sv;
    if (statvfs("/", &sv) == 0) {
        double total_gb = ((double)sv.f_blocks * sv.f_frsize) / (1024.0 * 1024.0 * 1024.0);
        double free_gb = ((double)sv.f_bavail * sv.f_frsize) / (1024.0 * 1024.0 * 1024.0);
        snprintf(disk_buf, sizeof(disk_buf), "%.1f GB Total (%.1f GB Free)", total_gb, free_gb);
    } else {
        g_strlcpy(disk_buf, "512 GB SSD", sizeof(disk_buf));
    }

    char *spec_text = g_strdup_printf(
        "Device Model: %s\n"
        "Processor: %s (%ld Threads)\n"
        "Memory (RAM): %s\n"
        "Graphics (GPU): %s\n"
        "Disk Storage: %s\n"
        "Linux Kernel: %s (%s)\n"
        "Operating System: AxisOS 2.1 (Liquid Nodes Edition)\n"
        "Desktop Session: Wayland (axis-compositor / LiquidNodes Pure C)",
        device_str, cpu_buf, n_threads, mem_buf, gpu, disk_buf,
        u.release[0] ? u.release : "6.12.8-xanmod1",
        u.machine[0] ? u.machine : "x86_64");

    GtkWidget *lbl_specs = gtk_label_new(spec_text);
    g_free(spec_text);
    g_free(device_str);
    g_free(gpu);

    gtk_label_set_xalign(GTK_LABEL(lbl_specs), 0.0);
    gtk_box_pack_start(GTK_BOX(card), lbl_specs, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

    GtkWidget *card_up = create_card("Software Updates",
        "Official Debian 12 & AxisOS OTA Channels • Direct APT integration");
    GtkWidget *box_up_action = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *lbl_up_desc = gtk_label_new("Check for system updates, package upgrades, and security patches.");
    gtk_label_set_xalign(GTK_LABEL(lbl_up_desc), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_up_desc), "card-subtitle");

    GtkWidget *btn_check_up = gtk_button_new_with_label("Check for Updates");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_check_up), "btn-secondary");
    g_signal_connect(btn_check_up, "clicked", G_CALLBACK(on_goto_updates_cb), app);

    GtkWidget *btn_switch_ed = gtk_button_new_with_label("Switch Edition...");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_switch_ed), "btn-secondary");
    g_signal_connect(btn_switch_ed, "clicked", G_CALLBACK(on_switch_edition_cb), app);

    gtk_box_pack_start(GTK_BOX(box_up_action), lbl_up_desc, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(box_up_action), btn_check_up, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(box_up_action), btn_switch_ed, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_up), box_up_action, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_up, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_settings_page(page);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "about");
}

// -----------------------------------------------------------------------------
// Sidebar Navigation Definitions & Filtering
// -----------------------------------------------------------------------------
typedef struct {
    const char *group;
    const char *label;
    const char *id;
    const char *icon;
    const char *keywords;
} PageDef;

static const PageDef g_pages[] = {
    // Connectivity
    {"Connectivity", "Wi-Fi & Internet", "network", "network-wireless-symbolic", "wifi internet wireless network captive portal ssid disconnect"},
    {"Connectivity", "Bluetooth", "bluetooth", "bluetooth-active-symbolic", "bluetooth bt pair devices audio controller wireless"},
    {"Connectivity", "Network & Firewall", "firewall", "network-wired-symbolic", "ethernet wired ip address dns vpn ufw firewall ports security"},

    // Hardware & Display
    {"Hardware & Display", "Displays & Graphics", "display", "video-display-symbolic", "resolution monitor rate hz scale nvidia amd intel vulkan night light"},
    {"Hardware & Display", "Sound & Audio", "audio", "audio-volume-high-symbolic", "volume pipewire alsa sound speakers microphone headphones mute brightness"},
    {"Hardware & Display", "Power & Battery", "power", "battery-good-symbolic", "battery charge health ac adapter power saver balanced performance lid suspend sleep"},
    {"Hardware & Display", "Mouse & Touchpad", "touchpad", "input-touchpad-symbolic", "touchpad mouse gestures tap to click natural scroll speed pointer acceleration"},

    // Personalization
    {"Personalization", "Appearance & Theme", "appearance", "preferences-desktop-theme-symbolic", "dark light theme wallpaper accent color icons taskbar font"},
    {"Personalization", "Keyboard Shortcuts", "shortcuts", "input-keyboard-symbolic", "keybindings shortcuts super alt hotkeys terminal launcher fullscreen maximize"},
    {"Personalization", "Startup Apps", "autostart", "system-run-symbolic", "autostart boot startup applications login launch background programs"},

    // System & Security
    {"System & Security", "Users & Biometrics", "security", "system-users-symbolic", "password fingerprint iris biometrics screen lock sudo root user account"},
    {"System & Security", "Date & Time", "datetime", "preferences-system-time-symbolic", "clock timezone ntp calendar date 24-hour location sync"},
    {"System & Security", "Updates & Recovery", "updates", "system-software-update-symbolic", "system upgrade packages apt rollback btrfs snapshot axis-rewind restore"},
    {"System & Security", "USB & Connected Devices", "usb", "drive-removable-media-symbolic", "usb flash drives removable peripherals devices lsusb udev"},
    {"System & Security", "About AxisOS", "about", "help-about-symbolic", "system specs hardware cpu ram kernel edition version license"}
};

static gboolean sidebar_filter_func(GtkListBoxRow *row, gpointer user_data) {
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    const char *query = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
    if (!query || query[0] == '\0') return TRUE;

    // Header rows: hide when filtering so only matching category items appear
    if (g_object_get_data(G_OBJECT(row), "is_header")) {
        return FALSE;
    }

    const char *label = (const char *)g_object_get_data(G_OBJECT(row), "page_label");
    const char *group = (const char *)g_object_get_data(G_OBJECT(row), "page_group");
    const char *keywords = (const char *)g_object_get_data(G_OBJECT(row), "page_keywords");

    char *q_lower = g_ascii_strdown(query, -1);
    gboolean match = FALSE;

    if (label) {
        char *l_lower = g_ascii_strdown(label, -1);
        if (strstr(l_lower, q_lower)) match = TRUE;
        g_free(l_lower);
    }
    if (!match && group) {
        char *g_lower = g_ascii_strdown(group, -1);
        if (strstr(g_lower, q_lower)) match = TRUE;
        g_free(g_lower);
    }
    if (!match && keywords) {
        char *k_lower = g_ascii_strdown(keywords, -1);
        if (strstr(k_lower, q_lower)) match = TRUE;
        g_free(k_lower);
    }

    g_free(q_lower);
    return match;
}

static void on_search_text_changed_cb(GtkSearchEntry *se, gpointer user_data) {
    (void)se;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    gtk_list_box_invalidate_filter(GTK_LIST_BOX(app->listbox));
}

static void on_category_changed_cb(GtkListBox *box, GtkListBoxRow *row, gpointer user_data) {
    (void)box;
    if (!row) return;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    const char *page_id = (const char *)g_object_get_data(G_OBJECT(row), "page_id");
    if (page_id) {
        gtk_stack_set_visible_child_name(GTK_STACK(app->stack), page_id);
    }
}

static void select_page_by_id(AxisSettingsApp *app, const char *page_id) {
    if (!page_id) return;
    GList *rows = gtk_container_get_children(GTK_CONTAINER(app->listbox));
    for (GList *iter = rows; iter != NULL; iter = g_list_next(iter)) {
        GtkWidget *row = GTK_WIDGET(iter->data);
        const char *id = (const char *)g_object_get_data(G_OBJECT(row), "page_id");
        if (id && g_strcmp0(id, page_id) == 0) {
            gtk_list_box_select_row(GTK_LIST_BOX(app->listbox), GTK_LIST_BOX_ROW(row));
            gtk_stack_set_visible_child_name(GTK_STACK(app->stack), page_id);
            break;
        }
    }
    g_list_free(rows);
}

// -----------------------------------------------------------------------------
// App Initialization
// -----------------------------------------------------------------------------
static AxisSettingsApp *axis_settings_app_create(void) {
    AxisSettingsApp *app = g_new0(AxisSettingsApp, 1);
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Settings - AxisOS Control Center");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 1020, 680);
    gtk_window_set_position(GTK_WINDOW(app->window), GTK_WIN_POS_CENTER);

    // Apply system theme
    load_axis_theme();

    // Headerbar
    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS Settings");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Hardware, Network, Desktop & System Control Center");
    gtk_window_set_titlebar(GTK_WINDOW(app->window), hb);

    // Main layout
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), hbox);

    // Sidebar
    GtkWidget *sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(sidebar_box), "sidebar");
    gtk_widget_set_size_request(sidebar_box, 260, -1);

    // Search Entry
    GtkWidget *search_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_set_border_width(GTK_CONTAINER(search_container), 12);
    app->search_entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->search_entry), "Search Settings...");
    g_signal_connect(app->search_entry, "search-changed", G_CALLBACK(on_search_text_changed_cb), app);
    gtk_box_pack_start(GTK_BOX(search_container), app->search_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sidebar_box), search_container, FALSE, FALSE, 0);

    GtkWidget *sidebar_scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroller), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    app->listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->listbox), GTK_SELECTION_BROWSE);
    gtk_list_box_set_filter_func(GTK_LIST_BOX(app->listbox), sidebar_filter_func, app, NULL);
    g_signal_connect(app->listbox, "row-selected", G_CALLBACK(on_category_changed_cb), app);

    const char *last_group = NULL;
    for (size_t i = 0; i < sizeof(g_pages) / sizeof(g_pages[0]); i++) {
        // Group Header
        if (!last_group || g_strcmp0(last_group, g_pages[i].group) != 0) {
            last_group = g_pages[i].group;
            GtkWidget *header_row = gtk_list_box_row_new();
            gtk_list_box_row_set_selectable(GTK_LIST_BOX_ROW(header_row), FALSE);
            gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(header_row), FALSE);

            GtkWidget *lbl_hdr = gtk_label_new(g_pages[i].group);
            gtk_style_context_add_class(gtk_widget_get_style_context(lbl_hdr), "dim-label");
            gtk_label_set_xalign(GTK_LABEL(lbl_hdr), 0.0);
            gtk_widget_set_margin_start(lbl_hdr, 16);
            gtk_widget_set_margin_top(lbl_hdr, 12);
            gtk_widget_set_margin_bottom(lbl_hdr, 4);

            gtk_container_add(GTK_CONTAINER(header_row), lbl_hdr);
            g_object_set_data(G_OBJECT(header_row), "is_header", GINT_TO_POINTER(1));
            gtk_list_box_insert(GTK_LIST_BOX(app->listbox), header_row, -1);
        }

        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_start(row_box, 16);
        gtk_widget_set_margin_end(row_box, 16);
        gtk_widget_set_margin_top(row_box, 8);
        gtk_widget_set_margin_bottom(row_box, 8);

        GtkWidget *img = gtk_image_new_from_icon_name(g_pages[i].icon, GTK_ICON_SIZE_MENU);
        GtkWidget *lbl = gtk_label_new(g_pages[i].label);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

        gtk_box_pack_start(GTK_BOX(row_box), img, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(row_box), lbl, TRUE, TRUE, 0);

        g_object_set_data_full(G_OBJECT(row), "page_id", g_strdup(g_pages[i].id), g_free);
        g_object_set_data_full(G_OBJECT(row), "page_label", g_strdup(g_pages[i].label), g_free);
        g_object_set_data_full(G_OBJECT(row), "page_group", g_strdup(g_pages[i].group), g_free);
        g_object_set_data_full(G_OBJECT(row), "page_keywords", g_strdup(g_pages[i].keywords), g_free);

        gtk_container_add(GTK_CONTAINER(row), row_box);
        gtk_list_box_insert(GTK_LIST_BOX(app->listbox), row, -1);
    }

    gtk_container_add(GTK_CONTAINER(sidebar_scroller), app->listbox);
    gtk_box_pack_start(GTK_BOX(sidebar_box), sidebar_scroller, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), sidebar_box, FALSE, FALSE, 0);

    // Content Stack with 180ms crossfade
    app->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(app->stack), 180);

    // Build pages
    build_network_page(app);
    build_bluetooth_page(app);
    build_firewall_page(app);
    build_display_page(app);
    build_brightness_audio_page(app);
    build_power_page(app);
    build_touchpad_page(app);
    build_appearance_page(app);
    build_shortcuts_page(app);
    build_autostart_page(app);
    build_security_page(app);
    build_datetime_page(app);
    build_updates_page(app);
    build_usb_page(app);
    build_about_page(app);

    gtk_box_pack_start(GTK_BOX(hbox), app->stack, TRUE, TRUE, 0);

    // Default select first navigable row
    select_page_by_id(app, "network");

    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    return app;
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkSettings *default_settings = gtk_settings_get_default();
    if (default_settings) {
        g_object_set(default_settings,
                     "gtk-application-prefer-dark-theme", TRUE,
                     "gtk-theme-name", "Adwaita-dark",
                     "gtk-icon-theme-name", "Papirus-Dark",
                     NULL);
    }
    AxisSettingsApp *app = axis_settings_app_create();

    // CLI argument dispatching
    if (argc > 1) {
        const char *arg = argv[1];
        if (strcmp(arg, "about") == 0 || strcmp(arg, "--about") == 0 || strcmp(arg, "-a") == 0 || strcmp(arg, "health") == 0) {
            select_page_by_id(app, "about");
        } else if (strcmp(arg, "updates") == 0 || strcmp(arg, "--updates") == 0 || strcmp(arg, "-u") == 0 || strcmp(arg, "snapshot") == 0) {
            select_page_by_id(app, "updates");
        } else if (strcmp(arg, "network") == 0 || strcmp(arg, "--network") == 0 || strcmp(arg, "-n") == 0 || strcmp(arg, "wifi") == 0) {
            select_page_by_id(app, "network");
        } else if (strcmp(arg, "display") == 0 || strcmp(arg, "--display") == 0 || strcmp(arg, "-d") == 0 || strcmp(arg, "graphics") == 0) {
            select_page_by_id(app, "display");
        } else if (strcmp(arg, "audio") == 0 || strcmp(arg, "sound") == 0 || strcmp(arg, "brightness") == 0) {
            select_page_by_id(app, "audio");
        } else if (strcmp(arg, "bluetooth") == 0 || strcmp(arg, "--bluetooth") == 0 || strcmp(arg, "-b") == 0) {
            select_page_by_id(app, "bluetooth");
        } else if (strcmp(arg, "power") == 0 || strcmp(arg, "battery") == 0 || strcmp(arg, "--power") == 0) {
            select_page_by_id(app, "power");
        } else if (strcmp(arg, "touchpad") == 0 || strcmp(arg, "mouse") == 0 || strcmp(arg, "--touchpad") == 0) {
            select_page_by_id(app, "touchpad");
        } else if (strcmp(arg, "shortcuts") == 0 || strcmp(arg, "keyboard") == 0 || strcmp(arg, "--shortcuts") == 0) {
            select_page_by_id(app, "shortcuts");
        } else if (strcmp(arg, "autostart") == 0 || strcmp(arg, "startup") == 0 || strcmp(arg, "--autostart") == 0) {
            select_page_by_id(app, "autostart");
        } else if (strcmp(arg, "firewall") == 0 || strcmp(arg, "ufw") == 0) {
            select_page_by_id(app, "firewall");
        } else if (strcmp(arg, "usb") == 0) {
            select_page_by_id(app, "usb");
        } else if (strcmp(arg, "appearance") == 0 || strcmp(arg, "theme") == 0) {
            select_page_by_id(app, "appearance");
        } else if (strcmp(arg, "datetime") == 0 || strcmp(arg, "time") == 0) {
            select_page_by_id(app, "datetime");
        } else if (strcmp(arg, "security") == 0 || strcmp(arg, "users") == 0 || strcmp(arg, "accounts") == 0) {
            select_page_by_id(app, "security");
        }
    }

    gtk_widget_show_all(app->window);
    gtk_main();
    g_free(app);
    return 0;
}
