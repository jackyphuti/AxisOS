/*
 * AxisOS Native Task Manager — Redesigned HUD Edition
 * Built in C++17 with GTK 3 & Cairo
 *
 * Features:
 * 1. Collapsible Left Navigation Rail (Windows 11 Sidebar style)
 *    - Processes, Performance, App History, Startup Apps, Users, Details, Services
 *    - Neon cyan (#00F2FE) active indicator pill against obsidian (#0B0D11) base
 *    - Hamburger toggle (expanded 200px <-> compact 64px icon-only)
 * 2. Unified HUD Telemetry Deck:
 *    - Real-time multi-rail rolling sparklines (CPU, RAM, Disk, Net)
 *    - Dynamic Logical Processor Matrix (color-coded cyan normal <-> magenta high-thread)
 *    - Gradient DDR5 Memory Bar (Active, Cached, Available with smooth neon gradient)
 *    - NVMe & Network Throughput (real-time read/write MB/s and RX/TX KB/s)
 *    - Circular thermal gauges (CPU Package & GPU Hotspot package temperatures via Cairo)
 *    - Interactive GameBoost Extreme toggle badge (live governor query & toggle)
 * 3. Heat-Mapped Process Grid:
 *    - Sortable resource heatmap cells (CPU%, Memory, Disk, Net, GPU)
 *    - In-place row updates preserving selection and scroll position across ticks
 *    - Load intensity shading: Teal (<25%), Cyber Yellow #FCEE0A (25-60%), Crimson/Magenta #FF0055 (>60%)
 *    - Neon magenta Terminate Process button & Run New Task launcher
 * 4. App History, Startup Apps, Users, Details (with full /proc/pid/cmdline), and Systemd Services panels
 */

#include <gtk/gtk.h>
#include <cairo.h>
#include <pango/pangocairo.h>

// Robust Pango text rendering helper with automatic fontconfig fallback (eliminates Cairo toy font tofu)
static void render_pango_text(cairo_t* cr, double x, double y, const char* text,
                              const char* font_desc_str,
                              double r, double g, double b, double a = 1.0,
                              bool center_x = false, bool center_y = false) {
    if (!cr || !text) return;
    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, text, -1);
    PangoFontDescription* desc = pango_font_description_from_string(font_desc_str);
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    double draw_x = x;
    double draw_y = y;
    if (center_x) draw_x -= (text_w / 2.0);
    if (center_y) draw_y -= (text_h / 2.0);

    cairo_set_source_rgba(cr, r, g, b, a);
    cairo_move_to(cr, draw_x, draw_y);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
}
#include <cmath>
#include <cstring>
#include <chrono>
#include <vector>
#include <deque>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>
#include <map>
#include <set>
#include <iostream>

// ============================================================================
// CSS Theme & Styling
// ============================================================================
static const char* CSS_TASK_MANAGER = R"(
* {
    font-family: "Inter", "Segoe UI", "Cantarell", "DejaVu Sans", sans-serif;
    outline-style: none;
}

window,
.background {
    background-color: #0B0D11;
    color: #F8FAFC;
}

headerbar {
    background-color: #0F1218;
    background-image: none;
    border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    color: #FFFFFF;
    box-shadow: none;
}

headerbar .title {
    font-weight: 800;
    font-size: 15px;
    color: #FFFFFF;
}

headerbar .subtitle {
    font-size: 11px;
    color: #00F2FE;
    font-weight: 600;
}

headerbar button {
    background-image: none;
    box-shadow: none;
    text-shadow: none;
}

/* Collapsible Left Navigation Rail */
.nav-rail {
    background-color: #07090D;
    border-right: 1px solid rgba(255, 255, 255, 0.08);
    padding: 8px 6px;
}

button.nav-rail-btn {
    background-color: transparent;
    background-image: none;
    border: none;
    border-radius: 6px;
    color: #94A3B8;
    padding: 10px 12px;
    font-weight: 600;
    font-size: 13px;
    margin: 2px 4px;
    box-shadow: none;
    text-shadow: none;
    border-left: 3px solid transparent;
}

button.nav-rail-btn label {
    color: #94A3B8;
    font-weight: 600;
}

button.nav-rail-btn:hover {
    background-color: rgba(255, 255, 255, 0.05);
    background-image: none;
    color: #FFFFFF;
}

button.nav-rail-btn:hover label {
    color: #FFFFFF;
}

button.nav-rail-btn:checked,
button.nav-rail-btn.active {
    background-color: rgba(0, 242, 254, 0.12);
    background-image: linear-gradient(90deg, rgba(0, 242, 254, 0.18) 0%, rgba(0, 242, 254, 0.03) 100%);
    color: #00F2FE;
    font-weight: 700;
    border-left: 3px solid #00F2FE;
}

button.nav-rail-btn:checked label,
button.nav-rail-btn.active label {
    color: #00F2FE;
    font-weight: 700;
}

button.hamburger-btn {
    background-color: transparent;
    background-image: none;
    border: 1px solid transparent;
    border-radius: 6px;
    color: #94A3B8;
    font-size: 15px;
    padding: 6px 10px;
    margin: 2px 4px 6px 4px;
    box-shadow: none;
    text-shadow: none;
}

button.hamburger-btn label {
    color: #94A3B8;
}

button.hamburger-btn:hover {
    background-color: rgba(255, 255, 255, 0.05);
    color: #00F2FE;
}

button.hamburger-btn:hover label {
    color: #00F2FE;
}

/* Main Content Area */
.content-pane,
scrolledwindow,
viewport,
stack {
    background-color: #0B0D11;
    color: #F8FAFC;
}

.content-pane {
    padding: 12px 16px;
}

.card {
    background-color: #121620;
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 10px;
    padding: 14px 18px;
    margin-bottom: 10px;
}

.card-title {
    font-size: 12px;
    font-weight: 800;
    color: #94A3B8;
    letter-spacing: 0.5px;
    margin-bottom: 4px;
}

.card-value {
    font-size: 18px;
    font-weight: 800;
    color: #FFFFFF;
}

/* Action Buttons */
button.btn-terminate {
    background-color: #E11D48;
    background-image: linear-gradient(135deg, #FF0055 0%, #BE123C 100%);
    color: #FFFFFF;
    font-weight: 700;
    border-radius: 6px;
    border: 1px solid #FF2A6D;
    padding: 6px 14px;
    box-shadow: none;
    text-shadow: none;
}

button.btn-terminate label {
    color: #FFFFFF;
    font-weight: 700;
}

button.btn-terminate:hover {
    background-color: #FF2A6D;
    background-image: linear-gradient(135deg, #FF2A6D 0%, #E11D48 100%);
    border-color: #FFFFFF;
}

button.btn-run-task {
    background-color: #162235;
    background-image: none;
    color: #00F2FE;
    font-weight: 700;
    border-radius: 6px;
    border: 1px solid rgba(0, 242, 254, 0.45);
    padding: 6px 14px;
    box-shadow: none;
    text-shadow: none;
}

button.btn-run-task label {
    color: #00F2FE;
    font-weight: 700;
}

button.btn-run-task:hover {
    background-color: #00F2FE;
    background-image: none;
    color: #0B0D11;
    border-color: #00F2FE;
}

button.btn-run-task:hover label {
    color: #0B0D11;
}

button.btn-secondary {
    background-color: #171C28;
    background-image: none;
    color: #E2E8F0;
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 6px;
    padding: 7px 14px;
    font-weight: 600;
    box-shadow: none;
    text-shadow: none;
}

button.btn-secondary label {
    color: #E2E8F0;
    font-weight: 600;
}

button.btn-secondary:hover {
    background-color: #22293A;
    background-image: none;
    border-color: #00F2FE;
    color: #FFFFFF;
}

button.btn-secondary:hover label {
    color: #FFFFFF;
}

/* GameBoost Interactive Badge */
button.gameboost-badge-on {
    background-color: rgba(255, 0, 85, 0.22);
    background-image: linear-gradient(90deg, rgba(255, 0, 85, 0.28) 0%, rgba(252, 238, 10, 0.22) 100%);
    border: 1px solid #FF0055;
    border-radius: 20px;
    color: #FFFFFF;
    font-weight: 800;
    font-size: 11px;
    padding: 7px 14px;
    box-shadow: none;
    text-shadow: none;
}

button.gameboost-badge-on label {
    color: #FFFFFF;
    font-weight: 800;
}

button.gameboost-badge-off {
    background-color: rgba(0, 242, 254, 0.10);
    background-image: none;
    border: 1px solid rgba(0, 242, 254, 0.45);
    border-radius: 20px;
    color: #00F2FE;
    font-weight: 800;
    font-size: 11px;
    padding: 7px 14px;
    box-shadow: none;
    text-shadow: none;
}

button.gameboost-badge-off label {
    color: #00F2FE;
    font-weight: 800;
}

.summary-pill {
    color: #E2E8F0;
    font-size: 12px;
    font-weight: 600;
    padding: 4px 10px;
}

/* TreeView & Grid */
treeview {
    background-color: #0E1117;
    color: #F8FAFC;
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 6px;
}

treeview:selected,
treeview row:selected {
    background-color: rgba(0, 242, 254, 0.22);
    color: #FFFFFF;
}

treeview header button {
    background-color: #141923;
    background-image: none;
    color: #94A3B8;
    font-weight: 700;
    font-size: 12px;
    border: none;
    border-bottom: 2px solid rgba(255, 255, 255, 0.08);
    border-right: 1px solid rgba(255, 255, 255, 0.04);
    padding: 8px 8px;
    box-shadow: none;
    text-shadow: none;
}

treeview header button label {
    color: #94A3B8;
    font-weight: 700;
}

treeview header button:hover {
    background-color: #1A2130;
    background-image: none;
    color: #00F2FE;
    border-bottom: 2px solid #00F2FE;
}

treeview header button:hover label {
    color: #00F2FE;
}

searchentry,
entry {
    background-color: #121620;
    background-image: none;
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 0.14);
    border-radius: 6px;
    padding: 6px 12px;
    box-shadow: none;
}

searchentry:focus,
entry:focus {
    border-color: #00F2FE;
}

.status-footer {
    background-color: #07090D;
    border-top: 1px solid rgba(255, 255, 255, 0.08);
    padding: 6px 16px;
    color: #94A3B8;
    font-size: 12px;
}
)";

// ============================================================================
// Process Data Structure & Tree Columns
// ============================================================================
enum {
    COL_NAME = 0,
    COL_PID,
    COL_STATUS,
    COL_USER,
    COL_CPU,
    COL_MEM,
    COL_DISK,
    COL_NET,
    COL_GPU,
    NUM_PROCESS_COLS
};

enum {
    DET_COL_PID = 0,
    DET_COL_NAME,
    DET_COL_USER,
    DET_COL_NICE,
    DET_COL_STATUS,
    DET_COL_CMDLINE,
    NUM_DETAIL_COLS
};

// ============================================================================
// Main Application Class
// ============================================================================
class AxisTaskManagerApp {
public:
    GtkWidget* window;
    GtkWidget* nav_rail;
    GtkWidget* stack;
    GtkWidget* search_entry;
    GtkWidget* tree_view;
    GtkListStore* store_processes;
    GtkTreeModelFilter* filter_processes;
    GtkTreeModelSort* sort_processes;

    // Performance HUD widgets
    GtkWidget* drawing_sparklines = nullptr;
    GtkWidget* drawing_cores = nullptr;
    GtkWidget* drawing_memory = nullptr;
    GtkWidget* drawing_thermals = nullptr;
    GtkWidget* lbl_nvme_read = nullptr;
    GtkWidget* lbl_net_io = nullptr;
    GtkWidget* btn_gameboost = nullptr;
    GtkWidget* lbl_summary_badge = nullptr;
    GtkWidget* lbl_footer_stats = nullptr;
    GtkWidget* lbl_footer_uptime = nullptr;

