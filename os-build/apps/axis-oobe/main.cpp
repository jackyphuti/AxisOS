/*
 * AxisOS Native Out-of-Box Experience (OOBE) First-Boot Wizard
 * Written in C++ using GTK 3
 * Features:
 *  - Hardware detection (CPU, RAM, GPUs)
 *  - Network connectivity enforcement
 *  - System onboarding and ready state
 */

#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <array>
#include <memory>
#include <string>
#include <cstdlib>
#include <unistd.h>

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

static const char* CSS_OOBE = R"(
window {
    background-color: #0A0B0D;
    color: #F8FAFC;
}
headerbar {
    background: #14161A;
    border-bottom: 1px solid rgba(255, 255, 255, 0.12);
    color: #FFFFFF;
}
.card {
    background: #14161A;
    border: 1px solid rgba(255, 255, 255, 0.10);
    border-radius: 8px;
    padding: 20px;
    margin: 8px 16px;
}
.btn-primary {
    background: #FFFFFF;
    color: #0A0B0D;
    font-weight: 700;
    border-radius: 6px;
    border: 1px solid #FFFFFF;
    padding: 10px 24px;
}
.btn-primary:hover {
    background: #E2E8F0;
}
.btn-secondary {
    background: #202329;
    color: #F8FAFC;
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: 6px;
    padding: 8px 18px;
    font-weight: 600;
}
.btn-secondary:hover {
    background: rgba(255, 255, 255, 0.14);
    color: #FFFFFF;
}
)";

class AxisOobeApp {
public:
    GtkWidget* window;
    GtkWidget* stack;
    GtkWidget* lbl_net_status;

    AxisOobeApp() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Welcome to AxisOS Setup");
        gtk_window_set_default_size(GTK_WINDOW(window), 780, 520);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        // Apply CSS
        GtkCssProvider* prov = gtk_css_provider_new();
        gtk_css_provider_load_from_data(prov, CSS_OOBE, -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(prov),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        g_object_unref(prov);

        // Headerbar
        GtkWidget* hb = gtk_header_bar_new();
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
        gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS Out-of-Box Experience");
        gtk_window_set_titlebar(GTK_WINDOW(window), hb);

        stack = gtk_stack_new();
        gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
        gtk_container_add(GTK_CONTAINER(window), stack);

        build_welcome_step();
        build_network_step();
        build_finish_step();

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    }

    void show() {
        gtk_widget_show_all(window);
    }

private:
    void build_welcome_step() {
        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);

        GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

        GtkWidget* lbl_t = gtk_label_new("Welcome to your newly installed AxisOS PC");
        gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
        PangoAttrList* attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
        pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        gtk_label_set_attributes(GTK_LABEL(lbl_t), attrs);
        pango_attr_list_unref(attrs);
        gtk_box_pack_start(GTK_BOX(card), lbl_t, FALSE, FALSE, 0);

        GtkWidget* lbl_d = gtk_label_new("Let's finish getting your hardware and network configured so you can start gaming and coding right away.");
        gtk_label_set_xalign(GTK_LABEL(lbl_d), 0.0);
        gtk_box_pack_start(GTK_BOX(card), lbl_d, FALSE, FALSE, 0);

        // Hardware details
        std::string cpu = exec_cmd("grep -m1 'model name' /proc/cpuinfo | cut -d: -f2 || echo 'x86_64 CPU'");
        std::string mem = exec_cmd("free -h | awk '/^Mem:/ {print $2}' || echo '8 GB'");
        GtkWidget* lbl_hw = gtk_label_new(("Hardware Profile:\n• Processor: " + cpu + "\n• Memory: " + mem + " RAM\n• Compositor: Wayland Native (wlroots)").c_str());
        gtk_label_set_xalign(GTK_LABEL(lbl_hw), 0.0);
        gtk_box_pack_start(GTK_BOX(card), lbl_hw, FALSE, FALSE, 12);

        gtk_box_pack_start(GTK_BOX(vbox), card, TRUE, TRUE, 0);

