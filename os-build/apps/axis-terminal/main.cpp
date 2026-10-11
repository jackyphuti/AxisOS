/*
 * AxisOS Native Terminal Emulator
 * Written in C++ using GTK 3 and VTE 2.91
 * High-performance, GPU-accelerated PTY shell with tabs, custom colors & clipboard.
 */

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <iostream>
#include <vector>
#include <string>

static const char* CSS_DATA = R"(
window {
    background-color: #0a0c10;
}
headerbar {
    background-color: #111827;
    border-bottom: 2px solid #2563eb;
    color: #ffffff;
    min-height: 38px;
    padding: 0 6px;
}
headerbar .title {
    font-weight: 700;
    font-size: 13px;
    color: #f8fafc;
}
notebook header {
    background-color: #0d1117;
    border-bottom: 1px solid rgba(255, 255, 255, 0.08);
}
notebook tab {
    background-color: #111827;
    color: #94a3b8;
    padding: 6px 14px;
    font-weight: 600;
    font-size: 12px;
    border: none;
    border-bottom: 2px solid transparent;
}
notebook tab:checked {
    background-color: #1e293b;
    color: #ffffff;
    border-bottom: 2px solid #2563eb;
}
button.tab-close {
    padding: 0;
    margin: 0;
    min-width: 16px;
    min-height: 16px;
    border: none;
    background: transparent;
    color: #94a3b8;
}
button.tab-close:hover {
    color: #ef4444;
}
)";

class AxisTerminalApp {
public:
    GtkWidget* window;
    GtkWidget* notebook;
    GtkWidget* headerbar;
    double font_scale = 1.0;

    AxisTerminalApp() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Debian Terminal - AxisOS");
        gtk_window_set_default_size(GTK_WINDOW(window), 880, 520);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        // Apply dark styling
        GtkCssProvider* provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider, CSS_DATA, -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        g_object_unref(provider);

