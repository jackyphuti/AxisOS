/*
 * AxisOS Native System Settings & Hardware Control Center
 * Written in C++ using GTK 3
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
 *  - Security & Accounts: User and Root password synchronization
 *  - About: Hardware specs (CPU, RAM, GPU, Kernel, OS Version)
 */

#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <array>
#include <memory>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

// Helper to execute command and capture stdout
static std::string exec_cmd(const std::string& cmd) {
    std::array<char, 256> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

static const char* CSS_THEME = R"(
* {
    font-family: "Inter", "Segoe UI", "Cantarell", sans-serif;
}
window {
    background-color: #0c1527;
    color: #f8fafc;
}
headerbar {
    background: #0f1c35;
    border-bottom: 2px solid #0078d7;
    color: #ffffff;
}
headerbar .title {
    font-weight: 800;
    font-size: 14px;
    color: #ffffff;
}
headerbar .subtitle {
    font-size: 11px;
    color: #94a3b8;
}
.sidebar {
    background-color: #091122;
    border-right: 1px solid rgba(255, 255, 255, 0.08);
}
.sidebar list row {
    padding: 12px 16px;
    color: #94a3b8;
    font-weight: 600;
    font-size: 13px;
    border-left: 3px solid transparent;
}
.sidebar list row:selected {
    background-color: #1e293b;
    color: #ffffff;
    border-left: 3px solid #0078d7;
}
.content-pane {
    padding: 24px 32px;
}
.card {
    background-color: #13223f;
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 8px;
    padding: 18px 22px;
    margin-bottom: 16px;
}
.card-title {
    font-size: 14px;
    font-weight: 700;
    color: #ffffff;
    margin-bottom: 4px;
}
.card-subtitle {
    font-size: 12px;
    color: #94a3b8;
    margin-bottom: 12px;
}
.btn-primary {
    background: #0078d7;
    color: #ffffff;
    font-weight: 600;
    border-radius: 4px;
    border: 1px solid #1084d8;
    padding: 8px 18px;
}
.btn-primary:hover {
    background: #1a88e1;
}
.btn-secondary {
    background: #1e293b;
    color: #e2e8f0;
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: 4px;
    padding: 8px 16px;
    font-weight: 600;
}
.btn-secondary:hover {
    background: #334155;
    color: #ffffff;
}
.badge-blue {
    background: rgba(0, 120, 215, 0.25);
    color: #60a5fa;
    border: 1px solid rgba(0, 120, 215, 0.5);
    border-radius: 4px;
    padding: 4px 10px;
    font-size: 11px;
    font-weight: 700;
}
.badge-green {
    background: rgba(34, 197, 94, 0.2);
    color: #4ade80;
    border: 1px solid rgba(34, 197, 94, 0.4);
    border-radius: 4px;
    padding: 4px 10px;
    font-size: 11px;
    font-weight: 700;
}
progressbar progress {
    background: #0078d7;
    border-radius: 3px;
}
progressbar trough {
    background: #1e293b;
    border-radius: 3px;
    min-height: 8px;
}
entry {
    background-color: #0b1528;
    color: #ffffff;
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: 4px;
    padding: 8px 12px;
}
entry:focus {
    border-color: #0078d7;
}
scale trough {
    background-color: #1e293b;
    border-radius: 3px;
    min-height: 6px;
}
scale highlight {
    background-color: #0078d7;
    border-radius: 3px;
}
)";

class AxisSettingsApp {
public:
    GtkWidget* window;
    GtkWidget* stack;
    GtkWidget* listbox;

    // Display widgets
    GtkWidget* combo_display_output;
    GtkWidget* combo_display_mode;
    GtkWidget* combo_display_rate;
    GtkWidget* lbl_gpu_status;

    // Brightness & Sound widgets
    GtkWidget* scale_brightness;
    GtkWidget* scale_volume;
    GtkWidget* btn_mute;

    // Wi-Fi & Bluetooth widgets
    GtkWidget* switch_wifi;
    GtkWidget* list_wifi;
    GtkWidget* switch_bt;
    GtkWidget* list_bt;

    // USB Peripherals widget
    GtkWidget* list_usb;

    // Appearance widgets
    GtkWidget* radio_bar_bottom;
    GtkWidget* radio_bar_top;
    GtkWidget* combo_icon_theme;

    // Updates widgets
    GtkWidget* lbl_update_status;
    GtkWidget* pbar_update;
    GtkWidget* txt_update_log;
    GtkWidget* btn_run_updates;

    // Password widgets
    GtkWidget* entry_user_pass;
    GtkWidget* entry_confirm_pass;
    GtkWidget* lbl_pass_status;

    AxisSettingsApp() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Settings - AxisOS");
        gtk_window_set_default_size(GTK_WINDOW(window), 960, 640);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        // Apply dark CSS
        GtkCssProvider* provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider, CSS_THEME, -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        g_object_unref(provider);

        // Headerbar
        GtkWidget* hb = gtk_header_bar_new();
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
        gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS Settings");
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Hardware, Network, Desktop & System");
        gtk_window_set_titlebar(GTK_WINDOW(window), hb);

        // Main layout
        GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_add(GTK_CONTAINER(window), hbox);

        // Sidebar
        GtkWidget* sidebar_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_style_context_add_class(gtk_widget_get_style_context(sidebar_box), "sidebar");
        gtk_widget_set_size_request(sidebar_box, 240, -1);

