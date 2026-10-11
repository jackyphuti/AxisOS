/*
 * AxisOS Native Terminal Emulator
 * Written in C++ using GTK 3 and VTE 2.91
 * High-performance, GPU-accelerated PTY shell with tabs, custom colors,
 * Debian command line compatibility (-e flag, login shell), and clipboard.
 * HarmonyOS Design System styling.
 */

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>

static const char* CSS_DATA = R"(
window {
    background-color: #0E1411;
    border-radius: 16px;
    border: 1px solid rgba(139, 195, 74, 0.18);
}
headerbar {
    background: #18211D;
    background-color: #18211D;
    border-bottom: 1px solid rgba(139, 195, 74, 0.18);
    color: #E6EDE8;
    min-height: 42px;
    padding: 0 8px;
}
headerbar .title {
    font-weight: 600;
    font-size: 14px;
    color: #E6EDE8;
}
headerbar .subtitle {
    font-size: 11px;
    color: #8A9A92;
}
notebook header {
    background-color: #18211D;
    border-bottom: 1px solid rgba(139, 195, 74, 0.12);
    padding: 0 4px;
}
notebook tab {
    background-color: #18211D;
    color: #8A9A92;
    padding: 6px 16px;
    font-weight: 500;
    font-size: 12px;
    border: none;
    border-bottom: 2px solid transparent;
    transition: all 180ms ease;
}
notebook tab:checked {
    background-color: #223029;
    color: #E6EDE8;
    border-bottom: 2px solid #8BC34A;
}
button.tab-close {
    padding: 0;
    margin: 0;
    min-width: 16px;
    min-height: 16px;
    border: none;
    background: transparent;
    color: #8A9A92;
}
button.tab-close:hover {
    color: #E05D5D;
}
button.header-btn {
    border-radius: 9999px;
    background-color: #223029;
    color: #E6EDE8;
    border: 1px solid rgba(139, 195, 74, 0.18);
    padding: 4px 10px;
}
button.header-btn:hover {
    background-color: rgba(139, 195, 74, 0.22);
    border-color: #8BC34A;
}
)";

class AxisTerminalApp {
public:
    GtkWidget* window;
    GtkWidget* notebook;
    GtkWidget* headerbar;
    bool is_fullscreen = false;
    std::vector<std::string> initial_cmd;

