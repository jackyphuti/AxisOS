#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

/**
 * AxisOS Native Calculator (axis-calc)
 * Strictly Pure C11 (-std=gnu11 / -std=c17)
 * Adapted from pure C calculator reference architecture.
 *
 * Provides a responsive GTK 3 desktop calculator and headless CLI math engine.
 */

typedef struct {
    GtkWidget *window;
    GtkWidget *lbl_history;
    GtkWidget *lbl_display;
    
    double accumulator;
    double current_value;
    double memory;
    char pending_op;
    bool reset_display;
    char display_buffer[128];
} AxisCalculatorApp;

static void update_display(AxisCalculatorApp *app) {
    gtk_label_set_text(GTK_LABEL(app->lbl_display), app->display_buffer);
}

static void set_display_number(AxisCalculatorApp *app, double val) {
    if (isnan(val)) {
        snprintf(app->display_buffer, sizeof(app->display_buffer), "Error");
    } else if (isinf(val)) {
        snprintf(app->display_buffer, sizeof(app->display_buffer), "Infinity");
    } else if (val == (long long)val && fabs(val) < 1e12) {
        snprintf(app->display_buffer, sizeof(app->display_buffer), "%lld", (long long)val);
    } else {
        snprintf(app->display_buffer, sizeof(app->display_buffer), "%.8g", val);
    }
    update_display(app);
}

static void on_clear_all(AxisCalculatorApp *app) {
    app->accumulator = 0;
    app->current_value = 0;
    app->pending_op = '\0';
    app->reset_display = false;
    snprintf(app->display_buffer, sizeof(app->display_buffer), "0");
    gtk_label_set_text(GTK_LABEL(app->lbl_history), "");
    update_display(app);
}

static void on_clear_entry(AxisCalculatorApp *app) {
    snprintf(app->display_buffer, sizeof(app->display_buffer), "0");
    app->reset_display = false;
    update_display(app);
}

static void on_digit_clicked(AxisCalculatorApp *app, const char *digit) {
    if (app->reset_display || strcmp(app->display_buffer, "0") == 0 || strcmp(app->display_buffer, "Error") == 0) {
        snprintf(app->display_buffer, sizeof(app->display_buffer), "%s", digit);
        app->reset_display = false;
    } else {
        if (strlen(app->display_buffer) < sizeof(app->display_buffer) - 2) {
            strncat(app->display_buffer, digit, sizeof(app->display_buffer) - strlen(app->display_buffer) - 1);
        }
    }
    update_display(app);
}

static void on_dot_clicked(AxisCalculatorApp *app) {
    if (app->reset_display) {
        snprintf(app->display_buffer, sizeof(app->display_buffer), "0.");
        app->reset_display = false;
    } else if (!strchr(app->display_buffer, '.')) {
        strncat(app->display_buffer, ".", sizeof(app->display_buffer) - strlen(app->display_buffer) - 1);
    }
    update_display(app);
}

static void on_backspace_clicked(AxisCalculatorApp *app) {
    if (app->reset_display || strcmp(app->display_buffer, "Error") == 0) return;
    int len = (int)strlen(app->display_buffer);
    if (len > 1) {
        app->display_buffer[len - 1] = '\0';
    } else {
        snprintf(app->display_buffer, sizeof(app->display_buffer), "0");
    }
    update_display(app);
}

static void evaluate_pending(AxisCalculatorApp *app) {
    double current = atof(app->display_buffer);
    if (app->pending_op == '+') {
        app->accumulator += current;
    } else if (app->pending_op == '-') {
        app->accumulator -= current;
    } else if (app->pending_op == '*' || app->pending_op == 'x') {
        app->accumulator *= current;
    } else if (app->pending_op == '/') {
        if (current == 0.0) {
            snprintf(app->display_buffer, sizeof(app->display_buffer), "Error");
            update_display(app);
            app->pending_op = '\0';
            app->reset_display = true;
            return;
        }
        app->accumulator /= current;
    } else {
        app->accumulator = current;
    }
    set_display_number(app, app->accumulator);
}

