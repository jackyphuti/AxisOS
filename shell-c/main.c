#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "system_controller.h"
#include "auth_manager.h"
#include "ui_shell.h"

static void load_axis_css(void) {
    const char *css_paths[] = {
        "data/theme/axis-tokens.css",
        "data/theme/axis-dark.css",
        "data/theme/axis-components.css",
        "/usr/share/axisos/theme/axis-tokens.css",
        "/usr/share/axisos/theme/axis-dark.css",
        "/usr/share/axisos/theme/axis-components.css",
        "/usr/local/share/axisos/theme/axis-tokens.css",
        "/usr/local/share/axisos/theme/axis-dark.css",
        "/usr/local/share/axisos/theme/axis-components.css",
        NULL
    };

    GdkScreen *screen = gdk_screen_get_default();
    if (!screen) return;

    for (int i = 0; css_paths[i] != NULL; ++i) {
        if (access(css_paths[i], R_OK) == 0) {
            GtkCssProvider *provider = gtk_css_provider_new();
            GError *error = NULL;
            if (gtk_css_provider_load_from_path(provider, css_paths[i], &error)) {
                gtk_style_context_add_provider_for_screen(
                    screen,
                    GTK_STYLE_PROVIDER(provider),
                    GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
                );
            } else if (error) {
                g_warning("Failed to load CSS %s: %s", css_paths[i], error->message);
                g_error_free(error);
            }
            g_object_unref(provider);
        }
    }
}

int main(int argc, char *argv[]) {
    gboolean is_greeter = FALSE;
    gboolean is_lock = FALSE;
    gboolean print_version = FALSE;

    GOptionEntry entries[] = {
        {"greeter", 'g', 0, G_OPTION_ARG_NONE, &is_greeter, "Run in login greeter mode", NULL},
        {"lock", 'l', 0, G_OPTION_ARG_NONE, &is_lock, "Start with screen locked", NULL},
        {"version", 'v', 0, G_OPTION_ARG_NONE, &print_version, "Show version information", NULL},
        {NULL, 0, 0, 0, NULL, NULL, NULL}
    };

    GError *error = NULL;
    GOptionContext *context = g_option_context_new("- AxisOS Pure C Wayland Desktop Shell");
    g_option_context_add_main_entries(context, entries, NULL);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));

    if (!g_option_context_parse(context, &argc, &argv, &error)) {
        g_printerr("Option parsing failed: %s\n", error ? error->message : "Unknown error");
        if (error) g_error_free(error);
        g_option_context_free(context);
        return 1;
    }
    g_option_context_free(context);

    if (print_version) {
        g_print("AxisOS Shell (LiquidNodesOS Pure C) 2.0.0\n");
        g_print("Pure C11 / GTK 3 / gtk-layer-shell / Cairo\n");
        return 0;
    }

    gtk_init(&argc, &argv);
    load_axis_css();

    SystemController *sys_ctrl = system_controller_create();
    AuthManager *auth_mgr = auth_manager_create();

    UiShell *shell = ui_shell_create(sys_ctrl, auth_mgr, is_greeter);

    if (is_lock && !is_greeter) {
        ui_shell_lock(shell);
    }

    g_print("[AxisOS Shell] Pure C Wayland desktop shell initialized.\n");

    gtk_main();

    ui_shell_destroy(shell);
    auth_manager_destroy(auth_mgr);
    system_controller_destroy(sys_ctrl);

    return 0;
}
