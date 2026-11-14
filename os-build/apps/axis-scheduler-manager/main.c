#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *status_label;
    GtkWidget *cpu_usage_bar;
    GtkWidget *cpu_freq_label;
    GtkWidget *sched_combo;
    GtkWidget *gov_combo;
    GtkWidget *io_combo;
    GtkWidget *swappiness_scale;
    GtkWidget *scx_status_badge;
    GtkWidget *start_btn;
    GtkWidget *stop_btn;

    gboolean scx_supported;
    gboolean scx_running;
    char active_scheduler[64];
    char active_governor[64];
    char active_io_sched[64];

    unsigned long long prev_idle;
    unsigned long long prev_total;
    guint telemetry_timer_id;
} SchedulerApp;

static void check_scx_support(SchedulerApp *app) {
    app->scx_supported = (access("/sys/kernel/sched_ext/state", F_OK) == 0 ||
                          access("/sys/kernel/sched_ext", F_OK) == 0);
}

static void read_active_governor(SchedulerApp *app) {
    FILE *f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "r");
    if (f) {
        if (fscanf(f, "%63s", app->active_governor) == 1) {
            // success
        }
        fclose(f);
    } else {
        g_strlcpy(app->active_governor, "schedutil", sizeof(app->active_governor));
    }
}

static void read_swappiness(SchedulerApp *app) {
    FILE *f = fopen("/proc/sys/vm/swappiness", "r");
    if (f) {
        int val = 60;
        if (fscanf(f, "%d", &val) == 1 && app->swappiness_scale) {
            gtk_range_set_value(GTK_RANGE(app->swappiness_scale), val);
        }
        fclose(f);
    }
}

static void read_io_scheduler(SchedulerApp *app) {
    DIR *dir = opendir("/sys/block");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strncmp(ent->d_name, "sd", 2) == 0 || strncmp(ent->d_name, "nvme", 4) == 0) {
                char path[512];
                snprintf(path, sizeof(path), "/sys/block/%s/queue/scheduler", ent->d_name);
                FILE *f = fopen(path, "r");
                if (f) {
                    char line[256] = {0};
                    if (fgets(line, sizeof(line), f)) {
                        char *open_bracket = strchr(line, '[');
                        char *close_bracket = strchr(line, ']');
                        if (open_bracket && close_bracket && close_bracket > open_bracket) {
                            *close_bracket = '\0';
                            g_strlcpy(app->active_io_sched, open_bracket + 1, sizeof(app->active_io_sched));
                        }
                    }
                    fclose(f);
                    break;
                }
            }
        }
        closedir(dir);
    }
    if (!app->active_io_sched[0]) {
        g_strlcpy(app->active_io_sched, "mq-deadline", sizeof(app->active_io_sched));
    }
}

static gboolean update_telemetry(gpointer user_data) {
    SchedulerApp *app = (SchedulerApp *)user_data;

    // 1. CPU Usage
    FILE *f = fopen("/proc/stat", "r");
    if (f) {
        unsigned long long u, n, s, idle, io, irq, sirq, steal;
        if (fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &u, &n, &s, &idle, &io, &irq, &sirq, &steal) == 8) {
            unsigned long long cur_idle = idle + io;
            unsigned long long cur_total = u + n + s + cur_idle + irq + sirq + steal;
            if (app->prev_total > 0 && cur_total > app->prev_total) {
                unsigned long long total_diff = cur_total - app->prev_total;
                unsigned long long idle_diff = cur_idle - app->prev_idle;
                double pct = 100.0 * (1.0 - (double)idle_diff / (double)total_diff);
                if (pct < 0.0) pct = 0.0;
                if (pct > 100.0) pct = 100.0;
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->cpu_usage_bar), pct / 100.0);
                char text[32];
                snprintf(text, sizeof(text), "%.1f%% Load", pct);
                gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->cpu_usage_bar), text);
            }
            app->prev_idle = cur_idle;
            app->prev_total = cur_total;
        }
        fclose(f);
    }

    // 2. CPU Frequency
    double ghz = 0.0;
    f = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
    if (f) {
        unsigned long khz = 0;
        if (fscanf(f, "%lu", &khz) == 1) {
            ghz = khz / 1000000.0;
        }
        fclose(f);
    } else {
        f = fopen("/proc/cpuinfo", "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                if (strncmp(line, "cpu MHz", 7) == 0) {
                    char *col = strchr(line, ':');
                    if (col) {
                        ghz = atof(col + 1) / 1000.0;
                        break;
                    }
                }
            }
            fclose(f);
        }
    }

    if (ghz > 0.0 && app->cpu_freq_label) {
        char freq_str[64];
        snprintf(freq_str, sizeof(freq_str), "Clock: %.2f GHz", ghz);
        gtk_label_set_text(GTK_LABEL(app->cpu_freq_label), freq_str);
    }

    return G_SOURCE_CONTINUE;
}

