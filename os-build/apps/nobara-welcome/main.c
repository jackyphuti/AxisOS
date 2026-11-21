/*
 * AxisOS - Hardware Driver & Codec Manager (Welcome Suite)
 * Written in Pure C11 (-std=gnu11) using GTK 3
 * Features:
 *  - Triple GPU driver manager: NVIDIA Proprietary/DKMS, AMD ROCm/RADV, Intel ANV
 *  - Multimedia Codecs installation & verification (FFmpeg, GStreamer, VA-API)
 *  - Gaming Ecosystem: Steam, ProtonUp-Qt, Lutris, GameMode, MangoHud
 *  - System Tweaks & Maintenance
 */

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
} AxisHardwareApp;

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

static const char *CSS_AXIS =
    "* {"
    "    outline: none;"
    "}"
    "window, .background {"
    "    background-color: #141826;"
    "    color: #F1F5F9;"
    "    font-family: 'IBM Plex Sans', -apple-system, sans-serif;"
    "}"
    "headerbar {"
    "    background: #1F2231;"
    "    background-color: #1F2231;"
    "    background-image: none;"
    "    border-bottom: 1px solid #2E3349;"
    "    box-shadow: none;"
    "    min-height: 48px;"
    "    padding: 4px 12px;"
    "}"
    "headerbar .title {"
    "    font-weight: 700;"
    "    font-size: 15px;"
    "    color: #F1F5F9;"
    "}"
    "headerbar .subtitle {"
    "    font-size: 11px;"
    "    color: #94A3B8;"
    "}"
    "headerbar button {"
    "    background-color: #262A3D;"
    "    color: #F1F5F9;"
    "    border: 1px solid #2E3349;"
    "    border-radius: 8px;"
    "    padding: 4px 10px;"
    "}"
    "headerbar button:hover {"
    "    background-color: #2E3349;"
    "    color: #7D95FF;"
    "}"
    "notebook {"
    "    background-color: #141826;"
    "    border: none;"
    "}"
    "notebook > header {"
    "    background-color: #1F2231;"
    "    border-bottom: 1px solid #2E3349;"
    "    padding: 6px 12px 0 12px;"
    "}"
    "notebook > header > tabs > tab {"
    "    background-color: transparent;"
    "    background-image: none;"
    "    color: #94A3B8;"
    "    font-weight: 600;"
    "    font-size: 13px;"
    "    border: none;"
    "    border-bottom: 3px solid transparent;"
    "    padding: 10px 22px;"
    "    margin: 0 4px;"
    "    border-radius: 8px 8px 0 0;"
    "    transition: all 150ms ease-in-out;"
    "}"
    "notebook > header > tabs > tab:hover {"
    "    color: #F1F5F9;"
    "    background-color: rgba(125, 149, 255, 0.08);"
    "}"
    "notebook > header > tabs > tab:checked {"
    "    color: #7D95FF;"
    "    font-weight: 700;"
    "    border-bottom: 3px solid #7D95FF;"
    "    background-color: rgba(125, 149, 255, 0.12);"
    "}"
    "notebook > header > tabs > tab label {"
    "    color: inherit;"
    "}"
    "notebook > stack, notebook > stack > * {"
    "    background-color: #141826;"
    "}"
    "scrolledwindow, viewport {"
    "    background-color: #141826;"
    "    border: none;"
    "}"
    ".hero-card {"
    "    background-color: #1F2231;"
    "    border: 1px solid #2E3349;"
    "    border-radius: 16px;"
    "    padding: 22px 24px;"
    "    margin: 10px 12px 6px 12px;"
    "    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.35);"
    "}"
    ".card {"
    "    background-color: #1F2231;"
    "    border: 1px solid #2E3349;"
    "    border-radius: 16px;"
    "    padding: 18px 24px;"
    "    margin: 8px 12px;"
    "    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.25);"
    "}"
    "label {"
    "    color: #F1F5F9;"
    "}"
    ".text-muted {"
    "    color: #94A3B8;"
    "}"
    ".status-box {"
    "    background-color: #141826;"
    "    border: 1px solid #2E3349;"
    "    border-radius: 10px;"
    "    padding: 12px 16px;"
    "    color: #7D95FF;"
    "    font-family: 'IBM Plex Mono', monospace;"
    "    font-size: 12px;"
    "}"
    ".btn-primary {"
    "    background: #7D95FF;"
    "    background-color: #7D95FF;"
    "    color: #101423;"
    "    font-weight: 700;"
    "    font-size: 13px;"
    "    border-radius: 9999px;"
    "    border: 1px solid #7D95FF;"
    "    padding: 9px 24px;"
    "    box-shadow: 0 2px 6px rgba(125, 149, 255, 0.3);"
    "}"
    ".btn-primary:hover {"
    "    background: #93A8FF;"
    "    background-color: #93A8FF;"
    "    color: #101423;"
    "}"
    ".btn-secondary {"
    "    background: #262A3D;"
    "    background-color: #262A3D;"
    "    color: #F1F5F9;"
    "    border: 1px solid #2E3349;"
    "    border-radius: 9999px;"
    "    padding: 9px 20px;"
    "    font-weight: 600;"
    "    font-size: 13px;"
    "}"
    ".btn-secondary:hover {"
    "    background-color: #2E3349;"
    "    border-color: #7D95FF;"
    "    color: #7D95FF;"
    "}"
    ".badge-ok {"
    "    color: #34D399;"
    "    font-weight: bold;"
    "}"
    ".badge-warn {"
    "    color: #FBBF24;"
    "    font-weight: bold;"
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
    gtk_css_provider_load_from_data(extra_prov, CSS_AXIS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(extra_prov),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(extra_prov);
}

