/*
 * AxisOS - Hardware Drivers, Firmware & Multimedia Codecs Suite
 * Written in 100% Pure C11 (-std=gnu11) using GTK 3 & Cairo
 *
 * Features:
 *  - Live Hardware Telemetry (CPU, Governor, GPU/Driver, RAM/ZRAM, Audio/Wi-Fi)
 *  - Triple GPU Driver Manager (Auto-Detect, NVIDIA DKMS/Vulkan, AMD RADV, Intel ANV/VA-API)
 *  - Full Multimedia Codecs & Hardware Video Acceleration (FFmpeg, GStreamer, VA-API, VDPAU)
 *  - Gaming & Proton Stack (Steam, Lutris, Heroic, ProtonUp-Qt, GameMode, MangoHud, Wine)
 *  - Non-blocking live Operation Log with progress bar, Copy Log, and instant status refresh
 */

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

typedef struct {
    GtkWidget *badge_widget;
    const char *pkg_primary;
    const char *pkg_alt;
} StatusBadgeRef;

typedef struct {
    GtkWidget *window;
    GtkWidget *notebook;
    GtkWidget *status_label;
    GtkWidget *progress_bar;
    GtkWidget *log_expander;
    GtkWidget *log_view;
    GtkTextBuffer *log_buffer;

    /* Live telemetry labels */
    GtkWidget *lbl_cpu_val;
    GtkWidget *lbl_gpu_val;
    GtkWidget *lbl_mem_val;
    GtkWidget *lbl_audio_val;
    GtkWidget *lbl_gpu_raw;

    GHashTable *installed_set;
    GHashTable *icon_cache;
    GPtrArray *status_badges;
    GPid child_pid;
} AxisHardwareApp;

static char *exec_cmd_short(const char *cmd) {
    if (!cmd) return g_strdup("");
    FILE *pipe = popen(cmd, "r");
    if (!pipe) return g_strdup("");
    GString *res = g_string_new("");
    char buf[512];
    while (fgets(buf, sizeof(buf), pipe) != NULL) {
        g_string_append(res, buf);
    }
    pclose(pipe);
    return g_string_free(res, FALSE);
}

static void scan_flatpak_dir(GHashTable *set, const char *path) {
    DIR *d = opendir(path);
    if (!d) return;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        g_hash_table_insert(set, g_strdup(ent->d_name), GINT_TO_POINTER(1));
    }
    closedir(d);
}

static void refresh_installed_packages(AxisHardwareApp *app) {
    if (!app->installed_set) {
        app->installed_set = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
    } else {
        g_hash_table_remove_all(app->installed_set);
    }

    FILE *fp = fopen("/var/lib/dpkg/status", "r");
    if (fp) {
        static char iobuf[65536];
        setvbuf(fp, iobuf, _IOFBF, sizeof(iobuf));
        char line[256];
        char cur_pkg[128] = {0};
        while (fgets(line, sizeof(line), fp)) {
            if (line[0] == 'P' && strncmp(line, "Package: ", 9) == 0) {
                char *p = g_strstrip(line + 9);
                snprintf(cur_pkg, sizeof(cur_pkg), "%s", p);
            } else if (line[0] == 'S' && cur_pkg[0] &&
                       strncmp(line, "Status: install ok installed", 28) == 0) {
                g_hash_table_insert(app->installed_set, g_strdup(cur_pkg), GINT_TO_POINTER(1));
                cur_pkg[0] = '\0';
            } else if (line[0] == '\n') {
                cur_pkg[0] = '\0';
            }
        }
        fclose(fp);
    }

    scan_flatpak_dir(app->installed_set, "/var/lib/flatpak/app");
    const char *home = g_get_home_dir();
    if (home && *home) {
        char user_fp[512];
        snprintf(user_fp, sizeof(user_fp), "%s/.local/share/flatpak/app", home);
        scan_flatpak_dir(app->installed_set, user_fp);
    }
}

static gboolean is_pkg_installed(AxisHardwareApp *app, const char *p1, const char *p2) {
    if (!app || !app->installed_set) return FALSE;
    if (p1 && g_hash_table_contains(app->installed_set, p1)) return TRUE;
    if (p2 && g_hash_table_contains(app->installed_set, p2)) return TRUE;
    return FALSE;
}

static void update_badge_widget(GtkWidget *badge, gboolean installed) {
    GtkStyleContext *ctx = gtk_widget_get_style_context(badge);
    gtk_style_context_remove_class(ctx, "badge-ok");
    gtk_style_context_remove_class(ctx, "badge-avail");
    if (installed) {
        gtk_label_set_text(GTK_LABEL(badge), "● Installed & Active");
        gtk_style_context_add_class(ctx, "badge-ok");
    } else {
        gtk_label_set_text(GTK_LABEL(badge), "○ Ready to Install");
        gtk_style_context_add_class(ctx, "badge-avail");
    }
}

static void refresh_all_badges(AxisHardwareApp *app) {
    if (!app || !app->status_badges) return;
    for (guint i = 0; i < app->status_badges->len; ++i) {
        StatusBadgeRef *ref = (StatusBadgeRef *)g_ptr_array_index(app->status_badges, i);
        if (ref && ref->badge_widget) {
            gboolean inst = is_pkg_installed(app, ref->pkg_primary, ref->pkg_alt);
            update_badge_widget(ref->badge_widget, inst);
        }
    }
}

/* -----------------------------------------------------------------------------
 * Pure C Cairo 3D Realistic Hardware & Codec Icons
 * ----------------------------------------------------------------------------- */
static void cairo_rounded_rect(cairo_t *cr, double x, double y, double w, double h, double r) {
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -M_PI / 2.0, 0.0);
    cairo_arc(cr, x + w - r, y + h - r, r, 0.0, M_PI / 2.0);
    cairo_arc(cr, x + r, y + h - r, r, M_PI / 2.0, M_PI);
    cairo_arc(cr, x + r, y + r, r, M_PI, 3.0 * M_PI / 2.0);
    cairo_close_path(cr);
}