static void on_operator_clicked(AxisCalculatorApp *app, char op, const char *op_str) {
    if (app->pending_op != '\0' && !app->reset_display) {
        evaluate_pending(app);
    } else {
        app->accumulator = atof(app->display_buffer);
    }

    app->pending_op = op;
    app->reset_display = true;

    char hist[256];
    snprintf(hist, sizeof(hist), "%.8g %s", app->accumulator, op_str);
    gtk_label_set_text(GTK_LABEL(app->lbl_history), hist);
}

static void on_equals_clicked(AxisCalculatorApp *app) {
    if (app->pending_op == '\0') return;
    
    char hist[256];
    snprintf(hist, sizeof(hist), "%.8g %c %s =", app->accumulator, app->pending_op, app->display_buffer);
    gtk_label_set_text(GTK_LABEL(app->lbl_history), hist);

    evaluate_pending(app);
    app->pending_op = '\0';
    app->reset_display = true;
}

static void on_unary_clicked(AxisCalculatorApp *app, const char *type) {
    double v = atof(app->display_buffer);
    if (strcmp(type, "neg") == 0) {
        v = -v;
    } else if (strcmp(type, "sqrt") == 0) {
        if (v < 0) {
            snprintf(app->display_buffer, sizeof(app->display_buffer), "Error");
            update_display(app);
            app->reset_display = true;
            return;
        }
        v = sqrt(v);
    } else if (strcmp(type, "sqr") == 0) {
        v = v * v;
    } else if (strcmp(type, "inv") == 0) {
        if (v == 0.0) {
            snprintf(app->display_buffer, sizeof(app->display_buffer), "Error");
            update_display(app);
            app->reset_display = true;
            return;
        }
        v = 1.0 / v;
    } else if (strcmp(type, "pct") == 0) {
        v = v / 100.0;
    }
    set_display_number(app, v);
    app->reset_display = true;
}

static void on_button_clicked(GtkButton *btn, gpointer user_data) {
    AxisCalculatorApp *app = (AxisCalculatorApp *)user_data;
    const char *label = gtk_button_get_label(btn);

    if (strcmp(label, "C") == 0) {
        on_clear_all(app);
    } else if (strcmp(label, "CE") == 0) {
        on_clear_entry(app);
    } else if (strcmp(label, "⌫") == 0) {
        on_backspace_clicked(app);
    } else if (strcmp(label, "+/-") == 0) {
        on_unary_clicked(app, "neg");
    } else if (strcmp(label, "√") == 0) {
        on_unary_clicked(app, "sqrt");
    } else if (strcmp(label, "x²") == 0) {
        on_unary_clicked(app, "sqr");
    } else if (strcmp(label, "1/x") == 0) {
        on_unary_clicked(app, "inv");
    } else if (strcmp(label, "%") == 0) {
        on_unary_clicked(app, "pct");
    } else if (strcmp(label, "+") == 0) {
        on_operator_clicked(app, '+', "+");
    } else if (strcmp(label, "-") == 0) {
        on_operator_clicked(app, '-', "-");
    } else if (strcmp(label, "×") == 0 || strcmp(label, "*") == 0) {
        on_operator_clicked(app, '*', "×");
    } else if (strcmp(label, "÷") == 0 || strcmp(label, "/") == 0) {
        on_operator_clicked(app, '/', "÷");
    } else if (strcmp(label, "=") == 0) {
        on_equals_clicked(app);
    } else if (strcmp(label, ".") == 0) {
        on_dot_clicked(app);
    } else if (label[0] >= '0' && label[0] <= '9') {
        on_digit_clicked(app, label);
    }
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    AxisCalculatorApp *app = (AxisCalculatorApp *)user_data;

    if (event->keyval >= GDK_KEY_0 && event->keyval <= GDK_KEY_9) {
        char d[2] = { (char)('0' + (event->keyval - GDK_KEY_0)), '\0' };
        on_digit_clicked(app, d);
        return TRUE;
    }
    if (event->keyval >= GDK_KEY_KP_0 && event->keyval <= GDK_KEY_KP_9) {
        char d[2] = { (char)('0' + (event->keyval - GDK_KEY_KP_0)), '\0' };
        on_digit_clicked(app, d);
        return TRUE;
    }

    switch (event->keyval) {
        case GDK_KEY_plus:
        case GDK_KEY_KP_Add:
            on_operator_clicked(app, '+', "+");
            return TRUE;
        case GDK_KEY_minus:
        case GDK_KEY_KP_Subtract:
            on_operator_clicked(app, '-', "-");
            return TRUE;
        case GDK_KEY_asterisk:
        case GDK_KEY_KP_Multiply:
            on_operator_clicked(app, '*', "×");
            return TRUE;
        case GDK_KEY_slash:
        case GDK_KEY_KP_Divide:
            on_operator_clicked(app, '/', "÷");
            return TRUE;
        case GDK_KEY_period:
        case GDK_KEY_KP_Decimal:
            on_dot_clicked(app);
            return TRUE;
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_equal:
            on_equals_clicked(app);
            return TRUE;
        case GDK_KEY_BackSpace:
            on_backspace_clicked(app);
            return TRUE;
        case GDK_KEY_Escape:
            on_clear_all(app);
            return TRUE;
    }
    return FALSE;
}