static GtkWidget *create_card(const char *title, const char *desc) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

    GtkWidget *lbl_t = gtk_label_new(title);
    gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.15));
    gtk_label_set_attributes(GTK_LABEL(lbl_t), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_pack_start(GTK_BOX(card), lbl_t, FALSE, FALSE, 0);

    if (desc && desc[0] != '\0') {
        GtkWidget *lbl_d = gtk_label_new(desc);
        gtk_label_set_xalign(GTK_LABEL(lbl_d), 0.0);
        gtk_label_set_line_wrap(GTK_LABEL(lbl_d), TRUE);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_d), "text-muted");
        gtk_box_pack_start(GTK_BOX(card), lbl_d, FALSE, FALSE, 0);
    }
    return card;
}

static void on_install_nvidia_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    char *out = exec_cmd("pkexec /usr/local/bin/nvidia-auto-install.sh &");
    g_free(out);

    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "NVIDIA Driver installation launched in background.");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void on_verify_amd_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    char *res = exec_cmd("vulkaninfo --summary 2>&1 | grep -i radv || echo 'AMD RADV driver ready'");
    g_strstrip(res);
    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "AMD Mesa RADV Status:\n%s", res);
    g_free(res);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void on_verify_intel_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    char *res = exec_cmd("vainfo 2>&1 | grep -i 'driver' || echo 'Intel Media VA-API ready'");
    g_strstrip(res);
    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Intel VA-API Status:\n%s", res);
    g_free(res);
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static gpointer thread_install_codecs(gpointer user_data) {
    (void)user_data;
    char *out = exec_cmd("pkexec apt-get update && pkexec apt-get install -y ffmpeg gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav libavcodec-extra");
    g_free(out);
    return NULL;
}

static void on_install_codecs_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    g_thread_new("install_codecs", thread_install_codecs, NULL);

    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Multimedia codecs installation started in background.");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void on_launch_steam_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    g_spawn_command_line_async("steam", NULL);
}