static GdkPixbuf *get_hw_icon(AxisHardwareApp *app, const char *kind, int size) {
    char key[96];
    snprintf(key, sizeof(key), "%s_%d", kind ? kind : "hw", size);
    if (app->icon_cache) {
        GdkPixbuf *cached = (GdkPixbuf *)g_hash_table_lookup(app->icon_cache, key);
        if (cached) return cached;
    }

    cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t *cr = cairo_create(surf);
    double s = (double)size;

    if (strcmp(kind, "gpu") == 0) {
        /* 3D Graphics Card Icon */
        cairo_pattern_t *g = cairo_pattern_create_linear(0, 0, s, s);
        cairo_pattern_add_color_stop_rgb(g, 0.0, 0.22, 0.74, 0.97);
        cairo_pattern_add_color_stop_rgb(g, 1.0, 0.39, 0.40, 0.95);
        cairo_rounded_rect(cr, s * 0.08, s * 0.20, s * 0.84, s * 0.56, s * 0.12);
        cairo_set_source(cr, g);
        cairo_fill(cr);
        cairo_pattern_destroy(g);

        cairo_set_source_rgba(cr, 0.05, 0.08, 0.14, 0.85);
        cairo_arc(cr, s * 0.34, s * 0.48, s * 0.17, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_arc(cr, s * 0.68, s * 0.48, s * 0.17, 0, 2 * M_PI);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.68, 0.92, 1.0);
        cairo_arc(cr, s * 0.34, s * 0.48, s * 0.06, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_arc(cr, s * 0.68, s * 0.48, s * 0.06, 0, 2 * M_PI);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.96, 0.76, 0.26);
        cairo_rectangle(cr, s * 0.22, s * 0.78, s * 0.50, s * 0.09);
        cairo_fill(cr);
    } else if (strcmp(kind, "codecs") == 0) {
        /* 3D Film / Audio Codec Play Icon */
        cairo_pattern_t *g = cairo_pattern_create_linear(0, 0, s, s);
        cairo_pattern_add_color_stop_rgb(g, 0.0, 0.66, 0.33, 0.97);
        cairo_pattern_add_color_stop_rgb(g, 1.0, 0.22, 0.74, 0.97);
        cairo_rounded_rect(cr, s * 0.10, s * 0.14, s * 0.80, s * 0.72, s * 0.16);
        cairo_set_source(cr, g);
        cairo_fill(cr);
        cairo_pattern_destroy(g);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_move_to(cr, s * 0.40, s * 0.32);
        cairo_line_to(cr, s * 0.70, s * 0.50);
        cairo_line_to(cr, s * 0.40, s * 0.68);
        cairo_close_path(cr);
        cairo_fill(cr);
    } else if (strcmp(kind, "gaming") == 0) {
        /* 3D Gamepad Icon */
        cairo_pattern_t *g = cairo_pattern_create_linear(0, 0, s, s);
        cairo_pattern_add_color_stop_rgb(g, 0.0, 0.20, 0.83, 0.60);
        cairo_pattern_add_color_stop_rgb(g, 1.0, 0.06, 0.56, 0.66);
        cairo_rounded_rect(cr, s * 0.08, s * 0.24, s * 0.84, s * 0.52, s * 0.22);
        cairo_set_source(cr, g);
        cairo_fill(cr);
        cairo_pattern_destroy(g);

        cairo_set_source_rgb(cr, 0.05, 0.10, 0.16);
        cairo_rectangle(cr, s * 0.24, s * 0.45, s * 0.18, s * 0.07);
        cairo_rectangle(cr, s * 0.295, s * 0.39, s * 0.07, s * 0.19);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_arc(cr, s * 0.66, s * 0.44, s * 0.045, 0, 2 * M_PI);
        cairo_fill(cr);
        cairo_arc(cr, s * 0.74, s * 0.53, s * 0.045, 0, 2 * M_PI);
        cairo_fill(cr);
    } else if (strcmp(kind, "cpu") == 0) {
        /* 3D Silicon Processor Die */
        cairo_pattern_t *g = cairo_pattern_create_linear(0, 0, s, s);
        cairo_pattern_add_color_stop_rgb(g, 0.0, 0.25, 0.56, 0.98);
        cairo_pattern_add_color_stop_rgb(g, 1.0, 0.13, 0.32, 0.72);
        cairo_rounded_rect(cr, s * 0.18, s * 0.18, s * 0.64, s * 0.64, s * 0.10);
        cairo_set_source(cr, g);
        cairo_fill(cr);
        cairo_pattern_destroy(g);

        cairo_set_source_rgb(cr, 0.56, 0.86, 1.0);
        cairo_rounded_rect(cr, s * 0.33, s * 0.33, s * 0.34, s * 0.34, s * 0.05);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.96, 0.76, 0.26);
        for (int i = 0; i < 3; ++i) {
            double p = s * (0.30 + i * 0.18);
            cairo_rectangle(cr, p, s * 0.06, s * 0.06, s * 0.10);
            cairo_rectangle(cr, p, s * 0.84, s * 0.06, s * 0.10);
            cairo_rectangle(cr, s * 0.06, p, s * 0.10, s * 0.06);
            cairo_rectangle(cr, s * 0.84, p, s * 0.10, s * 0.06);
        }
        cairo_fill(cr);
    } else if (strcmp(kind, "audio") == 0) {
        /* 3D Audio / Wireless Icon */
        cairo_pattern_t *g = cairo_pattern_create_linear(0, 0, s, s);
        cairo_pattern_add_color_stop_rgb(g, 0.0, 0.98, 0.57, 0.24);
        cairo_pattern_add_color_stop_rgb(g, 1.0, 0.92, 0.34, 0.52);
        cairo_arc(cr, s * 0.50, s * 0.50, s * 0.42, 0, 2 * M_PI);
        cairo_set_source(cr, g);
        cairo_fill(cr);
        cairo_pattern_destroy(g);

        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_set_line_width(cr, s * 0.08);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
        cairo_arc(cr, s * 0.48, s * 0.50, s * 0.16, -0.8, 0.8);
        cairo_stroke(cr);
        cairo_arc(cr, s * 0.48, s * 0.50, s * 0.28, -0.8, 0.8);
        cairo_stroke(cr);
        cairo_arc(cr, s * 0.38, s * 0.50, s * 0.08, 0, 2 * M_PI);
        cairo_fill(cr);
    } else {
        /* AxisOS Emblem */
        cairo_pattern_t *g = cairo_pattern_create_linear(0, 0, s, s);
        cairo_pattern_add_color_stop_rgb(g, 0.0, 0.22, 0.74, 0.97);
        cairo_pattern_add_color_stop_rgb(g, 1.0, 0.49, 0.58, 1.0);
        cairo_rounded_rect(cr, s * 0.06, s * 0.06, s * 0.88, s * 0.88, s * 0.22);
        cairo_set_source(cr, g);
        cairo_fill(cr);
        cairo_pattern_destroy(g);

        cairo_set_source_rgb(cr, 0.05, 0.09, 0.16);
        cairo_move_to(cr, s * 0.50, s * 0.20);
        cairo_line_to(cr, s * 0.78, s * 0.76);
        cairo_line_to(cr, s * 0.22, s * 0.76);
        cairo_close_path(cr);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.95, 0.98, 1.0);
        cairo_move_to(cr, s * 0.50, s * 0.36);
        cairo_line_to(cr, s * 0.66, s * 0.68);
        cairo_line_to(cr, s * 0.34, s * 0.68);
        cairo_close_path(cr);
        cairo_fill(cr);
    }

    cairo_destroy(cr);
    GdkPixbuf *pix = gdk_pixbuf_get_from_surface(surf, 0, 0, size, size);
    cairo_surface_destroy(surf);
    if (pix && app->icon_cache) {
        g_hash_table_insert(app->icon_cache, g_strdup(key), pix);
    }
    return pix;
}

/* -----------------------------------------------------------------------------
 * Valid GTK 3 Obsidian Dark CSS (ZERO !important rules!)
 * ----------------------------------------------------------------------------- */