        // Headerbar
        headerbar = gtk_header_bar_new();
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(headerbar), TRUE);
        gtk_header_bar_set_title(GTK_HEADER_BAR(headerbar), "Debian Terminal");
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(headerbar), "bash");
        gtk_window_set_titlebar(GTK_WINDOW(window), headerbar);

        // New tab button in header
        GtkWidget* btn_new_tab = gtk_button_new_from_icon_name("tab-new-symbolic", GTK_ICON_SIZE_BUTTON);
        gtk_widget_set_tooltip_text(btn_new_tab, "New Tab (Ctrl+Shift+T)");
        g_signal_connect_swapped(btn_new_tab, "clicked", G_CALLBACK(on_new_tab_action), this);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(headerbar), btn_new_tab);

        // Notebook for tabs
        notebook = gtk_notebook_new();
        gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
        gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
        gtk_container_add(GTK_CONTAINER(window), notebook);

        // Key shortcuts
        g_signal_connect(window, "key-press-event", G_CALLBACK(on_key_press), this);
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

        // Spawn first tab
        add_terminal_tab();
    }

    void add_terminal_tab() {
        GtkWidget* term = vte_terminal_new();
        
        // Colors: Black, Blue, White
        GdkRGBA bg_color = { 0.04, 0.05, 0.07, 0.96 }; // #0a0c10
        GdkRGBA fg_color = { 0.97, 0.98, 0.99, 1.0 };  // #f8fafc
        GdkRGBA cursor_color = { 0.15, 0.39, 0.92, 1.0 }; // #2563eb
        
        // ANSI 16 colors (Standard Linux Terminal)
        GdkRGBA palette[16] = {
            { 0.04, 0.05, 0.07, 1.0 }, // 0 Black
            { 0.94, 0.27, 0.27, 1.0 }, // 1 Red
            { 0.13, 0.77, 0.37, 1.0 }, // 2 Green
            { 0.98, 0.80, 0.08, 1.0 }, // 3 Yellow
            { 0.15, 0.39, 0.92, 1.0 }, // 4 Blue
            { 0.66, 0.33, 0.98, 1.0 }, // 5 Magenta
            { 0.02, 0.71, 0.83, 1.0 }, // 6 Cyan
            { 0.95, 0.96, 0.98, 1.0 }, // 7 White
            { 0.30, 0.35, 0.42, 1.0 }, // 8 Bright Black
            { 0.97, 0.44, 0.44, 1.0 }, // 9 Bright Red
            { 0.29, 0.87, 0.50, 1.0 }, // 10 Bright Green
            { 0.99, 0.88, 0.28, 1.0 }, // 11 Bright Yellow
            { 0.38, 0.65, 0.98, 1.0 }, // 12 Bright Blue
            { 0.78, 0.53, 0.99, 1.0 }, // 13 Bright Magenta
            { 0.20, 0.83, 0.93, 1.0 }, // 14 Bright Cyan
            { 1.00, 1.00, 1.00, 1.0 }  // 15 Bright White
        };

        vte_terminal_set_colors(VTE_TERMINAL(term), &fg_color, &bg_color, palette, 16);
        vte_terminal_set_color_cursor(VTE_TERMINAL(term), &cursor_color);
        vte_terminal_set_cursor_shape(VTE_TERMINAL(term), VTE_CURSOR_SHAPE_IBEAM);
        vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(term), VTE_CURSOR_BLINK_ON);
        vte_terminal_set_scrollback_lines(VTE_TERMINAL(term), 10000);
        vte_terminal_set_mouse_autohide(VTE_TERMINAL(term), TRUE);
        vte_terminal_set_allow_hyperlink(VTE_TERMINAL(term), TRUE);

        // Monospace font
        PangoFontDescription* font_desc = pango_font_description_from_string("DejaVu Sans Mono 11");
        vte_terminal_set_font(VTE_TERMINAL(term), font_desc);
        pango_font_description_free(font_desc);

        // Spawn bash
        const char* shell = g_getenv("SHELL");
        if (!shell || !*shell) shell = "/bin/bash";
        const char* const argv[] = { shell, NULL };

        vte_terminal_spawn_async(
            VTE_TERMINAL(term),
            VTE_PTY_DEFAULT,
            NULL,               // working dir
            (char**)argv,
            NULL,               // env
            (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH),
            NULL, NULL,         // child setup
            NULL,               // child pid
            -1,                 // timeout
            NULL,               // cancellable
            NULL, NULL          // callback
        );

        // Tab label with close button
        GtkWidget* tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget* tab_label = gtk_label_new("Terminal");
        GtkWidget* btn_close = gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_close), "tab-close");

        gtk_box_pack_start(GTK_BOX(tab_box), tab_label, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(tab_box), btn_close, FALSE, FALSE, 0);
        gtk_widget_show_all(tab_box);

        int page_num = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), term, tab_box);
        gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(notebook), term, TRUE);
        gtk_widget_show_all(window);
        gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), page_num);
        gtk_widget_grab_focus(term);

        // Close tab handler
        g_signal_connect_swapped(btn_close, "clicked", G_CALLBACK(on_close_tab), term);

        // Child exited handler
        g_signal_connect_swapped(term, "child-exited", G_CALLBACK(on_close_tab), term);

        // Window title update
        g_signal_connect(term, "window-title-changed", G_CALLBACK(on_title_changed), this);
    }

    static void on_new_tab_action(AxisTerminalApp* app) {
        app->add_terminal_tab();
    }

    static void on_close_tab(GtkWidget* term) {
        GtkWidget* notebook = gtk_widget_get_parent(term);
        if (notebook) {
            int page = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), term);
            if (page != -1) {
                gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page);
            }
            if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(notebook)) == 0) {
                GtkWidget* win = gtk_widget_get_toplevel(notebook);
                if (win && gtk_widget_is_toplevel(win)) {
                    gtk_widget_destroy(win);
                }
            }
        }
    }

    static void on_title_changed(VteTerminal* term, AxisTerminalApp* app) {
        const char* title = vte_terminal_get_window_title(term);
        if (title && *title) {
            gtk_header_bar_set_subtitle(GTK_HEADER_BAR(app->headerbar), title);
        }
    }

    static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, AxisTerminalApp* app) {
        guint state = event->state & gtk_accelerator_get_default_mod_mask();
        
        // Ctrl + Shift + C: Copy
        if ((state == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) && (event->keyval == GDK_KEY_C || event->keyval == GDK_KEY_c)) {
            int current = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
            if (current >= 0) {
                GtkWidget* term = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), current);
                vte_terminal_copy_clipboard_format(VTE_TERMINAL(term), VTE_FORMAT_TEXT);
            }
            return TRUE;
        }

        // Ctrl + Shift + V: Paste
        if ((state == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) && (event->keyval == GDK_KEY_V || event->keyval == GDK_KEY_v)) {
            int current = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
            if (current >= 0) {
                GtkWidget* term = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), current);
                vte_terminal_paste_clipboard(VTE_TERMINAL(term));
            }
            return TRUE;
        }

        // Ctrl + Shift + T: New Tab
        if ((state == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) && (event->keyval == GDK_KEY_T || event->keyval == GDK_KEY_t)) {
            app->add_terminal_tab();
            return TRUE;
        }

        // Ctrl + Shift + W: Close Tab
        if ((state == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) && (event->keyval == GDK_KEY_W || event->keyval == GDK_KEY_w)) {
            int current = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
            if (current >= 0) {
                GtkWidget* term = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), current);
                on_close_tab(term);
            }
            return TRUE;
        }

        // Ctrl + Plus / Ctrl + Equal: Zoom in
        if ((state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_plus || event->keyval == GDK_KEY_equal)) {
            int current = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
            if (current >= 0) {
                GtkWidget* term = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), current);
                double scale = vte_terminal_get_font_scale(VTE_TERMINAL(term));
                vte_terminal_set_font_scale(VTE_TERMINAL(term), scale + 0.1);
            }
            return TRUE;
        }

        // Ctrl + Minus: Zoom out
        if ((state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_minus)) {
            int current = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
            if (current >= 0) {
                GtkWidget* term = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), current);
                double scale = vte_terminal_get_font_scale(VTE_TERMINAL(term));
                if (scale > 0.4) {
                    vte_terminal_set_font_scale(VTE_TERMINAL(term), scale - 0.1);
                }
            }
            return TRUE;
        }

        // Ctrl + 0: Reset zoom
        if ((state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_0)) {
            int current = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
            if (current >= 0) {
                GtkWidget* term = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), current);
                vte_terminal_set_font_scale(VTE_TERMINAL(term), 1.0);
            }
            return TRUE;
        }

        return FALSE;
    }
};

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    AxisTerminalApp app;
    gtk_widget_show_all(app.window);
    gtk_main();
    return 0;
}