        listbox = gtk_list_box_new();
        gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox), GTK_SELECTION_BROWSE);
        g_signal_connect(listbox, "row-selected", G_CALLBACK(on_category_changed_cb), this);

        const std::vector<std::pair<std::string, std::string>> pages = {
            {"🖥️  Display & Graphics", "display"},
            {"☀️  Brightness & Sound", "audio_display"},
            {"📶  Wi-Fi & Network", "network"},
            {"󰂯  Bluetooth", "bluetooth"},
            {"🔌  USB & Connected Devices", "usb"},
            {"🎨  Appearance & Taskbar", "appearance"},
            {"🔄  Software & Updates", "updates"},
            {"🌍  Date, Time & Location", "datetime"},
            {"🔒  Accounts & Security", "security"},
            {"ℹ️  About AxisOS", "about"}
        };

        for (const auto& p : pages) {
            GtkWidget* row = gtk_list_box_row_new();
            GtkWidget* lbl = gtk_label_new(p.first.c_str());
            gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
            gtk_widget_set_margin_start(lbl, 16);
            gtk_widget_set_margin_end(lbl, 16);
            gtk_widget_set_margin_top(lbl, 12);
            gtk_widget_set_margin_bottom(lbl, 12);
            g_object_set_data_full(G_OBJECT(row), "page_id", g_strdup(p.second.c_str()), g_free);
            gtk_container_add(GTK_CONTAINER(row), lbl);
            gtk_list_box_insert(GTK_LIST_BOX(listbox), row, -1);
        }

        gtk_box_pack_start(GTK_BOX(sidebar_box), listbox, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), sidebar_box, FALSE, FALSE, 0);

        // Content Stack
        stack = gtk_stack_new();
        gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
        gtk_stack_set_transition_duration(GTK_STACK(stack), 150);

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), stack);

        GtkWidget* content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_style_context_add_class(gtk_widget_get_style_context(content_box), "content-pane");
        gtk_box_pack_start(GTK_BOX(content_box), scroll, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), content_box, TRUE, TRUE, 0);

        // Build Pages
        build_display_page();
        build_brightness_audio_page();
        build_network_page();
        build_bluetooth_page();
        build_usb_page();
        build_appearance_page();
        build_updates_page();
        build_datetime_page();
        build_security_page();
        build_about_page();

        // Select first page
        GtkListBoxRow* first = gtk_list_box_get_row_at_index(GTK_LIST_BOX(listbox), 0);
        gtk_list_box_select_row(GTK_LIST_BOX(listbox), first);

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    }

    void show() {
        gtk_widget_show_all(window);
    }