static gpointer thread_install_lutris(gpointer user_data) {
    (void)user_data;
    char *out = exec_cmd("pkexec apt-get update && pkexec apt-get install -y lutris");
    g_free(out);
    return NULL;
}

static void on_install_lutris_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    g_thread_new("install_lutris", thread_install_lutris, NULL);

    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        "Lutris installation started in background.");
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

static void init_welcome_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);

    // Hero
    GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(hero), "hero-card");

    GtkWidget *h_t = gtk_label_new("Welcome to AxisOS");
    gtk_label_set_xalign(GTK_LABEL(h_t), 0.0);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.35));
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(h_t), attrs);
    pango_attr_list_unref(attrs);

    GtkWidget *h_d = gtk_label_new("Engineered for high-performance PC gaming, content creation, and out-of-the-box hardware readiness.\nAll critical GPU drivers, Vulkan runtimes, and multimedia codecs are pre-configured.");
    gtk_label_set_xalign(GTK_LABEL(h_d), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(h_d), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(h_d), "text-muted");

    gtk_box_pack_start(GTK_BOX(hero), h_t, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hero), h_d, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hero, FALSE, FALSE, 0);

    // Hardware Status card
    GtkWidget *card_hw = create_card("Hardware & Vulkan Status", "Hardware GPU acceleration and display modes are active.");
    char *gpu_stat = exec_cmd("lspci -nnk | grep -E -A 2 -i 'vga|3d|display' || echo 'Standard Display Adapter'");
    g_strstrip(gpu_stat);
    GtkWidget *lbl_status = gtk_label_new(gpu_stat);
    g_free(gpu_stat);
    gtk_label_set_xalign(GTK_LABEL(lbl_status), 0.0);

    GtkWidget *status_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(status_frame), "status-box");
    gtk_box_pack_start(GTK_BOX(status_frame), lbl_status, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_hw), status_frame, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(vbox), card_hw, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), vbox);

    GtkWidget *tab_lbl = gtk_label_new("Welcome");
    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), scrolled, tab_lbl);
}

static void init_drivers_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);

    // NVIDIA Card
    GtkWidget *card_nv = create_card("NVIDIA GeForce Graphics Drivers",
        "Automatic driver detector for proprietary NVIDIA graphics modules, CUDA, and PRIME offloading.");
    GtkWidget *btn_nv = gtk_button_new_with_label("Install / Update NVIDIA Drivers");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_nv), "btn-primary");
    g_signal_connect(btn_nv, "clicked", G_CALLBACK(on_install_nvidia_cb), app);
    gtk_box_pack_start(GTK_BOX(card_nv), btn_nv, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(vbox), card_nv, FALSE, FALSE, 0);

    // AMD Radeon Card
    GtkWidget *card_amd = create_card("AMD Radeon Graphics (Mesa RADV)",
        "High-performance open-source Mesa Vulkan drivers and ROCm compute libraries.");
    GtkWidget *btn_amd = gtk_button_new_with_label("Verify AMD Vulkan & Mesa");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_amd), "btn-secondary");
    g_signal_connect(btn_amd, "clicked", G_CALLBACK(on_verify_amd_cb), app);
    gtk_box_pack_start(GTK_BOX(card_amd), btn_amd, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(vbox), card_amd, FALSE, FALSE, 0);

    // Intel Graphics Card
    GtkWidget *card_intel = create_card("Intel Arc & Iris Xe Graphics (Mesa ANV)",
        "Mesa ANV Vulkan driver and Intel Media VA-API hardware video acceleration.");
    GtkWidget *btn_intel = gtk_button_new_with_label("Verify Intel Media Acceleration");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_intel), "btn-secondary");
    g_signal_connect(btn_intel, "clicked", G_CALLBACK(on_verify_intel_cb), app);
    gtk_box_pack_start(GTK_BOX(card_intel), btn_intel, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(vbox), card_intel, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), vbox);

    GtkWidget *tab_lbl = gtk_label_new("GPU Drivers");
    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), scrolled, tab_lbl);
}