static const char *CSS_AXIS =
    "window, window.background, .background {"
    "    background-color: #0B0F1A;"
    "    background-image: none;"
    "    color: #F1F5F9;"
    "    font-family: 'IBM Plex Sans', 'Segoe UI', sans-serif;"
    "}"
    "scrolledwindow, viewport, notebook, notebook stack, box, grid {"
    "    background-color: transparent;"
    "    background-image: none;"
    "    border: none;"
    "}"
    "label {"
    "    color: #F1F5F9;"
    "}"
    ".top-banner {"
    "    background-color: #131A2C;"
    "    border-bottom: 1px solid #24304D;"
    "    padding: 14px 20px;"
    "}"
    ".top-title {"
    "    font-weight: 700;"
    "    font-size: 18px;"
    "    color: #FFFFFF;"
    "}"
    ".top-subtitle {"
    "    font-size: 12px;"
    "    color: #94A3B8;"
    "}"
    "notebook header {"
    "    background-color: #101625;"
    "    border-bottom: 1px solid #24304D;"
    "    padding: 4px 16px 0 16px;"
    "}"
    "notebook tab {"
    "    background-color: transparent;"
    "    background-image: none;"
    "    box-shadow: none;"
    "    color: #94A3B8;"
    "    font-weight: 600;"
    "    font-size: 13px;"
    "    border: none;"
    "    border-bottom: 3px solid transparent;"
    "    padding: 10px 18px;"
    "    margin: 0 3px;"
    "    border-radius: 6px 6px 0 0;"
    "}"
    "notebook tab label {"
    "    color: #94A3B8;"
    "}"
    "notebook tab:hover {"
    "    background-color: #192238;"
    "}"
    "notebook tab:hover label {"
    "    color: #FFFFFF;"
    "}"
    "notebook tab:checked {"
    "    background-color: #17223B;"
    "    border-bottom: 3px solid #38BDF8;"
    "}"
    "notebook tab:checked label {"
    "    color: #38BDF8;"
    "    font-weight: 700;"
    "}"
    ".hero-card {"
    "    background-color: #151E33;"
    "    border: 1px solid #2A3A5E;"
    "    border-radius: 12px;"
    "    padding: 18px 22px;"
    "}"
    ".card {"
    "    background-color: #131A2C;"
    "    border: 1px solid #24304D;"
    "    border-radius: 12px;"
    "    padding: 16px 20px;"
    "}"
    ".tele-card {"
    "    background-color: #131A2C;"
    "    border: 1px solid #24304D;"
    "    border-radius: 10px;"
    "    padding: 14px 16px;"
    "}"
    ".card-title {"
    "    font-weight: 700;"
    "    font-size: 14px;"
    "    color: #FFFFFF;"
    "}"
    ".text-muted {"
    "    color: #94A3B8;"
    "    font-size: 12px;"
    "}"
    ".tele-val {"
    "    color: #E2E8F0;"
    "    font-size: 13px;"
    "    font-weight: 600;"
    "}"
    ".status-box {"
    "    background-color: #0D1220;"
    "    border: 1px solid #222D48;"
    "    border-radius: 8px;"
    "    padding: 10px 14px;"
    "    color: #38BDF8;"
    "    font-family: 'IBM Plex Mono', monospace;"
    "    font-size: 12px;"
    "}"
    ".badge-ok {"
    "    background-color: #0F2E28;"
    "    color: #34D399;"
    "    border: 1px solid #1E5A4C;"
    "    border-radius: 12px;"
    "    padding: 3px 10px;"
    "    font-size: 11px;"
    "    font-weight: 700;"
    "}"
    ".badge-avail {"
    "    background-color: #172542;"
    "    color: #38BDF8;"
    "    border: 1px solid #294575;"
    "    border-radius: 12px;"
    "    padding: 3px 10px;"
    "    font-size: 11px;"
    "    font-weight: 700;"
    "}"
    "button {"
    "    background-image: none;"
    "    box-shadow: none;"
    "    text-shadow: none;"
    "}"
    "button.btn-primary {"
    "    background-color: #38BDF8;"
    "    color: #09111E;"
    "    border: 1px solid #38BDF8;"
    "    border-radius: 8px;"
    "    padding: 8px 18px;"
    "    font-weight: 700;"
    "    font-size: 13px;"
    "}"
    "button.btn-primary label {"
    "    color: #09111E;"
    "    font-weight: 700;"
    "}"
    "button.btn-primary:hover {"
    "    background-color: #7DD3FC;"
    "    border-color: #7DD3FC;"
    "}"
    "button.btn-primary:active {"
    "    background-color: #0EA5E9;"
    "}"
    "button.btn-secondary {"
    "    background-color: #1E2942;"
    "    color: #F1F5F9;"
    "    border: 1px solid #32436B;"
    "    border-radius: 8px;"
    "    padding: 8px 16px;"
    "    font-weight: 600;"
    "    font-size: 13px;"
    "}"
    "button.btn-secondary label {"
    "    color: #F1F5F9;"
    "    font-weight: 600;"
    "}"
    "button.btn-secondary:hover {"
    "    background-color: #283657;"
    "    border-color: #38BDF8;"
    "}"
    "button.btn-secondary:hover label {"
    "    color: #38BDF8;"
    "}"
    "button.btn-success {"
    "    background-color: #10B981;"
    "    color: #061A14;"
    "    border: 1px solid #10B981;"
    "    border-radius: 8px;"
    "    padding: 8px 16px;"
    "    font-weight: 700;"
    "    font-size: 13px;"
    "}"
    "button.btn-success label {"
    "    color: #061A14;"
    "    font-weight: 700;"
    "}"
    "button.btn-success:hover {"
    "    background-color: #34D399;"
    "}"
    ".bottom-bar {"
    "    background-color: #101625;"
    "    border-top: 1px solid #24304D;"
    "    padding: 10px 16px;"
    "}"
    "textview, textview text {"
    "    background-color: #0A0E18;"
    "    color: #CBD5E1;"
    "    font-family: 'IBM Plex Mono', monospace;"
    "    font-size: 11px;"
    "}"
    "progressbar trough {"
    "    background-color: #1B243B;"
    "    border-radius: 4px;"
    "    min-height: 6px;"
    "}"
    "progressbar progress {"
    "    background-color: #38BDF8;"
    "    border-radius: 4px;"
    "}"
    "expander title {"
    "    color: #94A3B8;"
    "    font-weight: 600;"
    "    font-size: 12px;"
    "}";

static void load_axis_theme(void) {
    GdkScreen *screen = gdk_screen_get_default();
    if (!screen) return;

    GtkCssProvider *provider = gtk_css_provider_new();
    GError *err = NULL;
    if (!gtk_css_provider_load_from_data(provider, CSS_AXIS, -1, &err)) {
        if (err) {
            fprintf(stderr, "axis-hardware-manager CSS warning: %s\n", err->message);
            g_error_free(err);
        }
    }
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(provider);
}

/* -----------------------------------------------------------------------------
 * Live Hardware Telemetry Refresh
 * ----------------------------------------------------------------------------- */