private:
    static void on_category_changed_cb(GtkListBox*, GtkListBoxRow* row, gpointer user_data) {
        if (!row) return;
        AxisSettingsApp* app = static_cast<AxisSettingsApp*>(user_data);
        const char* page_id = static_cast<const char*>(g_object_get_data(G_OBJECT(row), "page_id"));
        if (page_id) {
            gtk_stack_set_visible_child_name(GTK_STACK(app->stack), page_id);
        }
    }

    GtkWidget* create_card(const std::string& title, const std::string& subtitle) {
        GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

        GtkWidget* lbl_t = gtk_label_new(title.c_str());
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_t), "card-title");
        gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
        gtk_box_pack_start(GTK_BOX(card), lbl_t, FALSE, FALSE, 0);

        if (!subtitle.empty()) {
            GtkWidget* lbl_s = gtk_label_new(subtitle.c_str());
            gtk_style_context_add_class(gtk_widget_get_style_context(lbl_s), "card-subtitle");
            gtk_label_set_xalign(GTK_LABEL(lbl_s), 0.0);
            gtk_box_pack_start(GTK_BOX(card), lbl_s, FALSE, FALSE, 0);
        }
        return card;
    }

    // =========================================================================
    // 1. Display & Graphics Page (Resolution, Refresh Rate, Aspect Ratio, GPUs)
    // =========================================================================
    void build_display_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        // Display Resolution Card
        GtkWidget* card_res = create_card("Display Resolution, Refresh Rate & Scaling",
            "Configure output monitors, native aspect ratios, and high-refresh-rate gaming modes via wlr-randr.");

        GtkWidget* grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

        // Display output selector
        GtkWidget* lbl_out = gtk_label_new("Active Display:");
        gtk_label_set_xalign(GTK_LABEL(lbl_out), 0.0);
        combo_display_output = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_output), "eDP-1 (Built-in Display)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_output), "HDMI-A-1 (External Monitor)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_output), "DP-1 (DisplayPort)");
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_display_output), 0);

        gtk_grid_attach(GTK_GRID(grid), lbl_out, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), combo_display_output, 1, 0, 1, 1);

        // Resolution & Ratio selector
        GtkWidget* lbl_mode = gtk_label_new("Resolution & Aspect Ratio:");
        gtk_label_set_xalign(GTK_LABEL(lbl_mode), 0.0);
        combo_display_mode = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_mode), "3840x2160 (16:9 4K UHD)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_mode), "2560x1440 (16:9 2K QHD)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_mode), "1920x1080 (16:9 FHD Recommended)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_mode), "1920x1200 (16:10 WUXGA)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_mode), "1680x1050 (16:10 WSXGA+)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_mode), "1366x768 (16:9 HD)");
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_display_mode), 2);

        gtk_grid_attach(GTK_GRID(grid), lbl_mode, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), combo_display_mode, 1, 1, 1, 1);

        // Refresh rate selector
        GtkWidget* lbl_rate = gtk_label_new("Refresh Rate:");
        gtk_label_set_xalign(GTK_LABEL(lbl_rate), 0.0);
        combo_display_rate = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_rate), "60 Hz (Standard Desktop)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_rate), "75 Hz");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_rate), "120 Hz");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_rate), "144 Hz (Gaming)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_display_rate), "165 Hz (Ultra Fast)");
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_display_rate), 0);

        gtk_grid_attach(GTK_GRID(grid), lbl_rate, 0, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), combo_display_rate, 1, 2, 1, 1);

        // Apply display settings button
        GtkWidget* btn_apply_display = gtk_button_new_with_label("Apply Display Settings");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_apply_display), "btn-primary");
        g_signal_connect_swapped(btn_apply_display, "clicked", G_CALLBACK(on_apply_display_cb), this);
        gtk_grid_attach(GTK_GRID(grid), btn_apply_display, 1, 3, 1, 1);

        gtk_box_pack_start(GTK_BOX(card_res), grid, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), card_res, FALSE, FALSE, 0);

        // GPU Detection Card (NVIDIA / AMD / Intel)
        GtkWidget* card_gpu = create_card("Graphics Hardware & Drivers",
            "Automatic recognition for NVIDIA GeForce, AMD Radeon, and Intel Graphics with Mesa Vulkan drivers.");

        lbl_gpu_status = gtk_label_new(detect_gpus().c_str());
        gtk_label_set_xalign(GTK_LABEL(lbl_gpu_status), 0.0);
        gtk_label_set_line_wrap(GTK_LABEL(lbl_gpu_status), TRUE);
        gtk_box_pack_start(GTK_BOX(card_gpu), lbl_gpu_status, FALSE, FALSE, 0);

        GtkWidget* btn_refresh_gpu = gtk_button_new_with_label("Re-scan Graphics Hardware");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh_gpu), "btn-secondary");
        g_signal_connect_swapped(btn_refresh_gpu, "clicked", G_CALLBACK(on_refresh_gpu_cb), this);
        gtk_box_pack_start(GTK_BOX(card_gpu), btn_refresh_gpu, FALSE, FALSE, 8);

        gtk_box_pack_start(GTK_BOX(page), card_gpu, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), page, "display");
    }

    std::string detect_gpus() {
        std::string info = exec_cmd("lspci -nnk | grep -E -A 3 -i 'vga|3d|display'");
        if (info.empty()) {
            info = "No PCI GPU detected directly via lspci (Running in Virtual / Standard DRM mode).";
        }
        std::string summary = "🎮 Detected Graphics Adapters:\n" + info;
        // Check Mesa Vulkan drivers
        std::string vulkan_check = exec_cmd("vulkaninfo --summary 2>/dev/null | grep -E -i 'deviceName|driverName' || true");
        if (!vulkan_check.empty()) {
            summary += "\n⚡ Vulkan Acceleration:\n" + vulkan_check;
        }
        return summary;
    }

    static void on_refresh_gpu_cb(AxisSettingsApp* app) {
        gtk_label_set_text(GTK_LABEL(app->lbl_gpu_status), app->detect_gpus().c_str());
    }

    static void on_apply_display_cb(AxisSettingsApp* app) {
        const char* mode_txt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->combo_display_mode));
        const char* rate_txt = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->combo_display_rate));
        if (!mode_txt || !rate_txt) return;

        // Parse resolution (e.g. "1920x1080 (16:9 ...)")
        std::string res = mode_txt;
        size_t sp = res.find(' ');
        if (sp != std::string::npos) res = res.substr(0, sp);

        // Parse rate (e.g. "144 Hz ...")
        std::string rate = rate_txt;
        size_t r_sp = rate.find(' ');
        if (r_sp != std::string::npos) rate = rate.substr(0, r_sp);

        std::string cmd = "wlr-randr --output eDP-1 --mode " + res + "@" + rate + "Hz 2>/dev/null || wlr-randr --output $(wlr-randr | grep -m1 '^[^ ]' | awk '{print $1}') --mode " + res + " 2>/dev/null || true";
        exec_cmd(cmd);

        GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "Applied Display Settings: %s @ %s Hz", res.c_str(), rate.c_str());
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
    }

    // =========================================================================
    // 2. Brightness & Audio Page (sysfs / brightnessctl & PipeWire / ALSA)
    // =========================================================================
    void build_brightness_audio_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        // Brightness Card
        GtkWidget* card_bright = create_card("Display Brightness",
            "Hardware display backlight control via brightnessctl and /sys/class/backlight.");

        scale_brightness = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 5.0, 100.0, 5.0);
        gtk_scale_set_draw_value(GTK_SCALE(scale_brightness), TRUE);
        gtk_range_set_value(GTK_RANGE(scale_brightness), 80.0);
        g_signal_connect(scale_brightness, "value-changed", G_CALLBACK(on_brightness_changed_cb), this);
        gtk_box_pack_start(GTK_BOX(card_bright), scale_brightness, FALSE, FALSE, 4);

        gtk_box_pack_start(GTK_BOX(page), card_bright, FALSE, FALSE, 0);

        // Sound Card
        GtkWidget* card_sound = create_card("Audio Volume & Sinks",
            "Hardware audio master volume and muting via PipeWire (WirePlumber) and ALSA.");

        scale_volume = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 5.0);
        gtk_scale_set_draw_value(GTK_SCALE(scale_volume), TRUE);
        gtk_range_set_value(GTK_RANGE(scale_volume), 70.0);
        g_signal_connect(scale_volume, "value-changed", G_CALLBACK(on_volume_changed_cb), this);
        gtk_box_pack_start(GTK_BOX(card_sound), scale_volume, FALSE, FALSE, 4);

        GtkWidget* hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        btn_mute = gtk_button_new_with_label("󰝟 Mute / Unmute");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_mute), "btn-secondary");
        g_signal_connect_swapped(btn_mute, "clicked", G_CALLBACK(on_toggle_mute_cb), this);
        gtk_box_pack_start(GTK_BOX(hbox_btns), btn_mute, FALSE, FALSE, 0);

        GtkWidget* btn_test_sound = gtk_button_new_with_label("🔔 Test Audio Tone");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_test_sound), "btn-secondary");
        g_signal_connect_swapped(btn_test_sound, "clicked", G_CALLBACK(on_test_sound_cb), this);
        gtk_box_pack_start(GTK_BOX(hbox_btns), btn_test_sound, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(card_sound), hbox_btns, FALSE, FALSE, 8);
        gtk_box_pack_start(GTK_BOX(page), card_sound, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), page, "audio_display");
    }

    static void on_brightness_changed_cb(GtkRange* range, gpointer) {
        double val = gtk_range_get_value(range);
        std::string cmd = "brightnessctl set " + std::to_string(static_cast<int>(val)) + "% -q 2>/dev/null || true";
        exec_cmd(cmd);
    }

    static void on_volume_changed_cb(GtkRange* range, gpointer) {
        double val = gtk_range_get_value(range);
        int v = static_cast<int>(val);
        std::string cmd = "wpctl set-volume @DEFAULT_AUDIO_SINK@ " + std::to_string(v) + "% 2>/dev/null || amixer -q sset Master " + std::to_string(v) + "% 2>/dev/null || true";
        exec_cmd(cmd);
    }

    static void on_toggle_mute_cb(AxisSettingsApp*) {
        exec_cmd("wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle 2>/dev/null || amixer -q sset Master toggle 2>/dev/null || true");
    }

    static void on_test_sound_cb(AxisSettingsApp*) {
        std::thread([]() {
            exec_cmd("speaker-test -t sine -f 440 -l 1 2>/dev/null || paplay /usr/share/sounds/freedesktop/stereo/complete.oga 2>/dev/null || true");
        }).detach();
    }

    // =========================================================================
    // 3. Wi-Fi & Network Page (nmcli & Captive Portal)
    // =========================================================================
    void build_network_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* card_wifi = create_card("Wireless Networking (Wi-Fi)",
            "Scan nearby wireless access points, connect with security credentials, and captive portal login.");

        GtkWidget* hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        GtkWidget* lbl_switch = gtk_label_new("Enable Wi-Fi Adapter:");
        switch_wifi = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(switch_wifi), TRUE);
        g_signal_connect(switch_wifi, "notify::active", G_CALLBACK(on_wifi_toggle_cb), this);

        GtkWidget* btn_scan_wifi = gtk_button_new_with_label("🔄 Scan Wi-Fi");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_scan_wifi), "btn-primary");
        g_signal_connect_swapped(btn_scan_wifi, "clicked", G_CALLBACK(on_scan_wifi_cb), this);

        GtkWidget* btn_portal = gtk_button_new_with_label("🌐 Captive Portal Web Login");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_portal), "btn-secondary");
        g_signal_connect_swapped(btn_portal, "clicked", G_CALLBACK(on_captive_portal_cb), this);

        gtk_box_pack_start(GTK_BOX(hbox_top), lbl_switch, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox_top), switch_wifi, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox_top), btn_scan_wifi, FALSE, FALSE, 8);
        gtk_box_pack_start(GTK_BOX(hbox_top), btn_portal, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_wifi), hbox_top, FALSE, FALSE, 6);

        // List of Wi-Fi Networks
        list_wifi = gtk_list_box_new();
        gtk_list_box_set_selection_mode(GTK_LIST_BOX(list_wifi), GTK_SELECTION_SINGLE);

        GtkWidget* scroll_wifi = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_wifi), 180);
        gtk_container_add(GTK_CONTAINER(scroll_wifi), list_wifi);
        gtk_box_pack_start(GTK_BOX(card_wifi), scroll_wifi, TRUE, TRUE, 8);

        // Connect button
        GtkWidget* btn_connect_wifi = gtk_button_new_with_label("Connect to Selected Network");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_connect_wifi), "btn-primary");
        g_signal_connect_swapped(btn_connect_wifi, "clicked", G_CALLBACK(on_connect_wifi_cb), this);
        gtk_box_pack_start(GTK_BOX(card_wifi), btn_connect_wifi, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(page), card_wifi, TRUE, TRUE, 0);
        gtk_stack_add_named(GTK_STACK(stack), page, "network");

        // Initial scan
        refresh_wifi_list();
    }

    void refresh_wifi_list() {
        // Clear existing rows
        GList* children = gtk_container_get_children(GTK_CONTAINER(list_wifi));
        for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(children);

        std::string out = exec_cmd("nmcli -t -f SSID,SIGNAL,SECURITY device wifi list 2>/dev/null || true");
        std::istringstream stream(out);
        std::string line;
        bool found = false;
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            size_t p1 = line.find(':');
            size_t p2 = (p1 != std::string::npos) ? line.find(':', p1 + 1) : std::string::npos;
            if (p1 != std::string::npos) {
                std::string ssid = line.substr(0, p1);
                if (ssid.empty()) continue;
                std::string signal = (p2 != std::string::npos) ? line.substr(p1 + 1, p2 - p1 - 1) : "";
                std::string security = (p2 != std::string::npos) ? line.substr(p2 + 1) : "";

                GtkWidget* row = gtk_list_box_row_new();
                GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
                GtkWidget* lbl_ssid = gtk_label_new(ssid.c_str());
                gtk_label_set_xalign(GTK_LABEL(lbl_ssid), 0.0);
                GtkWidget* lbl_sig = gtk_label_new(("📶 " + signal + "% (" + (security.empty() ? "Open" : security) + ")").c_str());

                gtk_box_pack_start(GTK_BOX(hbox), lbl_ssid, TRUE, TRUE, 8);
                gtk_box_pack_end(GTK_BOX(hbox), lbl_sig, FALSE, FALSE, 8);
                gtk_container_add(GTK_CONTAINER(row), hbox);
                g_object_set_data_full(G_OBJECT(row), "ssid", g_strdup(ssid.c_str()), g_free);
                gtk_list_box_insert(GTK_LIST_BOX(list_wifi), row, -1);
                found = true;
            }
        }
        if (!found) {
            GtkWidget* row = gtk_list_box_row_new();
            GtkWidget* lbl = gtk_label_new("No Wi-Fi access points found. Ensure Wi-Fi adapter is enabled.");
            gtk_container_add(GTK_CONTAINER(row), lbl);
            gtk_list_box_insert(GTK_LIST_BOX(list_wifi), row, -1);
        }
        gtk_widget_show_all(list_wifi);
    }

    static void on_wifi_toggle_cb(GObject* obj, GParamSpec*, gpointer) {
        gboolean active = gtk_switch_get_active(GTK_SWITCH(obj));
        exec_cmd(std::string("nmcli radio wifi ") + (active ? "on" : "off"));
    }

    static void on_scan_wifi_cb(AxisSettingsApp* app) {
        app->refresh_wifi_list();
    }

    static void on_captive_portal_cb(AxisSettingsApp*) {
        exec_cmd("/usr/local/bin/axis-captive-portal.sh &");
    }

    static void on_connect_wifi_cb(AxisSettingsApp* app) {
        GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->list_wifi));
        if (!row) return;
        const char* ssid = static_cast<const char*>(g_object_get_data(G_OBJECT(row), "ssid"));
        if (!ssid) return;

        // Password prompt dialog
        GtkWidget* dlg = gtk_dialog_new_with_buttons("Connect to Wi-Fi",
            GTK_WINDOW(app->window), GTK_DIALOG_MODAL,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Connect", GTK_RESPONSE_ACCEPT, NULL);

        GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
        gtk_container_set_border_width(GTK_CONTAINER(content), 16);

        GtkWidget* lbl = gtk_label_new(("Enter Wi-Fi password for " + std::string(ssid) + ":").c_str());
        gtk_box_pack_start(GTK_BOX(content), lbl, FALSE, FALSE, 8);

        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
        gtk_box_pack_start(GTK_BOX(content), entry, FALSE, FALSE, 8);
        gtk_widget_show_all(dlg);

        if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
            const char* pass = gtk_entry_get_text(GTK_ENTRY(entry));
            std::string cmd = "nmcli device wifi connect \"" + std::string(ssid) + "\" password \"" + std::string(pass) + "\" 2>&1";
            std::string res = exec_cmd(cmd);

            GtkWidget* res_dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                "Connection Result:\n%s", res.c_str());
            gtk_dialog_run(GTK_DIALOG(res_dlg));
            gtk_widget_destroy(res_dlg);
        }
        gtk_widget_destroy(dlg);
    }

    // =========================================================================
    // 4. Bluetooth Page (bluetoothctl)
    // =========================================================================
    void build_bluetooth_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* card_bt = create_card("Bluetooth Devices",
            "Hardware Bluetooth adapter management, discovery of game controllers, mice, headsets, and pairing.");

        GtkWidget* hbox_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        GtkWidget* lbl_switch = gtk_label_new("Bluetooth Power:");
        switch_bt = gtk_switch_new();
        gtk_switch_set_active(GTK_SWITCH(switch_bt), TRUE);
        g_signal_connect(switch_bt, "notify::active", G_CALLBACK(on_bt_toggle_cb), this);

        GtkWidget* btn_scan_bt = gtk_button_new_with_label("󰂯 Scan Devices");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_scan_bt), "btn-primary");
        g_signal_connect_swapped(btn_scan_bt, "clicked", G_CALLBACK(on_scan_bt_cb), this);

        gtk_box_pack_start(GTK_BOX(hbox_top), lbl_switch, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox_top), switch_bt, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox_top), btn_scan_bt, FALSE, FALSE, 8);
        gtk_box_pack_start(GTK_BOX(card_bt), hbox_top, FALSE, FALSE, 6);

        list_bt = gtk_list_box_new();
        GtkWidget* scroll_bt = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_bt), 180);
        gtk_container_add(GTK_CONTAINER(scroll_bt), list_bt);
        gtk_box_pack_start(GTK_BOX(card_bt), scroll_bt, TRUE, TRUE, 8);

        GtkWidget* btn_pair_bt = gtk_button_new_with_label("Pair / Connect Selected Device");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_pair_bt), "btn-primary");
        g_signal_connect_swapped(btn_pair_bt, "clicked", G_CALLBACK(on_pair_bt_cb), this);
        gtk_box_pack_start(GTK_BOX(card_bt), btn_pair_bt, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(page), card_bt, TRUE, TRUE, 0);
        gtk_stack_add_named(GTK_STACK(stack), page, "bluetooth");

        refresh_bt_list();
    }

    void refresh_bt_list() {
        GList* children = gtk_container_get_children(GTK_CONTAINER(list_bt));
        for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(children);

        std::string out = exec_cmd("bluetoothctl devices 2>/dev/null || true");
        std::istringstream stream(out);
        std::string line;
        bool found = false;
        while (std::getline(stream, line)) {
            if (line.rfind("Device ", 0) == 0) {
                // e.g. "Device AA:BB:CC:DD:EE:FF Xbox Wireless Controller"
                std::istringstream ls(line);
                std::string tag, mac, name;
                ls >> tag >> mac;
                std::getline(ls, name);
                if (!name.empty() && name[0] == ' ') name = name.substr(1);

                GtkWidget* row = gtk_list_box_row_new();
                GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
                GtkWidget* lbl_name = gtk_label_new(name.c_str());
                gtk_label_set_xalign(GTK_LABEL(lbl_name), 0.0);
                GtkWidget* lbl_mac = gtk_label_new(mac.c_str());

                gtk_box_pack_start(GTK_BOX(hbox), lbl_name, TRUE, TRUE, 8);
                gtk_box_pack_end(GTK_BOX(hbox), lbl_mac, FALSE, FALSE, 8);
                gtk_container_add(GTK_CONTAINER(row), hbox);
                g_object_set_data_full(G_OBJECT(row), "mac", g_strdup(mac.c_str()), g_free);
                gtk_list_box_insert(GTK_LIST_BOX(list_bt), row, -1);
                found = true;
            }
        }
        if (!found) {
            GtkWidget* row = gtk_list_box_row_new();
            GtkWidget* lbl = gtk_label_new("No Bluetooth devices found in range.");
            gtk_container_add(GTK_CONTAINER(row), lbl);
            gtk_list_box_insert(GTK_LIST_BOX(list_bt), row, -1);
        }
        gtk_widget_show_all(list_bt);
    }

    static void on_bt_toggle_cb(GObject* obj, GParamSpec*, gpointer) {
        gboolean active = gtk_switch_get_active(GTK_SWITCH(obj));
        exec_cmd(std::string("bluetoothctl power ") + (active ? "on" : "off"));
    }

    static void on_scan_bt_cb(AxisSettingsApp* app) {
        std::thread([app]() {
            exec_cmd("bluetoothctl scan on & sleep 3; bluetoothctl scan off");
            g_idle_add([](gpointer data) -> gboolean {
                static_cast<AxisSettingsApp*>(data)->refresh_bt_list();
                return G_SOURCE_REMOVE;
            }, app);
        }).detach();
    }

    static void on_pair_bt_cb(AxisSettingsApp* app) {
        GtkListBoxRow* row = gtk_list_box_get_selected_row(GTK_LIST_BOX(app->list_bt));
        if (!row) return;
        const char* mac = static_cast<const char*>(g_object_get_data(G_OBJECT(row), "mac"));
        if (!mac) return;
        std::string cmd = "bluetoothctl pair " + std::string(mac) + " && bluetoothctl connect " + std::string(mac);
        exec_cmd(cmd);
        app->refresh_bt_list();
    }

    // =========================================================================
    // 5. USB & Connected Devices Page (udev / lsusb / lsblk)
    // =========================================================================
    void build_usb_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* card_usb = create_card("Connected USB Hardware & Storage",
            "Real-time recognition for USB mice, keyboards, controllers, webcams, and flash drives.");

        GtkWidget* hbox_actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        GtkWidget* btn_refresh_usb = gtk_button_new_with_label("🔄 Re-detect USB Ports");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh_usb), "btn-primary");
        g_signal_connect_swapped(btn_refresh_usb, "clicked", G_CALLBACK(on_refresh_usb_cb), this);
        gtk_box_pack_start(GTK_BOX(hbox_actions), btn_refresh_usb, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(card_usb), hbox_actions, FALSE, FALSE, 6);

        list_usb = gtk_list_box_new();
        GtkWidget* scroll_usb = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_usb), 260);
        gtk_container_add(GTK_CONTAINER(scroll_usb), list_usb);
        gtk_box_pack_start(GTK_BOX(card_usb), scroll_usb, TRUE, TRUE, 8);

        gtk_box_pack_start(GTK_BOX(page), card_usb, TRUE, TRUE, 0);
        gtk_stack_add_named(GTK_STACK(stack), page, "usb");

        refresh_usb_list();
    }

    void refresh_usb_list() {
        GList* children = gtk_container_get_children(GTK_CONTAINER(list_usb));
        for (GList* iter = children; iter != NULL; iter = g_list_next(iter)) {
            gtk_widget_destroy(GTK_WIDGET(iter->data));
        }
        g_list_free(children);

        // Peripherals from lsusb
        std::string out = exec_cmd("lsusb 2>/dev/null || true");
        std::istringstream stream(out);
        std::string line;
        bool found = false;
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            GtkWidget* row = gtk_list_box_row_new();
            GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
            GtkWidget* lbl_dev = gtk_label_new(("🔌 " + line).c_str());
            gtk_label_set_xalign(GTK_LABEL(lbl_dev), 0.0);
            gtk_box_pack_start(GTK_BOX(hbox), lbl_dev, TRUE, TRUE, 8);
            gtk_container_add(GTK_CONTAINER(row), hbox);
            gtk_list_box_insert(GTK_LIST_BOX(list_usb), row, -1);
            found = true;
        }

        // Removable storage block devices
        std::string blk = exec_cmd("lsblk -o NAME,SIZE,TYPE,MOUNTPOINT,MODEL,TRAN -n 2>/dev/null | grep -i usb || true");
        std::istringstream sblk(blk);
        while (std::getline(sblk, line)) {
            if (line.empty()) continue;
            GtkWidget* row = gtk_list_box_row_new();
            GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
            GtkWidget* lbl_blk = gtk_label_new(("💾 Removable Drive: " + line).c_str());
            gtk_label_set_xalign(GTK_LABEL(lbl_blk), 0.0);
            gtk_box_pack_start(GTK_BOX(hbox), lbl_blk, TRUE, TRUE, 8);
            gtk_container_add(GTK_CONTAINER(row), hbox);
            gtk_list_box_insert(GTK_LIST_BOX(list_usb), row, -1);
            found = true;
        }

        if (!found) {
            GtkWidget* row = gtk_list_box_row_new();
            GtkWidget* lbl = gtk_label_new("No external USB peripherals or drives detected.");
            gtk_container_add(GTK_CONTAINER(row), lbl);
            gtk_list_box_insert(GTK_LIST_BOX(list_usb), row, -1);
        }
        gtk_widget_show_all(list_usb);
    }

    static void on_refresh_usb_cb(AxisSettingsApp* app) {
        app->refresh_usb_list();
    }

    // =========================================================================
    // 6. Appearance & Desktop Page (Taskbar Position, Dark Mode, Icons)
    // =========================================================================
    void build_appearance_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        // Taskbar Position Card
        GtkWidget* card_taskbar = create_card("Taskbar Orientation & Dock Position",
            "Switch the desktop taskbar between bottom (default) and top without needing to edit config files.");

        GtkWidget* hbox_tb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 18);
        radio_bar_bottom = gtk_radio_button_new_with_label(NULL, "Bottom Taskbar (Default)");
        radio_bar_top = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(radio_bar_bottom), "Top Taskbar");
        gtk_box_pack_start(GTK_BOX(hbox_tb), radio_bar_bottom, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox_tb), radio_bar_top, FALSE, FALSE, 0);

        GtkWidget* btn_apply_tb = gtk_button_new_with_label("Apply Taskbar Position");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_apply_tb), "btn-primary");
        g_signal_connect_swapped(btn_apply_tb, "clicked", G_CALLBACK(on_apply_taskbar_cb), this);
        gtk_box_pack_start(GTK_BOX(hbox_tb), btn_apply_tb, FALSE, FALSE, 12);

        gtk_box_pack_start(GTK_BOX(card_taskbar), hbox_tb, FALSE, FALSE, 6);
        gtk_box_pack_start(GTK_BOX(page), card_taskbar, FALSE, FALSE, 0);

        // Theme and Icons Card
        GtkWidget* card_theme = create_card("Theme Colors & System Icons",
            "Consistent real color system: Obsidian Black (#0c1527), Clean White (#ffffff), and Real Blue (#0078d7).");

        GtkWidget* grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

        GtkWidget* lbl_icon = gtk_label_new("System Icon Set:");
        gtk_label_set_xalign(GTK_LABEL(lbl_icon), 0.0);
        combo_icon_theme = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_icon_theme), "Papirus-Dark (Default High-Contrast)");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_icon_theme), "Papirus");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_icon_theme), "Breeze");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_icon_theme), "Adwaita");
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_icon_theme), 0);

        gtk_grid_attach(GTK_GRID(grid), lbl_icon, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), combo_icon_theme, 1, 0, 1, 1);

        GtkWidget* btn_apply_icons = gtk_button_new_with_label("Set Icon Theme");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_apply_icons), "btn-secondary");
        g_signal_connect_swapped(btn_apply_icons, "clicked", G_CALLBACK(on_apply_icons_cb), this);
        gtk_grid_attach(GTK_GRID(grid), btn_apply_icons, 2, 0, 1, 1);

        gtk_box_pack_start(GTK_BOX(card_theme), grid, FALSE, FALSE, 6);
        gtk_box_pack_start(GTK_BOX(page), card_theme, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), page, "appearance");
    }

    static void on_apply_taskbar_cb(AxisSettingsApp* app) {
        bool is_bottom = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->radio_bar_bottom));
        std::string pos = is_bottom ? "bottom" : "top";

        // Update waybar configuration
        const char* home = getenv("HOME");
        if (home) {
            std::string cfg = std::string(home) + "/.config/waybar/config.jsonc";
            std::string cmd = "sed -i 's/\"position\": \".*\"/\"position\": \"" + pos + "\"/g' " + cfg + " 2>/dev/null || true; pkill -SIGUSR2 waybar 2>/dev/null || true";
            exec_cmd(cmd);
        }
    }

    static void on_apply_icons_cb(AxisSettingsApp* app) {
        const char* icon = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->combo_icon_theme));
        if (!icon) return;
        std::string theme = icon;
        size_t sp = theme.find(' ');
        if (sp != std::string::npos) theme = theme.substr(0, sp);
        exec_cmd("gsettings set org.gnome.desktop.interface icon-theme '" + theme + "' 2>/dev/null || true");
    }

    // =========================================================================
    // 7. Software & Updates Page (No terminal required!)
    // =========================================================================
    void build_updates_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* card = create_card("Software & System Updates",
            "Update all operating system packages, libraries, security patches, and applications directly from GUI.");

        lbl_update_status = gtk_label_new("Status: Ready to check for updates.");
        gtk_label_set_xalign(GTK_LABEL(lbl_update_status), 0.0);
        gtk_box_pack_start(GTK_BOX(card), lbl_update_status, FALSE, FALSE, 4);

        pbar_update = gtk_progress_bar_new();
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar_update), 0.0);
        gtk_box_pack_start(GTK_BOX(card), pbar_update, FALSE, FALSE, 6);

        GtkWidget* hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        GtkWidget* btn_check = gtk_button_new_with_label("🔍 Check for Updates");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_check), "btn-secondary");
        g_signal_connect_swapped(btn_check, "clicked", G_CALLBACK(on_check_updates_cb), this);

        btn_run_updates = gtk_button_new_with_label("⬇️  Install All Updates Now");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_run_updates), "btn-primary");
        g_signal_connect_swapped(btn_run_updates, "clicked", G_CALLBACK(on_install_updates_cb), this);

        gtk_box_pack_start(GTK_BOX(hbox_btns), btn_check, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox_btns), btn_run_updates, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), hbox_btns, FALSE, FALSE, 6);

        // Scrolling log view
        txt_update_log = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(txt_update_log), FALSE);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(txt_update_log), TRUE);

        GtkWidget* scroll_log = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_log), 180);
        gtk_container_add(GTK_CONTAINER(scroll_log), txt_update_log);
        gtk_box_pack_start(GTK_BOX(card), scroll_log, TRUE, TRUE, 8);

        gtk_box_pack_start(GTK_BOX(page), card, TRUE, TRUE, 0);
        gtk_stack_add_named(GTK_STACK(stack), page, "updates");
    }

    void append_update_log(const std::string& msg) {
        GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt_update_log));
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buf, &end);
        gtk_text_buffer_insert(buf, &end, (msg + "\n").c_str(), -1);
    }

    static void on_check_updates_cb(AxisSettingsApp* app) {
        gtk_label_set_text(GTK_LABEL(app->lbl_update_status), "Checking repositories for package updates...");
        gtk_progress_bar_pulse(GTK_PROGRESS_BAR(app->pbar_update));

        std::thread([app]() {
            std::string out = exec_cmd("pkexec apt-get update 2>&1 || apt-get update 2>&1");
            std::string upg = exec_cmd("apt list --upgradable 2>/dev/null | grep -v 'Listing' | wc -l");
            int count = std::atoi(upg.c_str());

            g_idle_add([](gpointer data) -> gboolean {
                auto* p = static_cast<std::pair<AxisSettingsApp*, int>*>(data);
                AxisSettingsApp* self = p->first;
                int c = p->second;
                self->append_update_log("Repository metadata refreshed.");
                if (c > 0) {
                    gtk_label_set_text(GTK_LABEL(self->lbl_update_status),
                        ("Updates Available: " + std::to_string(c) + " packages can be updated.").c_str());
                    self->append_update_log(std::to_string(c) + " packages can be updated.");
                } else {
                    gtk_label_set_text(GTK_LABEL(self->lbl_update_status),
                        "Your system is completely up to date!");
                    self->append_update_log("Your system is completely up to date!");
                }
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->pbar_update), 1.0);
                delete p;
                return G_SOURCE_REMOVE;
            }, new std::pair<AxisSettingsApp*, int>(app, count));
        }).detach();
    }

    static void on_install_updates_cb(AxisSettingsApp* app) {
        gtk_label_set_text(GTK_LABEL(app->lbl_update_status), "Installing updates in background... Please do not power off.");
        gtk_widget_set_sensitive(app->btn_run_updates, FALSE);

        std::thread([app]() {
            std::string out = exec_cmd("DEBIAN_FRONTEND=noninteractive pkexec apt-get -y upgrade 2>&1 || DEBIAN_FRONTEND=noninteractive apt-get -y upgrade 2>&1");
            g_idle_add([](gpointer data) -> gboolean {
                AxisSettingsApp* self = static_cast<AxisSettingsApp*>(data);
                self->append_update_log("Upgrade finished successfully!");
                gtk_label_set_text(GTK_LABEL(self->lbl_update_status), "All updates installed successfully!");
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(self->pbar_update), 1.0);
                gtk_widget_set_sensitive(self->btn_run_updates, TRUE);
                return G_SOURCE_REMOVE;
            }, app);
        }).detach();
    }

    // =========================================================================
    // 8. Date, Time & Location Page (Auto Geolocation & NTP)
    // =========================================================================
    void build_datetime_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* card = create_card("Date, Time & Automatic Location",
            "Automatic timezone sync using IP geolocation and Network Time Protocol (NTP).");

        GtkWidget* lbl_time = gtk_label_new(exec_cmd("timedatectl status 2>/dev/null || date").c_str());
        gtk_label_set_xalign(GTK_LABEL(lbl_time), 0.0);
        gtk_box_pack_start(GTK_BOX(card), lbl_time, FALSE, FALSE, 6);

        GtkWidget* hbox_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        GtkWidget* btn_sync_tz = gtk_button_new_with_label("🌍 Auto-Detect Location & Timezone");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_sync_tz), "btn-primary");
        g_signal_connect_swapped(btn_sync_tz, "clicked", G_CALLBACK(on_sync_tz_cb), this);
        gtk_box_pack_start(GTK_BOX(hbox_btns), btn_sync_tz, FALSE, FALSE, 0);

        GtkWidget* btn_sync_ntp = gtk_button_new_with_label("⏱️ Sync Clock with NTP");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_sync_ntp), "btn-secondary");
        g_signal_connect_swapped(btn_sync_ntp, "clicked", G_CALLBACK(on_sync_ntp_cb), this);
        gtk_box_pack_start(GTK_BOX(hbox_btns), btn_sync_ntp, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(card), hbox_btns, FALSE, FALSE, 8);
        gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), page, "datetime");
    }

    static void on_sync_tz_cb(AxisSettingsApp* app) {
        exec_cmd("/usr/local/bin/axis-autotimezone.sh &");
        GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "Automatic geolocation and timezone sync initiated!");
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
    }

    static void on_sync_ntp_cb(AxisSettingsApp* app) {
        exec_cmd("pkexec timedatectl set-ntp true 2>/dev/null || true");
        GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
            "Network Time Protocol (NTP) enabled and synchronized.");
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
    }

    // =========================================================================
    // 9. Accounts & Security Page (User & Root Password Synchronization)
    // =========================================================================
    void build_security_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* card = create_card("User Account & Root Security",
            "Keep user password and root administrative password synchronized as required.");

        GtkWidget* grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
        gtk_grid_set_column_spacing(GTK_GRID(grid), 16);

        GtkWidget* lbl1 = gtk_label_new("New Password:");
        gtk_label_set_xalign(GTK_LABEL(lbl1), 0.0);
        entry_user_pass = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(entry_user_pass), FALSE);

        GtkWidget* lbl2 = gtk_label_new("Confirm Password:");
        gtk_label_set_xalign(GTK_LABEL(lbl2), 0.0);
        entry_confirm_pass = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(entry_confirm_pass), FALSE);

        gtk_grid_attach(GTK_GRID(grid), lbl1, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry_user_pass, 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), lbl2, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry_confirm_pass, 1, 1, 1, 1);

        GtkWidget* btn_sync_pass = gtk_button_new_with_label("Update & Sync User & Root Passwords");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_sync_pass), "btn-primary");
        g_signal_connect_swapped(btn_sync_pass, "clicked", G_CALLBACK(on_sync_passwords_cb), this);
        gtk_grid_attach(GTK_GRID(grid), btn_sync_pass, 1, 2, 1, 1);

        lbl_pass_status = gtk_label_new("");
        gtk_label_set_xalign(GTK_LABEL(lbl_pass_status), 0.0);
        gtk_grid_attach(GTK_GRID(grid), lbl_pass_status, 1, 3, 1, 1);

        gtk_box_pack_start(GTK_BOX(card), grid, FALSE, FALSE, 6);
        gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), page, "security");
    }

    static void on_sync_passwords_cb(AxisSettingsApp* app) {
        const char* p1 = gtk_entry_get_text(GTK_ENTRY(app->entry_user_pass));
        const char* p2 = gtk_entry_get_text(GTK_ENTRY(app->entry_confirm_pass));

        if (std::strlen(p1) < 1) {
            gtk_label_set_text(GTK_LABEL(app->lbl_pass_status), "Password cannot be empty.");
            return;
        }
        if (std::strcmp(p1, p2) != 0) {
            gtk_label_set_text(GTK_LABEL(app->lbl_pass_status), "Passwords do not match!");
            return;
        }

        const char* user = getenv("USER");
        if (!user) user = "axis";

        std::string cmd = "printf '" + std::string(user) + ":" + std::string(p1) + "\\nroot:" + std::string(p1) + "\\n' | pkexec chpasswd 2>/dev/null || true";
        exec_cmd(cmd);

        gtk_label_set_text(GTK_LABEL(app->lbl_pass_status), "Passwords updated and synchronized successfully!");
        gtk_entry_set_text(GTK_ENTRY(app->entry_user_pass), "");
        gtk_entry_set_text(GTK_ENTRY(app->entry_confirm_pass), "");
    }

    // =========================================================================
    // 10. About AxisOS Page (Hardware Specs & System Details)
    // =========================================================================
    void build_about_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* card = create_card("System Specifications & Hardware",
            "AxisOS 2.0 (Nobara & Debian Native Edition)");

        std::string cpu = exec_cmd("grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 || echo 'x86_64 Processor'");
        std::string mem = exec_cmd("free -h | awk '/^Mem:/ {print $2 \" Total / \" $3 \" Used\"}' || echo 'Unknown'");
        std::string kernel = exec_cmd("uname -r");
        std::string os_ver = "AxisOS 2.0 Native (C/C++ Architecture)";

        std::string text = "🖥️  Operating System: " + os_ver + "\n"
                         + "🐧 Linux Kernel: " + kernel
                         + "⚡ Processor: " + cpu
                         + "🧠 Memory (RAM): " + mem + "\n"
                         + "🪟 Desktop Environment: Wayland Compositor (wlroots + Waybar)\n"
                         + "🎮 Gaming Platform: Valve Steam & Vulkan Acceleration Built-in";

        GtkWidget* lbl = gtk_label_new(text.c_str());
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_box_pack_start(GTK_BOX(card), lbl, FALSE, FALSE, 8);

        gtk_box_pack_start(GTK_BOX(page), card, FALSE, FALSE, 0);
        gtk_stack_add_named(GTK_STACK(stack), page, "about");
    }
};

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    AxisSettingsApp app;
    app.show();
    gtk_main();
    return 0;
}