    AxisTerminalApp(const std::vector<std::string>& cmd = {}) : initial_cmd(cmd) {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Debian Terminal - AxisOS");
        gtk_window_set_default_size(GTK_WINDOW(window), 920, 560);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        // Apply HarmonyOS styling
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
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_new_tab), "header-btn");
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
        add_terminal_tab(initial_cmd);
    }

    void add_terminal_tab(const std::vector<std::string>& exec_cmd = {}) {
        GtkWidget* term = vte_terminal_new();

        // HarmonyOS Dark Palette
        GdkRGBA bg_color = { 0.055, 0.078, 0.067, 0.98 }; // #0E1411 (Obsidian Base)
        GdkRGBA fg_color = { 0.902, 0.929, 0.910, 1.0 };  // #E6EDE8 (High contrast text)
        GdkRGBA cursor_color = { 0.545, 0.765, 0.290, 1.0 }; // #8BC34A (Harmony Lime Accent)

        // Full ANSI 16 palette
        GdkRGBA palette[16] = {
            { 0.055, 0.078, 0.067, 1.0 }, // 0 Black (Obsidian)
            { 0.878, 0.365, 0.365, 1.0 }, // 1 Red
            { 0.545, 0.765, 0.290, 1.0 }, // 2 Green (Lime)
            { 0.965, 0.757, 0.467, 1.0 }, // 3 Yellow
            { 0.180, 0.769, 0.714, 1.0 }, // 4 Blue/Teal (Harmony Teal)
            { 0.769, 0.655, 0.906, 1.0 }, // 5 Magenta
            { 0.337, 0.714, 0.761, 1.0 }, // 6 Cyan
            { 0.902, 0.929, 0.910, 1.0 }, // 7 White
            { 0.133, 0.188, 0.161, 1.0 }, // 8 Bright Black (Surface Raised)
            { 0.922, 0.435, 0.573, 1.0 }, // 9 Bright Red
            { 0.627, 0.831, 0.408, 1.0 }, // 10 Bright Green
            { 0.976, 0.886, 0.686, 1.0 }, // 11 Bright Yellow
            { 0.220, 0.882, 0.816, 1.0 }, // 12 Bright Teal
            { 0.867, 0.714, 0.949, 1.0 }, // 13 Bright Magenta
            { 0.537, 0.867, 1.000, 1.0 }, // 14 Bright Cyan
            { 1.000, 1.000, 1.000, 1.0 }  // 15 Bright White
        };

        vte_terminal_set_colors(VTE_TERMINAL(term), &fg_color, &bg_color, palette, 16);
        vte_terminal_set_color_cursor(VTE_TERMINAL(term), &cursor_color);
        vte_terminal_set_cursor_shape(VTE_TERMINAL(term), VTE_CURSOR_SHAPE_BLOCK);
        vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(term), VTE_CURSOR_BLINK_ON);
        vte_terminal_set_scrollback_lines(VTE_TERMINAL(term), 20000);
        vte_terminal_set_mouse_autohide(VTE_TERMINAL(term), TRUE);
        vte_terminal_set_allow_hyperlink(VTE_TERMINAL(term), TRUE);

        // Monospace font: Cascadia Code / DejaVu Sans Mono / monospace 11
        PangoFontDescription* font_desc = pango_font_description_from_string("DejaVu Sans Mono 11");
        vte_terminal_set_font(VTE_TERMINAL(term), font_desc);
        pango_font_description_free(font_desc);

        // Working directory (user home)
        const char* working_dir = g_get_home_dir();
        if (!working_dir || !*working_dir) working_dir = "/home/axis";
        if (access(working_dir, F_OK) != 0) working_dir = "/";

        // Determine shell executable
        const char* shell = g_getenv("SHELL");
        if (!shell || !*shell) shell = "/bin/bash";

        // Build argv: support login shell or custom -e command
        std::vector<const char*> argv_vec;
        if (!exec_cmd.empty()) {
            for (const auto& arg : exec_cmd) {
                argv_vec.push_back(arg.c_str());
            }
            argv_vec.push_back(NULL);
        } else {
            argv_vec.push_back(shell);
            argv_vec.push_back("--login"); // Ensure interactive login shell
            argv_vec.push_back(NULL);
        }

        // Environment variables ensuring all Debian commands are in PATH
        std::vector<std::string> env_list;
        env_list.push_back("TERM=xterm-256color");
        env_list.push_back("COLORTERM=truecolor");
        env_list.push_back("VTE_VERSION=6800");

        // Guarantee complete standard Debian sbin & bin path
        const char* cur_path = g_getenv("PATH");
        std::string path_str = "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin";
        if (cur_path && *cur_path) {
            path_str += ":";
            path_str += cur_path;
        }
        env_list.push_back(path_str);
        env_list.push_back(std::string("SHELL=") + shell);
        env_list.push_back(std::string("HOME=") + working_dir);

        // Inherit other environment variables (DISPLAY, WAYLAND_DISPLAY, USER, etc.)
        char** environ_ptr = environ;
        while (environ_ptr && *environ_ptr) {
            std::string entry = *environ_ptr;
            if (entry.rfind("PATH=", 0) != 0 &&
                entry.rfind("TERM=", 0) != 0 &&
                entry.rfind("COLORTERM=", 0) != 0 &&
                entry.rfind("SHELL=", 0) != 0 &&
                entry.rfind("HOME=", 0) != 0) {
                env_list.push_back(entry);
            }
            environ_ptr++;
        }

        std::vector<char*> envp_ptrs;
        for (auto& s : env_list) {
            envp_ptrs.push_back(const_cast<char*>(s.c_str()));
        }
        envp_ptrs.push_back(NULL);

        // Spawn PTY shell
        vte_terminal_spawn_async(
            VTE_TERMINAL(term),
            VTE_PTY_DEFAULT,
            working_dir,
            const_cast<char**>(argv_vec.data()),
            envp_ptrs.data(),
            (GSpawnFlags)(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH),
            NULL, NULL,
            NULL,
            -1,
            NULL,
            NULL, NULL
        );

        // Tab label with close button
        GtkWidget* tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        GtkWidget* tab_label = gtk_label_new(exec_cmd.empty() ? "bash" : exec_cmd[0].c_str());
        GtkWidget* btn_close = gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_close), "tab-close");

        gtk_box_pack_start(GTK_BOX(tab_box), tab_label, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(tab_box), btn_close, FALSE, FALSE, 0);
        gtk_widget_show_all(tab_box);

        // Associate tab label with terminal for dynamic title updates
        g_object_set_data(G_OBJECT(term), "tab_label", tab_label);

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
        GtkWidget* tab_label = GTK_WIDGET(g_object_get_data(G_OBJECT(term), "tab_label"));
        if (title && *title) {
            gtk_header_bar_set_subtitle(GTK_HEADER_BAR(app->headerbar), title);
            if (tab_label) {
                // Shorten title for tab
                std::string s = title;
                size_t colon = s.find(": ");
                if (colon != std::string::npos && colon + 2 < s.length()) {
                    s = s.substr(colon + 2);
                }
                gtk_label_set_text(GTK_LABEL(tab_label), s.c_str());
            }
        } else {
            gtk_header_bar_set_subtitle(GTK_HEADER_BAR(app->headerbar), "bash");
            if (tab_label) {
                gtk_label_set_text(GTK_LABEL(tab_label), "bash");
            }
        }
    }

    static gboolean on_key_press(GtkWidget* widget, GdkEventKey* event, AxisTerminalApp* app) {
        guint state = event->state & gtk_accelerator_get_default_mod_mask();

        // F11: Fullscreen toggle
        if (event->keyval == GDK_KEY_F11) {
            if (app->is_fullscreen) {
                gtk_window_unfullscreen(GTK_WINDOW(app->window));
                app->is_fullscreen = false;
            } else {
                gtk_window_fullscreen(GTK_WINDOW(app->window));
                app->is_fullscreen = true;
            }
            return TRUE;
        }

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

    // Parse command line arguments (e.g. -e <cmd> [args...])
    std::vector<std::string> exec_cmd;
    for (int i = 1; i < argc; ++i) {
        if ((strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--execute") == 0) && (i + 1 < argc)) {
            for (int j = i + 1; j < argc; ++j) {
                exec_cmd.push_back(argv[j]);
            }
            break;
        }
    }

    AxisTerminalApp app(exec_cmd);
    gtk_widget_show_all(app.window);
    gtk_main();
    return 0;
}
