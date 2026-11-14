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
    "window {"
    "    background-color: #0A0B0D;"
    "    color: #F8FAFC;"
    "}"
    "headerbar {"
    "    background: #14161A;"
    "    border-bottom: 1px solid rgba(255, 255, 255, 0.12);"
    "    color: #FFFFFF;"
    "}"
    ".card {"
    "    background: #14161A;"
    "    border: 1px solid rgba(255, 255, 255, 0.10);"
    "    border-radius: 8px;"
    "    padding: 20px;"
    "    margin: 8px 16px;"
    "}"
    ".btn-primary {"
    "    background: #FFFFFF;"
    "    color: #0A0B0D;"
    "    font-weight: 700;"
    "    border-radius: 6px;"
    "    border: 1px solid #FFFFFF;"
    "    padding: 10px 24px;"
    "}"
    ".btn-primary:hover {"
    "    background: #E2E8F0;"
    "}"
    ".btn-secondary {"
    "    background: #202329;"
    "    color: #F8FAFC;"
    "    border: 1px solid rgba(255, 255, 255, 0.15);"
    "    border-radius: 6px;"
    "    padding: 8px 18px;"
    "    font-weight: 600;"
    "}"
    ".btn-secondary:hover {"
    "    background: rgba(255, 255, 255, 0.14);"
    "    color: #FFFFFF;"
    "}";

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

static void on_goto_net_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisOobeApp *app = (AxisOobeApp *)user_data;
    gtk_stack_set_visible_child_name(GTK_STACK(app->stack), "step_net");
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

static void on_finish_cb(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisOobeApp *app = (AxisOobeApp *)user_data;
    unlink("/var/lib/axisos/first-boot-pending");
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
    char *mem = exec_cmd("free -h | awk '/^Mem:/ {print $2}' || echo '8 GB'");
    g_strstrip(cpu);
    g_strstrip(mem);

    char *hw_str = g_strdup_printf("Hardware Profile:\n• Processor: %s\n• Memory: %s RAM\n• Compositor: Wayland Native (wlroots)", cpu, mem);
    GtkWidget *lbl_hw = gtk_label_new(hw_str);
    gtk_label_set_xalign(GTK_LABEL(lbl_hw), 0.0);
    gtk_box_pack_start(GTK_BOX(card), lbl_hw, FALSE, FALSE, 12);

    g_free(cpu);
    g_free(mem);
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
    gtk_box_pack_start(GTK_BOX(card), app->lbl_net_status, FALSE, FALSE, 8);

    GtkWidget *hbox_b = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *btn_settings = gtk_button_new_with_label("Open Network Settings");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_settings), "btn-secondary");
    g_signal_connect(btn_settings, "clicked", G_CALLBACK(on_open_settings_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_b), btn_settings, FALSE, FALSE, 0);

    GtkWidget *btn_check = gtk_button_new_with_label("Check Connection");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_check), "btn-secondary");
    g_signal_connect(btn_check, "clicked", G_CALLBACK(on_check_net_cb), app);
    gtk_box_pack_start(GTK_BOX(hbox_b), btn_check, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(card), hbox_b, FALSE, FALSE, 8);
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

static AxisOobeApp *axis_oobe_app_create(void) {
    AxisOobeApp *app = g_new0(AxisOobeApp, 1);
    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "Welcome to AxisOS Setup");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 780, 520);
    gtk_window_set_position(GTK_WINDOW(app->window), GTK_WIN_POS_CENTER);

    // Apply CSS
    GtkCssProvider *prov = gtk_css_provider_new();
    gtk_css_provider_load_from_data(prov, CSS_OOBE, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(prov),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(prov);

    // Headerbar
    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS Out-of-Box Experience");
    gtk_window_set_titlebar(GTK_WINDOW(app->window), hb);

    app->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_container_add(GTK_CONTAINER(app->window), app->stack);

    build_welcome_step(app);
    build_network_step(app);
    build_finish_step(app);

    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
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
    AxisOobeApp *app = axis_oobe_app_create();
    gtk_widget_show_all(app->window);
    gtk_main();
    g_free(app);
    return 0;
}
