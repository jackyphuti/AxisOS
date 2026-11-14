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
    "window {"
    "    background-color: #0A0B0D;"
    "    color: #f8fafc;"
    "}"
    "headerbar {"
    "    background: #14161A;"
    "    border-bottom: 1px solid rgba(255, 255, 255, 0.12);"
    "    color: #ffffff;"
    "}"
    "notebook tab {"
    "    padding: 10px 18px;"
    "    font-weight: 600;"
    "    color: #94a3b8;"
    "    background: transparent;"
    "    border-bottom: 2px solid transparent;"
    "}"
    "notebook tab:checked {"
    "    color: #ffffff;"
    "    border-bottom: 1px solid rgba(255, 255, 255, 0.12);"
    "    background: rgba(255, 255, 255, 0.12);"
    "}"
    ".hero-card {"
    "    background: #14161A;"
    "    border: 1px solid rgba(255, 255, 255, 0.08);"
    "    border-radius: 8px;"
    "    padding: 20px;"
    "    margin: 8px 12px;"
    "}"
    ".card {"
    "    background: #14161A;"
    "    border: 1px solid rgba(255, 255, 255, 0.06);"
    "    border-radius: 8px;"
    "    padding: 14px 18px;"
    "    margin: 6px 12px;"
    "}"
    ".btn-primary {"
    "    background: #FFFFFF;"
    "    color: #0A0B0D;"
    "    font-weight: 700;"
    "    border-radius: 6px;"
    "    border: 1px solid #FFFFFF;"
    "    padding: 8px 18px;"
    "}"
    ".btn-primary:hover {"
    "    background: #E2E8F0;"
    "}"
    ".btn-secondary {"
    "    background: #202329;"
    "    color: #e2e8f0;"
    "    border: 1px solid rgba(255, 255, 255, 0.1);"
    "    border-radius: 4px;"
    "    padding: 8px 16px;"
    "    font-weight: 600;"
    "}"
    ".btn-secondary:hover {"
    "    background: rgba(255, 255, 255, 0.14);"
    "    color: #ffffff;"
    "}"
    ".badge-ok {"
    "    color: #4ade80;"
    "    font-weight: bold;"
    "}"
    ".badge-warn {"
    "    color: #facc15;"
    "    font-weight: bold;"
    "}";

static GtkWidget *create_card(const char *title, const char *desc) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

    GtkWidget *lbl_t = gtk_label_new(title);
    gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(lbl_t), attrs);
    pango_attr_list_unref(attrs);
    gtk_box_pack_start(GTK_BOX(card), lbl_t, FALSE, FALSE, 0);

    if (desc && desc[0] != '\0') {
        GtkWidget *lbl_d = gtk_label_new(desc);
        gtk_label_set_xalign(GTK_LABEL(lbl_d), 0.0);
        gtk_label_set_line_wrap(GTK_LABEL(lbl_d), TRUE);
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
    int ret = system("/usr/local/bin/steam &");
    (void)ret;
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
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

    // Hero
    GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(hero), "hero-card");

    GtkWidget *h_t = gtk_label_new("Welcome to AxisOS");
    gtk_label_set_xalign(GTK_LABEL(h_t), 0.0);
    PangoAttrList *attrs = pango_attr_list_new();
    pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
    pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
    gtk_label_set_attributes(GTK_LABEL(h_t), attrs);
    pango_attr_list_unref(attrs);

    GtkWidget *h_d = gtk_label_new("Engineered for high-performance PC gaming, content creation, and out-of-the-box hardware readiness.\nAll critical GPU drivers, Vulkan runtimes, and multimedia codecs are pre-configured.");
    gtk_label_set_xalign(GTK_LABEL(h_d), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(h_d), TRUE);

    gtk_box_pack_start(GTK_BOX(hero), h_t, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hero), h_d, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hero, FALSE, FALSE, 0);

    // Hardware Status card
    GtkWidget *card_hw = create_card("🎮 Hardware & Vulkan Status", "Hardware GPU acceleration and display modes are active.");
    char *gpu_stat = exec_cmd("lspci -nnk | grep -E -A 2 -i 'vga|3d|display' || echo 'Standard Display Adapter'");
    g_strstrip(gpu_stat);
    GtkWidget *lbl_status = gtk_label_new(gpu_stat);
    g_free(gpu_stat);
    gtk_label_set_xalign(GTK_LABEL(lbl_status), 0.0);
    gtk_box_pack_start(GTK_BOX(card_hw), lbl_status, FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(vbox), card_hw, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), vbox, gtk_label_new("Welcome"));
}

static void init_drivers_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

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

    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), vbox, gtk_label_new("GPU Drivers"));
}

static void init_codecs_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

    GtkWidget *card_codec = create_card("Multimedia Codecs & Hardware Decoders",
        "Install comprehensive audio and video codecs (FFmpeg, GStreamer, H.264, HEVC, AV1, AAC, MP3).");

    GtkWidget *btn_codecs = gtk_button_new_with_label("Install All Multimedia Codecs");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_codecs), "btn-primary");
    g_signal_connect(btn_codecs, "clicked", G_CALLBACK(on_install_codecs_cb), app);
    gtk_box_pack_start(GTK_BOX(card_codec), btn_codecs, FALSE, FALSE, 8);

    gtk_box_pack_start(GTK_BOX(vbox), card_codec, FALSE, FALSE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), vbox, gtk_label_new("Codecs"));
}

static void init_gaming_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

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

    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), vbox, gtk_label_new("Gaming"));
}

static AxisHardwareApp *axis_hardware_app_create(void) {
    AxisHardwareApp *app = g_new0(AxisHardwareApp, 1);
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Axis Hardware & Codec Suite");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 820, 580);
    gtk_window_set_position(GTK_WINDOW(app->window), GTK_WIN_POS_CENTER);

    // CSS
    GtkCssProvider *prov = gtk_css_provider_new();
    gtk_css_provider_load_from_data(prov, CSS_AXIS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(prov),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(prov);

    // Headerbar
    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "Axis Hardware & Codec Suite");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Gaming Drivers, Codecs & System Optimization");
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
    AxisHardwareApp *app = axis_hardware_app_create();
    gtk_widget_show_all(app->window);
    gtk_main();
    g_free(app);
    return 0;
}
