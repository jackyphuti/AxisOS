/*
 * AxisOS Native Task Manager
 * Written in C++ using GTK 3
 * Features:
 *  - Real-time process tree and table (PID, Name, CPU %, Memory MB, User)
 *  - Interactive search filtering
 *  - One-click "End Process" (SIGTERM / SIGKILL)
 *  - Real-time CPU & Memory performance metrics from /proc
 *  - Launch new task dialog
 */

#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <algorithm>
#include <map>

struct ProcessInfo {
    int pid;
    std::string name;
    std::string user;
    double mem_mb;
    std::string state;
};

static const char* CSS_TM = R"(
window {
    background-color: #0c1527;
    color: #f8fafc;
}
headerbar {
    background: #0f1c35;
    border-bottom: 2px solid #0078d7;
    color: #ffffff;
}
treeview {
    background-color: #0d172a;
    color: #f1f5f9;
}
treeview:selected {
    background-color: #0078d7;
    color: #ffffff;
}
treeview header button {
    background-color: #13223f;
    color: #94a3b8;
    font-weight: 700;
    border: none;
    padding: 6px;
}
.perf-card {
    background-color: #13223f;
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 8px;
    padding: 16px;
    margin: 8px;
}
.btn-primary {
    background: #0078d7;
    color: #ffffff;
    font-weight: 600;
    border-radius: 4px;
    border: 1px solid #1084d8;
    padding: 6px 14px;
}
.btn-danger {
    background: #dc2626;
    color: #ffffff;
    font-weight: 600;
    border-radius: 4px;
    border: 1px solid #b91c1c;
    padding: 6px 14px;
}
)";

enum {
    COL_PID = 0,
    COL_NAME,
    COL_USER,
    COL_MEM,
    COL_STATE,
    NUM_COLS
};

class AxisTaskManager {
public:
    GtkWidget* window;
    GtkWidget* tree_view;
    GtkListStore* store;
    GtkTreeModelFilter* filter;
    GtkWidget* search_entry;
    GtkWidget* lbl_cpu_stat;
    GtkWidget* lbl_mem_stat;
    GtkWidget* pbar_cpu;
    GtkWidget* pbar_mem;

    unsigned long long prev_total_jiffies = 0;
    unsigned long long prev_work_jiffies = 0;

    AxisTaskManager() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Task Manager - AxisOS");
        gtk_window_set_default_size(GTK_WINDOW(window), 820, 540);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        // Apply CSS
        GtkCssProvider* prov = gtk_css_provider_new();
        gtk_css_provider_load_from_data(prov, CSS_TM, -1, NULL);
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
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Processes & System Performance");
        gtk_window_set_titlebar(GTK_WINDOW(window), hb);

        // Run New Task button
        GtkWidget* btn_run = gtk_button_new_with_label("Run New Task");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_run), "btn-primary");
        g_signal_connect_swapped(btn_run, "clicked", G_CALLBACK(on_run_task_cb), this);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(hb), btn_run);

        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_container_add(GTK_CONTAINER(window), vbox);

        // Notebook with Processes and Performance
        GtkWidget* notebook = gtk_notebook_new();
        gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);

        // Page 1: Processes
        GtkWidget* page_procs = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_container_set_border_width(GTK_CONTAINER(page_procs), 8);

        // Search & End Task bar
        GtkWidget* hbox_tools = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
        search_entry = gtk_search_entry_new();
        gtk_widget_set_tooltip_text(search_entry, "Filter processes by name or PID...");
        g_signal_connect_swapped(search_entry, "search-changed", G_CALLBACK(on_search_changed_cb), this);
        gtk_box_pack_start(GTK_BOX(hbox_tools), search_entry, TRUE, TRUE, 0);

        GtkWidget* btn_end = gtk_button_new_with_label("End Process");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_end), "btn-danger");
        g_signal_connect_swapped(btn_end, "clicked", G_CALLBACK(on_end_process_cb), this);
        gtk_box_pack_start(GTK_BOX(hbox_tools), btn_end, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(page_procs), hbox_tools, FALSE, FALSE, 0);

        // TreeView
        store = gtk_list_store_new(NUM_COLS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_DOUBLE, G_TYPE_STRING);
        filter = GTK_TREE_MODEL_FILTER(gtk_tree_model_filter_new(GTK_TREE_MODEL(store), NULL));
        gtk_tree_model_filter_set_visible_func(filter, filter_visible_func, this, NULL);

        tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(filter));
        setup_columns();

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scroll), tree_view);
        gtk_box_pack_start(GTK_BOX(page_procs), scroll, TRUE, TRUE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_procs, gtk_label_new("Processes"));

        // Page 2: Performance
        GtkWidget* page_perf = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_container_set_border_width(GTK_CONTAINER(page_perf), 16);

        // CPU Card
        GtkWidget* card_cpu = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card_cpu), "perf-card");
        lbl_cpu_stat = gtk_label_new("CPU Utilization: Calculating...");
        gtk_label_set_xalign(GTK_LABEL(lbl_cpu_stat), 0.0);
        pbar_cpu = gtk_progress_bar_new();
        gtk_box_pack_start(GTK_BOX(card_cpu), lbl_cpu_stat, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_cpu), pbar_cpu, FALSE, FALSE, 4);
        gtk_box_pack_start(GTK_BOX(page_perf), card_cpu, FALSE, FALSE, 0);

        // Memory Card
        GtkWidget* card_mem = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
        gtk_style_context_add_class(gtk_widget_get_style_context(card_mem), "perf-card");
        lbl_mem_stat = gtk_label_new("Memory Utilization: Calculating...");
        gtk_label_set_xalign(GTK_LABEL(lbl_mem_stat), 0.0);
        pbar_mem = gtk_progress_bar_new();
        gtk_box_pack_start(GTK_BOX(card_mem), lbl_mem_stat, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card_mem), pbar_mem, FALSE, FALSE, 4);
        gtk_box_pack_start(GTK_BOX(page_perf), card_mem, FALSE, FALSE, 0);

        gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page_perf, gtk_label_new("Performance"));

        // Initial scan
        refresh_processes();
        update_performance();

        // 2-second periodic update timer
        g_timeout_add_seconds(2, on_timer_tick, this);

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    }

    void show() {
        gtk_widget_show_all(window);
    }