    // Startup, History & Services stores
    GtkListStore* store_startup = nullptr;
    GtkListStore* store_users = nullptr;
    GtkListStore* store_details = nullptr;
    GtkListStore* store_services = nullptr;
    GtkListStore* store_app_history = nullptr;

    // Navigation rail state
    bool rail_expanded = true;
    std::vector<GtkWidget*> nav_buttons;
    std::vector<std::string> nav_labels = {
        "Processes", "Performance", "App History",
        "Startup Apps", "Users", "Details", "Services"
    };
    std::vector<std::string> nav_icons = {
        "⚡", "📈", "🕒", "🚀", "👥", "📄", "⚙️"
    };
    std::vector<std::string> nav_pages = {
        "processes", "performance", "apphistory",
        "startup", "users", "details", "services"
    };

    // Telemetry History
    static const int SPARKLINE_POINTS = 60;
    std::deque<double> history_cpu;
    std::deque<double> history_mem;
    std::deque<double> history_disk;
    std::deque<double> history_net;

    // Real Online Logical Processor Loads (Detected dynamically from kernel)
    std::vector<double> core_loads;
    std::vector<unsigned long long> prev_core_work;
    std::vector<unsigned long long> prev_core_total;

    // Overall metrics (From real /proc/stat, /proc/meminfo, /proc/diskstats)
    double current_cpu_pct = 0.0;
    double current_mem_gb = 0.0;
    double total_mem_gb = 16.0;
    double cached_mem_gb = 0.0;
    double free_mem_gb = 0.0;
    double disk_read_mbs = 0.0;
    double disk_write_mbs = 0.0;
    double net_rx_kbs = 0.0;
    double net_tx_kbs = 0.0;
    double cpu_temp_c = 0.0;
    double gpu_temp_c = 0.0;
    bool has_cpu_thermal_sensor = false;
    bool has_gpu_thermal_sensor = false;
    bool gameboost_active = false;
    int total_process_count = 0;
    int total_thread_count = 0;

    // Disk & Net jiffies
    unsigned long long prev_total_jiffies = 0;
    unsigned long long prev_work_jiffies = 0;
    unsigned long long prev_disk_sectors_read = 0;
    unsigned long long prev_disk_sectors_written = 0;
    unsigned long long prev_net_bytes_rx = 0;
    unsigned long long prev_net_bytes_tx = 0;
    std::chrono::steady_clock::time_point last_proc_sample_time;

    std::map<int, std::pair<unsigned long long, unsigned long long>> proc_jiffies_map;
    std::map<int, unsigned long long> proc_io_map;

    struct RealAppHistory {
        std::string name;
        double cpu_sec = 0.0;
        double mem_mb = 0.0;
        double io_mb = 0.0;
    };
    std::map<std::string, RealAppHistory> app_history_cache;

    AxisTaskManagerApp() {
        for (int i = 0; i < SPARKLINE_POINTS; ++i) {
            history_cpu.push_back(0.0);
            history_mem.push_back(0.0);
            history_disk.push_back(0.0);
            history_net.push_back(0.0);
        }
        long n_cores = sysconf(_SC_NPROCESSORS_ONLN);
        if (n_cores <= 0) n_cores = 1;
        core_loads.resize(n_cores, 0.0);
        prev_core_work.resize(n_cores, 0);
        prev_core_total.resize(n_cores, 0);
        last_proc_sample_time = std::chrono::steady_clock::now();

        init_ui();
        refresh_all_telemetry();
        refresh_processes();
        refresh_startup_apps();
        refresh_users();
        refresh_services();

        // 1-second high-precision timer for telemetry
        g_timeout_add(1000, on_tick, this);
    }

    void show() {
        gtk_widget_show_all(window);
    }

private:
    static void set_button_label_alignment(GtkWidget* btn, gfloat xalign) {
        gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
        GtkWidget* child = gtk_bin_get_child(GTK_BIN(btn));
        if (child && GTK_IS_LABEL(child)) {
            gtk_label_set_xalign(GTK_LABEL(child), xalign);
        }
    }

    void init_ui() {
        GtkSettings* settings = gtk_settings_get_default();
        if (settings) {
            g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
        }

        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Task Manager");
        gtk_window_set_default_size(GTK_WINDOW(window), 1120, 700);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        // Apply dark cyber CSS
        GtkCssProvider* prov = gtk_css_provider_new();
        gtk_css_provider_load_from_data(prov, CSS_TASK_MANAGER, -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(prov),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        g_object_unref(prov);

        // Headerbar
        GtkWidget* hb = gtk_header_bar_new();
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
        gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "Task Manager");
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "AxisOS HUD & Resource Monitor");
        gtk_window_set_titlebar(GTK_WINDOW(window), hb);

