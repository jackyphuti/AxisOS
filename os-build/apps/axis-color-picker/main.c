#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * AxisOS Native Color Picker (axis-color-picker)
 * Strictly Pure C11 (-std=gnu11 / -std=c17)
 * Adapted from reference color-picker math routines.
 *
 * Provides a responsive GTK 3 color chooser and CLI color converter.
 */

typedef struct {
    double r, g, b; // 0.0 - 1.0
    double h, s, v; // h: 0-360, s: 0-1, v: 0-1
    char hex[16];
} ColorState;

typedef struct {
    GtkWidget *window;
    GtkWidget *color_chooser;
    GtkWidget *entry_hex;
    GtkWidget *entry_rgb;
    GtkWidget *entry_hsv;
    ColorState current;
} AxisColorPickerApp;

// HSV to RGB algorithm adapted from reference
static void hsv_to_rgb(double h, double s, double v, double *r, double *g, double *b) {
    if (s <= 0.0) {
        *r = v; *g = v; *b = v;
        return;
    }
    double hh = fmod(h, 360.0);
    if (hh < 0) hh += 360.0;
    hh /= 60.0;
    int i = (int)hh;
    double ff = hh - i;
    double p = v * (1.0 - s);
    double q = v * (1.0 - (s * ff));
    double t = v * (1.0 - (s * (1.0 - ff)));

    switch (i) {
        case 0: *r = v; *g = t; *b = p; break;
        case 1: *r = q; *g = v; *b = p; break;
        case 2: *r = p; *g = v; *b = t; break;
        case 3: *r = p; *g = q; *b = v; break;
        case 4: *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

// RGB to HSV algorithm adapted from reference
static void rgb_to_hsv(double r, double g, double b, double *h, double *s, double *v) {
    double c_max = fmax(r, fmax(g, b));
    double c_min = fmin(r, fmin(g, b));
    double delta = c_max - c_min;

    *v = c_max;
    *s = (c_max > 0.0) ? (delta / c_max) : 0.0;

    if (delta <= 0.00001) {
        *h = 0.0;
    } else {
        if (c_max == r) {
            *h = 60.0 * fmod(((g - b) / delta), 6.0);
        } else if (c_max == g) {
            *h = 60.0 * (((b - r) / delta) + 2.0);
        } else {
            *h = 60.0 * (((r - g) / delta) + 4.0);
        }
        if (*h < 0.0) *h += 360.0;
    }
}

static int clamp_int(int v, int min_val, int max_val) {
    if (v < min_val) return min_val;
    if (v > max_val) return max_val;
    return v;
}

static void update_fields(AxisColorPickerApp *app) {
    int ir = clamp_int((int)round(app->current.r * 255.0), 0, 255);
    int ig = clamp_int((int)round(app->current.g * 255.0), 0, 255);
    int ib = clamp_int((int)round(app->current.b * 255.0), 0, 255);

    snprintf(app->current.hex, sizeof(app->current.hex), "#%02X%02X%02X", ir, ig, ib);
    gtk_entry_set_text(GTK_ENTRY(app->entry_hex), app->current.hex);

    char rgb_buf[64];
    snprintf(rgb_buf, sizeof(rgb_buf), "rgb(%d, %d, %d)", ir, ig, ib);
    gtk_entry_set_text(GTK_ENTRY(app->entry_rgb), rgb_buf);

    char hsv_buf[64];
    snprintf(hsv_buf, sizeof(hsv_buf), "hsv(%.0f°, %.0f%%, %.0f%%)",
             app->current.h, app->current.s * 100.0, app->current.v * 100.0);
    gtk_entry_set_text(GTK_ENTRY(app->entry_hsv), hsv_buf);
}

static void on_color_activated(GtkColorChooser *chooser, GdkRGBA *color, gpointer user_data) {
    (void)chooser;
    AxisColorPickerApp *app = (AxisColorPickerApp *)user_data;
    app->current.r = color->red;
    app->current.g = color->green;
    app->current.b = color->blue;
    rgb_to_hsv(app->current.r, app->current.g, app->current.b,
               &app->current.h, &app->current.s, &app->current.v);
    update_fields(app);
}

static void on_copy_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    GtkEntry *entry = GTK_ENTRY(user_data);
    const char *text = gtk_entry_get_text(entry);
    GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(cb, text, -1);
}

static void apply_theme(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    const char *theme_path = "/usr/share/themes/Axis-Dark/gtk-3.0/gtk.css";
    if (access(theme_path, R_OK) != 0) {
        theme_path = "data/theme/axis-dark.css";
    }
    if (access(theme_path, R_OK) == 0) {
        gtk_css_provider_load_from_path(provider, theme_path, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
    }
    g_object_unref(provider);
}

static int parse_hex_color(const char *hex, double *r, double *g, double *b) {
    if (hex[0] == '#') hex++;
    if (strlen(hex) != 6) return -1;
    unsigned int val = 0;
    if (sscanf(hex, "%x", &val) != 1) return -1;
    *r = ((val >> 16) & 0xFF) / 255.0;
    *g = ((val >> 8) & 0xFF) / 255.0;
    *b = (val & 0xFF) / 255.0;
    return 0;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("AxisOS Native Color Picker & Converter (Pure C11)\n");
            printf("Usage: axis-color-picker [OPTION]\n\n");
            printf("Options:\n");
            printf("  --hex <#RRGGBB>   Convert hex color to RGB and HSV\n");
            printf("  --rgb <r> <g> <b> Convert RGB (0-255) to Hex and HSV\n");
            printf("  --help, -h        Display this help text and exit\n");
            printf("  --version, -v     Output version information\n");
            return 0;
        }
        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            printf("axis-color-picker 2.0.0 (Pure C11 -std=gnu11)\n");
            return 0;
        }
        if (strcmp(argv[1], "--hex") == 0 && argc > 2) {
            double r, g, b, h, s, v;
            if (parse_hex_color(argv[2], &r, &g, &b) == 0) {
                rgb_to_hsv(r, g, b, &h, &s, &v);
                int ir = (int)round(r * 255.0);
                int ig = (int)round(g * 255.0);
                int ib = (int)round(b * 255.0);
                printf("HEX: #%02X%02X%02X\n", ir, ig, ib);
                printf("RGB: rgb(%d, %d, %d)\n", ir, ig, ib);
                printf("HSV: hsv(%.1f°, %.1f%%, %.1f%%)\n", h, s * 100.0, v * 100.0);
                return 0;
            } else {
                fprintf(stderr, "Invalid hex code: %s\n", argv[2]);
                return 1;
            }
        }
        if (strcmp(argv[1], "--hsv") == 0 && argc > 4) {
            double h = atof(argv[2]);
            double s = atof(argv[3]) / 100.0;
            double v = atof(argv[4]) / 100.0;
            double r, g, b;
            hsv_to_rgb(h, s, v, &r, &g, &b);
            int ir = clamp_int((int)round(r * 255.0), 0, 255);
            int ig = clamp_int((int)round(g * 255.0), 0, 255);
            int ib = clamp_int((int)round(b * 255.0), 0, 255);
            printf("HEX: #%02X%02X%02X\n", ir, ig, ib);
            printf("RGB: rgb(%d, %d, %d)\n", ir, ig, ib);
            printf("HSV: hsv(%.1f°, %.1f%%, %.1f%%)\n", h, s * 100.0, v * 100.0);
            return 0;
        }
    }

    gtk_init(&argc, &argv);
    apply_theme();

    AxisColorPickerApp app = {0};
    // Default One UI Accent Color (#1428A0)
    app.current.r = 0.078;
    app.current.g = 0.157;
    app.current.b = 0.627;
    rgb_to_hsv(app.current.r, app.current.g, app.current.b,
               &app.current.h, &app.current.s, &app.current.v);

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Color Picker");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 420, 520);
    gtk_window_set_resizable(GTK_WINDOW(app.window), FALSE);
    gtk_window_set_position(GTK_WINDOW(app.window), GTK_WIN_POS_CENTER);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);

    // Color Chooser
    app.color_chooser = gtk_color_chooser_widget_new();
    gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(app.color_chooser), FALSE);
    GdkRGBA init_color = { app.current.r, app.current.g, app.current.b, 1.0 };
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(app.color_chooser), &init_color);
    g_signal_connect(app.color_chooser, "color-activated", G_CALLBACK(on_color_activated), &app);
    gtk_box_pack_start(GTK_BOX(vbox), app.color_chooser, TRUE, TRUE, 0);

    // Format Entries Card
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "card");

    // HEX Row
    GtkWidget *row_hex = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *lbl_hex = gtk_label_new("HEX:");
    gtk_widget_set_size_request(lbl_hex, 50, -1);
    gtk_widget_set_halign(lbl_hex, GTK_ALIGN_START);
    app.entry_hex = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(app.entry_hex), FALSE);
    GtkWidget *btn_copy_hex = gtk_button_new_with_label("Copy");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_copy_hex), "btn-primary");
    g_signal_connect(btn_copy_hex, "clicked", G_CALLBACK(on_copy_clicked), app.entry_hex);
    gtk_box_pack_start(GTK_BOX(row_hex), lbl_hex, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_hex), app.entry_hex, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row_hex), btn_copy_hex, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), row_hex, FALSE, FALSE, 0);

    // RGB Row
    GtkWidget *row_rgb = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *lbl_rgb = gtk_label_new("RGB:");
    gtk_widget_set_size_request(lbl_rgb, 50, -1);
    gtk_widget_set_halign(lbl_rgb, GTK_ALIGN_START);
    app.entry_rgb = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(app.entry_rgb), FALSE);
    GtkWidget *btn_copy_rgb = gtk_button_new_with_label("Copy");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_copy_rgb), "btn-secondary");
    g_signal_connect(btn_copy_rgb, "clicked", G_CALLBACK(on_copy_clicked), app.entry_rgb);
    gtk_box_pack_start(GTK_BOX(row_rgb), lbl_rgb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_rgb), app.entry_rgb, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row_rgb), btn_copy_rgb, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), row_rgb, FALSE, FALSE, 0);

    // HSV Row
    GtkWidget *row_hsv = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *lbl_hsv = gtk_label_new("HSV:");
    gtk_widget_set_size_request(lbl_hsv, 50, -1);
    gtk_widget_set_halign(lbl_hsv, GTK_ALIGN_START);
    app.entry_hsv = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(app.entry_hsv), FALSE);
    GtkWidget *btn_copy_hsv = gtk_button_new_with_label("Copy");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_copy_hsv), "btn-secondary");
    g_signal_connect(btn_copy_hsv, "clicked", G_CALLBACK(on_copy_clicked), app.entry_hsv);
    gtk_box_pack_start(GTK_BOX(row_hsv), lbl_hsv, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_hsv), app.entry_hsv, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row_hsv), btn_copy_hsv, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), row_hsv, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), card, FALSE, FALSE, 0);

    update_fields(&app);

    gtk_widget_show_all(app.window);
    gtk_main();

    return 0;
}