static void on_governor_changed(GtkComboBoxText *combo, gpointer user_data) {
    SchedulerApp *app = (SchedulerApp *)user_data;
    const gchar *gov = gtk_combo_box_text_get_active_text(combo);
    if (!gov) return;

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pkexec bash -c 'echo %s | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor'", gov);
    g_spawn_command_line_async(cmd, NULL);

    char msg[128];
    snprintf(msg, sizeof(msg), "Active Governor: %s", gov);
    gtk_label_set_text(GTK_LABEL(app->status_label), msg);
}

static void on_io_sched_changed(GtkComboBoxText *combo, gpointer user_data) {
    SchedulerApp *app = (SchedulerApp *)user_data;
    const gchar *sched = gtk_combo_box_text_get_active_text(combo);
    if (!sched) return;

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pkexec bash -c 'echo %s | tee /sys/block/*/queue/scheduler'", sched);
    g_spawn_command_line_async(cmd, NULL);

    char msg[128];
    snprintf(msg, sizeof(msg), "I/O Scheduler set to: %s", sched);
    gtk_label_set_text(GTK_LABEL(app->status_label), msg);
}

static void on_swappiness_changed(GtkRange *range, gpointer user_data) {
    SchedulerApp *app = (SchedulerApp *)user_data;
    int val = (int)gtk_range_get_value(range);

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "pkexec sysctl -w vm.swappiness=%d", val);
    g_spawn_command_line_async(cmd, NULL);

    char msg[128];
    snprintf(msg, sizeof(msg), "vm.swappiness updated to %d", val);
    gtk_label_set_text(GTK_LABEL(app->status_label), msg);
}

static void on_start_scx_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    SchedulerApp *app = (SchedulerApp *)user_data;
    const gchar *sched = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(app->sched_combo));
    if (!sched) return;

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "pkexec systemctl start scx || pkexec %s", sched);
    g_spawn_command_line_async(cmd, NULL);

    app->scx_running = TRUE;
    gtk_label_set_text(GTK_LABEL(app->scx_status_badge), "Running (Active)");
    gtk_label_set_text(GTK_LABEL(app->status_label), "Kernel BPF scheduler active.");
}

