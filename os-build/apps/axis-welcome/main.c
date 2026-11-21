#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/utsname.h>

typedef struct {
    char kernel[256];
    char cpu_model[128];
    int cpu_cores;
    char total_ram[64];
    char gpu_model[128];
    gboolean is_live;
} SystemInfo;

typedef struct {
    GtkWidget *window;
    SystemInfo sys_info;
    GtkWidget *autostart_check;
} WelcomeApp;

static void read_system_info(SystemInfo *info) {
    memset(info, 0, sizeof(*info));

    struct utsname u;
    if (uname(&u) == 0) {
        snprintf(info->kernel, sizeof(info->kernel), "%s (%s)", u.release, u.machine);
    } else {
        g_strlcpy(info->kernel, "Linux XanMod / C11 Kernel", sizeof(info->kernel));
    }

    info->cpu_cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (info->cpu_cores <= 0) info->cpu_cores = 1;

    FILE *f = fopen("/proc/cpuinfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *col = strchr(line, ':');
                if (col) {
                    char *trimmed = g_strstrip(g_strdup(col + 1));
                    g_strlcpy(info->cpu_model, trimmed, sizeof(info->cpu_model));
                    g_free(trimmed);
                    break;
                }
            }
        }
        fclose(f);
    }
    if (!info->cpu_model[0]) {
        g_strlcpy(info->cpu_model, "x86_64 Processor", sizeof(info->cpu_model));
    }

    f = fopen("/proc/meminfo", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                unsigned long total_kb = 0;
                if (sscanf(line + 9, "%lu", &total_kb) == 1) {
                    snprintf(info->total_ram, sizeof(info->total_ram), "%.1f GB", total_kb / (1024.0 * 1024.0));
                }
                break;
            }
        }
        fclose(f);
    }
    if (!info->total_ram[0]) {
        g_strlcpy(info->total_ram, "8.0 GB", sizeof(info->total_ram));
    }

    // Live session detection
    info->is_live = (access("/run/live", F_OK) == 0 ||
                     access("/etc/axisos-installed", F_OK) != 0);

    // Detect GPU (lspci + /sys/class/drm fallback)
    FILE *fp_gpu = popen("lspci 2>/dev/null | grep -iE 'vga|3d|display' | sed 's/.*: //g' | head -n1", "r");
    if (fp_gpu) {
        if (fgets(info->gpu_model, sizeof(info->gpu_model), fp_gpu)) {
            g_strstrip(info->gpu_model);
        }
        pclose(fp_gpu);
    }
    if (!info->gpu_model[0]) {
        char drv_buf[256] = {0};
        ssize_t len = readlink("/sys/class/drm/card0/device/driver", drv_buf, sizeof(drv_buf) - 1);
        if (len > 0) {
            drv_buf[len] = '\0';
            const char *base = strrchr(drv_buf, '/');
            snprintf(info->gpu_model, sizeof(info->gpu_model), "DRM Graphics (%s)", base ? base + 1 : drv_buf);
        } else {
            g_strlcpy(info->gpu_model, "Hardware Accelerated Wayland (wlroots/drm)", sizeof(info->gpu_model));
        }
    }
}

static const char *CSS_WELCOME =
    "@define-color axis_base #141826;"
    "@define-color axis_surface #1F2231;"
    "@define-color axis_surface_raised #262A3D;"
    "@define-color axis_outline #2E3349;"
    "@define-color axis_accent #7D95FF;"
    "@define-color axis_accent_btn_text #0F1422;"
    "@define-color axis_text #E2E8F0;"
    "@define-color axis_text_muted #94A3B8;"
    "window, headerbar, button, checkbutton {"
    "    background-image: none;"
    "    box-shadow: none;"
    "}"
    "window {"
    "    background-color: @axis_base;"
    "    color: @axis_text;"
    "    font-family: 'IBM Plex Sans', sans-serif;"
    "}"
    "headerbar {"
    "    background-color: @axis_surface;"
    "    color: @axis_text;"
    "    border-bottom: 1px solid @axis_outline;"
    "}"
    "frame, frame > border {"
    "    background-color: @axis_surface;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 14px;"
    "}"
    "button.welcome-card {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 14px;"
    "}"
    "button.welcome-card:hover {"
    "    background-color: alpha(@axis_accent, 0.14);"
    "    border-color: @axis_accent;"
    "}"
    "button.btn-close {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 9999px;"
    "    padding: 6px 20px;"
    "    font-weight: 600;"
    "}"
    "button.btn-close:hover {"
    "    border-color: @axis_accent;"
    "    color: @axis_accent;"
    "}";

