#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <glib-unix.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "system_controller.h"
#include "auth_manager.h"
#include "ui_shell.h"

static GtkCssProvider *g_theme_provider = NULL;

void ui_shell_apply_theme(gboolean dark_mode) {
    GdkScreen *screen = gdk_screen_get_default();
    if (!screen) return;

    if (!g_theme_provider) {
        g_theme_provider = gtk_css_provider_new();
        gtk_style_context_add_provider_for_screen(
            screen,
            GTK_STYLE_PROVIDER(g_theme_provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER
        );
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
    for (int i = 0; paths[i] != NULL; ++i) {
        if (access(paths[i], R_OK) == 0) {
            GError *error = NULL;
            if (gtk_css_provider_load_from_path(g_theme_provider, paths[i], &error)) {
                break;
            } else if (error) {
                g_warning("Failed to load CSS %s: %s", paths[i], error->message);
                g_error_free(error);
            }
        }
    }
}

static gboolean on_sigusr1_signal(gpointer user_data) {
    UiShell *shell = (UiShell *)user_data;
    if (shell) {
        ui_shell_toggle_launcher(shell);
    }
    return G_SOURCE_CONTINUE;
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

    GtkSettings *settings = gtk_settings_get_default();
    if (settings) {
        g_object_set(settings,
            "gtk-application-prefer-dark-theme", TRUE,
            "gtk-theme-name", "Adwaita-dark",
            "gtk-icon-theme-name", "Papirus-Dark",
            "gtk-font-name", "IBM Plex Sans 10",
            NULL);
    }

    SystemController *sys_ctrl = system_controller_create();
    AuthManager *auth_mgr = auth_manager_create();

    ui_shell_apply_theme(sys_ctrl ? sys_ctrl->metrics.is_dark_mode : TRUE);

    UiShell *shell = ui_shell_create(sys_ctrl, auth_mgr, is_greeter);

    // Register SIGUSR1 handler to toggle native app launcher
    g_unix_signal_add(SIGUSR1, on_sigusr1_signal, shell);

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