static void refresh_hardware_telemetry(AxisHardwareApp *app) {
    /* 1. CPU & Governor */
    char cpu_model[128] = "x86_64 Processor";
    int cores = (int)sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0) cores = 1;
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *col = strchr(line, ':');
                if (col) {
                    char *t = g_strstrip(col + 1);
                    snprintf(cpu_model, sizeof(cpu_model), "%s", t);
                    break;
                }
            }
        }
        fclose(fp);
    }
    char gov[64] = "schedutil";
    fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "r");
    if (fp) {
        if (fgets(gov, sizeof(gov), fp)) g_strstrip(gov);
        fclose(fp);
    }
    if (app->lbl_cpu_val) {
        char buf[256];
        snprintf(buf, sizeof(buf), "%s\n%d Threads  •  Governor: %s", cpu_model, cores, gov);
        gtk_label_set_text(GTK_LABEL(app->lbl_cpu_val), buf);
    }

    /* 2. GPU & Kernel Driver */
    char *gpu_raw = exec_cmd_short("lspci -nnk 2>/dev/null | grep -E -A 2 -i 'vga|3d|display' || echo 'Standard Display Adapter'");
    g_strstrip(gpu_raw);
    if (app->lbl_gpu_raw) {
        gtk_label_set_text(GTK_LABEL(app->lbl_gpu_raw), gpu_raw[0] ? gpu_raw : "Hardware Display Adapter Active");
    }
    char *gpu_short = exec_cmd_short("lspci 2>/dev/null | grep -iE 'vga|3d|display' | sed 's/.*: //' | head -n1");
    g_strstrip(gpu_short);
    char drv_name[64] = "modesetting/DRM";
    char linkbuf[256] = {0};
    ssize_t l = readlink("/sys/class/drm/card0/device/driver", linkbuf, sizeof(linkbuf) - 1);
    if (l > 0) {
        linkbuf[l] = '\0';
        const char *base = strrchr(linkbuf, '/');
        snprintf(drv_name, sizeof(drv_name), "%s", base ? base + 1 : linkbuf);
    }
    const char *sess = getenv("WAYLAND_DISPLAY") ? "Wayland" : (getenv("DISPLAY") ? "X11 (Plasma)" : "Console");
    if (app->lbl_gpu_val) {
        char buf[320];
        snprintf(buf, sizeof(buf), "%s\nDriver: %s  •  Display: %s",
                 gpu_short[0] ? gpu_short : "Integrated / Hardware GPU", drv_name, sess);
        gtk_label_set_text(GTK_LABEL(app->lbl_gpu_val), buf);
    }
    g_free(gpu_raw);
    g_free(gpu_short);

    /* 3. RAM & ZRAM */
    unsigned long mem_tot_kb = 0, mem_avail_kb = 0;
    fp = fopen("/proc/meminfo", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strncmp(line, "MemTotal:", 9) == 0) sscanf(line + 9, "%lu", &mem_tot_kb);
            else if (strncmp(line, "MemAvailable:", 13) == 0) sscanf(line + 13, "%lu", &mem_avail_kb);
        }
        fclose(fp);
    }
    gboolean has_zram = FALSE;
    fp = fopen("/proc/swaps", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "zram")) { has_zram = TRUE; break; }
        }
        fclose(fp);
    }
    if (app->lbl_mem_val) {
        double tot_gb = mem_tot_kb / (1024.0 * 1024.0);
        double used_gb = (mem_tot_kb > mem_avail_kb) ? (mem_tot_kb - mem_avail_kb) / (1024.0 * 1024.0) : 0.0;
        char buf[192];
        snprintf(buf, sizeof(buf), "%.1f / %.1f GiB In Use\nZRAM Swap: %s",
                 used_gb, tot_gb, has_zram ? "Active (zstd)" : "Standby");
        gtk_label_set_text(GTK_LABEL(app->lbl_mem_val), buf);
    }

    /* 4. Audio, Wi-Fi & Codecs Summary */
    gboolean has_ffmpeg = is_pkg_installed(app, "ffmpeg", NULL);
    gboolean has_gst = is_pkg_installed(app, "gstreamer1.0-libav", "gstreamer1.0-plugins-bad");
    gboolean has_pipewire = is_pkg_installed(app, "pipewire", "pipewire-audio");
    gboolean has_bt = is_pkg_installed(app, "bluez", NULL);
    if (app->lbl_audio_val) {
        char buf[192];
        snprintf(buf, sizeof(buf), "Audio: %s  •  Bluetooth: %s\nCodecs: %s",
                 has_pipewire ? "PipeWire Ready" : "ALSA",
                 has_bt ? "BlueZ Ready" : "Not Installed",
                 (has_ffmpeg && has_gst) ? "FFmpeg + GStreamer Active" : "Click Install Codecs");
        gtk_label_set_text(GTK_LABEL(app->lbl_audio_val), buf);
    }
}

/* -----------------------------------------------------------------------------
 * Async Live Command Runner (Non-blocking with Real Output Log & Progress Bar)
 * ----------------------------------------------------------------------------- */
static void append_log(AxisHardwareApp *app, const char *text) {
    if (!app || !app->log_buffer || !text) return;
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(app->log_buffer, &end);
    gtk_text_buffer_insert(app->log_buffer, &end, text, -1);
    GtkTextMark *mark = gtk_text_buffer_get_insert(app->log_buffer);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->log_view), mark, 0.0, FALSE, 0.0, 1.0);
}

static gboolean on_read_job_output(GIOChannel *chan, GIOCondition cond, gpointer user_data) {
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    if (cond & G_IO_IN) {
        gchar buf[1024];
        gsize bytes_read = 0;
        while (g_io_channel_read_chars(chan, buf, sizeof(buf) - 1, &bytes_read, NULL) == G_IO_STATUS_NORMAL && bytes_read > 0) {
            buf[bytes_read] = '\0';
            append_log(app, buf);
            gtk_progress_bar_pulse(GTK_PROGRESS_BAR(app->progress_bar));
        }
    }
    if (cond & (G_IO_HUP | G_IO_ERR)) {
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static void on_job_exit(GPid pid, gint status, gpointer user_data) {
    (void)pid;
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    g_spawn_close_pid(app->child_pid);
    app->child_pid = 0;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 1.0);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Completed successfully!");
        append_log(app, "\n[SUCCESS] Operation completed cleanly.\n");
    } else {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Operation finished with warnings/errors — see Operation Log.");
        append_log(app, "\n[ERROR] Command exited with non-zero status. Check output above.\n");
    }

    refresh_installed_packages(app);
    refresh_all_badges(app);
    refresh_hardware_telemetry(app);
}

static void run_hw_job(AxisHardwareApp *app, const char *title, const char *shell_cmd) {
    if (!app || !shell_cmd) return;
    if (app->child_pid > 0) {
        gtk_label_set_text(GTK_LABEL(app->status_label), "Another task is already running — please wait...");
        return;
    }

    if (app->log_expander) {
        gtk_expander_set_expanded(GTK_EXPANDER(app->log_expander), TRUE);
    }
    gtk_text_buffer_set_text(app->log_buffer, "", -1);
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 0.12);
    gtk_label_set_text(GTK_LABEL(app->status_label), title);

    char hdr[1536];
    snprintf(hdr, sizeof(hdr), ">>> %s\n\n", title);
    append_log(app, hdr);

    char wrapped[3072];
    snprintf(wrapped, sizeof(wrapped), "( %s ) 2>&1", shell_cmd);
    gchar *argv[] = {"/bin/bash", "-c", wrapped, NULL};

    GError *err = NULL;
    gint out_fd = -1;
    if (g_spawn_async_with_pipes(NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH,
                                 NULL, NULL, &app->child_pid, NULL, &out_fd, NULL, &err)) {
        GIOChannel *ch = g_io_channel_unix_new(out_fd);
        g_io_channel_set_encoding(ch, NULL, NULL);
        g_io_channel_set_flags(ch, G_IO_FLAG_NONBLOCK, NULL);
        g_io_add_watch(ch, G_IO_IN | G_IO_HUP | G_IO_ERR, on_read_job_output, app);
        g_io_channel_unref(ch);
        g_child_watch_add(app->child_pid, on_job_exit, app);
    } else if (err) {
        append_log(app, err->message);
        g_error_free(err);
    }
}

static void run_apt_install(AxisHardwareApp *app, const char *title, const char *packages) {
    const char *elev = (geteuid() == 0) ? "" : "pkexec ";
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "%s/bin/bash -c 'export DEBIAN_FRONTEND=noninteractive; "
             "dpkg --configure -a 2>/dev/null || true; "
             "apt-get -o DPkg::Lock::Timeout=60 update && "
             "apt-get -o DPkg::Lock::Timeout=60 install -y --fix-broken %s'",
             elev, packages);
    run_hw_job(app, title, cmd);
}