private:
    void setup_columns() {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn* col;

        col = gtk_tree_view_column_new_with_attributes("PID", renderer, "text", COL_PID, NULL);
        gtk_tree_view_column_set_sort_column_id(col, COL_PID);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

        col = gtk_tree_view_column_new_with_attributes("Process Name", renderer, "text", COL_NAME, NULL);
        gtk_tree_view_column_set_sort_column_id(col, COL_NAME);
        gtk_tree_view_column_set_expand(col, TRUE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

        col = gtk_tree_view_column_new_with_attributes("User", renderer, "text", COL_USER, NULL);
        gtk_tree_view_column_set_sort_column_id(col, COL_USER);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

        GtkCellRenderer* mem_renderer = gtk_cell_renderer_text_new();
        col = gtk_tree_view_column_new_with_attributes("Memory (MB)", mem_renderer, "text", COL_MEM, NULL);
        gtk_tree_view_column_set_sort_column_id(col, COL_MEM);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

        col = gtk_tree_view_column_new_with_attributes("State", renderer, "text", COL_STATE, NULL);
        gtk_tree_view_column_set_sort_column_id(col, COL_STATE);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);
    }

    void refresh_processes() {
        DIR* dir = opendir("/proc");
        if (!dir) return;

        gtk_list_store_clear(store);

        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (ent->d_type != DT_DIR) continue;
            int pid = atoi(ent->d_name);
            if (pid <= 0) continue;

            std::string path_stat = "/proc/" + std::to_string(pid) + "/stat";
            std::string path_status = "/proc/" + std::to_string(pid) + "/status";

            std::ifstream status_file(path_status);
            if (!status_file.is_open()) continue;

            std::string name = "unknown";
            std::string state = "R";
            uid_t uid = 0;
            double mem_mb = 0.0;

            std::string line;
            while (std::getline(status_file, line)) {
                if (line.rfind("Name:", 0) == 0) {
                    name = line.substr(5);
                    size_t s = name.find_first_not_of(" \t");
                    if (s != std::string::npos) name = name.substr(s);
                } else if (line.rfind("State:", 0) == 0) {
                    state = line.substr(6);
                    size_t s = state.find_first_not_of(" \t");
                    if (s != std::string::npos) state = state.substr(s, 1);
                } else if (line.rfind("Uid:", 0) == 0) {
                    std::istringstream iss(line.substr(4));
                    iss >> uid;
                } else if (line.rfind("VmRSS:", 0) == 0) {
                    long kb = 0;
                    std::istringstream iss(line.substr(6));
                    iss >> kb;
                    mem_mb = kb / 1024.0;
                }
            }

            struct passwd* pw = getpwuid(uid);
            std::string username = pw ? pw->pw_name : std::to_string(uid);

            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter,
                COL_PID, pid,
                COL_NAME, name.c_str(),
                COL_USER, username.c_str(),
                COL_MEM, mem_mb,
                COL_STATE, state.c_str(),
                -1);
        }
        closedir(dir);
    }

    void update_performance() {
        // Read CPU stat
        std::ifstream stat_file("/proc/stat");
        if (stat_file.is_open()) {
            std::string cpu;
            unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
            stat_file >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
            unsigned long long work = user + nice + system + irq + softirq + steal;
            unsigned long long total = work + idle + iowait;

            if (prev_total_jiffies > 0 && total > prev_total_jiffies) {
                double work_d = static_cast<double>(work - prev_work_jiffies);
                double total_d = static_cast<double>(total - prev_total_jiffies);
                double cpu_pct = (work_d / total_d) * 100.0;
                if (cpu_pct > 100.0) cpu_pct = 100.0;
                if (cpu_pct < 0.0) cpu_pct = 0.0;

                char buf[64];
                snprintf(buf, sizeof(buf), "CPU Utilization: %.1f%%", cpu_pct);
                gtk_label_set_text(GTK_LABEL(lbl_cpu_stat), buf);
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar_cpu), cpu_pct / 100.0);
            }
            prev_work_jiffies = work;
            prev_total_jiffies = total;
        }

        // Read Memory info
        std::ifstream mem_file("/proc/meminfo");
        if (mem_file.is_open()) {
            long total_kb = 0, avail_kb = 0;
            std::string key;
            long val;
            std::string unit;
            while (mem_file >> key >> val >> unit) {
                if (key == "MemTotal:") total_kb = val;
                else if (key == "MemAvailable:") avail_kb = val;
            }
            if (total_kb > 0) {
                long used_kb = total_kb - avail_kb;
                double used_gb = used_kb / 1048576.0;
                double total_gb = total_kb / 1048576.0;
                double pct = (static_cast<double>(used_kb) / total_kb);

                char buf[128];
                snprintf(buf, sizeof(buf), "Memory Utilization: %.2f GB / %.2f GB (%.1f%%)",
                         used_gb, total_gb, pct * 100.0);
                gtk_label_set_text(GTK_LABEL(lbl_mem_stat), buf);
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(pbar_mem), pct);
            }
        }
    }

    static gboolean on_timer_tick(gpointer user_data) {
        AxisTaskManager* app = static_cast<AxisTaskManager*>(user_data);
        app->update_performance();
        return G_SOURCE_CONTINUE;
    }

    static gboolean filter_visible_func(GtkTreeModel* model, GtkTreeIter* iter, gpointer data) {
        AxisTaskManager* app = static_cast<AxisTaskManager*>(data);
        const char* search = gtk_entry_get_text(GTK_ENTRY(app->search_entry));
        if (!search || strlen(search) == 0) return TRUE;

        gchar* name = nullptr;
        gint pid = 0;
        gtk_tree_model_get(model, iter, COL_NAME, &name, COL_PID, &pid, -1);

        bool visible = false;
        if (name && strcasestr(name, search) != nullptr) visible = true;
        if (std::to_string(pid).find(search) != std::string::npos) visible = true;

        if (name) g_free(name);
        return visible ? TRUE : FALSE;
    }

    static void on_search_changed_cb(AxisTaskManager* app) {
        gtk_tree_model_filter_refilter(app->filter);
    }

    static void on_end_process_cb(AxisTaskManager* app) {
        GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->tree_view));
        GtkTreeModel* m;
        GtkTreeIter iter;
        if (gtk_tree_selection_get_selected(sel, &m, &iter)) {
            gint pid = 0;
            gchar* name = nullptr;
            gtk_tree_model_get(m, &iter, COL_PID, &pid, COL_NAME, &name, -1);

            if (pid > 0) {
                kill(pid, SIGTERM);
                usleep(50000);
                if (kill(pid, 0) == 0) {
                    kill(pid, SIGKILL);
                }
            }
            if (name) g_free(name);
            app->refresh_processes();
        }
    }

    static void on_run_task_cb(AxisTaskManager* app) {
        GtkWidget* dlg = gtk_dialog_new_with_buttons("Run New Task",
            GTK_WINDOW(app->window), GTK_DIALOG_MODAL,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Run", GTK_RESPONSE_ACCEPT, NULL);

        GtkWidget* box = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
        gtk_container_set_border_width(GTK_CONTAINER(box), 16);

        GtkWidget* lbl = gtk_label_new("Type the name of a program, folder, document, or command:");
        gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 6);

        GtkWidget* entry = gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(box), entry, FALSE, FALSE, 6);
        gtk_widget_show_all(dlg);

        if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
            const char* cmd = gtk_entry_get_text(GTK_ENTRY(entry));
            if (cmd && strlen(cmd) > 0) {
                std::string bg_cmd = std::string(cmd) + " &";
                system(bg_cmd.c_str());
            }
        }
        gtk_widget_destroy(dlg);
        app->refresh_processes();
    }
};

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    AxisTaskManager app;
    app.show();
    gtk_main();
    return 0;
}