        GtkWidget* btn_next = gtk_button_new_with_label("Next: Network Setup →");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_next), "btn-primary");
        g_signal_connect_swapped(btn_next, "clicked", G_CALLBACK(on_goto_net_cb), this);
        gtk_box_pack_end(GTK_BOX(vbox), btn_next, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), vbox, "step_welcome");
    }

    void build_network_step() {
        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);

        GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

        GtkWidget* lbl_t = gtk_label_new("Internet Connectivity");
        gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
        PangoAttrList* attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
        pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        gtk_label_set_attributes(GTK_LABEL(lbl_t), attrs);
        pango_attr_list_unref(attrs);
        gtk_box_pack_start(GTK_BOX(card), lbl_t, FALSE, FALSE, 0);

        lbl_net_status = gtk_label_new(check_network().c_str());
        gtk_label_set_xalign(GTK_LABEL(lbl_net_status), 0.0);
        gtk_box_pack_start(GTK_BOX(card), lbl_net_status, FALSE, FALSE, 8);

        GtkWidget* hbox_b = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        GtkWidget* btn_settings = gtk_button_new_with_label("Open Network Settings");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_settings), "btn-secondary");
        g_signal_connect_swapped(btn_settings, "clicked", G_CALLBACK(on_open_settings_cb), this);
        gtk_box_pack_start(GTK_BOX(hbox_b), btn_settings, FALSE, FALSE, 0);

        GtkWidget* btn_check = gtk_button_new_with_label("Check Connection");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_check), "btn-secondary");
        g_signal_connect_swapped(btn_check, "clicked", G_CALLBACK(on_check_net_cb), this);
        gtk_box_pack_start(GTK_BOX(hbox_b), btn_check, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(card), hbox_b, FALSE, FALSE, 8);
        gtk_box_pack_start(GTK_BOX(vbox), card, TRUE, TRUE, 0);

        GtkWidget* btn_next = gtk_button_new_with_label("Next: Complete Setup →");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_next), "btn-primary");
        g_signal_connect_swapped(btn_next, "clicked", G_CALLBACK(on_goto_finish_cb), this);
        gtk_box_pack_end(GTK_BOX(vbox), btn_next, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), vbox, "step_net");
    }

    void build_finish_step() {
        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 24);

        GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

        GtkWidget* lbl_t = gtk_label_new("You're all set!");
        gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
        PangoAttrList* attrs = pango_attr_list_new();
        pango_attr_list_insert(attrs, pango_attr_scale_new(1.3));
        pango_attr_list_insert(attrs, pango_attr_weight_new(PANGO_WEIGHT_BOLD));
        gtk_label_set_attributes(GTK_LABEL(lbl_t), attrs);
        pango_attr_list_unref(attrs);
        gtk_box_pack_start(GTK_BOX(card), lbl_t, FALSE, FALSE, 0);

        GtkWidget* lbl_d = gtk_label_new("AxisOS is ready for use.\nEnjoy high FPS gaming on Steam, fast development in VS Code, and a sleek Wayland desktop.");
        gtk_label_set_xalign(GTK_LABEL(lbl_d), 0.0);
        gtk_box_pack_start(GTK_BOX(card), lbl_d, FALSE, FALSE, 8);

        gtk_box_pack_start(GTK_BOX(vbox), card, TRUE, TRUE, 0);

        GtkWidget* btn_done = gtk_button_new_with_label("Start Using AxisOS");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_done), "btn-primary");
        g_signal_connect_swapped(btn_done, "clicked", G_CALLBACK(on_finish_cb), this);
        gtk_box_pack_end(GTK_BOX(vbox), btn_done, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), vbox, "step_finish");
    }

    std::string check_network() {
        std::string ping = exec_cmd("ping -c 1 -W 2 1.1.1.1 2>/dev/null && echo 'CONNECTED' || echo 'OFFLINE'");
        if (ping.find("CONNECTED") != std::string::npos) {
            return "✓ Connected to the Internet. Online services, updates, and Steam are accessible.";
        }
        return "⚠ No active internet connection detected. You can configure Wi-Fi in Settings or proceed offline.";
    }

    static void on_goto_net_cb(AxisOobeApp* app) {
        gtk_stack_set_visible_child_name(GTK_STACK(app->stack), "step_net");
    }

    static void on_goto_finish_cb(AxisOobeApp* app) {
        gtk_stack_set_visible_child_name(GTK_STACK(app->stack), "step_finish");
    }

    static void on_open_settings_cb(AxisOobeApp*) {
        system("gnome-control-center &");
    }

    static void on_check_net_cb(AxisOobeApp* app) {
        gtk_label_set_text(GTK_LABEL(app->lbl_net_status), app->check_network().c_str());
    }

    static void on_finish_cb(AxisOobeApp* app) {
        unlink("/var/lib/axisos/first-boot-pending");
        gtk_widget_destroy(app->window);
    }
};

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    AxisOobeApp app;
    app.show();
    gtk_main();
    return 0;
}