static void run_flatpak_install(AxisHardwareApp *app, const char *title, const char *app_id) {
    const char *elev = (geteuid() == 0) ? "" : "pkexec ";
    char *quoted = g_shell_quote(app_id);
    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "if ! command -v flatpak >/dev/null 2>&1; then "
             "%s/bin/bash -c 'export DEBIAN_FRONTEND=noninteractive; apt-get -o DPkg::Lock::Timeout=60 update && apt-get -o DPkg::Lock::Timeout=60 install -y flatpak ca-certificates'; fi && "
             "{ flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo 2>/dev/null || true; "
             "flatpak remote-modify --user --enable flathub 2>/dev/null || true; "
             "flatpak install --user -y --noninteractive flathub %s; } || "
             "%s/bin/bash -c 'flatpak remote-add --if-not-exists --system flathub https://dl.flathub.org/repo/flathub.flatpakrepo 2>/dev/null || true; "
             "flatpak remote-modify --system --enable flathub 2>/dev/null || true; "
             "flatpak install --system -y --noninteractive flathub \"$1\"' _ %s",
             elev, quoted, elev, quoted);
    g_free(quoted);
    run_hw_job(app, title, cmd);
}

/* -----------------------------------------------------------------------------
 * Action Callbacks (100% Real & Functional)
 * ----------------------------------------------------------------------------- */
static void on_refresh_all_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    refresh_installed_packages(app);
    refresh_all_badges(app);
    refresh_hardware_telemetry(app);
    gtk_label_set_text(GTK_LABEL(app->status_label), "Hardware telemetry & codec status refreshed.");
}

static void on_copy_log_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(app->log_buffer, &start, &end);
    gchar *text = gtk_text_buffer_get_text(app->log_buffer, &start, &end, FALSE);
    if (text && *text) {
        GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        gtk_clipboard_set_text(cb, text, -1);
        gtk_clipboard_store(cb);
        gtk_label_set_text(GTK_LABEL(app->status_label), "Operation Log copied to clipboard.");
    }
    g_free(text);
}

static void on_install_complete_pack_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_apt_install((AxisHardwareApp *)user_data,
        "Installing Complete Multimedia Codecs, Hardware Video Acceleration & Firmware Pack...",
        "ffmpeg libavcodec-extra gstreamer1.0-plugins-base gstreamer1.0-plugins-good "
        "gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-vaapi "
        "mesa-vulkan-drivers mesa-va-drivers mesa-vdpau-drivers va-driver-all vulkan-tools vainfo "
        "firmware-linux-nonfree firmware-misc-nonfree firmware-amd-graphics firmware-iwlwifi firmware-realtek");
}

static void on_auto_gpu_setup_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    const char *elev = (geteuid() == 0) ? "" : "pkexec ";
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "if [ -x /usr/local/bin/axis-gpu-setup.sh ]; then %s/usr/local/bin/axis-gpu-setup.sh; "
             "else %s/bin/bash -c 'export DEBIAN_FRONTEND=noninteractive; apt-get -o DPkg::Lock::Timeout=60 update && "
             "apt-get -o DPkg::Lock::Timeout=60 install -y mesa-vulkan-drivers mesa-va-drivers mesa-vdpau-drivers libvulkan1 vulkan-tools vainfo firmware-linux-nonfree firmware-amd-graphics'; fi",
             elev, elev);
    run_hw_job(app, "Running AxisOS Automatic GPU Hardware Detection & Driver Installer...", cmd);
}

static void on_install_nvidia_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_apt_install((AxisHardwareApp *)user_data,
        "Installing Proprietary NVIDIA Drivers, Kernel DKMS & Vulkan ICD...",
        "linux-headers-amd64 nvidia-driver nvidia-kernel-dkms firmware-misc-nonfree vulkan-tools");
}

static void on_install_amd_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_apt_install((AxisHardwareApp *)user_data,
        "Installing AMD Radeon Firmware, Mesa RADV Vulkan & VA-API Drivers...",
        "firmware-amd-graphics mesa-vulkan-drivers mesa-va-drivers mesa-vdpau-drivers libvulkan1 vulkan-tools vainfo");
}

static void on_verify_amd_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_hw_job((AxisHardwareApp *)user_data,
        "Probing Vulkan & Mesa GPU Drivers (vulkaninfo)...",
        "lspci -nnk | grep -E -A 3 -i 'vga|3d|display'; echo ''; "
        "if command -v vulkaninfo >/dev/null 2>&1; then vulkaninfo --summary 2>&1; "
        "else echo 'vulkan-tools not installed yet — click Install AMD/Intel Vulkan above.'; fi");
}

static void on_install_intel_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_apt_install((AxisHardwareApp *)user_data,
        "Installing Intel Arc / Iris Xe Mesa ANV Vulkan & Media VA-API Drivers...",
        "intel-media-va-driver i965-va-driver mesa-vulkan-drivers libva-drm2 libva-x11-2 vainfo vulkan-tools");
}

static void on_verify_intel_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_hw_job((AxisHardwareApp *)user_data,
        "Probing Hardware Video Decode / VA-API Profiles (vainfo)...",
        "if command -v vainfo >/dev/null 2>&1; then vainfo 2>&1; "
        "else echo 'vainfo not installed yet — click Install Hardware Video Acceleration above.'; fi");
}

static void on_install_firmware_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_apt_install((AxisHardwareApp *)user_data,
        "Installing Wi-Fi, Bluetooth & Audio Hardware Firmware Pack...",
        "firmware-linux-nonfree firmware-misc-nonfree firmware-iwlwifi firmware-realtek "
        "firmware-atheros firmware-brcm80211 firmware-intel-sound firmware-sof-signed bluez pipewire-audio");
}

static void on_install_codecs_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_apt_install((AxisHardwareApp *)user_data,
        "Installing Full Multimedia Codecs (FFmpeg, GStreamer, H.264, HEVC, AV1, AAC, MP3)...",
        "ffmpeg libavcodec-extra gstreamer1.0-plugins-base gstreamer1.0-plugins-good "
        "gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav gstreamer1.0-vaapi");
}

static void on_install_vaapi_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_apt_install((AxisHardwareApp *)user_data,
        "Installing VA-API & VDPAU Hardware Video Acceleration...",
        "va-driver-all mesa-va-drivers mesa-vdpau-drivers intel-media-va-driver i965-va-driver libva2 vainfo");
}

static void on_test_codecs_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_hw_job((AxisHardwareApp *)user_data,
        "Verifying Active Audio & Video Decoders (FFmpeg + GStreamer)...",
        "echo '=== FFmpeg Version ==='; ffmpeg -version 2>&1 | head -n 3; "
        "echo ''; echo '=== Hardware & Modern Video Codecs (H.264 / HEVC / AV1 / VP9) ==='; "
        "ffmpeg -hide_banner -decoders 2>/dev/null | grep -E ' (h264|hevc|av1|vp9|aac|mp3|opus) ' | head -n 20; "
        "echo ''; echo '=== VA-API Hardware Acceleration ==='; "
        "if command -v vainfo >/dev/null 2>&1; then vainfo 2>&1 | head -n 25; else echo 'vainfo not installed'; fi");
}

