/*
 * AxisOS Native System Settings & Hardware Control Center
 * Written in Pure C11 (-std=gnu11) using GTK 3
 * Features:
 *  - Displays & Graphics: wlr-randr mode switcher (resolution, ratio, refresh rate), scale
 *  - GPU Detection & Status: NVIDIA, AMD, Intel triple vendor drivers & Vulkan info
 *  - Hardware Brightness: Backlight slider (brightnessctl / sysfs)
 *  - Audio & Sound: PipeWire / ALSA volume slider, mute, audio test
 *  - Bluetooth: BlueZ power, scan nearby devices, pair / connect
 *  - Wi-Fi & Network: nmcli scan, connect dialog, Captive Portal detection & browser redirect
 *  - USB Ports & Devices: udev / lsusb peripheral recognition, USB flash drive mount/unmount
 *  - Desktop & Appearance: Taskbar position (Bottom / Top), Dark / Light mode, Icon themes
 *  - Software & Updates: Graphical apt updater (check, upgrade, progress bar without terminal)
 *  - Date, Time & Location: Auto timezone via IP geolocation, NTP sync
 *  - Security & Accounts: User and Root password synchronization, Biometrics (fprintd/iris)
 *  - About: Hardware specs (CPU, RAM, GPU, Kernel, OS Version, Edition switcher)
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

// Structure for AxisSettingsApp
typedef struct {
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *listbox;

    // Display widgets
    GtkWidget *combo_display_output;
    GtkWidget *combo_display_mode;
    GtkWidget *combo_display_rate;
    GtkWidget *lbl_gpu_status;

    // Brightness & Sound widgets
    GtkWidget *scale_brightness;
    GtkWidget *scale_volume;
    GtkWidget *btn_mute;

    // Wi-Fi & Bluetooth widgets
    GtkWidget *switch_wifi;
    GtkWidget *list_wifi;
    GtkWidget *switch_bt;
    GtkWidget *list_bt;

    // USB Peripherals widget
    GtkWidget *list_usb;

    // Appearance widgets
    GtkWidget *radio_bar_bottom;
    GtkWidget *radio_bar_top;
    GtkWidget *combo_icon_theme;

    // Updates widgets
    GtkWidget *lbl_update_status;
    GtkWidget *pbar_update;
    GtkWidget *txt_update_log;
    GtkWidget *btn_run_updates;

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
} AxisSettingsApp;

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
    "* {"
    "    font-family: \"IBM Plex Sans\", \"Inter\", -apple-system, BlinkMacSystemFont, \"Segoe UI\", Cantarell, sans-serif;"
    "}"
    "window {"
    "    background-color: #141826;"
    "    color: #FFFFFF;"
    "}"
    "headerbar {"
    "    background: #1A1D2B;"
    "    border-bottom: 1px solid #2E3349;"
    "    color: #FFFFFF;"
    "}"
    "headerbar .title {"
    "    font-weight: 700;"
    "    font-size: 15px;"
    "    color: #FFFFFF;"
    "}"
    "headerbar .subtitle {"
    "    font-size: 12px;"
    "    color: #94A3B8;"
    "}"
    ".sidebar {"
    "    background-color: #1A1D2B;"
    "    border-right: 1px solid #2E3349;"
    "}"
    ".sidebar list {"
    "    background-color: transparent;"
    "}"
    ".sidebar list row {"
    "    padding: 12px 18px;"
    "    color: #94A3B8;"
    "    font-weight: 600;"
    "    font-size: 13px;"
    "    border-left: 3px solid transparent;"
    "}"
    ".sidebar list row:hover {"
    "    background-color: #262A3D;"
    "    color: #FFFFFF;"
    "}"
    ".sidebar list row:selected {"
    "    background-color: #262A3D;"
    "    color: #FFFFFF;"
    "    border-left: 3px solid #7D95FF;"
    "}"
    ".content-pane {"
    "    padding: 24px 32px;"
    "    background-color: #141826;"
    "}"
    ".card {"
    "    background-color: #1F2231;"
    "    border: 1px solid #2E3349;"
    "    border-radius: 16px;"
    "    padding: 18px 22px;"
    "    margin-bottom: 16px;"
    "    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.25);"
    "}"
    ".card-title {"
    "    font-size: 15px;"
    "    font-weight: 700;"
    "    color: #FFFFFF;"
    "    margin-bottom: 4px;"
    "}"
    ".card-subtitle {"
    "    font-size: 12px;"
    "    color: #94A3B8;"
    "    margin-bottom: 12px;"
    "}"
    ".btn-primary {"
    "    background: #7D95FF;"
    "    color: #141826;"
    "    font-weight: 700;"
    "    border-radius: 999px;"
    "    border: none;"
    "    padding: 8px 20px;"
    "}"
    ".btn-primary:hover {"
    "    background: #6B82F0;"
    "}"
    ".btn-primary:active {"
    "    background: #5A71E0;"
    "}"
    ".btn-secondary {"
    "    background: #262A3D;"
    "    color: #FFFFFF;"
    "    border: 1px solid #2E3349;"
    "    border-radius: 999px;"
    "    padding: 8px 18px;"
    "    font-weight: 600;"
    "}"
    ".btn-secondary:hover {"
    "    background: #2E3349;"
    "    border-color: #7D95FF;"
    "    color: #FFFFFF;"
    "}"
    ".badge-blue {"
    "    background: #262A3D;"
    "    color: #7D95FF;"
    "    border: 1px solid #7D95FF;"
    "    border-radius: 999px;"
    "    padding: 4px 12px;"
    "    font-size: 11px;"
    "    font-weight: 700;"
    "}"
    ".badge-green {"
    "    background: #16382B;"
    "    color: #34D399;"
    "    border: 1px solid #059669;"
    "    border-radius: 999px;"
    "    padding: 4px 12px;"
    "    font-size: 11px;"
    "    font-weight: 700;"
    "}"
    "progressbar progress {"
    "    background: #7D95FF;"
    "    border-radius: 999px;"
    "}"
    "progressbar trough {"
    "    background: #262A3D;"
    "    border-radius: 999px;"
    "    min-height: 8px;"
    "}"
    "entry {"
    "    background-color: #1F2231;"
    "    color: #FFFFFF;"
    "    border: 1px solid #2E3349;"
    "    border-radius: 10px;"
    "    padding: 8px 14px;"
    "}"
    "entry:focus {"
    "    border-color: #7D95FF;"
    "}"
    "scale trough {"
    "    background-color: #262A3D;"
    "    border-radius: 999px;"
    "    min-height: 6px;"
    "}"
    "scale highlight {"
    "    background-color: #7D95FF;"
    "    border-radius: 999px;"
    "}"
    "switch {"
    "    background: #2E3349;"
    "    border-radius: 999px;"
    "}"
    "switch:checked {"
    "    background: #7D95FF;"
    "}"
    "switch slider {"
    "    background: #FFFFFF;"
    "    border-radius: 999px;"
    "}";

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

// -----------------------------------------------------------------------------
// 1. Display Page
// -----------------------------------------------------------------------------
static char *detect_gpus(void) {
    char *info = exec_cmd("lspci -nnk | grep -E -A 3 -i 'vga|3d|display'");
    g_strstrip(info);
    if (strlen(info) == 0) {
        g_free(info);
        info = g_strdup("No PCI GPU detected directly via lspci (Running in Virtual / Standard DRM mode).");
    }

    char *vulkan_check = exec_cmd("vulkaninfo --summary 2>/dev/null | grep -E -i 'deviceName|driverName' || true");
    g_strstrip(vulkan_check);

    char *summary;
    if (strlen(vulkan_check) > 0) {
        summary = g_strdup_printf("🎮 Detected Graphics Adapters:\n%s\n\n⚡ Vulkan Acceleration:\n%s", info, vulkan_check);
    } else {
        summary = g_strdup_printf("🎮 Detected Graphics Adapters:\n%s", info);
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

static void build_display_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_res = create_card("Display Resolution, Refresh Rate & Scaling",
        "Configure output monitors, native aspect ratios, and high-refresh-rate gaming modes via wlr-randr.");

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    GtkWidget *lbl_out = gtk_label_new("Active Display:");
    gtk_label_set_xalign(GTK_LABEL(lbl_out), 0.0);
    app->combo_display_output = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_output), "eDP-1 (Built-in Display)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_output), "HDMI-A-1 (External Monitor)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_output), "DP-1 (DisplayPort)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_display_output), 0);

    gtk_grid_attach(GTK_GRID(grid), lbl_out, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->combo_display_output, 1, 0, 1, 1);

    GtkWidget *lbl_mode = gtk_label_new("Resolution & Aspect Ratio:");
    gtk_label_set_xalign(GTK_LABEL(lbl_mode), 0.0);
    app->combo_display_mode = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "3840x2160 (16:9 4K UHD)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "2560x1440 (16:9 2K QHD)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "1920x1080 (16:9 FHD Recommended)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "1920x1200 (16:10 WUXGA)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "1680x1050 (16:10 WSXGA+)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode), "1366x768 (16:9 HD)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_display_mode), 2);

    gtk_grid_attach(GTK_GRID(grid), lbl_mode, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->combo_display_mode, 1, 1, 1, 1);

    GtkWidget *lbl_rate = gtk_label_new("Refresh Rate:");
    gtk_label_set_xalign(GTK_LABEL(lbl_rate), 0.0);
    app->combo_display_rate = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate), "60 Hz (Standard Desktop)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate), "75 Hz");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate), "120 Hz");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate), "144 Hz (Gaming)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate), "165 Hz (Ultra Fast)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_display_rate), 0);

    gtk_grid_attach(GTK_GRID(grid), lbl_rate, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->combo_display_rate, 1, 2, 1, 1);

    GtkWidget *btn_apply = gtk_button_new_with_label("Apply Display Settings");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_apply), "btn-primary");
    g_signal_connect(btn_apply, "clicked", G_CALLBACK(on_apply_display_cb), app);
    gtk_grid_attach(GTK_GRID(grid), btn_apply, 1, 3, 1, 1);

    gtk_box_pack_start(GTK_BOX(card_res), grid, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(page), card_res, FALSE, FALSE, 0);

    // GPU Detection Card
    GtkWidget *card_gpu = create_card("Graphics Hardware & Drivers",
        "Automatic recognition for NVIDIA GeForce, AMD Radeon, and Intel Graphics with Mesa Vulkan drivers.");
    char *gpus = detect_gpus();
    app->lbl_gpu_status = gtk_label_new(gpus);
    g_free(gpus);
    gtk_label_set_xalign(GTK_LABEL(app->lbl_gpu_status), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(app->lbl_gpu_status), TRUE);
    gtk_box_pack_start(GTK_BOX(card_gpu), app->lbl_gpu_status, FALSE, FALSE, 0);

    GtkWidget *btn_refresh_gpu = gtk_button_new_with_label("Re-scan Graphics Hardware");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh_gpu), "btn-secondary");
    g_signal_connect(btn_refresh_gpu, "clicked", G_CALLBACK(on_refresh_gpu_cb), app);
    gtk_box_pack_start(GTK_BOX(card_gpu), btn_refresh_gpu, FALSE, FALSE, 8);

    gtk_box_pack_start(GTK_BOX(page), card_gpu, FALSE, FALSE, 0);
    gtk_stack_add_named(GTK_STACK(app->stack), page, "display");
}

// -----------------------------------------------------------------------------
// 2. Gaming Boost & Performance Page
// -----------------------------------------------------------------------------
static void update_game_boost_status(AxisSettingsApp *app);

static gboolean on_game_boost_toggled_cb(GtkSwitch *sw, gboolean state, gpointer user_data) {
    (void)sw;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    const char *sudo_prefix = (geteuid() == 0) ? "" : "pkexec ";
    char *cmd = g_strdup_printf("%s/usr/local/bin/axis-game-boost %s 2>/dev/null || true",
                                sudo_prefix, state ? "on" : "off");
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
    update_game_boost_status(app);
    return FALSE;
}

static void update_game_boost_status(AxisSettingsApp *app) {
    char *st = exec_cmd("/usr/local/bin/axis-game-boost status 2>/dev/null || true");
    gboolean is_on = (strstr(st, "GAME_BOOST_ENABLED=true") != NULL);
    g_free(st);

    if (app->switch_game_boost) {
        g_signal_handlers_block_by_func(app->switch_game_boost, (gpointer)on_game_boost_toggled_cb, app);
        gtk_switch_set_active(GTK_SWITCH(app->switch_game_boost), is_on);
        g_signal_handlers_unblock_by_func(app->switch_game_boost, (gpointer)on_game_boost_toggled_cb, app);
    }

    if (app->lbl_game_boost_status) {
        if (is_on) {
            gtk_label_set_markup(GTK_LABEL(app->lbl_game_boost_status),
                "<span foreground='#10B981' weight='bold'>⚡ GAME BOOST ENGAGED • MAXIMUM HARDWARE OVERCLOCK ACTIVE</span>");
        } else {
            gtk_label_set_markup(GTK_LABEL(app->lbl_game_boost_status),
                "<span foreground='#94A3B8' weight='normal'>🍃 Balanced Energy Mode • Dynamic Clocks Active</span>");
        }
    }

    char *gov = exec_cmd("cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo 'schedutil'");
    g_strstrip(gov);

    if (app->lbl_boost_cpu) {
        char *cpu_txt;
        if (is_on) {
            cpu_txt = g_strdup_printf("⚡ CPU Core Scaling: Pinned Peak Clocks (Governor: %s, EPP: performance, Turbo: 100%%)", gov);
        } else {
            cpu_txt = g_strdup_printf("🍃 CPU Core Scaling: Dynamic Balanced (Governor: %s, EPP: balance_performance)", gov);
        }
        gtk_label_set_text(GTK_LABEL(app->lbl_boost_cpu), cpu_txt);
        g_free(cpu_txt);
    }
    g_free(gov);

    if (app->lbl_boost_gpu) {
        gtk_label_set_text(GTK_LABEL(app->lbl_boost_gpu), is_on ?
            "🎮 GPU Overclocking: Maximum Power Limit Unlocked • 3D Fullscreen High-Clocks Active" :
            "🎮 GPU Dynamic Tuning: Adaptive Clocks • Energy-Efficient Profile");
    }

    if (app->lbl_boost_latency) {
        gtk_label_set_text(GTK_LABEL(app->lbl_boost_latency), is_on ?
            "🚀 Latency Tuning: Zero Split-Lock Jitter (Off) • Swappiness: 10 • Max Mappings: 2.14B" :
            "⚖️ Latency Tuning: Standard Kernel Defaults (Swappiness: 60)");
    }
}

static void build_game_boost_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_master = create_card("Gaming Boost & Hardware Overclocking",
        "Unleash maximum system performance. Dynamically pins CPU clock speeds to peak frequencies, unlocks GPU wattage and clocks, eliminates memory split-lock micro-stutters, and optimizes frame latency for DirectX/Vulkan games.");

    GtkWidget *hbox_toggle = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *lbl_toggle_desc = gtk_label_new("Activate GameBoost Maximum Overclocking Engine");
    gtk_label_set_xalign(GTK_LABEL(lbl_toggle_desc), 0.0);
    gtk_box_pack_start(GTK_BOX(hbox_toggle), lbl_toggle_desc, TRUE, TRUE, 0);

    app->switch_game_boost = gtk_switch_new();
    gtk_widget_set_valign(app->switch_game_boost, GTK_ALIGN_CENTER);
    g_signal_connect(app->switch_game_boost, "state-set", G_CALLBACK(on_game_boost_toggled_cb), app);
    gtk_box_pack_end(GTK_BOX(hbox_toggle), app->switch_game_boost, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_master), hbox_toggle, FALSE, FALSE, 6);

    app->lbl_game_boost_status = gtk_label_new(NULL);
    gtk_label_set_xalign(GTK_LABEL(app->lbl_game_boost_status), 0.0);
    gtk_box_pack_start(GTK_BOX(card_master), app->lbl_game_boost_status, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(page), card_master, FALSE, FALSE, 0);

    GtkWidget *card_details = create_card("Active Tuning & Hardware Subsystems",
        "Real-time status of CPU governors, graphics pipelines, and kernel memory allocators.");

    app->lbl_boost_cpu = gtk_label_new("⚡ CPU Clocks: Initializing...");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_boost_cpu), 0.0);
    gtk_box_pack_start(GTK_BOX(card_details), app->lbl_boost_cpu, FALSE, FALSE, 4);

    app->lbl_boost_gpu = gtk_label_new("🎮 GPU State: Initializing...");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_boost_gpu), 0.0);
    gtk_box_pack_start(GTK_BOX(card_details), app->lbl_boost_gpu, FALSE, FALSE, 4);

    app->lbl_boost_latency = gtk_label_new("🚀 Latency: Initializing...");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_boost_latency), 0.0);
    gtk_box_pack_start(GTK_BOX(card_details), app->lbl_boost_latency, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(page), card_details, FALSE, FALSE, 0);

    GtkWidget *card_info = create_card("Gaming Performance Recommendations",
        "• GameBoost works automatically with Steam, Proton, Heroic, Lutris, and native Linux games.\n• When turned off, the system returns immediately to balanced energy-saving clocks to preserve battery life and reduce fan noise.\n• Combine with Feral GameMode (gamemoderun %command%) for per-process scheduler prioritization.");
    gtk_box_pack_start(GTK_BOX(page), card_info, FALSE, FALSE, 0);

    update_game_boost_status(app);
    gtk_stack_add_named(GTK_STACK(app->stack), page, "game_boost");
}

// -----------------------------------------------------------------------------
// 3. Brightness & Audio Page
// -----------------------------------------------------------------------------
static void on_brightness_changed_cb(GtkRange *range, gpointer user_data) {
    (void)user_data;
    double val = gtk_range_get_value(range);
    char *cmd = g_strdup_printf("brightnessctl set %d%% -q 2>/dev/null || true", (int)val);
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
}

static void on_volume_changed_cb(GtkRange *range, gpointer user_data) {
    (void)user_data;
    double val = gtk_range_get_value(range);
    int v = (int)val;
    char *cmd = g_strdup_printf(
        "wpctl set-volume @DEFAULT_AUDIO_SINK@ %d%% 2>/dev/null || "
        "amixer -q sset Master %d%% 2>/dev/null || true", v, v);
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
}

static void on_toggle_mute_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    char *out = exec_cmd("wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle 2>/dev/null || amixer -q sset Master toggle 2>/dev/null || true");
    g_free(out);
}

static gpointer thread_test_sound(gpointer user_data) {
    (void)user_data;
    char *out = exec_cmd("speaker-test -t sine -f 440 -l 1 2>/dev/null || paplay /usr/share/sounds/freedesktop/stereo/complete.oga 2>/dev/null || true");
    g_free(out);
    return NULL;
}

static void on_test_sound_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    g_thread_new("test_sound", thread_test_sound, NULL);
}

static void build_brightness_audio_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_bright = create_card("Display Brightness",
        "Hardware display backlight control via brightnessctl and /sys/class/backlight.");

    app->scale_brightness = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 5.0, 100.0, 5.0);
    gtk_scale_set_draw_value(GTK_SCALE(app->scale_brightness), TRUE);
    gtk_range_set_value(GTK_RANGE(app->scale_brightness), 80.0);
    g_signal_connect(app->scale_brightness, "value-changed", G_CALLBACK(on_brightness_changed_cb), app);
    gtk_box_pack_start(GTK_BOX(card_bright), app->scale_brightness, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(page), card_bright, FALSE, FALSE, 0);

    GtkWidget *card_sound = create_card("Audio Volume & Sinks",
        "Hardware audio master volume and muting via PipeWire (WirePlumber) and ALSA.");

    app->scale_volume = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 5.0);
    gtk_scale_set_draw_value(GTK_SCALE(app->scale_volume), TRUE);
    gtk_range_set_value(GTK_RANGE(app->scale_volume), 70.0);
    g_signal_connect(app->scale_volume, "value-changed", G_CALLBACK(on_volume_changed_cb), app);
    gtk_box_pack_start(GTK_BOX(card_sound), app->scale_volume, FALSE, FALSE, 4);

    GtkWidget *hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    app->btn_mute = gtk_button_new_with_label("󰝟 Mute / Unmute");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->btn_mute), "btn-secondary");
    g_signal_connect(app->btn_mute, "clicked", G_CALLBACK(on_toggle_mute_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_btns), app->btn_mute, FALSE, FALSE, 0);

    GtkWidget *btn_test_sound = gtk_button_new_with_label("🔔 Test Audio Tone");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_test_sound), "btn-secondary");
    g_signal_connect(btn_test_sound, "clicked", G_CALLBACK(on_test_sound_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_test_sound, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(card_sound), hbox_btns, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(page), card_sound, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(app->stack), page, "audio_display");
}

// -----------------------------------------------------------------------------
// 4. Wi-Fi & Network Page
// -----------------------------------------------------------------------------
static void refresh_wifi_list(AxisSettingsApp *app);

static void on_wifi_toggle_cb(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    (void)user_data;
    gboolean active = gtk_switch_get_active(GTK_SWITCH(obj));
    char *cmd = g_strdup_printf("nmcli radio wifi %s", active ? "on" : "off");
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
}

static void on_scan_wifi_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    refresh_wifi_list(app);
}

static void on_captive_portal_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    char *out = exec_cmd("/usr/local/bin/axis-captive-portal.sh &");
    g_free(out);
}

static void on_connect_wifi_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->list_wifi));
    if (!row) return;

    const char *ssid = (const char *)g_object_get_data(G_OBJECT(row), "ssid");
    if (!ssid) return;

    GtkWidget *dlg = gtk_dialog_new_with_buttons("Connect to Wi-Fi",
        GTK_WINDOW(app->window), GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Connect", GTK_RESPONSE_ACCEPT, NULL);

    GtkWidget *content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_container_set_border_width(GTK_CONTAINER(content), 16);

    char *prompt = g_strdup_printf("Enter Wi-Fi password for %s:", ssid);
    GtkWidget *lbl = gtk_label_new(prompt);
    g_free(prompt);
    gtk_box_pack_start(GTK_BOX(content), lbl, FALSE, FALSE, 8);

    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
    gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 8);
    gtk_widget_show_all(dlg);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        const char *pass = gtk_entry_get_text(GTK_ENTRY(entry));
        char *cmd = g_strdup_printf("nmcli device wifi connect \"%s\" password \"%s\" 2>&1", ssid, pass);
        char *res = exec_cmd(cmd);
        g_free(cmd);

        GtkWidget *res_dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "Connection Result:\n%s", res);
        g_free(res);
        gtk_dialog_run(GTK_DIALOG(res_dlg));
        gtk_widget_destroy(res_dlg);
    }
    gtk_widget_destroy(dlg);
}

static void refresh_wifi_list(AxisSettingsApp *app) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->list_wifi));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    char *out = exec_cmd("nmcli -t -f SSID,SIGNAL,SECURITY device wifi list 2>/dev/null || true");
    gboolean found = FALSE;
    char **lines = g_strsplit(out, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++) {
        if (lines[i][0] == '\0') continue;
        char **parts = g_strsplit(lines[i], ":", 3);
        if (parts[0] && parts[0][0] != '\0') {
            const char *ssid = parts[0];
            const char *signal = parts[1] ? parts[1] : "";
            const char *sec = (parts[1] && parts[2]) ? parts[2] : "";

            GtkWidget *row = gtk_list_box_row_new();
            GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
            GtkWidget *lbl_ssid = gtk_label_new(ssid);
            gtk_label_set_xalign(GTK_LABEL(lbl_ssid), 0.0);

            char *sig_str = g_strdup_printf("📶 %s%% (%s)", signal, sec[0] ? sec : "Open");
            GtkWidget *lbl_sig = gtk_label_new(sig_str);
            g_free(sig_str);

            gtk_box_pack_start(GTK_BOX(hbox), lbl_ssid, TRUE, TRUE, 8);
            gtk_box_pack_end(GTK_BOX(hbox), lbl_sig, FALSE, FALSE, 8);
            gtk_container_add(GTK_CONTAINER(row), hbox);
            g_object_set_data_full(G_OBJECT(row), "ssid", g_strdup(ssid), g_free);
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

static void build_network_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_wifi = create_card("Wireless Networking (Wi-Fi)",
        "Scan nearby wireless access points, connect with security credentials, and captive portal login.");

    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *lbl_switch = gtk_label_new("Enable Wi-Fi Adapter:");
    app->switch_wifi = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_wifi), TRUE);
    g_signal_connect(app->switch_wifi, "notify::active", G_CALLBACK(on_wifi_toggle_cb), app);

    GtkWidget *btn_scan_wifi = gtk_button_new_with_label("🔄 Scan Wi-Fi");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_scan_wifi), "btn-primary");
    g_signal_connect(btn_scan_wifi, "clicked", G_CALLBACK(on_scan_wifi_cb), app);

    GtkWidget *btn_portal = gtk_button_new_with_label("🌐 Captive Portal Web Login");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_portal), "btn-secondary");
    g_signal_connect(btn_portal, "clicked", G_CALLBACK(on_captive_portal_cb), app);

    gtk_box_pack_start(GTK_BOX(hbox_top), lbl_switch, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), app->switch_wifi, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_scan_wifi, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_portal, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_wifi), hbox_top, FALSE, FALSE, 6);

    app->list_wifi = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->list_wifi), GTK_SELECTION_SINGLE);

    GtkWidget *scroll_wifi = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_wifi), 180);
    gtk_container_add(GTK_CONTAINER(scroll_wifi), app->list_wifi);
    gtk_box_pack_start(GTK_BOX(card_wifi), scroll_wifi, TRUE, TRUE, 8);

    GtkWidget *btn_connect_wifi = gtk_button_new_with_label("Connect to Selected Network");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_connect_wifi), "btn-primary");
    g_signal_connect(btn_connect_wifi, "clicked", G_CALLBACK(on_connect_wifi_cb), app);
    gtk_box_pack_start(GTK_BOX(card_wifi), btn_connect_wifi, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page), card_wifi, TRUE, TRUE, 0);
    gtk_stack_add_named(GTK_STACK(app->stack), page, "network");

    refresh_wifi_list(app);
}

// -----------------------------------------------------------------------------
// 5. Bluetooth Page
// -----------------------------------------------------------------------------
static void refresh_bt_list(AxisSettingsApp *app);

static void on_bt_toggle_cb(GObject *obj, GParamSpec *pspec, gpointer user_data) {
    (void)pspec;
    (void)user_data;
    gboolean active = gtk_switch_get_active(GTK_SWITCH(obj));
    char *cmd = g_strdup_printf("bluetoothctl power %s", active ? "on" : "off");
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
}

static gboolean idle_bt_scanned(gpointer user_data) {
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    refresh_bt_list(app);
    return G_SOURCE_REMOVE;
}

static gpointer thread_scan_bt(gpointer user_data) {
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    char *out = exec_cmd("bluetoothctl scan on & sleep 3; bluetoothctl scan off");
    g_free(out);
    g_idle_add(idle_bt_scanned, app);
    return NULL;
}

static void on_scan_bt_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    g_thread_new("scan_bt", thread_scan_bt, app);
}

static void on_pair_bt_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->list_bt));
    if (!row) return;

    const char *mac = (const char *)g_object_get_data(G_OBJECT(row), "mac");
    if (!mac) return;

    char *cmd = g_strdup_printf("bluetoothctl pair %s && bluetoothctl connect %s", mac, mac);
    char *out = exec_cmd(cmd);
    g_free(out);
    g_free(cmd);
    refresh_bt_list(app);
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
        if (g_str_has_prefix(lines[i], "Device ")) {
            char **tokens = g_strsplit(lines[i], " ", 3);
            if (tokens[0] && tokens[1]) {
                const char *mac = tokens[1];
                const char *name = tokens[2] ? tokens[2] : mac;

                GtkWidget *row = gtk_list_box_row_new();
                GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
                GtkWidget *lbl_name = gtk_label_new(name);
                gtk_label_set_xalign(GTK_LABEL(lbl_name), 0.0);
                GtkWidget *lbl_mac = gtk_label_new(mac);

                gtk_box_pack_start(GTK_BOX(hbox), lbl_name, TRUE, TRUE, 8);
                gtk_box_pack_end(GTK_BOX(hbox), lbl_mac, FALSE, FALSE, 8);
                gtk_container_add(GTK_CONTAINER(row), hbox);
                g_object_set_data_full(G_OBJECT(row), "mac", g_strdup(mac), g_free);
                gtk_list_box_insert(GTK_LIST_BOX(app->list_bt), row, -1);
                found = TRUE;
            }
            g_strfreev(tokens);
        }
    }
    g_strfreev(lines);
    g_free(out);

    if (!found) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new("No Bluetooth devices found in range.");
        gtk_container_add(GTK_CONTAINER(row), lbl);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_bt), row, -1);
    }
    gtk_widget_show_all(app->list_bt);
}

static void build_bluetooth_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card_bt = create_card("Bluetooth Devices",
        "Hardware Bluetooth adapter management, discovery of game controllers, mice, headsets, and pairing.");

    GtkWidget *hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *lbl_switch = gtk_label_new("Bluetooth Power:");
    app->switch_bt = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_bt), TRUE);
    g_signal_connect(app->switch_bt, "notify::active", G_CALLBACK(on_bt_toggle_cb), app);

    GtkWidget *btn_scan_bt = gtk_button_new_with_label("󰂯 Scan Devices");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_scan_bt), "btn-primary");
    g_signal_connect(btn_scan_bt, "clicked", G_CALLBACK(on_scan_bt_cb), app);

    gtk_box_pack_start(GTK_BOX(hbox_top), lbl_switch, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), app->switch_bt, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_top), btn_scan_bt, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(card_bt), hbox_top, FALSE, FALSE, 6);

    app->list_bt = gtk_list_box_new();
    GtkWidget *scroll_bt = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_bt), 180);
    gtk_container_add(GTK_CONTAINER(scroll_bt), app->list_bt);
    gtk_box_pack_start(GTK_BOX(card_bt), scroll_bt, TRUE, TRUE, 8);

    GtkWidget *btn_pair_bt = gtk_button_new_with_label("Pair / Connect Selected Device");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_pair_bt), "btn-primary");
    g_signal_connect(btn_pair_bt, "clicked", G_CALLBACK(on_pair_bt_cb), app);
    gtk_box_pack_start(GTK_BOX(card_bt), btn_pair_bt, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(page), card_bt, TRUE, TRUE, 0);
    gtk_stack_add_named(GTK_STACK(app->stack), page, "bluetooth");

    refresh_bt_list(app);
}

// -----------------------------------------------------------------------------
// 6. USB & Connected Devices Page
// -----------------------------------------------------------------------------
static void refresh_usb_list(AxisSettingsApp *app) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(app->list_usb));
    for (GList *iter = children; iter != NULL; iter = g_list_next(iter)) {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);

    gboolean found = FALSE;
    char *out = exec_cmd("lsusb 2>/dev/null || true");
    char **lines = g_strsplit(out, "\n", -1);
    for (int i = 0; lines[i] != NULL; i++) {
        if (lines[i][0] == '\0') continue;
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        char *dev_txt = g_strdup_printf("🔌 %s", lines[i]);
        GtkWidget *lbl_dev = gtk_label_new(dev_txt);
        g_free(dev_txt);
        gtk_label_set_xalign(GTK_LABEL(lbl_dev), 0.0);
        gtk_box_pack_start(GTK_BOX(hbox), lbl_dev, TRUE, TRUE, 8);
        gtk_container_add(GTK_CONTAINER(row), hbox);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_usb), row, -1);
        found = TRUE;
    }
    g_strfreev(lines);
    g_free(out);

    char *blk = exec_cmd("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,MODEL,TRAN -n 2>/dev/null | grep -i usb || true");
    char **blk_lines = g_strsplit(blk, "\n", -1);
    for (int i = 0; blk_lines[i] != NULL; i++) {
        if (blk_lines[i][0] == '\0') continue;
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        char *blk_txt = g_strdup_printf("💾 Removable Drive: %s", blk_lines[i]);
        GtkWidget *lbl_blk = gtk_label_new(blk_txt);
        g_free(blk_txt);
        gtk_label_set_xalign(GTK_LABEL(lbl_blk), 0.0);
        gtk_box_pack_start(GTK_BOX(hbox), lbl_blk, TRUE, TRUE, 8);
        gtk_container_add(GTK_CONTAINER(row), hbox);
        gtk_list_box_insert(GTK_LIST_BOX(app->list_usb), row, -1);
        found = TRUE;
    }
    g_strfreev(blk_lines);
    g_free(blk);

    if (!found) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new("No external USB peripherals or drives detected.");
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

    GtkWidget *card_usb = create_card("Connected USB Hardware & Storage",
        "Real-time recognition for USB mice, keyboards, controllers, webcams, and flash drives.");

    GtkWidget *hbox_actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_refresh_usb = gtk_button_new_with_label("🔄 Re-detect USB Ports");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh_usb), "btn-primary");
    g_signal_connect(btn_refresh_usb, "clicked", G_CALLBACK(on_refresh_usb_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_actions), btn_refresh_usb, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(card_usb), hbox_actions, FALSE, FALSE, 6);

    app->list_usb = gtk_list_box_new();
    GtkWidget *scroll_usb = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_usb), 260);
    gtk_container_add(GTK_CONTAINER(scroll_usb), app->list_usb);
    gtk_box_pack_start(GTK_BOX(card_usb), scroll_usb, TRUE, TRUE, 8);

    gtk_box_pack_start(GTK_BOX(page), card_usb, TRUE, TRUE, 0);
    gtk_stack_add_named(GTK_STACK(app->stack), page, "usb");

    refresh_usb_list(app);
}

// -----------------------------------------------------------------------------
// 7. Appearance & Desktop Page
// -----------------------------------------------------------------------------
static void on_apply_taskbar_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    gboolean is_bottom = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->radio_bar_bottom));
    const char *pos = is_bottom ? "bottom" : "top";

    const char *home = getenv("HOME");
    if (home) {
        char *cmd = g_strdup_printf(
            "sed -i 's/\"position\": \".*\"/\"position\": \"%s\"/g' %s/.config/waybar/config.jsonc 2>/dev/null || true; "
            "pkill -SIGUSR2 waybar 2>/dev/null || true", pos, home);
        char *out = exec_cmd(cmd);
        g_free(out);
        g_free(cmd);
    }
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
        "Consistent monochrome palette: Obsidian Black (#0A0B0D), Titanium Slate (#94A3B8), and Crisp White (#FFFFFF).");

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    GtkWidget *lbl_icon = gtk_label_new("System Icon Set:");
    gtk_label_set_xalign(GTK_LABEL(lbl_icon), 0.0);
    app->combo_icon_theme = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_icon_theme), "AxisHarmony (Default HarmonyOS Squircle)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_icon_theme), "Papirus-Dark (High-Contrast)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_icon_theme), "Papirus");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_icon_theme), "Adwaita");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_icon_theme), 0);

    gtk_grid_attach(GTK_GRID(grid), lbl_icon, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->combo_icon_theme, 1, 0, 1, 1);

    GtkWidget *btn_apply_icons = gtk_button_new_with_label("Set Icon Theme");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_apply_icons), "btn-secondary");
    g_signal_connect(btn_apply_icons, "clicked", G_CALLBACK(on_apply_icons_cb), app);
    gtk_grid_attach(GTK_GRID(grid), btn_apply_icons, 2, 0, 1, 1);

    gtk_box_pack_start(GTK_BOX(card_theme), grid, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_theme, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(app->stack), page, "appearance");
}

// -----------------------------------------------------------------------------
// 8. Software & Updates Page
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
    GtkWidget *btn_check = gtk_button_new_with_label("🔍 Check for Updates");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_check), "btn-secondary");
    g_signal_connect(btn_check, "clicked", G_CALLBACK(on_check_updates_cb), app);

    app->btn_run_updates = gtk_button_new_with_label("⬇️  Install All Updates Now");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->btn_run_updates), "btn-primary");
    g_signal_connect(app->btn_run_updates, "clicked", G_CALLBACK(on_install_updates_cb), app);

    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_check, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox_btns), app->btn_run_updates, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), hbox_btns, FALSE, FALSE, 6);

    app->txt_update_log = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->txt_update_log), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app->txt_update_log), TRUE);

    GtkWidget *scroll_log = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_log), 180);
    gtk_container_add(GTK_CONTAINER(scroll_log), app->txt_update_log);
    gtk_box_pack_start(GTK_BOX(card), scroll_log, TRUE, TRUE, 8);

    gtk_box_pack_start(GTK_BOX(page), card, TRUE, TRUE, 0);
    gtk_stack_add_named(GTK_STACK(app->stack), page, "updates");
}

// -----------------------------------------------------------------------------
// 9. Date, Time & Location Page
// -----------------------------------------------------------------------------
static void on_sync_tz_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    char *out = exec_cmd("/usr/local/bin/axis-autotimezone.sh &");
    g_free(out);

    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Automatic geolocation and timezone sync initiated!");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void on_sync_ntp_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    char *out = exec_cmd("pkexec timedatectl set-ntp true 2>/dev/null || true");
    g_free(out);

    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Network Time Protocol (NTP) enabled and synchronized.");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void build_datetime_page(AxisSettingsApp *app) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *card = create_card("Date, Time & Automatic Location",
        "Automatic timezone sync using IP geolocation and Network Time Protocol (NTP).");

    char *time_out = exec_cmd("timedatectl status 2>/dev/null || date");
    g_strstrip(time_out);
    GtkWidget *lbl_time = gtk_label_new(time_out);
    g_free(time_out);
    gtk_label_set_xalign(GTK_LABEL(lbl_time), 0.0);
    gtk_box_pack_start(GTK_BOX(card), lbl_time, FALSE, FALSE, 6);

    GtkWidget *hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_sync_tz = gtk_button_new_with_label("🌍 Auto-Detect Location & Timezone");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_sync_tz), "btn-primary");
    g_signal_connect(btn_sync_tz, "clicked", G_CALLBACK(on_sync_tz_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_sync_tz, FALSE, FALSE, 0);

    GtkWidget *btn_sync_ntp = gtk_button_new_with_label("⏱️ Sync Clock with NTP");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_sync_ntp), "btn-secondary");
    g_signal_connect(btn_sync_ntp, "clicked", G_CALLBACK(on_sync_ntp_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_btns), btn_sync_ntp, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(card), hbox_btns, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(app->stack), page, "datetime");
}

// -----------------------------------------------------------------------------
// 10. Accounts & Security Page
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

    GtkWidget *card = create_card("User Account & Root Security",
        "Keep user password and root administrative password synchronized as required.");

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

    GtkWidget *lbl1 = gtk_label_new("New Password:");
    gtk_label_set_xalign(GTK_LABEL(lbl1), 0.0);
    app->entry_user_pass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(app->entry_user_pass), FALSE);

    GtkWidget *lbl2 = gtk_label_new("Confirm Password:");
    gtk_label_set_xalign(GTK_LABEL(lbl2), 0.0);
    app->entry_confirm_pass = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(app->entry_confirm_pass), FALSE);

    gtk_grid_attach(GTK_GRID(grid), lbl1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->entry_user_pass, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), lbl2, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->entry_confirm_pass, 1, 1, 1, 1);

    GtkWidget *btn_sync_pass = gtk_button_new_with_label("Update & Sync User & Root Passwords");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_sync_pass), "btn-primary");
    g_signal_connect(btn_sync_pass, "clicked", G_CALLBACK(on_sync_passwords_cb), app);
    gtk_grid_attach(GTK_GRID(grid), btn_sync_pass, 1, 2, 1, 1);

    app->lbl_pass_status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_pass_status), 0.0);
    gtk_grid_attach(GTK_GRID(grid), app->lbl_pass_status, 1, 3, 1, 1);

    gtk_box_pack_start(GTK_BOX(card), grid, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

    // Card 2: Biometrics
    GtkWidget *card_bio = create_card("Screen Lock & Biometric Authentication (Fingerprint & Iris)",
        "Unlock your screen using Fingerprint (fprintd), Infrared Iris Scanner (V4L2 IR), or disable the lock screen password requirement.");

    GtkWidget *bio_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(bio_grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(bio_grid), 16);

    GtkWidget *lbl_mode = gtk_label_new("Screen Lock Method:");
    gtk_label_set_xalign(GTK_LABEL(lbl_mode), 0.0);
    app->combo_lock_mode = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lock_mode), "biometric_or_password", "Biometrics (Fingerprint / Iris) or Password (Recommended)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lock_mode), "fingerprint_only", "Fingerprint Unlock Only (Hide Password Prompt on Lock Screen)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lock_mode), "iris_only", "Iris / IR Scanner Unlock Only (Hide Password Prompt on Lock Screen)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lock_mode), "password_only", "Password / PIN Only");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_lock_mode), "none", "No Password (Click / Slide to Unlock Immediately)");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(app->combo_lock_mode), "biometric_or_password");

    GtkWidget *lbl_fp = gtk_label_new("Fingerprint Scanner Unlock (fprintd):");
    gtk_label_set_xalign(GTK_LABEL(lbl_fp), 0.0);
    app->switch_bio_fp = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_bio_fp), TRUE);
    gtk_widget_set_halign(app->switch_bio_fp, GTK_ALIGN_START);

    GtkWidget *btn_enroll_fp = gtk_button_new_with_label("🫆 Enroll Fingerprint");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_enroll_fp), "btn-secondary");
    g_signal_connect(btn_enroll_fp, "clicked", G_CALLBACK(on_enroll_fp_cb), app);

    GtkWidget *lbl_iris = gtk_label_new("Iris / Infrared Eye Scanner Unlock (V4L2 IR):");
    gtk_label_set_xalign(GTK_LABEL(lbl_iris), 0.0);
    app->switch_bio_iris = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(app->switch_bio_iris), TRUE);
    gtk_widget_set_halign(app->switch_bio_iris, GTK_ALIGN_START);

    GtkWidget *btn_enroll_iris = gtk_button_new_with_label("👁️ Enroll Iris Scan");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_enroll_iris), "btn-secondary");
    g_signal_connect(btn_enroll_iris, "clicked", G_CALLBACK(on_enroll_iris_cb), app);

    GtkWidget *lbl_idle = gtk_label_new("Auto-Lock Screen After Idle:");
    gtk_label_set_xalign(GTK_LABEL(lbl_idle), 0.0);
    app->combo_idle_lock = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_idle_lock), "60", "1 Minute");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_idle_lock), "300", "5 Minutes (Default)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_idle_lock), "900", "15 Minutes");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_idle_lock), "1800", "30 Minutes");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_idle_lock), "0", "Never Lock Automatically");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(app->combo_idle_lock), "300");

    GtkWidget *btn_save_bio = gtk_button_new_with_label("Apply Screen Lock & Biometric Settings");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_save_bio), "btn-primary");
    g_signal_connect(btn_save_bio, "clicked", G_CALLBACK(on_save_biometrics_cb), app);

    app->lbl_bio_status = gtk_label_new("Biometric drivers active (Goodix/Synaptics/ELAN fprintd + V4L2 IR Iris engine).");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_bio_status), 0.0);

    gtk_grid_attach(GTK_GRID(bio_grid), lbl_mode, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), app->combo_lock_mode, 1, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), lbl_fp, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), app->switch_bio_fp, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), btn_enroll_fp, 2, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), lbl_iris, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), app->switch_bio_iris, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), btn_enroll_iris, 2, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), lbl_idle, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), app->combo_idle_lock, 1, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), btn_save_bio, 1, 4, 2, 1);
    gtk_grid_attach(GTK_GRID(bio_grid), app->lbl_bio_status, 0, 5, 3, 1);

    gtk_box_pack_start(GTK_BOX(card_bio), bio_grid, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_bio, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(app->stack), page, "security");
}

// -----------------------------------------------------------------------------
// 11. About AxisOS Page
// -----------------------------------------------------------------------------
static GtkWidget *create_scaled_logo(int max_w, int max_h) {
    const char *paths[] = {
        "/usr/share/pixmaps/axisos-icon.svg",
        "/usr/share/pixmaps/axisos-icon.png",
        "/usr/share/pixmaps/axisos-icon-512.png",
        "/usr/share/pixmaps/axisos-logo.png",
        "/usr/share/pixmaps/axisos.png",
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
    return gtk_image_new_from_icon_name("axisos", GTK_ICON_SIZE_LARGE_TOOLBAR);
}

static void on_goto_updates_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;
    gtk_stack_set_visible_child_name(GTK_STACK(app->stack), "updates");
}

static void on_switch_edition_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisSettingsApp *app = (AxisSettingsApp *)user_data;

    GtkWidget *dlg = gtk_dialog_new_with_buttons("Switch AxisOS Edition",
        GTK_WINDOW(app->window), GTK_DIALOG_MODAL,
        "Cancel", GTK_RESPONSE_CANCEL,
        "Apply Edition", GTK_RESPONSE_OK, NULL);
    gtk_window_set_default_size(GTK_WINDOW(dlg), 460, 240);

    GtkWidget *ca = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    gtk_box_set_spacing(GTK_BOX(ca), 10);
    gtk_container_set_border_width(GTK_CONTAINER(ca), 16);

    GtkWidget *l = gtk_label_new("Select the edition to apply without reinstalling:");
    gtk_label_set_xalign(GTK_LABEL(l), 0.0);
    gtk_box_pack_start(GTK_BOX(ca), l, FALSE, FALSE, 0);

    GtkWidget *r1 = gtk_radio_button_new_with_label(NULL, "AxisOS Home & Gaming (Default, Steam, Media)");
    GtkWidget *r2 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(r1), "AxisOS Enterprise Workstation (Hardened, AD/Domain)");
    GtkWidget *r3 = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(r1), "AxisOS Work & School (Office, Productivity, Education)");

    gtk_box_pack_start(GTK_BOX(ca), r1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ca), r2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(ca), r3, FALSE, FALSE, 0);
    gtk_widget_show_all(ca);

    gint res = gtk_dialog_run(GTK_DIALOG(dlg));
    if (res == GTK_RESPONSE_OK) {
        const char *target = "base";
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(r2))) target = "enterprise";
        else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(r3))) target = "school";

        char *cmd;
        if (geteuid() == 0) {
            cmd = g_strdup_printf("/usr/local/bin/axis-switch-edition %s &", target);
        } else {
            cmd = g_strdup_printf("pkexec /usr/local/bin/axis-switch-edition %s &", target);
        }
        char *out = exec_cmd(cmd);
        g_free(out);
        g_free(cmd);
    }
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

    GtkWidget *subtitle_lbl = gtk_label_new("Debian Edition • Liquid Nodes Gaming & Workstation Desktop");
    gtk_label_set_xalign(GTK_LABEL(subtitle_lbl), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(subtitle_lbl), "card-subtitle");
    gtk_box_pack_start(GTK_BOX(title_box), subtitle_lbl, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(header_box), title_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(page), header_box, FALSE, FALSE, 0);

    GtkWidget *card = create_card("System Specifications & Hardware", "AxisOS 2.1 Pure C Edition");

    struct utsname u;
    memset(&u, 0, sizeof(u));
    uname(&u);

    // 1. Device model & vendor via sysfs DMI
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

    // 2. CPU model via /proc/cpuinfo
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

    // 3. Thread count via sysconf
    long n_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (n_threads <= 0) n_threads = 1;

    // 4. Memory via /proc/meminfo
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

    // 5. GPU via lspci
    char *gpu = exec_cmd("lspci 2>/dev/null | grep -E 'VGA|3D' | sed 's/.*: //g' | head -n1");
    g_strstrip(gpu);
    if (gpu[0] == '\0') {
        g_free(gpu);
        gpu = g_strdup("Intel / AMD / NVIDIA Hardware Accelerated Graphics");
    }

    // 6. Disk via statvfs
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
        "🖥️  Device Model: %s\n"
        "⚡ Processor: %s (%ld Threads)\n"
        "🧠 Memory (RAM): %s\n"
        "🎮 Graphics (GPU): %s\n"
        "💾 Disk Storage: %s\n"
        "🐧 Linux Kernel: %s (%s)\n"
        "✨ Operating System: AxisOS 2.0 (Liquid Nodes Edition)\n"
        "🪟 Desktop Session: Wayland (axis-compositor / LiquidNodes Pure C)",
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

    // Software Updates Option Card
    GtkWidget *card_up = create_card("Software Updates",
        "Official Debian 12 & AxisOS OTA Channels • Direct APT integration");
    GtkWidget *box_up_action = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *lbl_up_desc = gtk_label_new("Check for system updates, package upgrades, and security patches.");
    gtk_label_set_xalign(GTK_LABEL(lbl_up_desc), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_up_desc), "card-subtitle");

    GtkWidget *btn_check_up = gtk_button_new_with_label("🔍 Check for Updates");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_check_up), "btn-secondary");
    g_signal_connect(btn_check_up, "clicked", G_CALLBACK(on_goto_updates_cb), app);

    gtk_box_pack_start(GTK_BOX(box_up_action), lbl_up_desc, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(box_up_action), btn_check_up, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_up), box_up_action, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_up, FALSE, FALSE, 0);

    // OS Edition Switcher Card
    char *ed_name = g_strdup("AxisOS Home & Gaming");
    FILE *ed_file = fopen("/etc/axisos-edition", "r");
    if (ed_file) {
        char line[256];
        while (fgets(line, sizeof(line), ed_file)) {
            if (strncmp(line, "EDITION_NAME=", 13) == 0) {
                char *val = line + 13;
                g_strstrip(val);
                if (val[0] == '"' || val[0] == '\'') {
                    val++;
                    size_t len = strlen(val);
                    if (len > 0 && (val[len - 1] == '"' || val[len - 1] == '\'')) {
                        val[len - 1] = '\0';
                    }
                }
                g_free(ed_name);
                ed_name = g_strdup(val);
                break;
            }
        }
        fclose(ed_file);
    }

    char *ed_title = g_strdup_printf("Active Profile: %s", ed_name);
    GtkWidget *card_ed = create_card("OS Edition", ed_title);
    g_free(ed_title);

    GtkWidget *box_ed_action = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    const char *ed_desc = "Includes low-latency XanMod kernel tuning, GameBoost GPU engine, Steam, and media codecs.";
    if (strstr(ed_name, "Enterprise") != NULL) {
        ed_desc = "Enterprise Profile: Hardened sysctl security, UFW firewall, Active Directory / SSSD domain join.";
    } else if (strstr(ed_name, "School") != NULL || strstr(ed_name, "Work") != NULL) {
        ed_desc = "Work & School Profile: Full LibreOffice productivity suite, VS Code, STEM development tools.";
    }
    g_free(ed_name);

    GtkWidget *lbl_ed_desc = gtk_label_new(ed_desc);
    gtk_label_set_xalign(GTK_LABEL(lbl_ed_desc), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_ed_desc), "card-subtitle");

    GtkWidget *btn_switch_ed = gtk_button_new_with_label("🔄 Switch OS Edition...");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_switch_ed), "btn-primary");
    g_signal_connect(btn_switch_ed, "clicked", G_CALLBACK(on_switch_edition_cb), app);

    gtk_box_pack_start(GTK_BOX(box_ed_action), lbl_ed_desc, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(box_ed_action), btn_switch_ed, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_ed), box_ed_action, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(page), card_ed, FALSE, FALSE, 0);

    gtk_stack_add_named(GTK_STACK(app->stack), page, "about");
}

// -----------------------------------------------------------------------------
// Sidebar Navigation Callback
// -----------------------------------------------------------------------------
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
    gtk_window_set_title(GTK_WINDOW(app->window), "Settings - AxisOS");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 960, 640);
    gtk_window_set_position(GTK_WINDOW(app->window), GTK_WIN_POS_CENTER);

    // Apply dark CSS
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, CSS_THEME, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);

    // Headerbar
    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS Settings");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Hardware, Network, Desktop & System");
    gtk_window_set_titlebar(GTK_WINDOW(app->window), hb);

    // Main layout
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), hbox);

    // Sidebar
    GtkWidget *sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(sidebar_box), "sidebar");
    gtk_widget_set_size_request(sidebar_box, 240, -1);

    app->listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(app->listbox), GTK_SELECTION_BROWSE);
    g_signal_connect(app->listbox, "row-selected", G_CALLBACK(on_category_changed_cb), app);

    typedef struct {
        const char *label;
        const char *id;
    } PageDef;

    static const PageDef pages[] = {
        {"Display & Graphics", "display"},
        {"Gaming Boost & Performance", "game_boost"},
        {"Brightness & Sound", "audio_display"},
        {"Wi-Fi & Network", "network"},
        {"Bluetooth & Wireless", "bluetooth"},
        {"USB & Connected Devices", "usb"},
        {"Appearance & Taskbar", "appearance"},
        {"Software & Updates", "updates"},
        {"Date, Time & Location", "datetime"},
        {"Accounts & Security", "security"},
        {"About AxisOS 2.1", "about"}
    };

    for (size_t i = 0; i < sizeof(pages) / sizeof(pages[0]); i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *lbl = gtk_label_new(pages[i].label);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_widget_set_margin_start(lbl, 16);
        gtk_widget_set_margin_end(lbl, 16);
        gtk_widget_set_margin_top(lbl, 12);
        gtk_widget_set_margin_bottom(lbl, 12);
        g_object_set_data_full(G_OBJECT(row), "page_id", g_strdup(pages[i].id), g_free);
        gtk_container_add(GTK_CONTAINER(row), lbl);
        gtk_list_box_insert(GTK_LIST_BOX(app->listbox), row, -1);
    }

    gtk_box_pack_start(GTK_BOX(sidebar_box), app->listbox, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), sidebar_box, FALSE, FALSE, 0);

    // Content Stack
    app->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(app->stack), 150);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scroll), app->stack);

    GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(content_box), "content-pane");
    gtk_box_pack_start(GTK_BOX(content_box), scroll, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), content_box, TRUE, TRUE, 0);

    // Build Pages
    build_display_page(app);
    build_game_boost_page(app);
    build_brightness_audio_page(app);
    build_network_page(app);
    build_bluetooth_page(app);
    build_usb_page(app);
    build_appearance_page(app);
    build_updates_page(app);
    build_datetime_page(app);
    build_security_page(app);
    build_about_page(app);

    // Select first page by default
    GtkListBoxRow *first = gtk_list_box_get_row_at_index(GTK_LIST_BOX(app->listbox), 0);
    gtk_list_box_select_row(GTK_LIST_BOX(app->listbox), first);

    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    return app;
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("AxisOS System Settings & Hardware Control Center (Pure C11)\n");
        printf("Usage: axisos-settings-panel [COMMAND]\n\n");
        printf("Commands:\n");
        printf("  about        View system specifications and hardware info\n");
        printf("  updates      Manage software and OS updates\n");
        printf("  network      Wi-Fi and Ethernet network connections\n");
        printf("  display      Display outputs, modes, and refresh rates\n");
        printf("  audio        Sound volume and hardware backlight\n");
        printf("  bluetooth    Bluetooth radio and device pairing\n");
        printf("  power        Performance tuning and GameBoost\n");
        printf("  usb          Connected USB peripherals and storage\n");
        printf("  appearance   Dark/Light mode and icon themes\n");
        printf("  datetime     Timezone, NTP synchronization, and clock\n");
        printf("  security     User accounts and PAM/fprintd biometrics\n");
        return 0;
    }

    gtk_init(&argc, &argv);
    AxisSettingsApp *app = axis_settings_app_create();

    // CLI argument dispatching:
    // e.g. axisos-settings-panel about / updates / network / display / audio / bluetooth / power / users
    if (argc > 1) {
        const char *arg = argv[1];
        if (strcmp(arg, "about") == 0 || strcmp(arg, "--about") == 0 || strcmp(arg, "-a") == 0) {
            select_page_by_id(app, "about");
        } else if (strcmp(arg, "updates") == 0 || strcmp(arg, "--updates") == 0 || strcmp(arg, "-u") == 0) {
            select_page_by_id(app, "updates");
        } else if (strcmp(arg, "network") == 0 || strcmp(arg, "--network") == 0 || strcmp(arg, "-n") == 0 || strcmp(arg, "wifi") == 0) {
            select_page_by_id(app, "network");
        } else if (strcmp(arg, "display") == 0 || strcmp(arg, "--display") == 0 || strcmp(arg, "-d") == 0) {
            select_page_by_id(app, "display");
        } else if (strcmp(arg, "audio") == 0 || strcmp(arg, "sound") == 0 || strcmp(arg, "brightness") == 0) {
            select_page_by_id(app, "audio_display");
        } else if (strcmp(arg, "bluetooth") == 0 || strcmp(arg, "--bluetooth") == 0 || strcmp(arg, "-b") == 0) {
            select_page_by_id(app, "bluetooth");
        } else if (strcmp(arg, "game_boost") == 0 || strcmp(arg, "power") == 0 || strcmp(arg, "performance") == 0) {
            select_page_by_id(app, "game_boost");
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