static const char *CSS_CALC =
    "@define-color axis_base #141826;"
    "@define-color axis_surface #1F2231;"
    "@define-color axis_surface_raised #262A3D;"
    "@define-color axis_outline #2E3349;"
    "@define-color axis_accent #7D95FF;"
    "@define-color axis_accent_btn_text #0F1422;"
    "@define-color axis_text #F2F3F7;"
    "@define-color axis_text_muted #9EA3B5;"
    "window, headerbar, button {"
    "    background-image: none;"
    "    box-shadow: none;"
    "}"
    "window {"
    "    background-color: @axis_base;"
    "    color: @axis_text;"
    "    font-family: 'IBM Plex Sans', sans-serif;"
    "}"
    ".card {"
    "    background-color: @axis_surface_raised;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 14px;"
    "    padding: 12px 16px;"
    "}"
    "button.btn-flat {"
    "    background-color: @axis_surface;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 12px;"
    "    font-size: 16px;"
    "    font-weight: 600;"
    "}"
    "button.btn-flat:hover {"
    "    background-color: @axis_surface_raised;"
    "    border-color: @axis_accent;"
    "}"
    "button.btn-secondary {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_accent;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 12px;"
    "    font-size: 16px;"
    "    font-weight: 700;"
    "}"
    "button.btn-secondary:hover {"
    "    background-color: alpha(@axis_accent, 0.16);"
    "    border-color: @axis_accent;"
    "}"
    "button.btn-primary {"
    "    background-color: @axis_accent;"
    "    color: @axis_accent_btn_text;"
    "    border: 1px solid @axis_accent;"
    "    border-radius: 12px;"
    "    font-size: 16px;"
    "    font-weight: 700;"
    "}";