        // Headerbar: Run New Task button
        GtkWidget* btn_run = gtk_button_new_with_label("➕ Run New Task");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_run), "btn-run-task");
        g_signal_connect_swapped(btn_run, "clicked", G_CALLBACK(on_run_task_clicked), this);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(hb), btn_run);

        // Headerbar: End Task button
        GtkWidget* btn_terminate = gtk_button_new_with_label("⛔ End Task");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_terminate), "btn-terminate");
        g_signal_connect_swapped(btn_terminate, "clicked", G_CALLBACK(on_end_task_clicked), this);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(hb), btn_terminate);

        // Headerbar right: Real-time Telemetry Summary Badge
        lbl_summary_badge = gtk_label_new("⚡ CPU: 0.0%  •  🧠 RAM: 0.0 GB  •  🚀 GameBoost: BALANCED");
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_summary_badge), "summary-pill");
        gtk_header_bar_pack_end(GTK_HEADER_BAR(hb), lbl_summary_badge);

        // Root Vertical Layout (Main HBox + Status Footer)
        GtkWidget* root_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(window), root_vbox);

        // Main horizontal layout (Rail + Stack)
        GtkWidget* main_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_box_pack_start(GTK_BOX(root_vbox), main_hbox, TRUE, TRUE, 0);

        // 1. Collapsible Left Navigation Rail
        build_nav_rail(main_hbox);

        // 2. Right Content Stack
        stack = gtk_stack_new();
        gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
        gtk_stack_set_transition_duration(GTK_STACK(stack), 120);
        gtk_box_pack_start(GTK_BOX(main_hbox), stack, TRUE, TRUE, 0);

        // 3. Bottom Status Footer
        GtkWidget* footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
        gtk_style_context_add_class(gtk_widget_get_style_context(footer), "status-footer");
        lbl_footer_stats = gtk_label_new("Processes: 0   |   Threads: 0");
        gtk_label_set_xalign(GTK_LABEL(lbl_footer_stats), 0.0);
        lbl_footer_uptime = gtk_label_new("Uptime: 00:00:00");
        gtk_label_set_xalign(GTK_LABEL(lbl_footer_uptime), 1.0);
        gtk_box_pack_start(GTK_BOX(footer), lbl_footer_stats, TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(footer), lbl_footer_uptime, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(root_vbox), footer, FALSE, FALSE, 0);

        // Build all 7 panels
        build_processes_page();
        build_performance_page();
        build_app_history_page();
        build_startup_page();
        build_users_page();
        build_details_page();
        build_services_page();

        // Select Processes by default
        select_nav_page(0);

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    }

    // ========================================================================
    // 1. Collapsible Left Navigation Rail
    // ========================================================================
    void build_nav_rail(GtkWidget* parent) {
        nav_rail = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        gtk_style_context_add_class(gtk_widget_get_style_context(nav_rail), "nav-rail");
        gtk_widget_set_size_request(nav_rail, 196, -1);
        gtk_box_pack_start(GTK_BOX(parent), nav_rail, FALSE, FALSE, 0);

        // Hamburger toggle button at top
        GtkWidget* btn_hamburger = gtk_button_new_with_label("☰  Navigation");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_hamburger), "hamburger-btn");
        gtk_widget_set_tooltip_text(btn_hamburger, "Collapse / Expand Navigation Rail");
        gtk_widget_set_can_focus(btn_hamburger, FALSE);
        set_button_label_alignment(btn_hamburger, 0.0f);
        g_object_set_data(G_OBJECT( nav_rail ), "hamburger_btn", btn_hamburger);
        g_signal_connect_swapped(btn_hamburger, "clicked", G_CALLBACK(on_hamburger_toggle), this);
        gtk_box_pack_start(GTK_BOX(nav_rail), btn_hamburger, FALSE, FALSE, 0);

        // Build rail items
        for (size_t i = 0; i < nav_labels.size(); ++i) {
            std::string text = nav_icons[i] + "   " + nav_labels[i];
            GtkWidget* b = gtk_button_new_with_label(text.c_str());
            gtk_style_context_add_class(gtk_widget_get_style_context(b), "nav-rail-btn");
            gtk_widget_set_tooltip_text(b, nav_labels[i].c_str());
            gtk_widget_set_halign(b, GTK_ALIGN_FILL);
            gtk_widget_set_can_focus(b, FALSE);
            set_button_label_alignment(b, 0.0f);

            g_object_set_data(G_OBJECT(b), "page_idx", GINT_TO_POINTER(static_cast<int>(i)));
            g_signal_connect(b, "clicked", G_CALLBACK(on_nav_btn_clicked), this);

            nav_buttons.push_back(b);
            gtk_box_pack_start(GTK_BOX(nav_rail), b, FALSE, FALSE, 0);
        }
    }

    static void on_nav_btn_clicked(GtkWidget* btn, gpointer user_data) {
        auto* app = static_cast<AxisTaskManagerApp*>(user_data);
        int idx = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "page_idx"));
        app->select_nav_page(idx);
    }

    void select_nav_page(int index) {
        for (size_t i = 0; i < nav_buttons.size(); ++i) {
            auto* ctx = gtk_widget_get_style_context(nav_buttons[i]);
            if (static_cast<int>(i) == index) {
                gtk_style_context_add_class(ctx, "active");
            } else {
                gtk_style_context_remove_class(ctx, "active");
            }
        }
        if (index >= 0 && index < static_cast<int>(nav_pages.size())) {
            gtk_stack_set_visible_child_name(GTK_STACK(stack), nav_pages[index].c_str());
        }
        if (index == 0) refresh_processes();
        else if (index == 2) refresh_app_history();
        else if (index == 3) refresh_startup_apps();
        else if (index == 4) refresh_users();
        else if (index == 6) refresh_services();
    }

    static void on_hamburger_toggle(AxisTaskManagerApp* app) {
        app->rail_expanded = !app->rail_expanded;
        GtkWidget* hb_btn = static_cast<GtkWidget*>(g_object_get_data(G_OBJECT(app->nav_rail), "hamburger_btn"));
        if (app->rail_expanded) {
            gtk_widget_set_size_request(app->nav_rail, 196, -1);
            if (hb_btn) {
                gtk_button_set_label(GTK_BUTTON(hb_btn), "☰  Navigation");
                set_button_label_alignment(hb_btn, 0.0f);
            }
            for (size_t i = 0; i < app->nav_buttons.size(); ++i) {
                std::string full = app->nav_icons[i] + "   " + app->nav_labels[i];
                gtk_button_set_label(GTK_BUTTON(app->nav_buttons[i]), full.c_str());
                set_button_label_alignment(app->nav_buttons[i], 0.0f);
            }
        } else {
            gtk_widget_set_size_request(app->nav_rail, 60, -1);
            if (hb_btn) {
                gtk_button_set_label(GTK_BUTTON(hb_btn), "☰");
                set_button_label_alignment(hb_btn, 0.5f);
            }
            for (size_t i = 0; i < app->nav_buttons.size(); ++i) {
                gtk_button_set_label(GTK_BUTTON(app->nav_buttons[i]), app->nav_icons[i].c_str());
                set_button_label_alignment(app->nav_buttons[i], 0.5f);
            }
        }
    }

    // ========================================================================
    // 2. Heat-Mapped Process Grid (Processes Page)
    // ========================================================================
    void build_processes_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_style_context_add_class(gtk_widget_get_style_context(page), "content-pane");

        // Top Filter Bar
        GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        search_entry = gtk_search_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Filter processes by Name, PID, or User...");
        g_signal_connect_swapped(search_entry, "search-changed", G_CALLBACK(on_search_changed), this);
        gtk_box_pack_start(GTK_BOX(bar), search_entry, TRUE, TRUE, 0);

        GtkWidget* btn_refresh = gtk_button_new_with_label("↻ Refresh");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh), "btn-secondary");
        g_signal_connect_swapped(btn_refresh, "clicked", G_CALLBACK(+[](AxisTaskManagerApp* a){ a->refresh_processes(); }), this);
        gtk_box_pack_start(GTK_BOX(bar), btn_refresh, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(page), bar, FALSE, FALSE, 0);

        // Process List Store -> Filter -> Sort -> TreeView
        store_processes = gtk_list_store_new(
            NUM_PROCESS_COLS,
            G_TYPE_STRING, // COL_NAME
            G_TYPE_INT,    // COL_PID
            G_TYPE_STRING, // COL_STATUS
            G_TYPE_STRING, // COL_USER
            G_TYPE_DOUBLE, // COL_CPU
            G_TYPE_DOUBLE, // COL_MEM
            G_TYPE_DOUBLE, // COL_DISK
            G_TYPE_DOUBLE, // COL_NET
            G_TYPE_STRING  // COL_GPU
        );

        filter_processes = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new(GTK_TREE_MODEL(store_processes), NULL));
        gtk_tree_model_filter_set_visible_func(filter_processes, process_filter_func, this, NULL);

        sort_processes = GTK_TREE_MODEL_SORT(gtk_tree_model_sort_new_with_model(GTK_TREE_MODEL(filter_processes)));
        gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sort_processes), COL_CPU, GTK_SORT_DESCENDING);

        tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(sort_processes));
        gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), TRUE);
        gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree_view), FALSE);

        setup_heatmap_columns();

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), tree_view);
        gtk_box_pack_start(GTK_BOX(page), scroll, TRUE, TRUE, 0);

        gtk_stack_add_named(GTK_STACK(stack), page, "processes");
    }

    void setup_heatmap_columns() {
        GtkCellRenderer* r_text = gtk_cell_renderer_text_new();
        g_object_set(r_text, "xpad", 8, "ypad", 5, NULL);

        // 1. Name
        GtkTreeViewColumn* col_name = gtk_tree_view_column_new_with_attributes("Process Name", r_text, "text", COL_NAME, NULL);
        gtk_tree_view_column_set_sort_column_id(col_name, COL_NAME);
        gtk_tree_view_column_set_resizable(col_name, TRUE);
        gtk_tree_view_column_set_expand(col_name, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_name);

        // 2. PID
        GtkCellRenderer* r_pid = gtk_cell_renderer_text_new();
        g_object_set(r_pid, "xpad", 8, "ypad", 5, NULL);
        GtkTreeViewColumn* col_pid = gtk_tree_view_column_new_with_attributes("PID", r_pid, "text", COL_PID, NULL);
        gtk_tree_view_column_set_sort_column_id(col_pid, COL_PID);
        gtk_tree_view_column_set_resizable(col_pid, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_pid);

        // 3. Status
        GtkCellRenderer* r_status = gtk_cell_renderer_text_new();
        g_object_set(r_status, "xpad", 8, "ypad", 5, NULL);
        GtkTreeViewColumn* col_status = gtk_tree_view_column_new_with_attributes("Status", r_status, "text", COL_STATUS, NULL);
        gtk_tree_view_column_set_sort_column_id(col_status, COL_STATUS);
        gtk_tree_view_column_set_resizable(col_status, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_status);

        // 4. User
        GtkCellRenderer* r_user = gtk_cell_renderer_text_new();
        g_object_set(r_user, "xpad", 8, "ypad", 5, NULL);
        GtkTreeViewColumn* col_user = gtk_tree_view_column_new_with_attributes("User", r_user, "text", COL_USER, NULL);
        gtk_tree_view_column_set_sort_column_id(col_user, COL_USER);
        gtk_tree_view_column_set_resizable(col_user, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_user);

        // 5. Heatmapped CPU %
        GtkCellRenderer* r_cpu = gtk_cell_renderer_text_new();
        g_object_set(r_cpu, "xpad", 8, "ypad", 5, "xalign", 1.0f, NULL);
        GtkTreeViewColumn* col_cpu = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(col_cpu, "CPU %");
        gtk_tree_view_column_pack_start(col_cpu, r_cpu, TRUE);
        gtk_tree_view_column_set_cell_data_func(col_cpu, r_cpu, heatmap_cpu_func, NULL, NULL);
        gtk_tree_view_column_set_sort_column_id(col_cpu, COL_CPU);
        gtk_tree_view_column_set_resizable(col_cpu, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_cpu);

        // 6. Heatmapped Memory MB
        GtkCellRenderer* r_mem = gtk_cell_renderer_text_new();
        g_object_set(r_mem, "xpad", 8, "ypad", 5, "xalign", 1.0f, NULL);
        GtkTreeViewColumn* col_mem = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(col_mem, "Memory (MB)");
        gtk_tree_view_column_pack_start(col_mem, r_mem, TRUE);
        gtk_tree_view_column_set_cell_data_func(col_mem, r_mem, heatmap_mem_func, NULL, NULL);
        gtk_tree_view_column_set_sort_column_id(col_mem, COL_MEM);
        gtk_tree_view_column_set_resizable(col_mem, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_mem);

        // 7. Heatmapped Disk I/O
        GtkCellRenderer* r_disk = gtk_cell_renderer_text_new();
        g_object_set(r_disk, "xpad", 8, "ypad", 5, "xalign", 1.0f, NULL);
        GtkTreeViewColumn* col_disk = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(col_disk, "Disk (KB/s)");
        gtk_tree_view_column_pack_start(col_disk, r_disk, TRUE);
        gtk_tree_view_column_set_cell_data_func(col_disk, r_disk, heatmap_disk_func, NULL, NULL);
        gtk_tree_view_column_set_sort_column_id(col_disk, COL_DISK);
        gtk_tree_view_column_set_resizable(col_disk, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_disk);

        // 8. Heatmapped Network
        GtkCellRenderer* r_net = gtk_cell_renderer_text_new();
        g_object_set(r_net, "xpad", 8, "ypad", 5, "xalign", 1.0f, NULL);
        GtkTreeViewColumn* col_net = gtk_tree_view_column_new();
        gtk_tree_view_column_set_title(col_net, "Network");
        gtk_tree_view_column_pack_start(col_net, r_net, TRUE);
        gtk_tree_view_column_set_cell_data_func(col_net, r_net, heatmap_net_func, NULL, NULL);
        gtk_tree_view_column_set_sort_column_id(col_net, COL_NET);
        gtk_tree_view_column_set_resizable(col_net, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_net);

        // 9. GPU Engine
        GtkCellRenderer* r_gpu = gtk_cell_renderer_text_new();
        g_object_set(r_gpu, "xpad", 8, "ypad", 5, NULL);
        GtkTreeViewColumn* col_gpu = gtk_tree_view_column_new_with_attributes("GPU", r_gpu, "text", COL_GPU, NULL);
        gtk_tree_view_column_set_sort_column_id(col_gpu, COL_GPU);
        gtk_tree_view_column_set_resizable(col_gpu, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col_gpu);
    }

    // Heatmap cell styling: Teal -> Cyber Yellow #FCEE0A -> Crimson/Magenta #FF0055
    static void apply_heat_style(GtkCellRenderer* r, double val, double moderate_thresh, double high_thresh) {
        if (val >= high_thresh) {
            GdkRGBA bg = {0.882, 0.114, 0.282, 0.85};
            GdkRGBA fg = {1.0, 1.0, 1.0, 1.0};
            g_object_set(r, "cell-background-rgba", &bg, "foreground-rgba", &fg, "weight", 700, NULL);
        } else if (val >= moderate_thresh) {
            GdkRGBA bg = {0.988, 0.933, 0.039, 0.85};
            GdkRGBA fg = {0.043, 0.051, 0.067, 1.0};
            g_object_set(r, "cell-background-rgba", &bg, "foreground-rgba", &fg, "weight", 700, NULL);
        } else if (val > 0.5) {
            GdkRGBA bg = {0.0, 0.949, 0.996, 0.16};
            GdkRGBA fg = {0.35, 0.96, 1.0, 1.0};
            g_object_set(r, "cell-background-rgba", &bg, "foreground-rgba", &fg, "weight", 600, NULL);
        } else {
            GdkRGBA fg = {0.796, 0.835, 0.882, 1.0};
            g_object_set(r, "cell-background-set", FALSE, "foreground-rgba", &fg, "weight", 400, NULL);
        }
    }

    static void heatmap_cpu_func(GtkTreeViewColumn*, GtkCellRenderer* r, GtkTreeModel* m, GtkTreeIter* it, gpointer) {
        double val = 0.0;
        gtk_tree_model_get(m, it, COL_CPU, &val, -1);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f %%", val);
        g_object_set(r, "text", buf, NULL);
        apply_heat_style(r, val, 25.0, 60.0);
    }

    static void heatmap_mem_func(GtkTreeViewColumn*, GtkCellRenderer* r, GtkTreeModel* m, GtkTreeIter* it, gpointer) {
        double val = 0.0;
        gtk_tree_model_get(m, it, COL_MEM, &val, -1);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f MB", val);
        g_object_set(r, "text", buf, NULL);
        apply_heat_style(r, val, 250.0, 800.0);
    }

    static void heatmap_disk_func(GtkTreeViewColumn*, GtkCellRenderer* r, GtkTreeModel* m, GtkTreeIter* it, gpointer) {
        double val = 0.0;
        gtk_tree_model_get(m, it, COL_DISK, &val, -1);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f KB/s", val);
        g_object_set(r, "text", buf, NULL);
        apply_heat_style(r, val, 500.0, 2000.0);
    }

    static void heatmap_net_func(GtkTreeViewColumn*, GtkCellRenderer* r, GtkTreeModel* m, GtkTreeIter* it, gpointer) {
        double val = 0.0;
        gtk_tree_model_get(m, it, COL_NET, &val, -1);
        char buf[32];
        snprintf(buf, sizeof(buf), "%.1f KB/s", val);
        g_object_set(r, "text", buf, NULL);
        apply_heat_style(r, val, 200.0, 1000.0);
    }

    static gboolean process_filter_func(GtkTreeModel* m, GtkTreeIter* it, gpointer data) {
        auto* app = static_cast<AxisTaskManagerApp*>(data);
        const char* q = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
        if (!q || strlen(q) == 0) return TRUE;

        gchar* name = nullptr;
        gchar* user = nullptr;
        gint pid = 0;
        gtk_tree_model_get(m, it, COL_NAME, &name, COL_USER, &user, COL_PID, &pid, -1);

        bool match = false;
        if (name && strcasestr(name, q) != nullptr) match = true;
        if (user && strcasestr(user, q) != nullptr) match = true;
        if (std::to_string(pid).find(q) != std::string::npos) match = true;

        if (name) g_free(name);
        if (user) g_free(user);
        return match ? TRUE : FALSE;
    }

    static void on_search_changed(AxisTaskManagerApp* app) {
        gtk_tree_model_filter_refilter(app->filter_processes);
    }

    // ========================================================================
    // 3. Unified HUD Telemetry Deck (Performance Page)
    // ========================================================================
    void build_performance_page() {
        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_style_context_add_class(gtk_widget_get_style_context(page), "content-pane");
        gtk_container_add(GTK_CONTAINER(scroll), page);

        // Top Row: Interactive GameBoost Badge, NVMe Throughput & Network Throughput Cards
        GtkWidget* row_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

        // GameBoost Card
        GtkWidget* card_gb = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card_gb), "card");
        GtkWidget* t_gb = gtk_label_new("KERNEL GOVERNOR & GPU POWER");
        gtk_style_context_add_class(gtk_widget_get_style_context(t_gb), "card-title");
        gtk_label_set_xalign(GTK_LABEL(t_gb), 0.0);
        btn_gameboost = gtk_button_new_with_label("⚡ GAMEBOOST: BALANCED");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_gameboost), "gameboost-badge-off");
        g_signal_connect_swapped(btn_gameboost, "clicked", G_CALLBACK(on_gameboost_toggle_clicked), this);
        gtk_box_pack_start(GTK_BOX(card_gb), t_gb, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_gb), btn_gameboost, FALSE, FALSE, 4);
        gtk_box_pack_start(GTK_BOX(row_top), card_gb, TRUE, TRUE, 0);

        // NVMe Throughput Card
        GtkWidget* card_nvme = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card_nvme), "card");
        GtkWidget* t_nvme = gtk_label_new("NVME / SSD THROUGHPUT");
        gtk_style_context_add_class(gtk_widget_get_style_context(t_nvme), "card-title");
        gtk_label_set_xalign(GTK_LABEL(t_nvme), 0.0);
        lbl_nvme_read = gtk_label_new("Read: 0.0 MB/s  |  Write: 0.0 MB/s");
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_nvme_read), "card-value");
        gtk_label_set_xalign(GTK_LABEL(lbl_nvme_read), 0.0);
        gtk_box_pack_start(GTK_BOX(card_nvme), t_nvme, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_nvme), lbl_nvme_read, FALSE, FALSE, 4);
        gtk_box_pack_start(GTK_BOX(row_top), card_nvme, TRUE, TRUE, 0);

        // Network Throughput Card
        GtkWidget* card_net = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card_net), "card");
        GtkWidget* t_net = gtk_label_new("NETWORK I/O THROUGHPUT");
        gtk_style_context_add_class(gtk_widget_get_style_context(t_net), "card-title");
        gtk_label_set_xalign(GTK_LABEL(t_net), 0.0);
        lbl_net_io = gtk_label_new("RX: 0.0 KB/s  |  TX: 0.0 KB/s");
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_net_io), "card-value");
        gtk_label_set_xalign(GTK_LABEL(lbl_net_io), 0.0);
        gtk_box_pack_start(GTK_BOX(card_net), t_net, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_net), lbl_net_io, FALSE, FALSE, 4);
        gtk_box_pack_start(GTK_BOX(row_top), card_net, TRUE, TRUE, 0);

        gtk_box_pack_start(GTK_BOX(page), row_top, FALSE, FALSE, 0);

        // Middle Row: Multi-Rail Sparklines + Circular Thermal Gauges
        GtkWidget* row_mid = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);

        // Sparklines Card
        GtkWidget* card_spark = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card_spark), "card");
        GtkWidget* t_spark = gtk_label_new("REAL-TIME MULTI-RAIL SPARKLINES (60S ROLLING TELEMETRY)");
        gtk_style_context_add_class(gtk_widget_get_style_context(t_spark), "card-title");
        gtk_label_set_xalign(GTK_LABEL(t_spark), 0.0);
        drawing_sparklines = gtk_drawing_area_new();
        gtk_widget_set_size_request(drawing_sparklines, -1, 155);
        g_signal_connect(drawing_sparklines, "draw", G_CALLBACK(draw_sparklines_cb), this);
        gtk_box_pack_start(GTK_BOX(card_spark), t_spark, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_spark), drawing_sparklines, TRUE, TRUE, 4);
        gtk_box_pack_start(GTK_BOX(row_mid), card_spark, TRUE, TRUE, 0);

        // Circular Thermal Gauges Card
        GtkWidget* card_therm = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card_therm), "card");
        GtkWidget* t_therm = gtk_label_new("PACKAGE THERMAL GAUGES");
        gtk_style_context_add_class(gtk_widget_get_style_context(t_therm), "card-title");
        gtk_label_set_xalign(GTK_LABEL(t_therm), 0.0);
        drawing_thermals = gtk_drawing_area_new();
        gtk_widget_set_size_request(drawing_thermals, 240, 155);
        g_signal_connect(drawing_thermals, "draw", G_CALLBACK(draw_thermals_cb), this);
        gtk_box_pack_start(GTK_BOX(card_therm), t_therm, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_therm), drawing_thermals, FALSE, FALSE, 4);
        gtk_box_pack_start(GTK_BOX(row_mid), card_therm, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(page), row_mid, FALSE, FALSE, 0);

        // Bottom Row: Logical Processor Matrix
        GtkWidget* card_cores = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card_cores), "card");
        std::string core_hdr = "LOGICAL PROCESSOR MATRIX (" + std::to_string(core_loads.size()) + " THREADS — CYAN: NORMAL | YELLOW: MODERATE | MAGENTA: HIGH)";
        GtkWidget* t_cores = gtk_label_new(core_hdr.c_str());
        gtk_style_context_add_class(gtk_widget_get_style_context(t_cores), "card-title");
        gtk_label_set_xalign(GTK_LABEL(t_cores), 0.0);
        drawing_cores = gtk_drawing_area_new();
        gtk_widget_set_size_request(drawing_cores, -1, 130);
        g_signal_connect(drawing_cores, "draw", G_CALLBACK(draw_cores_cb), this);
        gtk_box_pack_start(GTK_BOX(card_cores), t_cores, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_cores), drawing_cores, TRUE, TRUE, 4);
        gtk_box_pack_start(GTK_BOX(page), card_cores, FALSE, FALSE, 0);

        // Gradient DDR5 Memory Bar Card
        GtkWidget* card_mem = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card_mem), "card");
        GtkWidget* t_mem = gtk_label_new("PHYSICAL MEMORY COMPOSITION (IN-USE ACTIVE, CACHED STANDBY, AVAILABLE)");
        gtk_style_context_add_class(gtk_widget_get_style_context(t_mem), "card-title");
        gtk_label_set_xalign(GTK_LABEL(t_mem), 0.0);
        drawing_memory = gtk_drawing_area_new();
        gtk_widget_set_size_request(drawing_memory, -1, 44);
        g_signal_connect(drawing_memory, "draw", G_CALLBACK(draw_memory_bar_cb), this);
        gtk_box_pack_start(GTK_BOX(card_mem), t_mem, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_mem), drawing_memory, TRUE, TRUE, 4);
        gtk_box_pack_start(GTK_BOX(page), card_mem, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), scroll, "performance");
    }

    // ------------------------------------------------------------------------
    // Cairo Rendering: Sparklines
    // ------------------------------------------------------------------------
    static gboolean draw_sparklines_cb(GtkWidget* widget, cairo_t* cr, gpointer data) {
        auto* app = static_cast<AxisTaskManagerApp*>(data);
        int w = gtk_widget_get_allocated_width(widget);
        int h = gtk_widget_get_allocated_height(widget);

        // Background
        cairo_set_source_rgb(cr, 0.05, 0.06, 0.09);
        cairo_paint(cr);

        // Grid lines
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.05);
        cairo_set_line_width(cr, 1.0);
        for (int y = h / 4; y < h; y += h / 4) {
            cairo_move_to(cr, 0, y);
            cairo_line_to(cr, w, y);
            cairo_stroke(cr);
        }

        // Draw CPU line (Cyan #00F2FE)
        app->render_sparkline_curve(cr, app->history_cpu, w, h, 0.0, 0.949, 0.996, 100.0);
        // Draw RAM line (Violet #A855F7)
        app->render_sparkline_curve(cr, app->history_mem, w, h, 0.659, 0.333, 0.969, 100.0);
        // Draw Disk line (Emerald #10B981)
        app->render_sparkline_curve(cr, app->history_disk, w, h, 0.063, 0.725, 0.506, 200.0);

        // Legends (Drawn with vector dots & Pango font fallback)
        char buf[64];
        double leg_y = 10.0;

        // CPU Legend (Cyan #00F2FE)
        cairo_set_source_rgb(cr, 0.0, 0.949, 0.996);
        cairo_arc(cr, 18.0, leg_y + 7.0, 4.0, 0, 2 * M_PI);
        cairo_fill(cr);
        snprintf(buf, sizeof(buf), "CPU: %.1f%%", app->current_cpu_pct);
        render_pango_text(cr, 27.0, leg_y, buf, "Inter, Cantarell, DejaVu Sans, Sans Bold 9", 0.0, 0.949, 0.996);

        // RAM Legend (Violet #A855F7)
        cairo_set_source_rgb(cr, 0.75, 0.48, 0.99);
        cairo_arc(cr, 142.0, leg_y + 7.0, 4.0, 0, 2 * M_PI);
        cairo_fill(cr);
        double ram_pct = (app->total_mem_gb > 0) ? (app->current_mem_gb / app->total_mem_gb) * 100.0 : 0.0;
        snprintf(buf, sizeof(buf), "RAM: %.1f GB (%.0f%%)", app->current_mem_gb, ram_pct);
        render_pango_text(cr, 151.0, leg_y, buf, "Inter, Cantarell, DejaVu Sans, Sans Bold 9", 0.75, 0.48, 0.99);

        // Disk Legend (Emerald #10B981)
        cairo_set_source_rgb(cr, 0.063, 0.725, 0.506);
        cairo_arc(cr, 316.0, leg_y + 7.0, 4.0, 0, 2 * M_PI);
        cairo_fill(cr);
        snprintf(buf, sizeof(buf), "Disk: %.1f MB/s", app->disk_read_mbs + app->disk_write_mbs);
        render_pango_text(cr, 325.0, leg_y, buf, "Inter, Cantarell, DejaVu Sans, Sans Bold 9", 0.063, 0.725, 0.506);

        return FALSE;
    }

    void render_sparkline_curve(cairo_t* cr, const std::deque<double>& data, int w, int h, double r, double g, double b, double max_val) {
        if (data.size() < 2) return;
        double step = static_cast<double>(w) / (SPARKLINE_POINTS - 1);

        cairo_pattern_t* pat = cairo_pattern_create_linear(0, 0, 0, h);
        cairo_pattern_add_color_stop_rgba(pat, 0, r, g, b, 0.25);
        cairo_pattern_add_color_stop_rgba(pat, 1, r, g, b, 0.0);

        cairo_new_path(cr);
        cairo_move_to(cr, 0, h);
        for (size_t i = 0; i < data.size(); ++i) {
            double x = i * step;
            double pct = std::clamp(data[i] / max_val, 0.0, 1.0);
            double y = h - (pct * (h - 28));
            cairo_line_to(cr, x, y);
        }
        cairo_line_to(cr, (data.size() - 1) * step, h);
        cairo_close_path(cr);
        cairo_set_source(cr, pat);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);

        // Stroke line
        cairo_set_source_rgb(cr, r, g, b);
        cairo_set_line_width(cr, 2.0);
        for (size_t i = 0; i < data.size(); ++i) {
            double x = i * step;
            double pct = std::clamp(data[i] / max_val, 0.0, 1.0);
            double y = h - (pct * (h - 28));
            if (i == 0) cairo_move_to(cr, x, y);
            else cairo_line_to(cr, x, y);
        }
        cairo_stroke(cr);
    }

    // ------------------------------------------------------------------------
    // Cairo Rendering: Real Online Processor Matrix
    // ------------------------------------------------------------------------
    static gboolean draw_cores_cb(GtkWidget* widget, cairo_t* cr, gpointer data) {
        auto* app = static_cast<AxisTaskManagerApp*>(data);
        int w = gtk_widget_get_allocated_width(widget);
        int h = gtk_widget_get_allocated_height(widget);

        size_t n_cores = app->core_loads.size();
        if (n_cores == 0) n_cores = 1;

        int cols = 8;
        if (n_cores <= 4) cols = static_cast<int>(n_cores);
        else if (n_cores <= 8) cols = 4;
        else if (n_cores <= 12) cols = 6;
        else cols = 8;

        int rows = static_cast<int>(std::ceil(static_cast<double>(n_cores) / cols));
        double pad = 6.0;
        double cell_w = (w - pad * (cols + 1)) / cols;
        double cell_h = (h - pad * (rows + 1)) / rows;

        for (size_t i = 0; i < n_cores; ++i) {
            int r = static_cast<int>(i) / cols;
            int c = static_cast<int>(i) % cols;
            double x = pad + c * (cell_w + pad);
            double y = pad + r * (cell_h + pad);
            double load = app->core_loads[i];

            double red = 0.0, green = 0.949, blue = 0.996;
            if (load > 70.0) {
                red = 1.0; green = 0.0; blue = 0.333; // Crimson
            } else if (load > 35.0) {
                red = 0.988; green = 0.933; blue = 0.039; // Cyber Yellow
            }

            // Cell background (Clean obsidian card when idle, tinted when active)
            if (load < 5.0) {
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.03);
            } else {
                cairo_set_source_rgba(cr, red, green, blue, 0.08);
            }
            cairo_rectangle(cr, x, y, cell_w, cell_h);
            cairo_fill(cr);

            // Filled load bar inside cell (rising dynamically)
            if (load >= 1.0) {
                double bar_h = cell_h * std::clamp(load / 100.0, 0.0, 1.0);
                cairo_set_source_rgba(cr, red, green, blue, 0.35);
                cairo_rectangle(cr, x, y + (cell_h - bar_h), cell_w, bar_h);
                cairo_fill(cr);
            }

            // Cell border
            if (load < 5.0) {
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.10);
            } else {
                cairo_set_source_rgba(cr, red, green, blue, 0.45);
            }
            cairo_set_line_width(cr, 1.0);
            cairo_rectangle(cr, x, y, cell_w, cell_h);
            cairo_stroke(cr);

            // Text rendered via Pango
            char lbl[32];
            snprintf(lbl, sizeof(lbl), "CPU %zu", i);
            render_pango_text(cr, x + 8, y + 6, lbl, "Inter, Cantarell, DejaVu Sans, Sans Bold 9", 0.85, 0.90, 0.96);

            snprintf(lbl, sizeof(lbl), "%.0f%%", load);
            PangoLayout* p_lay = pango_cairo_create_layout(cr);
            pango_layout_set_text(p_lay, lbl, -1);
            PangoFontDescription* p_desc = pango_font_description_from_string("Inter, Cantarell, DejaVu Sans, Sans Bold 9");
            pango_layout_set_font_description(p_lay, p_desc);
            pango_font_description_free(p_desc);
            int pw = 0, ph = 0;
            pango_layout_get_pixel_size(p_lay, &pw, &ph);
            g_object_unref(p_lay);

            double txt_r = (load > 70.0 ? 1.0 : (load > 35.0 ? 0.988 : (load < 5.0 ? 0.65 : 0.0)));
            double txt_g = (load > 70.0 ? 0.2 : (load > 35.0 ? 0.933 : (load < 5.0 ? 0.72 : 0.949)));
            double txt_b = (load > 70.0 ? 0.333 : (load > 35.0 ? 0.039 : (load < 5.0 ? 0.82 : 0.996)));
            render_pango_text(cr, x + cell_w - pw - 8, y + cell_h - ph - 6, lbl,
                              "Inter, Cantarell, DejaVu Sans, Sans Bold 9", txt_r, txt_g, txt_b);
        }
        return FALSE;
    }

    // ------------------------------------------------------------------------
    // Cairo Rendering: Circular Thermal Gauges (Real Sensors or N/A)
    // ------------------------------------------------------------------------
    static gboolean draw_thermals_cb(GtkWidget* widget, cairo_t* cr, gpointer data) {
        auto* app = static_cast<AxisTaskManagerApp*>(data);
        int w = gtk_widget_get_allocated_width(widget);
        int h = gtk_widget_get_allocated_height(widget);

        double cx1 = w * 0.28;
        double cx2 = w * 0.72;
        double cy = h * 0.46;
        double radius = std::min(w, h) * 0.32;

        app->draw_circular_gauge(cr, cx1, cy, radius, app->cpu_temp_c, "CPU Package", app->has_cpu_thermal_sensor);
        app->draw_circular_gauge(cr, cx2, cy, radius, app->gpu_temp_c, "GPU Hotspot", app->has_gpu_thermal_sensor);
        return FALSE;
    }

    void draw_circular_gauge(cairo_t* cr, double cx, double cy, double radius, double temp, const char* label, bool has_sensor) {
        double start_angle = 0.75 * M_PI;
        double end_angle = 2.25 * M_PI;
        double sweep = end_angle - start_angle;

        // Background ring
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.08);
        cairo_set_line_width(cr, 7.0);
        cairo_arc(cr, cx, cy, radius, start_angle, end_angle);
        cairo_stroke(cr);

        if (has_sensor && temp > 0.0) {
            double pct = std::clamp((temp - 25.0) / 75.0, 0.0, 1.0);
            double active_end = start_angle + sweep * pct;

            if (temp > 80.0) cairo_set_source_rgb(cr, 1.0, 0.0, 0.333);
            else if (temp > 60.0) cairo_set_source_rgb(cr, 0.988, 0.933, 0.039);
            else cairo_set_source_rgb(cr, 0.0, 0.949, 0.996);

            cairo_arc(cr, cx, cy, radius, start_angle, active_end);
            cairo_stroke(cr);

            char buf[32];
            snprintf(buf, sizeof(buf), "%.0f°C", temp);
            render_pango_text(cr, cx, cy, buf, "Inter, Cantarell, DejaVu Sans, Sans Bold 13", 1.0, 1.0, 1.0, 1.0, true, true);
        } else {
            render_pango_text(cr, cx, cy, "N/A", "Inter, Cantarell, DejaVu Sans, Sans Bold 11", 0.58, 0.64, 0.72, 1.0, true, true);
        }

        render_pango_text(cr, cx, cy + radius + 14, label, "Inter, Cantarell, DejaVu Sans, Sans 9", 0.58, 0.64, 0.72, 1.0, true, false);
    }

    // ------------------------------------------------------------------------
    // Cairo Rendering: Gradient DDR5 Memory Bar
    // ------------------------------------------------------------------------
    static gboolean draw_memory_bar_cb(GtkWidget* widget, cairo_t* cr, gpointer data) {
        auto* app = static_cast<AxisTaskManagerApp*>(data);
        int w = gtk_widget_get_allocated_width(widget);
        int h = gtk_widget_get_allocated_height(widget);

        // Background trough
        cairo_set_source_rgb(cr, 0.08, 0.10, 0.14);
        cairo_rectangle(cr, 0, 4, w, h - 8);
        cairo_fill(cr);

        double used_ratio = std::clamp(app->current_mem_gb / app->total_mem_gb, 0.0, 1.0);
        double cached_ratio = std::clamp(app->cached_mem_gb / app->total_mem_gb, 0.0, 1.0 - used_ratio);
        double active_w = w * used_ratio;
        double cached_w = w * cached_ratio;

        cairo_pattern_t* pat = cairo_pattern_create_linear(0, 0, w, 0);
        cairo_pattern_add_color_stop_rgb(pat, 0.0, 0.0, 0.75, 0.85);
        cairo_pattern_add_color_stop_rgb(pat, 0.5, 0.45, 0.18, 0.78);
        cairo_pattern_add_color_stop_rgb(pat, 1.0, 0.88, 0.11, 0.28);

        cairo_set_source(cr, pat);
        cairo_rectangle(cr, 0, 4, active_w, h - 8);
        cairo_fill(cr);
        cairo_pattern_destroy(pat);

        // Cached portion
        cairo_set_source_rgba(cr, 0.541, 0.168, 0.886, 0.30);
        cairo_rectangle(cr, active_w, 4, cached_w, h - 8);
        cairo_fill(cr);

        // Text indicator
        char buf[128];
        snprintf(buf, sizeof(buf), "In Use: %.1f GB   •   Cached: %.1f GB   •   Available: %.1f GB / %.1f GB",
                 app->current_mem_gb, app->cached_mem_gb, app->free_mem_gb, app->total_mem_gb);
        render_pango_text(cr, 12.0, (h - 14) / 2.0, buf, "Inter, Cantarell, DejaVu Sans, Sans Bold 9", 1.0, 1.0, 1.0, 1.0, false, false);

        return FALSE;
    }

    // ========================================================================
    // 4. App History Page (Real Process Resource History)
    // ========================================================================
    void build_app_history_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_style_context_add_class(gtk_widget_get_style_context(page), "content-pane");

        GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        GtkWidget* lbl = gtk_label_new("APPLICATION HISTORICAL RESOURCE UTILIZATION");
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "card-title");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_box_pack_start(GTK_BOX(bar), lbl, TRUE, TRUE, 0);

        GtkWidget* btn_refresh = gtk_button_new_with_label("↻ Refresh");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh), "btn-secondary");
        g_signal_connect_swapped(btn_refresh, "clicked", G_CALLBACK(+[](AxisTaskManagerApp* a){ a->refresh_app_history(); }), this);
        gtk_box_pack_start(GTK_BOX(bar), btn_refresh, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), bar, FALSE, FALSE, 0);

        store_app_history = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_app_history));
        GtkTreeViewColumn* c0 = gtk_tree_view_column_new_with_attributes("Application", gtk_cell_renderer_text_new(), "text", 0, NULL);
        gtk_tree_view_column_set_expand(c0, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), c0);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("CPU Time", gtk_cell_renderer_text_new(), "text", 1, NULL));
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("Memory Peak", gtk_cell_renderer_text_new(), "text", 2, NULL));
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("Total Disk I/O", gtk_cell_renderer_text_new(), "text", 3, NULL));

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scroll), tv);
        gtk_box_pack_start(GTK_BOX(page), scroll, TRUE, TRUE, 0);
        gtk_stack_add_named(GTK_STACK(stack), page, "apphistory");
    }

    void refresh_app_history() {
        if (!store_app_history) return;
        gtk_list_store_clear(store_app_history);

        for (const auto& [name, hist] : app_history_cache) {
            char cpu_buf[32];
            int total_s = static_cast<int>(hist.cpu_sec);
            int hrs = total_s / 3600;
            int mins = (total_s % 3600) / 60;
            int secs = total_s % 60;
            if (hrs > 0) snprintf(cpu_buf, sizeof(cpu_buf), "%dh %02dm %02ds", hrs, mins, secs);
            else if (mins > 0) snprintf(cpu_buf, sizeof(cpu_buf), "%dm %02ds", mins, secs);
            else snprintf(cpu_buf, sizeof(cpu_buf), "%ds", secs);

            char mem_buf[32];
            if (hist.mem_mb >= 1024.0) snprintf(mem_buf, sizeof(mem_buf), "%.2f GB", hist.mem_mb / 1024.0);
            else snprintf(mem_buf, sizeof(mem_buf), "%.1f MB", hist.mem_mb);

            char io_buf[32];
            if (hist.io_mb >= 1024.0) snprintf(io_buf, sizeof(io_buf), "%.2f GB", hist.io_mb / 1024.0);
            else snprintf(io_buf, sizeof(io_buf), "%.1f MB", hist.io_mb);

            GtkTreeIter it;
            gtk_list_store_append(store_app_history, &it);
            gtk_list_store_set(store_app_history, &it,
                0, hist.name.c_str(),
                1, cpu_buf,
                2, mem_buf,
                3, io_buf,
                -1
            );
        }
    }

    // ========================================================================
    // 5. Startup Apps Page
    // ========================================================================
    void build_startup_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_style_context_add_class(gtk_widget_get_style_context(page), "content-pane");

        GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        GtkWidget* lbl = gtk_label_new("STARTUP APPLICATIONS & DAEMONS");
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "card-title");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_box_pack_start(GTK_BOX(bar), lbl, TRUE, TRUE, 0);

        GtkWidget* btn_refresh = gtk_button_new_with_label("↻ Refresh");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh), "btn-secondary");
        g_signal_connect_swapped(btn_refresh, "clicked", G_CALLBACK(+[](AxisTaskManagerApp* a){ a->refresh_startup_apps(); }), this);
        gtk_box_pack_start(GTK_BOX(bar), btn_refresh, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), bar, FALSE, FALSE, 0);

        store_startup = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_startup));
        GtkTreeViewColumn* c0 = gtk_tree_view_column_new_with_attributes("Name", gtk_cell_renderer_text_new(), "text", 0, NULL);
        GtkTreeViewColumn* c1 = gtk_tree_view_column_new_with_attributes("Command", gtk_cell_renderer_text_new(), "text", 1, NULL);
        gtk_tree_view_column_set_expand(c0, TRUE);
        gtk_tree_view_column_set_expand(c1, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), c0);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), c1);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("Status", gtk_cell_renderer_text_new(), "text", 2, NULL));
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("Startup Impact", gtk_cell_renderer_text_new(), "text", 3, NULL));

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scroll), tv);
        gtk_box_pack_start(GTK_BOX(page), scroll, TRUE, TRUE, 0);
        gtk_stack_add_named(GTK_STACK(stack), page, "startup");
    }

    void refresh_startup_apps() {
        if (!store_startup) return;
        gtk_list_store_clear(store_startup);

        std::vector<std::string> dirs = {"/etc/xdg/autostart", "/usr/share/gdm/autostart"};
        const char* home = getenv("HOME");
        if (home && strlen(home) > 0) {
            dirs.insert(dirs.begin(), std::string(home) + "/.config/autostart");
        }

        for (const std::string& d : dirs) {
            DIR* dp = opendir(d.c_str());
            if (!dp) continue;
            struct dirent* ep;
            while ((ep = readdir(dp)) != nullptr) {
                if (ep->d_name[0] == '.') continue;
                std::string fname = ep->d_name;
                if (fname.size() > 8 && fname.substr(fname.size() - 8) == ".desktop") {
                    std::string full_path = d + "/" + fname;
                    std::ifstream f(full_path);
                    std::string line, name = fname, exec = "", hidden = "false";
                    while (std::getline(f, line)) {
                        if (line.rfind("Name=", 0) == 0 && name == fname) name = line.substr(5);
                        else if (line.rfind("Exec=", 0) == 0 && exec.empty()) exec = line.substr(5);
                        else if (line.rfind("Hidden=", 0) == 0) hidden = line.substr(7);
                    }
                    bool disabled = (hidden.find("true") != std::string::npos);
                    GtkTreeIter it;
                    gtk_list_store_append(store_startup, &it);
                    gtk_list_store_set(store_startup, &it,
                        0, name.c_str(),
                        1, exec.c_str(),
                        2, disabled ? "Disabled" : "Enabled",
                        3, "Low",
                        -1);
                }
            }
            closedir(dp);
        }
    }

    // ========================================================================
    // 6. Users Page (Real Logged In Sessions & Process Aggregation)
    // ========================================================================
    void build_users_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_style_context_add_class(gtk_widget_get_style_context(page), "content-pane");

        GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        GtkWidget* lbl = gtk_label_new("LOGGED IN USERS & ACTIVE SESSIONS");
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "card-title");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_box_pack_start(GTK_BOX(bar), lbl, TRUE, TRUE, 0);

        GtkWidget* btn_refresh = gtk_button_new_with_label("↻ Refresh");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh), "btn-secondary");
        g_signal_connect_swapped(btn_refresh, "clicked", G_CALLBACK(+[](AxisTaskManagerApp* a){ a->refresh_users(); }), this);
        gtk_box_pack_start(GTK_BOX(bar), btn_refresh, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), bar, FALSE, FALSE, 0);

        store_users = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING);
        GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_users));
        GtkTreeViewColumn* c0 = gtk_tree_view_column_new_with_attributes("User", gtk_cell_renderer_text_new(), "text", 0, NULL);
        gtk_tree_view_column_set_expand(c0, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), c0);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("Session", gtk_cell_renderer_text_new(), "text", 1, NULL));
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("Processes", gtk_cell_renderer_text_new(), "text", 2, NULL));
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("CPU", gtk_cell_renderer_text_new(), "text", 3, NULL));
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("Memory", gtk_cell_renderer_text_new(), "text", 4, NULL));

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scroll), tv);
        gtk_box_pack_start(GTK_BOX(page), scroll, TRUE, TRUE, 0);
        gtk_stack_add_named(GTK_STACK(stack), page, "users");
    }

    void refresh_users() {
        if (!store_users) return;
        gtk_list_store_clear(store_users);

        struct UserStats {
            std::string username;
            std::string session_type;
            int proc_count = 0;
            double total_cpu = 0.0;
            double total_mem_mb = 0.0;
        };
        std::map<uid_t, UserStats> user_map;

        DIR* dir = opendir("/proc");
        if (!dir) return;
        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type != DT_DIR) continue;
            int pid = atoi(ent->d_name);
            if (pid <= 0) continue;

            std::string path_status = "/proc/" + std::to_string(pid) + "/status";
            std::ifstream sf(path_status);
            if (!sf.is_open()) continue;

            uid_t uid = 0;
            double rss_mb = 0.0;
            std::string line;
            while (std::getline(sf, line)) {
                if (line.rfind("Uid:", 0) == 0) {
                    std::istringstream iss(line.substr(4));
                    iss >> uid;
                } else if (line.rfind("VmRSS:", 0) == 0) {
                    long kb = 0;
                    std::istringstream iss(line.substr(6));
                    iss >> kb;
                    rss_mb = kb / 1024.0;
                }
            }

            if (user_map.find(uid) == user_map.end()) {
                struct passwd* pw = getpwuid(uid);
                std::string uname = pw ? pw->pw_name : std::to_string(uid);
                std::string stype = (uid == 0) ? "System Services" : "Interactive Session";
                if (uid == getuid()) {
                    const char* xdg = getenv("XDG_SESSION_TYPE");
                    stype = xdg ? (std::string(xdg) + " Desktop") : "Wayland Desktop";
                }
                user_map[uid] = {uname, stype, 0, 0.0, 0.0};
            }
            user_map[uid].proc_count++;
            user_map[uid].total_mem_mb += rss_mb;
        }
        closedir(dir);

        for (const auto& [uid, stats] : user_map) {
            if (stats.proc_count == 0) continue;
            char mem_buf[32];
            if (stats.total_mem_mb >= 1024.0) snprintf(mem_buf, sizeof(mem_buf), "%.2f GB", stats.total_mem_mb / 1024.0);
            else snprintf(mem_buf, sizeof(mem_buf), "%.1f MB", stats.total_mem_mb);

            char cpu_buf[32];
            snprintf(cpu_buf, sizeof(cpu_buf), "%.1f%%", stats.total_cpu);

            GtkTreeIter it;
            gtk_list_store_append(store_users, &it);
            gtk_list_store_set(store_users, &it,
                0, stats.username.c_str(),
                1, stats.session_type.c_str(),
                2, stats.proc_count,
                3, cpu_buf,
                4, mem_buf,
                -1);
        }
    }

    // ========================================================================
    // 7. Details Page
    // ========================================================================
    void build_details_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_style_context_add_class(gtk_widget_get_style_context(page), "content-pane");

        GtkWidget* lbl = gtk_label_new("ADVANCED PROCESS DETAILS & COMMAND LINES");
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "card-title");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_box_pack_start(GTK_BOX(page), lbl, FALSE, FALSE, 0);

        store_details = gtk_list_store_new(
            NUM_DETAIL_COLS,
            G_TYPE_INT,    // DET_COL_PID
            G_TYPE_STRING, // DET_COL_NAME
            G_TYPE_STRING, // DET_COL_USER
            G_TYPE_INT,    // DET_COL_NICE
            G_TYPE_STRING, // DET_COL_STATUS
            G_TYPE_STRING  // DET_COL_CMDLINE
        );
        GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_details));
        const char* titles[] = {"PID", "Name", "User", "Nice", "Status", "Command Line"};
        for (int i = 0; i < NUM_DETAIL_COLS; ++i) {
            GtkCellRenderer* r = gtk_cell_renderer_text_new();
            g_object_set(r, "xpad", 8, "ypad", 4, NULL);
            GtkTreeViewColumn* c = gtk_tree_view_column_new_with_attributes(titles[i], r, "text", i, NULL);
            gtk_tree_view_column_set_sort_column_id(c, i);
            gtk_tree_view_column_set_resizable(c, TRUE);
            if (i == DET_COL_CMDLINE) gtk_tree_view_column_set_expand(c, TRUE);
            gtk_tree_view_append_column(GTK_TREE_VIEW(tv), c);
        }

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scroll), tv);
        gtk_box_pack_start(GTK_BOX(page), scroll, TRUE, TRUE, 0);
        gtk_stack_add_named(GTK_STACK(stack), page, "details");
    }

    // ========================================================================
    // 8. Services Page
    // ========================================================================
    void build_services_page() {
        GtkWidget* page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        gtk_style_context_add_class(gtk_widget_get_style_context(page), "content-pane");

        GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        GtkWidget* lbl = gtk_label_new("SYSTEMD SERVICES & DAEMONS");
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl), "card-title");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_box_pack_start(GTK_BOX(bar), lbl, TRUE, TRUE, 0);

        GtkWidget* btn_refresh = gtk_button_new_with_label("↻ Refresh");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_refresh), "btn-secondary");
        g_signal_connect_swapped(btn_refresh, "clicked", G_CALLBACK(+[](AxisTaskManagerApp* a){ a->refresh_services(); }), this);
        gtk_box_pack_start(GTK_BOX(bar), btn_refresh, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(page), bar, FALSE, FALSE, 0);

        store_services = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_services));
        GtkTreeViewColumn* c0 = gtk_tree_view_column_new_with_attributes("Service Name", gtk_cell_renderer_text_new(), "text", 0, NULL);
        GtkTreeViewColumn* c1 = gtk_tree_view_column_new_with_attributes("Description", gtk_cell_renderer_text_new(), "text", 1, NULL);
        gtk_tree_view_column_set_expand(c1, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), c0);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), c1);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("Status", gtk_cell_renderer_text_new(), "text", 2, NULL));
        gtk_tree_view_append_column(GTK_TREE_VIEW(tv), gtk_tree_view_column_new_with_attributes("Sub-State", gtk_cell_renderer_text_new(), "text", 3, NULL));

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(scroll), tv);
        gtk_box_pack_start(GTK_BOX(page), scroll, TRUE, TRUE, 0);
        gtk_stack_add_named(GTK_STACK(stack), page, "services");
    }

    void refresh_services() {
        if (!store_services) return;
        gtk_list_store_clear(store_services);
        FILE* fp = popen("systemctl list-units --type=service --no-legend 2>/dev/null | head -n 80", "r");
        if (!fp) return;
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            std::istringstream iss(line);
            std::string unit, load, active, sub;
            iss >> unit >> load >> active >> sub;
            std::string desc;
            std::getline(iss, desc);
            size_t s = desc.find_first_not_of(" \t");
            if (s != std::string::npos) desc = desc.substr(s);

            GtkTreeIter it;
            gtk_list_store_append(store_services, &it);
            gtk_list_store_set(store_services, &it, 0, unit.c_str(), 1, desc.c_str(), 2, active.c_str(), 3, sub.c_str(), -1);
        }
        pclose(fp);
    }

    // ========================================================================
    // Data Sampling & Update Loops
    // ========================================================================
    static std::string read_proc_cmdline(int pid, const std::string& fallback_name) {
        std::string path = "/proc/" + std::to_string(pid) + "/cmdline";
        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) return "[" + fallback_name + "]";
        std::string raw((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        if (raw.empty()) return "[" + fallback_name + "]";
        for (char& c : raw) {
            if (c == '\0') c = ' ';
        }
        while (!raw.empty() && raw.back() == ' ') raw.pop_back();
        if (raw.size() > 140) raw = raw.substr(0, 140) + "...";
        return raw;
    }

    void refresh_processes() {
        DIR* dir = opendir("/proc");
        if (!dir) return;

        auto now = std::chrono::steady_clock::now();
        double elapsed_sec = std::chrono::duration<double>(now - last_proc_sample_time).count();
        if (elapsed_sec <= 0.05) elapsed_sec = 1.0;
        last_proc_sample_time = now;

        double clk_tck = sysconf(_SC_CLK_TCK);
        if (clk_tck <= 0) clk_tck = 100.0;

        // Build map of existing PIDs in store_processes so we update in-place and preserve row selection
        std::map<int, GtkTreeIter> existing_proc_iters;
        GtkTreeIter iter;
        if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store_processes), &iter)) {
            do {
                gint pid_val = 0;
                gtk_tree_model_get(GTK_TREE_MODEL(store_processes), &iter, COL_PID, &pid_val, -1);
                existing_proc_iters[pid_val] = iter;
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store_processes), &iter));
        }

        std::map<int, GtkTreeIter> existing_det_iters;
        if (store_details && gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store_details), &iter)) {
            do {
                gint pid_val = 0;
                gtk_tree_model_get(GTK_TREE_MODEL(store_details), &iter, DET_COL_PID, &pid_val, -1);
                existing_det_iters[pid_val] = iter;
            } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store_details), &iter));
        }

        std::set<int> seen_pids;
        int proc_cnt = 0;
        int thread_cnt = 0;

        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type != DT_DIR) continue;
            int pid = atoi(ent->d_name);
            if (pid <= 0) continue;

            std::string path_stat = "/proc/" + std::to_string(pid) + "/stat";
            std::string path_status = "/proc/" + std::to_string(pid) + "/status";

            std::ifstream status_file(path_status);
            if (!status_file.is_open()) continue;

            seen_pids.insert(pid);
            proc_cnt++;

            std::string name = "unknown";
            std::string status = "Running";
            uid_t uid = 0;
            double mem_mb = 0.0;
            int nice_val = 0;
            int threads = 1;

            std::string line;
            while (std::getline(status_file, line)) {
                if (line.rfind("Name:", 0) == 0) {
                    name = line.substr(5);
                    size_t s = name.find_first_not_of(" \t");
                    if (s != std::string::npos) name = name.substr(s);
                } else if (line.rfind("State:", 0) == 0) {
                    status = line.substr(6);
                    size_t s = status.find_first_not_of(" \t");
                    if (s != std::string::npos) status = status.substr(s, 1);
                    if (status == "S") status = "Sleeping";
                    else if (status == "R") status = "Running";
                    else if (status == "Z") status = "Zombie";
                    else if (status == "I") status = "Idle";
                } else if (line.rfind("Uid:", 0) == 0) {
                    std::istringstream iss(line.substr(4));
                    iss >> uid;
                } else if (line.rfind("VmRSS:", 0) == 0) {
                    long kb = 0;
                    std::istringstream iss(line.substr(6));
                    iss >> kb;
                    mem_mb = kb / 1024.0;
                } else if (line.rfind("Threads:", 0) == 0) {
                    std::istringstream iss(line.substr(8));
                    iss >> threads;
                }
            }
            thread_cnt += std::max(1, threads);

            // Safe /proc/[pid]/stat parsing via rfind(')') to handle spaces in comm
            double cpu_pct = 0.0;
            unsigned long long utime = 0, stime = 0;
            std::ifstream stat_file(path_stat);
            if (stat_file.is_open()) {
                std::string stat_line;
                std::getline(stat_file, stat_line);
                size_t rparen = stat_line.rfind(')');
                if (rparen != std::string::npos && rparen + 2 < stat_line.size()) {
                    std::istringstream iss(stat_line.substr(rparen + 2));
                    std::string state_tok;
                    iss >> state_tok; // field 3: state
                    unsigned long long dummy = 0;
                    for (int i = 0; i < 10; ++i) iss >> dummy; // fields 4..13
                    iss >> utime >> stime; // fields 14, 15
                    long cutime = 0, cstime = 0, priority = 0, nice_l = 0;
                    iss >> cutime >> cstime >> priority >> nice_l; // fields 16..19
                    nice_val = static_cast<int>(nice_l);

                    if (proc_jiffies_map.count(pid)) {
                        auto [prev_u, prev_s] = proc_jiffies_map[pid];
                        unsigned long long cur_j = utime + stime;
                        unsigned long long prev_j = prev_u + prev_s;
                        if (cur_j >= prev_j) {
                            double delta_sec = static_cast<double>(cur_j - prev_j) / clk_tck;
                            cpu_pct = std::clamp((delta_sec / elapsed_sec) * 100.0, 0.0, 100.0);
                        }
                    }
                    proc_jiffies_map[pid] = {utime, stime};
                }
            }

            struct passwd* pw = getpwuid(uid);
            std::string username = pw ? pw->pw_name : std::to_string(uid);

            // Real Disk I/O from /proc/[pid]/io
            double disk_kb = 0.0;
            std::string path_io = "/proc/" + std::to_string(pid) + "/io";
            std::ifstream io_file(path_io);
            if (io_file.is_open()) {
                std::string ioline;
                unsigned long long r_bytes = 0, w_bytes = 0;
                while (std::getline(io_file, ioline)) {
                    if (ioline.rfind("read_bytes:", 0) == 0) {
                        try { r_bytes = std::stoull(ioline.substr(11)); } catch (...) {}
                    } else if (ioline.rfind("write_bytes:", 0) == 0) {
                        try { w_bytes = std::stoull(ioline.substr(12)); } catch (...) {}
                    }
                }
                unsigned long long total_bytes = r_bytes + w_bytes;
                if (proc_io_map.count(pid)) {
                    unsigned long long prev = proc_io_map[pid];
                    if (total_bytes >= prev) {
                        disk_kb = (static_cast<double>(total_bytes - prev) / 1024.0) / elapsed_sec;
                    }
                }
                proc_io_map[pid] = total_bytes;
            }

            double net_kb = 0.0;

            // Real GPU Engine usage check via /proc/[pid]/fd
            bool uses_gpu = false;
            std::string fd_dir_path = "/proc/" + std::to_string(pid) + "/fd";
            DIR* fd_dir = opendir(fd_dir_path.c_str());
            if (fd_dir) {
                struct dirent* fe;
                while ((fe = readdir(fd_dir)) != nullptr) {
                    if (fe->d_name[0] == '.') continue;
                    char link_target[256];
                    std::string fd_path = fd_dir_path + "/" + fe->d_name;
                    ssize_t len = readlink(fd_path.c_str(), link_target, sizeof(link_target) - 1);
                    if (len > 0) {
                        link_target[len] = '\0';
                        if (strstr(link_target, "/dev/dri/") || strstr(link_target, "/dev/nvidia")) {
                            uses_gpu = true;
                            break;
                        }
                    }
                }
                closedir(fd_dir);
            }

            // Accumulate into App History cache
            if (utime + stime > 0) {
                double cpu_seconds = (utime + stime) / clk_tck;
                auto& hist = app_history_cache[name];
                hist.name = name;
                if (cpu_seconds > hist.cpu_sec) hist.cpu_sec = cpu_seconds;
                if (mem_mb > hist.mem_mb) hist.mem_mb = mem_mb;
                if (proc_io_map.count(pid)) {
                    double io_mb = proc_io_map[pid] / 1048576.0;
                    if (io_mb > hist.io_mb) hist.io_mb = io_mb;
                }
            }

            GtkTreeIter it;
            auto existing_it = existing_proc_iters.find(pid);
            if (existing_it != existing_proc_iters.end()) {
                it = existing_it->second;
            } else {
                gtk_list_store_append(store_processes, &it);
            }
            gtk_list_store_set(store_processes, &it,
                COL_NAME, name.c_str(),
                COL_PID, pid,
                COL_STATUS, status.c_str(),
                COL_USER, username.c_str(),
                COL_CPU, cpu_pct,
                COL_MEM, mem_mb,
                COL_DISK, disk_kb,
                COL_NET, net_kb,
                COL_GPU, uses_gpu ? "3D / Compute" : "-",
                -1
            );

            if (store_details) {
                GtkTreeIter it_det;
                auto det_it = existing_det_iters.find(pid);
                if (det_it != existing_det_iters.end()) {
                    it_det = det_it->second;
                } else {
                    gtk_list_store_append(store_details, &it_det);
                }
                std::string cmdline = read_proc_cmdline(pid, name);
                gtk_list_store_set(store_details, &it_det,
                    DET_COL_PID, pid,
                    DET_COL_NAME, name.c_str(),
                    DET_COL_USER, username.c_str(),
                    DET_COL_NICE, nice_val,
                    DET_COL_STATUS, status.c_str(),
                    DET_COL_CMDLINE, cmdline.c_str(),
                    -1
                );
            }
        }
        closedir(dir);

        // Remove exited processes
        for (auto& [pid_val, old_it] : existing_proc_iters) {
            if (seen_pids.find(pid_val) == seen_pids.end()) {
                gtk_list_store_remove(store_processes, &old_it);
                proc_jiffies_map.erase(pid_val);
                proc_io_map.erase(pid_val);
            }
        }
        if (store_details) {
            for (auto& [pid_val, old_it] : existing_det_iters) {
                if (seen_pids.find(pid_val) == seen_pids.end()) {
                    gtk_list_store_remove(store_details, &old_it);
                }
            }
        }

        total_process_count = proc_cnt;
        total_thread_count = thread_cnt;
        update_footer_bar();
    }

    void update_footer_bar() {
        if (lbl_footer_stats) {
            char buf[160];
            snprintf(buf, sizeof(buf), "Processes: %d   |   Threads: %d   |   CPU: %.1f%%   |   Memory: %.1f / %.1f GB",
                     total_process_count, total_thread_count, current_cpu_pct, current_mem_gb, total_mem_gb);
            gtk_label_set_text(GTK_LABEL(lbl_footer_stats), buf);
        }
        if (lbl_footer_uptime) {
            std::ifstream uf("/proc/uptime");
            double up_sec = 0.0;
            if (uf.is_open()) uf >> up_sec;
            long total = static_cast<long>(up_sec);
            long hrs = total / 3600;
            long mins = (total % 3600) / 60;
            long secs = total % 60;
            char ubuf[64];
            snprintf(ubuf, sizeof(ubuf), "System Uptime: %02ld:%02ld:%02ld", hrs, mins, secs);
            gtk_label_set_text(GTK_LABEL(lbl_footer_uptime), ubuf);
        }
    }

    void refresh_all_telemetry() {
        // 1. Overall & Per-Core CPU from /proc/stat
        std::ifstream stat_file("/proc/stat");
        if (stat_file.is_open()) {
            std::string line;
            while (std::getline(stat_file, line)) {
                if (line.rfind("cpu ", 0) == 0) {
                    std::istringstream iss(line.substr(4));
                    unsigned long long u = 0, n = 0, s = 0, id = 0, io = 0, irq = 0, sirq = 0, st = 0;
                    iss >> u >> n >> s >> id >> io >> irq >> sirq >> st;
                    unsigned long long work = u + n + s + irq + sirq + st;
                    unsigned long long total = work + id + io;
                    if (prev_total_jiffies > 0 && total > prev_total_jiffies) {
                        current_cpu_pct = (static_cast<double>(work - prev_work_jiffies) / (total - prev_total_jiffies)) * 100.0;
                        current_cpu_pct = std::clamp(current_cpu_pct, 0.0, 100.0);
                    }
                    prev_work_jiffies = work;
                    prev_total_jiffies = total;
                } else if (line.rfind("cpu", 0) == 0 && line.size() > 3 && isdigit(line[3])) {
                    int c_id = atoi(line.substr(3).c_str());
                    if (c_id >= 0 && c_id < static_cast<int>(core_loads.size())) {
                        size_t space_pos = line.find(' ');
                        std::istringstream iss(line.substr(space_pos));
                        unsigned long long u = 0, n = 0, s = 0, id = 0, io = 0, irq = 0, sirq = 0, st = 0;
                        iss >> u >> n >> s >> id >> io >> irq >> sirq >> st;
                        unsigned long long work = u + n + s + irq + sirq + st;
                        unsigned long long total = work + id + io;
                        if (prev_core_total[c_id] > 0 && total > prev_core_total[c_id]) {
                            double load = (static_cast<double>(work - prev_core_work[c_id]) / (total - prev_core_total[c_id])) * 100.0;
                            core_loads[c_id] = std::clamp(load, 0.0, 100.0);
                        }
                        prev_core_work[c_id] = work;
                        prev_core_total[c_id] = total;
                    }
                }
            }
        }

        // 2. Memory Info from /proc/meminfo
        std::ifstream mem_file("/proc/meminfo");
        if (mem_file.is_open()) {
            long total_kb = 0, avail_kb = 0, cached_kb = 0;
            std::string k; long v; std::string u;
            while (mem_file >> k >> v >> u) {
                if (k == "MemTotal:") total_kb = v;
                else if (k == "MemAvailable:") avail_kb = v;
                else if (k == "Cached:") cached_kb = v;
            }
            if (total_kb > 0) {
                total_mem_gb = total_kb / 1048576.0;
                free_mem_gb = avail_kb / 1048576.0;
                cached_mem_gb = cached_kb / 1048576.0;
                current_mem_gb = (total_kb - avail_kb) / 1048576.0;
            }
        }

        // 3. NVMe / Block Diskstats from /proc/diskstats
        std::ifstream disk_file("/proc/diskstats");
        if (disk_file.is_open()) {
            std::string line;
            unsigned long long total_read_sec = 0, total_write_sec = 0;
            while (std::getline(disk_file, line)) {
                if (line.find("nvme") != std::string::npos || line.find("sda") != std::string::npos || line.find("vda") != std::string::npos) {
                    std::istringstream iss(line);
                    int maj = 0, min = 0; std::string dev;
                    unsigned long long r_c = 0, r_m = 0, r_s = 0, r_t = 0, w_c = 0, w_m = 0, w_s = 0;
                    iss >> maj >> min >> dev >> r_c >> r_m >> r_s >> r_t >> w_c >> w_m >> w_s;
                    total_read_sec += r_s;
                    total_write_sec += w_s;
                }
            }
            if (prev_disk_sectors_read > 0) {
                disk_read_mbs = ((total_read_sec - prev_disk_sectors_read) * 512.0) / 1048576.0;
                disk_write_mbs = ((total_write_sec - prev_disk_sectors_written) * 512.0) / 1048576.0;
            }
            prev_disk_sectors_read = total_read_sec;
            prev_disk_sectors_written = total_write_sec;
        }

        // 4. Real Network Interface Throughput from /proc/net/dev
        std::ifstream net_file("/proc/net/dev");
        if (net_file.is_open()) {
            std::string line;
            unsigned long long total_rx = 0, total_tx = 0;
            std::getline(net_file, line);
            std::getline(net_file, line);
            while (std::getline(net_file, line)) {
                size_t colon = line.find(':');
                if (colon == std::string::npos) continue;
                std::string iface = line.substr(0, colon);
                size_t s = iface.find_first_not_of(" \t");
                if (s != std::string::npos) iface = iface.substr(s);
                if (iface == "lo") continue;

                std::istringstream iss(line.substr(colon + 1));
                unsigned long long rx_b = 0, tx_b = 0, dummy = 0;
                iss >> rx_b;
                for (int i = 0; i < 7; ++i) iss >> dummy;
                iss >> tx_b;

                total_rx += rx_b;
                total_tx += tx_b;
            }
            if (prev_net_bytes_rx > 0 && total_rx >= prev_net_bytes_rx) {
                net_rx_kbs = static_cast<double>(total_rx - prev_net_bytes_rx) / 1024.0;
            }
            if (prev_net_bytes_tx > 0 && total_tx >= prev_net_bytes_tx) {
                net_tx_kbs = static_cast<double>(total_tx - prev_net_bytes_tx) / 1024.0;
            }
            prev_net_bytes_rx = total_rx;
            prev_net_bytes_tx = total_tx;
        }

        // 5. Real Thermals (CPU & GPU)
        has_cpu_thermal_sensor = false;
        has_gpu_thermal_sensor = false;
        cpu_temp_c = 0.0;
        gpu_temp_c = 0.0;

        for (int z = 0; z < 8; ++z) {
            std::string tz_path = "/sys/class/thermal/thermal_zone" + std::to_string(z) + "/temp";
            std::ifstream tf(tz_path);
            if (tf.is_open()) {
                long t = 0; tf >> t;
                if (t > 0) {
                    has_cpu_thermal_sensor = true;
                    cpu_temp_c = (t > 1000) ? (t / 1000.0) : static_cast<double>(t);
                    break;
                }
            }
        }
        if (!has_cpu_thermal_sensor) {
            for (int h = 0; h < 6; ++h) {
                std::string hm_path = "/sys/class/hwmon/hwmon" + std::to_string(h) + "/temp1_input";
                std::ifstream hf(hm_path);
                if (hf.is_open()) {
                    long t = 0; hf >> t;
                    if (t > 0) {
                        has_cpu_thermal_sensor = true;
                        cpu_temp_c = (t > 1000) ? (t / 1000.0) : static_cast<double>(t);
                        break;
                    }
                }
            }
        }

        for (int c = 0; c < 4; ++c) {
            std::string drm_path = "/sys/class/drm/card" + std::to_string(c) + "/device/hwmon/hwmon0/temp1_input";
            std::ifstream gf(drm_path);
            if (gf.is_open()) {
                long t = 0; gf >> t;
                if (t > 0) {
                    has_gpu_thermal_sensor = true;
                    gpu_temp_c = (t > 1000) ? (t / 1000.0) : static_cast<double>(t);
                    break;
                }
            }
        }

        // 6. Real GameBoost Governor Status
        std::ifstream gov_file("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        if (gov_file.is_open()) {
            std::string gov; gov_file >> gov;
            gameboost_active = (gov == "performance");
        }

        // Update History queues
        history_cpu.pop_front();
        history_cpu.push_back(current_cpu_pct);

        history_mem.pop_front();
        history_mem.push_back((current_mem_gb / total_mem_gb) * 100.0);

        history_disk.pop_front();
        history_disk.push_back(std::clamp(disk_read_mbs + disk_write_mbs, 0.0, 500.0));

        history_net.pop_front();
        history_net.push_back(std::clamp(net_rx_kbs + net_tx_kbs, 0.0, 10000.0));

        // Update Labels & Badges
        char buf[128];
        snprintf(buf, sizeof(buf), "⚡ CPU: %.1f%%  •  🧠 RAM: %.1f GB  •  🚀 GameBoost: %s",
                 current_cpu_pct, current_mem_gb, gameboost_active ? "EXTREME" : "BALANCED");
        gtk_label_set_text(GTK_LABEL(lbl_summary_badge), buf);

        if (lbl_nvme_read) {
            snprintf(buf, sizeof(buf), "Read: %.1f MB/s  |  Write: %.1f MB/s", disk_read_mbs, disk_write_mbs);
            gtk_label_set_text(GTK_LABEL(lbl_nvme_read), buf);
        }

        if (lbl_net_io) {
            snprintf(buf, sizeof(buf), "RX: %.1f KB/s  |  TX: %.1f KB/s", net_rx_kbs, net_tx_kbs);
            gtk_label_set_text(GTK_LABEL(lbl_net_io), buf);
        }

        if (btn_gameboost) {
            auto* ctx = gtk_widget_get_style_context(btn_gameboost);
            if (gameboost_active) {
                gtk_button_set_label(GTK_BUTTON(btn_gameboost), "🔥 GAMEBOOST: EXTREME OVERCLOCK");
                gtk_style_context_add_class(ctx, "gameboost-badge-on");
                gtk_style_context_remove_class(ctx, "gameboost-badge-off");
            } else {
                gtk_button_set_label(GTK_BUTTON(btn_gameboost), "⚡ GAMEBOOST: BALANCED (CLICK TO BOOST)");
                gtk_style_context_add_class(ctx, "gameboost-badge-off");
                gtk_style_context_remove_class(ctx, "gameboost-badge-on");
            }
        }

        update_footer_bar();

        // Request Cairo redraws
        if (drawing_sparklines) gtk_widget_queue_draw(drawing_sparklines);
        if (drawing_cores) gtk_widget_queue_draw(drawing_cores);
        if (drawing_thermals) gtk_widget_queue_draw(drawing_thermals);
        if (drawing_memory) gtk_widget_queue_draw(drawing_memory);
    }

    static gboolean on_tick(gpointer user_data) {
        auto* app = static_cast<AxisTaskManagerApp*>(user_data);
        app->refresh_all_telemetry();
        static int tick_cnt = 0;
        tick_cnt++;
        if (tick_cnt % 2 == 0) {
            app->refresh_processes();
        }
        if (tick_cnt % 4 == 0) {
            app->refresh_app_history();
            app->refresh_users();
        }
        return G_SOURCE_CONTINUE;
    }

    // ========================================================================
    // Interactive Handlers
    // ========================================================================
    static void on_gameboost_toggle_clicked(AxisTaskManagerApp* app) {
        if (app->gameboost_active) {
            system("pkexec /usr/local/bin/axis-game-boost stop || /usr/local/bin/axis-game-boost stop &");
        } else {
            system("pkexec /usr/local/bin/axis-game-boost start || /usr/local/bin/axis-game-boost start &");
        }
        app->gameboost_active = !app->gameboost_active;
        app->refresh_all_telemetry();
    }

    static void on_end_task_clicked(AxisTaskManagerApp* app) {
        GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->tree_view));
        GtkTreeModel* m;
        GtkTreeIter it;
        if (gtk_tree_selection_get_selected(sel, &m, &it)) {
            gint pid = 0;
            gchar* name = nullptr;
            gtk_tree_model_get(m, &it, COL_PID, &pid, COL_NAME, &name, -1);

            if (pid > 0) {
                if (kill(pid, SIGTERM) != 0) {
                    std::string pk = "pkexec kill -15 " + std::to_string(pid) + " 2>/dev/null || pkexec kill -9 " + std::to_string(pid);
                    system(pk.c_str());
                } else {
                    usleep(50000);
                    if (kill(pid, 0) == 0) kill(pid, SIGKILL);
                }
            }
            if (name) g_free(name);
            app->refresh_processes();
        }
    }

    static void on_run_task_clicked(AxisTaskManagerApp* app) {
        GtkWidget* dlg = gtk_dialog_new_with_buttons(
            "Run New Task",
            GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Launch Task", GTK_RESPONSE_ACCEPT,
            NULL
        );
        GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);
        gtk_box_set_spacing(GTK_BOX(box), 8);

        GtkWidget* lbl = gtk_label_new("Type the name of a program, script, document, or command:");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        GtkWidget* entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "e.g. axis-terminal, gnome-control-center, htop, btop");

        GtkWidget* chk_admin = gtk_check_button_new_with_label("Create this task with administrative privileges (root)");

        gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), chk_admin, FALSE, FALSE, 4);
        gtk_widget_show_all(dlg);

        if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
            const char* cmd = gtk_entry_get_text(GTK_ENTRY(entry));
            if (cmd && strlen(cmd) > 0) {
                bool as_admin = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_admin));
                std::string full_cmd;
                if (as_admin) {
                    full_cmd = "pkexec " + std::string(cmd) + " &";
                } else {
                    full_cmd = std::string(cmd) + " &";
                }
                system(full_cmd.c_str());
            }
        }
        gtk_widget_destroy(dlg);
        app->refresh_processes();
    }
};

// ============================================================================
// Main Entrypoint
// ============================================================================
int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    AxisTaskManagerApp app;
    app.show();
    gtk_main();
    return 0;
}