static void on_launch_action(GtkButton *btn, gpointer user_data) {
    const char *cmd = (const char *)user_data;
    if (cmd) {
        GError *err = NULL;
        g_spawn_command_line_async(cmd, &err);
        if (err) {
            g_warning("Failed to launch %s: %s", cmd, err->message);
            g_error_free(err);
        } else if (g_strcmp0(cmd, "install-axisos") == 0 || g_strcmp0(cmd, "axis-installer") == 0) {
            GtkWidget *toplevel = gtk_widget_get_toplevel(GTK_WIDGET(btn));
            if (GTK_IS_WINDOW(toplevel)) {
                gtk_window_close(GTK_WINDOW(toplevel));
            }
        }
    }
}

static void on_autostart_toggled(GtkToggleButton *btn, gpointer user_data) {
    (void)user_data;
    gboolean active = gtk_toggle_button_get_active(btn);
    gchar *autostart_dir = g_build_filename(g_get_user_config_dir(), "autostart", NULL);
    gchar *desktop_path = g_build_filename(autostart_dir, "axis-welcome.desktop", NULL);
    gchar *axis_cfg_dir = g_build_filename(g_get_user_config_dir(), "axisos", NULL);
    gchar *disabled_marker = g_build_filename(axis_cfg_dir, "welcome-disabled", NULL);

    g_mkdir_with_parents(axis_cfg_dir, 0755);
    if (active) {
        g_mkdir_with_parents(autostart_dir, 0755);
        const char *content =
            "[Desktop Entry]\n"
            "Type=Application\n"
            "Name=AxisOS Welcome\n"
            "Exec=axis-welcome\n"
            "Icon=help-about\n"
            "Terminal=false\n"
            "X-GNOME-Autostart-enabled=true\n";
        g_file_set_contents(desktop_path, content, -1, NULL);
        unlink(disabled_marker);
    } else {
        unlink(desktop_path);
        g_file_set_contents(disabled_marker, "1\n", -1, NULL);
    }
    g_free(autostart_dir);
    g_free(desktop_path);
    g_free(axis_cfg_dir);
    g_free(disabled_marker);
}