static void on_install_archives_fonts_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_apt_install((AxisHardwareApp *)user_data,
        "Installing Archive Extractors & Web/Emoji Fonts...",
        "p7zip-full unrar-free unzip zip fonts-liberation fonts-noto-color-emoji fonts-dejavu-core");
}

static void on_set_governor_perf_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *elev = (geteuid() == 0) ? "" : "pkexec ";
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "%s/bin/sh -c 'for g in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do [ -f \"$g\" ] && echo performance > \"$g\" 2>/dev/null || true; done; "
             "echo \"Active CPU Governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo unsupported)\"'",
             elev);
    run_hw_job((AxisHardwareApp *)user_data, "Switching CPU Scaling Governor to Performance Mode...", cmd);
}

static void on_set_governor_bal_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *elev = (geteuid() == 0) ? "" : "pkexec ";
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "%s/bin/sh -c 'for g in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do [ -f \"$g\" ] && (echo schedutil > \"$g\" 2>/dev/null || echo powersave > \"$g\" 2>/dev/null || true); done; "
             "echo \"Active CPU Governor: $(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo unsupported)\"'",
             elev);
    run_hw_job((AxisHardwareApp *)user_data, "Switching CPU Scaling Governor to Balanced (Schedutil) Mode...", cmd);
}

static void on_repair_audio_bt_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    const char *elev = (geteuid() == 0) ? "" : "pkexec ";
    char cmd[768];
    snprintf(cmd, sizeof(cmd),
             "systemctl --user restart pipewire pipewire-pulse wireplumber 2>&1 || true; "
             "%s/bin/sh -c 'rfkill unblock all 2>/dev/null || true; systemctl enable --now bluetooth.service 2>&1 || true; systemctl restart NetworkManager 2>&1 || true'; "
             "echo 'Audio (PipeWire), Bluetooth, and Wi-Fi services restarted.'",
             elev);
    run_hw_job((AxisHardwareApp *)user_data, "Restarting PipeWire Audio, Bluetooth & Wi-Fi Services...", cmd);
}

static void on_install_steam_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_flatpak_install((AxisHardwareApp *)user_data,
        "Installing Valve Steam (Flathub with Proton Support)...",
        "com.valvesoftware.Steam");
}

static void on_launch_steam_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    AxisHardwareApp *app = (AxisHardwareApp *)user_data;
    if (is_pkg_installed(app, "com.valvesoftware.Steam", NULL)) {
        g_spawn_command_line_async("flatpak run com.valvesoftware.Steam", NULL);
        gtk_label_set_text(GTK_LABEL(app->status_label), "Launching Steam (Flatpak)...");
    } else if (system("command -v steam >/dev/null 2>&1") == 0) {
        g_spawn_command_line_async("steam", NULL);
        gtk_label_set_text(GTK_LABEL(app->status_label), "Launching Steam...");
    } else {
        on_install_steam_cb(btn, user_data);
    }
}

static void on_install_lutris_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_flatpak_install((AxisHardwareApp *)user_data,
        "Installing Lutris Game Manager (Flathub)...",
        "net.lutris.Lutris");
}

static void on_install_heroic_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_flatpak_install((AxisHardwareApp *)user_data,
        "Installing Heroic Games Launcher — Epic, GOG & Amazon (Flathub)...",
        "com.heroicgameslauncher.hgl");
}

static void on_install_protonup_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_flatpak_install((AxisHardwareApp *)user_data,
        "Installing ProtonUp-Qt for GE-Proton & Wine-GE (Flathub)...",
        "net.davidotek.pupgui2");
}

static void on_install_gamestack_cb(GtkButton *btn, gpointer user_data) {
    (void)btn;
    run_apt_install((AxisHardwareApp *)user_data,
        "Installing GameMode, MangoHud, Gamescope, Wine & Vulkan Tools...",
        "gamemode mangohud gamescope wine winetricks vulkan-tools");
}

/* -----------------------------------------------------------------------------
 * UI Builders
 * ----------------------------------------------------------------------------- */
static GtkWidget *make_btn(const char *label, const char *css_class, GCallback cb, AxisHardwareApp *app) {
    GtkWidget *btn = gtk_button_new_with_label(label);
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), css_class);
    if (cb) g_signal_connect(btn, "clicked", cb, app);
    return btn;
}

static GtkWidget *create_feature_card(AxisHardwareApp *app, const char *icon_kind,
                                      const char *title, const char *desc,
                                      const char *pkg_primary, const char *pkg_alt,
                                      GtkWidget **out_btn_box) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

    GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *ico = gtk_image_new_from_pixbuf(get_hw_icon(app, icon_kind, 28));
    gtk_box_pack_start(GTK_BOX(hdr), ico, FALSE, FALSE, 0);

    GtkWidget *lbl_t = gtk_label_new(title);
    gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_t), "card-title");
    gtk_box_pack_start(GTK_BOX(hdr), lbl_t, TRUE, TRUE, 0);

    if (pkg_primary) {
        GtkWidget *badge = gtk_label_new("");
        update_badge_widget(badge, is_pkg_installed(app, pkg_primary, pkg_alt));
        gtk_box_pack_end(GTK_BOX(hdr), badge, FALSE, FALSE, 0);

        StatusBadgeRef *ref = g_new0(StatusBadgeRef, 1);
        ref->badge_widget = badge;
        ref->pkg_primary = pkg_primary;
        ref->pkg_alt = pkg_alt;
        g_ptr_array_add(app->status_badges, ref);
    }

    gtk_box_pack_start(GTK_BOX(card), hdr, FALSE, FALSE, 0);

    if (desc && desc[0]) {
        GtkWidget *lbl_d = gtk_label_new(desc);
        gtk_label_set_xalign(GTK_LABEL(lbl_d), 0.0);
        gtk_label_set_line_wrap(GTK_LABEL(lbl_d), TRUE);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_d), "text-muted");
        gtk_box_pack_start(GTK_BOX(card), lbl_d, FALSE, FALSE, 0);
    }

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(card), btn_box, FALSE, FALSE, 2);
    if (out_btn_box) *out_btn_box = btn_box;
    return card;
}

static GtkWidget *create_tele_card(AxisHardwareApp *app, const char *icon_kind,
                                   const char *title, GtkWidget **val_lbl_out) {
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "tele-card");

    GtkWidget *hdr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *ico = gtk_image_new_from_pixbuf(get_hw_icon(app, icon_kind, 22));
    GtkWidget *lbl_t = gtk_label_new(title);
    gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_t), "text-muted");
    gtk_box_pack_start(GTK_BOX(hdr), ico, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hdr), lbl_t, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(card), hdr, FALSE, FALSE, 0);

    GtkWidget *val = gtk_label_new("Detecting...");
    gtk_label_set_xalign(GTK_LABEL(val), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(val), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(val), "tele-val");
    gtk_box_pack_start(GTK_BOX(card), val, TRUE, TRUE, 0);
    if (val_lbl_out) *val_lbl_out = val;
    return card;
}

static GtkWidget *wrap_scrolled(GtkWidget *child) {
    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(sw), child);
    return sw;
}