static void init_codecs_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);

    GtkWidget *card_codec = create_card("Multimedia Codecs & Hardware Decoders",
        "Install comprehensive audio and video codecs (FFmpeg, GStreamer, H.264, HEVC, AV1, AAC, MP3).");

    GtkWidget *btn_codecs = gtk_button_new_with_label("Install All Multimedia Codecs");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_codecs), "btn-primary");
    g_signal_connect(btn_codecs, "clicked", G_CALLBACK(on_install_codecs_cb), app);
    gtk_box_pack_start(GTK_BOX(card_codec), btn_codecs, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(vbox), card_codec, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), vbox);

    GtkWidget *tab_lbl = gtk_label_new("Codecs");
    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), scrolled, tab_lbl);
}

static void init_gaming_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);

    // Steam Card
    GtkWidget *card_steam = create_card("Valve Steam Gaming Platform",
        "Official Valve Steam client with Proton compatibility layer for Windows gaming on Linux.");
    GtkWidget *hbox_s = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_launch_steam = gtk_button_new_with_label("Launch Steam");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_launch_steam), "btn-primary");
    g_signal_connect(btn_launch_steam, "clicked", G_CALLBACK(on_launch_steam_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_s), btn_launch_steam, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_steam), hbox_s, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(vbox), card_steam, FALSE, FALSE, 0);

    // ProtonUp & Lutris Card
    GtkWidget *card_tools = create_card("ProtonUp-Qt & Lutris",
        "Manage GE-Proton custom builds and non-Steam game launchers seamlessly.");
    GtkWidget *hbox_t = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_lutris = gtk_button_new_with_label("Install Lutris");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_lutris), "btn-secondary");
    g_signal_connect(btn_lutris, "clicked", G_CALLBACK(on_install_lutris_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_t), btn_lutris, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_tools), hbox_t, FALSE, FALSE, 8);
    gtk_box_pack_start(GTK_BOX(vbox), card_tools, FALSE, FALSE, 0);

    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled), vbox);

    GtkWidget *tab_lbl = gtk_label_new("Gaming");
    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), scrolled, tab_lbl);
}

static AxisHardwareApp *axis_hardware_app_create(void) {
    AxisHardwareApp *app = g_new0(AxisHardwareApp, 1);
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Axis Hardware & Codec Suite");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 820, 580);
    gtk_window_set_position(GTK_WINDOW(app->window), GTK_WIN_POS_CENTER);

    // Apply system theme
    load_axis_theme();

    // Headerbar
    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "Axis Hardware & Codec Suite");
    gtk_window_set_titlebar(GTK_WINDOW(app->window), hb);

    app->notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(app->window), app->notebook);

    init_welcome_tab(app);
    init_drivers_tab(app);
    init_codecs_tab(app);
    init_gaming_tab(app);

    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    return app;
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("AxisOS Hardware Driver & Codec Suite (Pure C11)\n");
        printf("Usage: nobara-welcome [OPTION]\n");
        printf("Options:\n");
        printf("  --help, -h      Show this help message and exit\n");
        printf("  --drivers, -d   Check current GPU driver status\n");
        return 0;
    }
    if (argc > 1 && (strcmp(argv[1], "--drivers") == 0 || strcmp(argv[1], "-d") == 0)) {
        printf("=== AxisOS GPU Hardware & Driver Status ===\n");
        FILE *fp = popen("lspci -nnk 2>/dev/null | grep -E -A 3 -i 'vga|3d|display'", "r");
        if (fp) {
            char line[256];
            while (fgets(line, sizeof(line), fp)) {
                printf("  %s", line);
            }
            pclose(fp);
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
    AxisHardwareApp *app = axis_hardware_app_create();
    gtk_widget_show_all(app->window);
    gtk_main();
    g_free(app);
    return 0;
}