static void on_stop_scx_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    SchedulerApp *app = (SchedulerApp *)user_data;
    g_spawn_command_line_async("pkexec systemctl stop scx || pkexec killall -9 scx_bpfland scx_lavd scx_rusty 2>/dev/null", NULL);

    app->scx_running = FALSE;
    gtk_label_set_text(GTK_LABEL(app->scx_status_badge), "Stopped (CFS/EEVDF)");
    gtk_label_set_text(GTK_LABEL(app->status_label), "Standard Linux kernel scheduler active.");
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    SchedulerApp app = {0};
    check_scx_support(&app);
    read_active_governor(&app);
    read_io_scheduler(&app);

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app.window), 680, 520);
    gtk_window_set_title(GTK_WINDOW(app.window), "AxisOS Scheduler & Kernel Performance Manager");

    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "Kernel Scheduler & Tuner");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "sched-ext / CPU Governor / I/O Scheduler");
    gtk_window_set_titlebar(GTK_WINDOW(app.window), hb);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 20);
    gtk_container_add(GTK_CONTAINER(app.window), main_box);

    // Section 1: Telemetry Dashboard
    GtkWidget *dash_frame = gtk_frame_new("CPU & Architecture Telemetry");
    GtkWidget *dash_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(dash_box), 12);

    GtkWidget *telemetry_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    app.cpu_freq_label = gtk_label_new("Clock: -- GHz");
    app.scx_status_badge = gtk_label_new(app.scx_supported ? "sched-ext: Supported" : "sched-ext: Inactive");
    gtk_box_pack_start(GTK_BOX(telemetry_row), app.cpu_freq_label, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(telemetry_row), app.scx_status_badge, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(dash_box), telemetry_row, FALSE, FALSE, 0);

    app.cpu_usage_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(app.cpu_usage_bar), TRUE);
    gtk_box_pack_start(GTK_BOX(dash_box), app.cpu_usage_bar, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(dash_frame), dash_box);
    gtk_box_pack_start(GTK_BOX(main_box), dash_frame, FALSE, FALSE, 0);

    // Section 2: Tuning Grid
    GtkWidget *tune_frame = gtk_frame_new("Kernel Performance Tuning");
    GtkWidget *tune_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(tune_grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(tune_grid), 16);
    gtk_container_set_border_width(GTK_CONTAINER(tune_grid), 14);

    // Row 0: sched-ext
    GtkWidget *sched_lbl = gtk_label_new("BPF Scheduler (sched-ext):");
    gtk_widget_set_halign(sched_lbl, GTK_ALIGN_START);
    app.sched_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app.sched_combo), "scx_bpfland");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app.sched_combo), "scx_lavd");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app.sched_combo), "scx_rusty");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app.sched_combo), "scx_central");
    gtk_combo_box_set_active(GTK_COMBO_BOX(app.sched_combo), 0);
    gtk_widget_set_hexpand(app.sched_combo, TRUE);

    GtkWidget *scx_actions = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    app.start_btn = gtk_button_new_with_label("Start");
    app.stop_btn = gtk_button_new_with_label("Stop");
    g_signal_connect(app.start_btn, "clicked", G_CALLBACK(on_start_scx_clicked), &app);
    g_signal_connect(app.stop_btn, "clicked", G_CALLBACK(on_stop_scx_clicked), &app);
    gtk_box_pack_start(GTK_BOX(scx_actions), app.start_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(scx_actions), app.stop_btn, FALSE, FALSE, 0);

    gtk_grid_attach(GTK_GRID(tune_grid), sched_lbl, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(tune_grid), app.sched_combo, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(tune_grid), scx_actions, 2, 0, 1, 1);

    // Row 1: CPU Scaling Governor
    GtkWidget *gov_lbl = gtk_label_new("CPU Scaling Governor:");
    gtk_widget_set_halign(gov_lbl, GTK_ALIGN_START);
    app.gov_combo = gtk_combo_box_text_new();
    const char *governors[] = {"performance", "powersave", "schedutil", "ondemand", NULL};
    int gov_active_idx = 0;
    for (int i = 0; governors[i] != NULL; ++i) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app.gov_combo), governors[i]);
        if (strcmp(governors[i], app.active_governor) == 0) {
            gov_active_idx = i;
        }
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(app.gov_combo), gov_active_idx);
    g_signal_connect(app.gov_combo, "changed", G_CALLBACK(on_governor_changed), &app);
    gtk_grid_attach(GTK_GRID(tune_grid), gov_lbl, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(tune_grid), app.gov_combo, 1, 1, 2, 1);

    // Row 2: I/O Scheduler
    GtkWidget *io_lbl = gtk_label_new("Disk I/O Scheduler:");
    gtk_widget_set_halign(io_lbl, GTK_ALIGN_START);
    app.io_combo = gtk_combo_box_text_new();
    const char *schedulers[] = {"mq-deadline", "bfq", "kyber", "none", NULL};
    int io_active_idx = 0;
    for (int i = 0; schedulers[i] != NULL; ++i) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app.io_combo), schedulers[i]);
        if (strcmp(schedulers[i], app.active_io_sched) == 0) {
            io_active_idx = i;
        }
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(app.io_combo), io_active_idx);
    g_signal_connect(app.io_combo, "changed", G_CALLBACK(on_io_sched_changed), &app);
    gtk_grid_attach(GTK_GRID(tune_grid), io_lbl, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(tune_grid), app.io_combo, 1, 2, 2, 1);

    // Row 3: Swappiness
    GtkWidget *swap_lbl = gtk_label_new("VM Swappiness (0-100):");
    gtk_widget_set_halign(swap_lbl, GTK_ALIGN_START);
    app.swappiness_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 100, 1);
    gtk_scale_set_draw_value(GTK_SCALE(app.swappiness_scale), TRUE);
    gtk_scale_set_value_pos(GTK_SCALE(app.swappiness_scale), GTK_POS_RIGHT);
    g_signal_connect(app.swappiness_scale, "value-changed", G_CALLBACK(on_swappiness_changed), &app);
    gtk_grid_attach(GTK_GRID(tune_grid), swap_lbl, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(tune_grid), app.swappiness_scale, 1, 3, 2, 1);

    gtk_container_add(GTK_CONTAINER(tune_frame), tune_grid);
    gtk_box_pack_start(GTK_BOX(main_box), tune_frame, FALSE, FALSE, 0);

    read_swappiness(&app);

    // Section 3: Status bar
    app.status_label = gtk_label_new("Kernel scheduler parameters loaded.");
    gtk_widget_set_halign(app.status_label, GTK_ALIGN_START);
    gtk_box_pack_end(GTK_BOX(main_box), app.status_label, FALSE, FALSE, 0);

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(app.window);

    app.telemetry_timer_id = g_timeout_add_seconds(1, update_telemetry, &app);
    update_telemetry(&app);

    gtk_main();

    if (app.telemetry_timer_id) g_source_remove(app.telemetry_timer_id);
    return 0;
}