static void init_overview_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 18);

    /* Hero Banner */
    GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_style_context_add_class(gtk_widget_get_style_context(hero), "hero-card");

    GtkWidget *h_t = gtk_label_new("AxisOS Hardware, Drivers & Codecs Control Center");
    gtk_label_set_xalign(GTK_LABEL(h_t), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(h_t), "top-title");

    GtkWidget *h_d = gtk_label_new(
        "Install GPU drivers (NVIDIA, AMD, Intel), full audio/video codecs (H.264, HEVC, AV1, AAC, MP3), "
        "hardware video acceleration (VA-API/VDPAU), Wi-Fi/Bluetooth firmware, and gaming runtimes with live progress.");
    gtk_label_set_xalign(GTK_LABEL(h_d), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(h_d), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(h_d), "text-muted");

    GtkWidget *hero_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(hero_btns),
        make_btn("One-Click Install All Codecs & Drivers", "btn-primary", G_CALLBACK(on_install_complete_pack_cb), app),
        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hero_btns),
        make_btn("Auto-Detect GPU Driver", "btn-secondary", G_CALLBACK(on_auto_gpu_setup_cb), app),
        FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hero_btns),
        make_btn("Repair Audio & Wi-Fi", "btn-secondary", G_CALLBACK(on_repair_audio_bt_cb), app),
        FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(hero), h_t, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hero), h_d, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hero), hero_btns, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hero, FALSE, FALSE, 0);

    /* 2x2 Live Telemetry Grid */
    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    GtkWidget *tc_cpu = create_tele_card(app, "cpu", "PROCESSOR & GOVERNOR", &app->lbl_cpu_val);
    GtkWidget *cpu_btns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_pack_start(GTK_BOX(cpu_btns), make_btn("Performance", "btn-secondary", G_CALLBACK(on_set_governor_perf_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cpu_btns), make_btn("Balanced", "btn-secondary", G_CALLBACK(on_set_governor_bal_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(tc_cpu), cpu_btns, FALSE, FALSE, 4);

    GtkWidget *tc_gpu = create_tele_card(app, "gpu", "GRAPHICS & DISPLAY", &app->lbl_gpu_val);
    GtkWidget *tc_mem = create_tele_card(app, "axis", "MEMORY & ZRAM SWAP", &app->lbl_mem_val);
    GtkWidget *tc_aud = create_tele_card(app, "audio", "AUDIO, BLUETOOTH & CODECS", &app->lbl_audio_val);

    gtk_grid_attach(GTK_GRID(grid), tc_cpu, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), tc_gpu, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), tc_mem, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), tc_aud, 1, 1, 1, 1);
    gtk_box_pack_start(GTK_BOX(vbox), grid, FALSE, FALSE, 0);

    /* Raw PCI GPU Hardware Box */
    GtkWidget *pci_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(pci_card), "card");
    GtkWidget *pci_title = gtk_label_new("Detected Display Hardware & Kernel Modules (lspci)");
    gtk_label_set_xalign(GTK_LABEL(pci_title), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(pci_title), "card-title");
    gtk_box_pack_start(GTK_BOX(pci_card), pci_title, FALSE, FALSE, 0);

    app->lbl_gpu_raw = gtk_label_new("Scanning PCI display adapters...");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_gpu_raw), 0.0);
    gtk_label_set_selectable(GTK_LABEL(app->lbl_gpu_raw), TRUE);
    gtk_label_set_line_wrap(GTK_LABEL(app->lbl_gpu_raw), TRUE);
    GtkWidget *status_frame = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(status_frame), "status-box");
    gtk_box_pack_start(GTK_BOX(status_frame), app->lbl_gpu_raw, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(pci_card), status_frame, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), pci_card, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), wrap_scrolled(vbox), gtk_label_new("Overview"));
}