static void apply_theme(void) {
    GdkScreen *screen = gdk_screen_get_default();
    if (!screen) return;

    const char *dark_paths[] = {
        "data/theme/axis-dark.css",
        "/usr/share/axisos/theme/axis-dark.css",
        "/usr/local/share/axisos/theme/axis-dark.css",
        NULL
    };
    GtkCssProvider *provider = gtk_css_provider_new();
    for (int i = 0; dark_paths[i] != NULL; ++i) {
        if (access(dark_paths[i], R_OK) == 0) {
            if (gtk_css_provider_load_from_path(provider, dark_paths[i], NULL)) {
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

    GtkCssProvider *extra = gtk_css_provider_new();
    gtk_css_provider_load_from_data(extra, CSS_CALC, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(extra),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(extra);
}

// Simple CLI math expression evaluator
static double eval_cli_expr(const char *expr) {
    double acc = 0;
    double val = 0;
    char op = '+';
    const char *p = expr;

    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;

        char *endptr;
        val = strtod(p, &endptr);
        if (endptr == p) {
            // Operator
            if (*p == '+' || *p == '-' || *p == '*' || *p == '/' || *p == 'x') {
                op = (*p == 'x') ? '*' : *p;
                p++;
                continue;
            }
            p++;
            continue;
        }

        if (op == '+') acc += val;
        else if (op == '-') acc -= val;
        else if (op == '*') acc *= val;
        else if (op == '/') {
            if (val != 0) acc /= val;
        }

        p = endptr;
    }
    return acc;
}

int main(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("AxisOS Native Calculator (Pure C11)\n");
            printf("Usage: axis-calc [EXPRESSION | OPTIONS]\n\n");
            printf("Examples:\n");
            printf("  axis-calc                  Launch graphical calculator\n");
            printf("  axis-calc \"12.5 * 4 + 8\"   Evaluate math expression in CLI mode\n");
            printf("  axis-calc --version        Show version\n");
            return 0;
        }
        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            printf("axis-calc 2.0.0 (Pure C11 -std=gnu11)\n");
            return 0;
        }
        // If not a flag, treat as math expression to evaluate
        if (argv[1][0] != '-') {
            double res = eval_cli_expr(argv[1]);
            printf("%.8g\n", res);
            return 0;
        }
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
    apply_theme();

    AxisCalculatorApp app = {0};
    snprintf(app.display_buffer, sizeof(app.display_buffer), "0");

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app.window), "Calculator");
    gtk_window_set_default_size(GTK_WINDOW(app.window), 340, 480);
    gtk_window_set_resizable(GTK_WINDOW(app.window), FALSE);
    gtk_window_set_position(GTK_WINDOW(app.window), GTK_WIN_POS_CENTER);
    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(app.window, "key-press-event", G_CALLBACK(on_key_press), &app);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 16);
    gtk_container_add(GTK_CONTAINER(app.window), vbox);

    // Display Card
    GtkWidget *disp_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_style_context_add_class(gtk_widget_get_style_context(disp_card), "card");

    app.lbl_history = gtk_label_new("");
    gtk_widget_set_halign(app.lbl_history, GTK_ALIGN_END);
    gtk_style_context_add_class(gtk_widget_get_style_context(app.lbl_history), "caption");
    gtk_box_pack_start(GTK_BOX(disp_card), app.lbl_history, FALSE, FALSE, 2);

    app.lbl_display = gtk_label_new("0");
    gtk_widget_set_halign(app.lbl_display, GTK_ALIGN_END);
    gtk_style_context_add_class(gtk_widget_get_style_context(app.lbl_display), "title-1");
    gtk_box_pack_start(GTK_BOX(disp_card), app.lbl_display, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(vbox), disp_card, FALSE, FALSE, 4);

    // Button Grid (5 rows x 4 columns)
    const char *buttons[5][4] = {
        { "C", "CE", "⌫", "÷" },
        { "7", "8",  "9", "×" },
        { "4", "5",  "6", "-" },
        { "1", "2",  "3", "+" },
        { "+/-", "0", ".", "=" }
    };

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_homogeneous(GTK_GRID(grid), TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

    for (int r = 0; r < 5; r++) {
        for (int c = 0; c < 4; c++) {
            const char *label = buttons[r][c];
            GtkWidget *btn = gtk_button_new_with_label(label);
            gtk_widget_set_size_request(btn, 64, 52);

            GtkStyleContext *sc = gtk_widget_get_style_context(btn);
            if (strcmp(label, "=") == 0) {
                gtk_style_context_add_class(sc, "btn-primary");
            } else if (strcmp(label, "+") == 0 || strcmp(label, "-") == 0 ||
                       strcmp(label, "×") == 0 || strcmp(label, "÷") == 0) {
                gtk_style_context_add_class(sc, "btn-secondary");
            } else {
                gtk_style_context_add_class(sc, "btn-flat");
            }

            g_signal_connect(btn, "clicked", G_CALLBACK(on_button_clicked), &app);
            gtk_grid_attach(GTK_GRID(grid), btn, c, r, 1, 1);
        }
    }

    gtk_box_pack_start(GTK_BOX(vbox), grid, TRUE, TRUE, 4);

    gtk_widget_show_all(app.window);
    gtk_main();

    return 0;
}