static GtkWidget *create_action_card(const char *icon_name, const char *title, const char *desc, const char *cmd) {
    GtkWidget *card = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "welcome-card");
    gtk_widget_set_size_request(card, 236, 98);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(box), 12);

    GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_DND);
    GtkWidget *t_lbl = gtk_label_new(NULL);
    char markup[256];
    snprintf(markup, sizeof(markup), "<b>%s</b>", title);
    gtk_label_set_markup(GTK_LABEL(t_lbl), markup);
    gtk_widget_set_halign(t_lbl, GTK_ALIGN_START);

    gtk_box_pack_start(GTK_BOX(hdr), icon, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hdr), t_lbl, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), hdr, FALSE, FALSE, 0);

    GtkWidget *d_lbl = gtk_label_new(desc);
    gtk_label_set_line_wrap(GTK_LABEL(d_lbl), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(d_lbl), 28);
    gtk_widget_set_halign(d_lbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(box), d_lbl, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(card), box);
    g_signal_connect(card, "clicked", G_CALLBACK(on_launch_action), (gpointer)cmd);

    return card;
}

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
    gtk_css_provider_load_from_data(extra_prov, CSS_WELCOME, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(extra_prov),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(extra_prov);
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("AxisOS Welcome Hub (Pure C11)\n");
        printf("Usage: axis-welcome [OPTION]\n");
        printf("Options:\n");
        printf("  --help, -h    Show this help message and exit\n");
        printf("  --info, -i    Print hardware and system information and exit\n");
        return 0;
    }

    WelcomeApp app = {0};
    read_system_info(&app.sys_info);

    if (argc > 1 && (strcmp(argv[1], "--info") == 0 || strcmp(argv[1], "-i") == 0)) {
        printf("=== AxisOS Hardware & System Information ===\n");
        printf("Kernel:    %s\n", app.sys_info.kernel);
        printf("Processor: %s (%d cores)\n", app.sys_info.cpu_model, app.sys_info.cpu_cores);
        printf("Memory:    %s\n", app.sys_info.total_ram);
        printf("Graphics:  %s\n", app.sys_info.gpu_model);
        printf("Session:   Pure C Wayland (axis-compositor / LiquidNodes)\n");
        printf("Mode:      %s\n", app.sys_info.is_live ? "Live Environment" : "Installed Production System");
        return 0;
    }

    gtk_init(&argc, &argv);
    load_axis_theme();
    GtkSettings *default_settings = gtk_settings_get_default();
    if (default_settings) {
        g_object_set(default_settings,
                     "gtk-application-prefer-dark-theme", TRUE,
                     "gtk-theme-name", "Adwaita-dark",
                     "gtk-icon-theme-name", "Papirus-Dark",
                     NULL);
    }

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app.window), 780, 540);
    gtk_window_set_position(GTK_WINDOW(app.window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(app.window), "Welcome to AxisOS");

    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS Welcome Hub");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Pure C High-Performance Operating System");
    gtk_window_set_titlebar(GTK_WINDOW(app.window), hb);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 20);
    gtk_container_add(GTK_CONTAINER(app.window), main_box);

    // Hero / Header Banner
    GtkWidget *hero_frame = gtk_frame_new(NULL);
    GtkWidget *hero_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(hero_box), 16);

    GtkWidget *title_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_lbl), "<span size='xx-large' weight='bold'>Welcome to AxisOS</span>");
    gtk_widget_set_halign(title_lbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(hero_box), title_lbl, FALSE, FALSE, 0);

    char spec_str[1024];
    snprintf(spec_str, sizeof(spec_str),
             "Kernel: %s  |  CPU: %s (%d cores)  |  RAM: %s  |  GPU: %s\nSession: Pure C Wayland (axis-compositor)  |  Mode: %s",
             app.sys_info.kernel, app.sys_info.cpu_model, app.sys_info.cpu_cores,
             app.sys_info.total_ram, app.sys_info.gpu_model,
             app.sys_info.is_live ? "Live ISO Environment" : "Installed Production System");
    GtkWidget *spec_lbl = gtk_label_new(spec_str);
    gtk_widget_set_halign(spec_lbl, GTK_ALIGN_START);
    gtk_box_pack_start(GTK_BOX(hero_box), spec_lbl, FALSE, FALSE, 2);

    gtk_container_add(GTK_CONTAINER(hero_frame), hero_box);
    gtk_box_pack_start(GTK_BOX(main_box), hero_frame, FALSE, FALSE, 0);

    // Grid of Quick Actions
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_halign(grid, GTK_ALIGN_CENTER);

    GtkWidget *card_set = create_action_card(
        "preferences-system",
        "System Settings",
        "Configure display, network, audio, Bluetooth, power, and themes.",
        "systemsettings"
    );
    GtkWidget *card_soft = create_action_card(
        "system-software-install",
        "Software Manager",
        "Install applications, games, tools, and .axis packages.",
        "axis-software-manager"
    );
    GtkWidget *card_sched = create_action_card(
        "utilities-system-monitor",
        "Kernel & Scheduler",
        "Tune CPU governors, sched-ext BPF schedulers, and I/O queue.",
        "axis-scheduler-manager"
    );
    GtkWidget *card_upd = create_action_card(
        "system-software-update",
        "System Updates",
        "Check for atomic OS updates, Btrfs snapshots, and rollbacks.",
        "axis-update-manager"
    );
    GtkWidget *card_term = create_action_card(
        "utilities-terminal",
        "Terminal Emulator",
        "Open the high-performance GPU-accelerated VTE terminal.",
        "axis-terminal"
    );

    if (app.sys_info.is_live) {
        GtkWidget *card_inst = create_action_card(
            "system-software-install",
            "Install AxisOS",
            "Launch the visual disk partitioner and installation wizard.",
            "install-axisos"
        );
        gtk_grid_attach(GTK_GRID(grid), card_inst, 0, 0, 1, 1);
    } else {
        GtkWidget *card_game = create_action_card(
            "input-gaming",
            "Game Station",
            "Play Steam, Lutris, RetroArch, and PlayStation titles.",
            "axis-game-runner"
        );
        gtk_grid_attach(GTK_GRID(grid), card_game, 0, 0, 1, 1);
    }
    gtk_grid_attach(GTK_GRID(grid), card_set, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card_soft, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card_sched, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card_upd, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), card_term, 1, 2, 1, 1);

    gtk_box_pack_start(GTK_BOX(main_box), grid, TRUE, TRUE, 4);

    // Footer
    GtkWidget *footer_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    app.autostart_check = gtk_check_button_new_with_label("Show this welcome dialog on startup");
    gchar *disabled_file = g_build_filename(g_get_user_config_dir(), "axisos", "welcome-disabled", NULL);
    gchar *autostart_file = g_build_filename(g_get_user_config_dir(), "autostart", "axis-welcome.desktop", NULL);
    gboolean show_on_start = app.sys_info.is_live ? (access(disabled_file, F_OK) != 0)
                                                  : (access(autostart_file, F_OK) == 0 && access(disabled_file, F_OK) != 0);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app.autostart_check), show_on_start);
    g_free(disabled_file);
    g_free(autostart_file);
    g_signal_connect(app.autostart_check, "toggled", G_CALLBACK(on_autostart_toggled), &app);
    gtk_box_pack_start(GTK_BOX(footer_box), app.autostart_check, FALSE, FALSE, 0);

    GtkWidget *close_btn = gtk_button_new_with_label("Close");
    gtk_style_context_add_class(gtk_widget_get_style_context(close_btn), "btn-close");
    g_signal_connect_swapped(close_btn, "clicked", G_CALLBACK(gtk_window_close), app.window);
    gtk_box_pack_end(GTK_BOX(footer_box), close_btn, FALSE, FALSE, 0);

    gtk_box_pack_end(GTK_BOX(main_box), footer_box, FALSE, FALSE, 0);

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(app.window);
    gtk_main();

    return 0;
}