static void init_drivers_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 18);

    GtkWidget *btns = NULL;
    GtkWidget *c_auto = create_feature_card(app, "gpu",
        "Automatic GPU Driver & Vulkan Setup",
        "Detects your installed GPU vendor (NVIDIA, AMD, or Intel) and installs the recommended Debian Bookworm kernel drivers, Vulkan ICD, and hardware decoders.",
        "mesa-vulkan-drivers", "nvidia-driver", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Auto-Detect & Install GPU Drivers", "btn-primary", G_CALLBACK(on_auto_gpu_setup_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Verify Vulkan (vulkaninfo)", "btn-secondary", G_CALLBACK(on_verify_amd_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_auto, FALSE, FALSE, 0);

    GtkWidget *c_nv = create_feature_card(app, "gpu",
        "NVIDIA GeForce Proprietary Drivers (DKMS + Vulkan)",
        "Installs nvidia-driver, nvidia-kernel-dkms, linux-headers-amd64, and non-free firmware for GeForce GTX / RTX GPUs.",
        "nvidia-driver", "nvidia-kernel-dkms", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install NVIDIA Proprietary Driver", "btn-primary", G_CALLBACK(on_install_nvidia_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_nv, FALSE, FALSE, 0);

    GtkWidget *c_amd = create_feature_card(app, "gpu",
        "AMD Radeon Graphics (Mesa RADV Vulkan + Firmware)",
        "Installs firmware-amd-graphics, Mesa RADV Vulkan driver, and VA-API / VDPAU hardware video decoding for Radeon RX & Ryzen APUs.",
        "firmware-amd-graphics", "mesa-vulkan-drivers", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install AMD Radeon Drivers", "btn-secondary", G_CALLBACK(on_install_amd_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Test AMD Vulkan", "btn-secondary", G_CALLBACK(on_verify_amd_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_amd, FALSE, FALSE, 0);

    GtkWidget *c_intel = create_feature_card(app, "gpu",
        "Intel Arc & Iris Xe Graphics (Mesa ANV + Intel Media VA-API)",
        "Installs intel-media-va-driver, i965-va-driver, and Mesa ANV Vulkan for smooth 4K hardware video playback and gaming.",
        "intel-media-va-driver", "i965-va-driver", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install Intel Media & Vulkan", "btn-secondary", G_CALLBACK(on_install_intel_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Test VA-API (vainfo)", "btn-secondary", G_CALLBACK(on_verify_intel_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_intel, FALSE, FALSE, 0);

    GtkWidget *c_fw = create_feature_card(app, "audio",
        "Wi-Fi, Bluetooth & Audio Hardware Firmware",
        "Installs Intel/Realtek/Atheros/Broadcom Wi-Fi firmware, Intel SOF audio DSP firmware, BlueZ Bluetooth stack, and PipeWire audio.",
        "firmware-iwlwifi", "firmware-linux-nonfree", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install All Hardware Firmware", "btn-secondary", G_CALLBACK(on_install_firmware_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_fw, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), wrap_scrolled(vbox), gtk_label_new("GPU & Firmware"));
}

static void init_codecs_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 18);

    GtkWidget *btns = NULL;
    GtkWidget *c_all = create_feature_card(app, "codecs",
        "Complete Audio & Video Codecs (FFmpeg + GStreamer)",
        "Full playback & encoding support for MP4 (H.264/AVC), HEVC (H.265), AV1, VP9, MKV, AAC, MP3, FLAC, Opus, and browser HTML5 video streams.",
        "gstreamer1.0-libav", "libavcodec-extra", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install / Repair All Multimedia Codecs", "btn-primary", G_CALLBACK(on_install_codecs_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Verify Active Codecs", "btn-secondary", G_CALLBACK(on_test_codecs_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_all, FALSE, FALSE, 0);

    GtkWidget *c_va = create_feature_card(app, "gpu",
        "Hardware Video Acceleration (VA-API & VDPAU)",
        "Offloads 1080p/4K video decoding in Firefox, Chromium, VLC, and OBS Studio directly to your GPU for lower CPU usage and longer battery life.",
        "va-driver-all", "mesa-va-drivers", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install VA-API & VDPAU Drivers", "btn-primary", G_CALLBACK(on_install_vaapi_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Check VA-API Support", "btn-secondary", G_CALLBACK(on_verify_intel_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_va, FALSE, FALSE, 0);

    GtkWidget *c_arch = create_feature_card(app, "axis",
        "Archives, Fonts & Media Compatibility",
        "Adds 7-Zip, RAR, ZIP extractors plus Liberation & Noto Color Emoji fonts for crisp web pages, documents, and terminals.",
        "p7zip-full", "fonts-noto-color-emoji", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install Archives & Fonts Pack", "btn-secondary", G_CALLBACK(on_install_archives_fonts_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_arch, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), wrap_scrolled(vbox), gtk_label_new("Codecs & Video"));
}

static void init_gaming_tab(AxisHardwareApp *app) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 18);

    GtkWidget *btns = NULL;
    GtkWidget *c_steam = create_feature_card(app, "gaming",
        "Valve Steam Gaming Platform + Proton",
        "Play native Linux and Windows games seamlessly via Valve Proton & Vulkan DXVK/VKD3D (installed from Flathub with full 32-bit runtime).",
        "com.valvesoftware.Steam", "steam-installer", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install Steam (Flathub)", "btn-primary", G_CALLBACK(on_install_steam_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Launch Steam", "btn-success", G_CALLBACK(on_launch_steam_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_steam, FALSE, FALSE, 0);

    GtkWidget *c_lutris = create_feature_card(app, "gaming",
        "Lutris & Heroic Games Launcher (Epic / GOG / Amazon)",
        "Run Epic Games, GOG, Battle.net, EA, and custom Wine prefixes from unified Linux launchers.",
        "net.lutris.Lutris", "lutris", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install Lutris", "btn-secondary", G_CALLBACK(on_install_lutris_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install Heroic Launcher", "btn-secondary", G_CALLBACK(on_install_heroic_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_lutris, FALSE, FALSE, 0);

    GtkWidget *c_pup = create_feature_card(app, "gaming",
        "ProtonUp-Qt (GE-Proton & Wine-GE Manager)",
        "Download and manage the latest GloriousEggroll GE-Proton builds for Steam and Lutris with one click.",
        "net.davidotek.pupgui2", NULL, &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install ProtonUp-Qt", "btn-secondary", G_CALLBACK(on_install_protonup_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_pup, FALSE, FALSE, 0);

    GtkWidget *c_stack = create_feature_card(app, "cpu",
        "GameMode, MangoHud, Gamescope & Wine Suite",
        "System-level gaming optimizations: Feral GameMode CPU governor boost, MangoHud FPS overlay, Valve Gamescope micro-compositor, and Wine.",
        "gamemode", "mangohud", &btns);
    gtk_box_pack_start(GTK_BOX(btns), make_btn("Install Gaming Optimization Stack", "btn-primary", G_CALLBACK(on_install_gamestack_cb), app), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), c_stack, FALSE, FALSE, 0);

    gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), wrap_scrolled(vbox), gtk_label_new("Gaming & Proton"));
}

static AxisHardwareApp *axis_hardware_app_create(void) {
    AxisHardwareApp *app = g_new0(AxisHardwareApp, 1);
    app->icon_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_object_unref);
    app->status_badges = g_ptr_array_new_with_free_func(g_free);
    refresh_installed_packages(app);

    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "AxisOS Hardware & Codecs");
    gtk_window_set_icon(GTK_WINDOW(app->window), get_hw_icon(app, "axis", 48));
    gtk_window_set_default_size(GTK_WINDOW(app->window), 920, 680);
    gtk_window_set_position(GTK_WINDOW(app->window), GTK_WIN_POS_CENTER);

    load_axis_theme();

    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), root_box);

    /* Top Banner */
    GtkWidget *top_banner = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);
    gtk_style_context_add_class(gtk_widget_get_style_context(top_banner), "top-banner");

    GtkWidget *logo = gtk_image_new_from_pixbuf(get_hw_icon(app, "axis", 38));
    gtk_box_pack_start(GTK_BOX(top_banner), logo, FALSE, FALSE, 0);

    GtkWidget *title_col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *title_lbl = gtk_label_new("AxisOS Hardware & Codecs");
    gtk_widget_set_halign(title_lbl, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(title_lbl), "top-title");

    GtkWidget *subtitle_lbl = gtk_label_new("GPU Drivers, Multimedia Codecs, Hardware Acceleration & Gaming Suite");
    gtk_widget_set_halign(subtitle_lbl, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(subtitle_lbl), "top-subtitle");

    gtk_box_pack_start(GTK_BOX(title_col), title_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(title_col), subtitle_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(top_banner), title_col, TRUE, TRUE, 0);

    GtkWidget *btn_ref = make_btn("Refresh Status", "btn-secondary", G_CALLBACK(on_refresh_all_cb), app);
    gtk_box_pack_end(GTK_BOX(top_banner), btn_ref, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root_box), top_banner, FALSE, FALSE, 0);

    /* Notebook Tabs */
    app->notebook = gtk_notebook_new();
    gtk_box_pack_start(GTK_BOX(root_box), app->notebook, TRUE, TRUE, 0);

    init_overview_tab(app);
    init_drivers_tab(app);
    init_codecs_tab(app);
    init_gaming_tab(app);

    /* Bottom Progress & Live Operation Log Drawer */
    GtkWidget *bottom_bar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(bottom_bar), "bottom-bar");

    GtkWidget *st_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    app->status_label = gtk_label_new("Ready — Select any driver, codec, or gaming action above.");
    gtk_label_set_xalign(GTK_LABEL(app->status_label), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->status_label), "text-muted");
    gtk_box_pack_start(GTK_BOX(st_row), app->status_label, TRUE, TRUE, 0);

    GtkWidget *btn_copy_log = make_btn("Copy Log", "btn-secondary", G_CALLBACK(on_copy_log_cb), app);
    gtk_box_pack_end(GTK_BOX(st_row), btn_copy_log, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(bottom_bar), st_row, FALSE, FALSE, 0);

    app->progress_bar = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(bottom_bar), app->progress_bar, FALSE, FALSE, 0);

    app->log_expander = gtk_expander_new("Live Operation Log");
    GtkWidget *log_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(log_scroll, -1, 140);
    app->log_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->log_view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app->log_view), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->log_view), GTK_WRAP_WORD_CHAR);
    app->log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->log_view));
    gtk_container_add(GTK_CONTAINER(log_scroll), app->log_view);
    gtk_container_add(GTK_CONTAINER(app->log_expander), log_scroll);
    gtk_box_pack_start(GTK_BOX(bottom_bar), app->log_expander, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root_box), bottom_bar, FALSE, FALSE, 0);

    refresh_hardware_telemetry(app);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    return app;
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("AxisOS Hardware Driver & Codec Suite (Pure C11)\n");
        printf("Usage: nobara-welcome [OPTION]\n");
        printf("Options:\n");
        printf("  --help, -h      Show this help message and exit\n");
        printf("  --version, -v   Print version information\n");
        printf("  --drivers, -d   Check current GPU driver status\n");
        return 0;
    }
    if (argc > 1 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        printf("axis-hardware-manager 2.2.0 (Pure C11, GTK 3, Cairo)\n");
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

    if (app->installed_set) g_hash_table_destroy(app->installed_set);
    if (app->icon_cache) g_hash_table_destroy(app->icon_cache);
    if (app->status_badges) g_ptr_array_free(app->status_badges, TRUE);
    g_free(app);
    return 0;
}
