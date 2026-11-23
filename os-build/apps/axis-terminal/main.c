#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <vterm.h>
#include <pty.h>
#include <utmp.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DEFAULT_ROWS 24
#define DEFAULT_COLS 80
#define TERM_CHAR_WIDTH 9.0
#define TERM_CHAR_HEIGHT 18.0

typedef struct {
    GtkWidget *drawing_area;
    GtkWidget *tab_box;
    GtkWidget *tab_label;

    VTerm *vt;
    VTermScreen *vts;
    int pty_fd;
    pid_t child_pid;
    guint io_watch_id;

    int rows;
    int cols;
    int cursor_row;
    int cursor_col;
    gboolean cursor_visible;

    gpointer app;
} TerminalTab;

typedef struct {
    GtkWidget *window;
    GtkWidget *header_bar;
    GtkWidget *notebook;
    GList *tabs;
} TerminalApp;

static int term_damage(VTermRect rect, void *user) {
    TerminalTab *tab = (TerminalTab *)user;
    if (tab && tab->drawing_area && gtk_widget_get_window(tab->drawing_area)) {
        GdkRectangle r = {
            .x = (int)(rect.start_col * TERM_CHAR_WIDTH),
            .y = (int)(rect.start_row * TERM_CHAR_HEIGHT),
            .width = (int)((rect.end_col - rect.start_col) * TERM_CHAR_WIDTH + 2),
            .height = (int)((rect.end_row - rect.start_row) * TERM_CHAR_HEIGHT + 2)
        };
        gdk_window_invalidate_rect(gtk_widget_get_window(tab->drawing_area), &r, FALSE);
    }
    return 1;
}

static int term_movecursor(VTermPos pos, VTermPos oldpos, int visible, void *user) {
    (void)oldpos;
    TerminalTab *tab = (TerminalTab *)user;
    if (!tab) return 1;

    tab->cursor_row = pos.row;
    tab->cursor_col = pos.col;
    tab->cursor_visible = visible;

    if (tab->drawing_area && gtk_widget_get_window(tab->drawing_area)) {
        gtk_widget_queue_draw(tab->drawing_area);
    }
    return 1;
}

static VTermScreenCallbacks screen_cbs = {
    .damage = term_damage,
    .moverect = NULL,
    .movecursor = term_movecursor,
    .settermprop = NULL,
    .bell = NULL,
    .resize = NULL,
    .sb_pushline = NULL,
    .sb_popline = NULL
};

static gboolean on_pty_read(GIOChannel *source, GIOCondition condition, gpointer data) {
    TerminalTab *tab = (TerminalTab *)data;
    (void)source;

    if (condition & (G_IO_IN | G_IO_PRI)) {
        char buf[4096];
        ssize_t bytes = read(tab->pty_fd, buf, sizeof(buf));
        if (bytes > 0) {
            vterm_input_write(tab->vt, buf, (size_t)bytes);
        } else if (bytes <= 0) {
            return G_SOURCE_REMOVE;
        }
    }

    if (condition & (G_IO_HUP | G_IO_ERR)) {
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static const char *CSS_TERMINAL =
    "@define-color axis_base #141826;"
    "@define-color axis_surface #1F2231;"
    "@define-color axis_surface_raised #262A3D;"
    "@define-color axis_outline #2E3349;"
    "@define-color axis_accent #7D95FF;"
    "@define-color axis_accent_btn_text #0F1422;"
    "@define-color axis_text #F2F3F7;"
    "@define-color axis_text_muted #9EA3B5;"
    "window, headerbar, notebook, notebook header, button {"
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
    "notebook, notebook stack {"
    "    background-color: @axis_base;"
    "    border: none;"
    "}"
    "notebook header {"
    "    background-color: @axis_surface;"
    "    border-bottom: 1px solid @axis_outline;"
    "}"
    "notebook tab {"
    "    background-color: @axis_surface;"
    "    color: @axis_text_muted;"
    "    padding: 4px 12px;"
    "    border-right: 1px solid @axis_outline;"
    "}"
    "notebook tab:checked {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_text;"
    "    border-bottom: 2px solid @axis_accent;"
    "}";

static gboolean on_draw_terminal(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    TerminalTab *tab = (TerminalTab *)user_data;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    // AxisOS Obsidian Dark background #0E1320
    cairo_set_source_rgb(cr, 14.0 / 255.0, 19.0 / 255.0, 32.0 / 255.0);
    cairo_paint(cr);

    if (!tab || !tab->vts) return FALSE;

    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, 13.0);

    VTermPos pos;
    for (pos.row = 0; pos.row < tab->rows; ++pos.row) {
        for (pos.col = 0; pos.col < tab->cols; ++pos.col) {
            VTermScreenCell cell;
            if (vterm_screen_get_cell(tab->vts, pos, &cell) && cell.chars[0]) {
                double x = pos.col * TERM_CHAR_WIDTH;
                double y = pos.row * TERM_CHAR_HEIGHT;

                vterm_screen_convert_color_to_rgb(tab->vts, &cell.bg);
                vterm_screen_convert_color_to_rgb(tab->vts, &cell.fg);

                // Cell background if specified (ignore default pitch black 0,0,0 so #141826 base shows)
                if (VTERM_COLOR_IS_RGB(&cell.bg) &&
                    !(cell.bg.rgb.red == 0 && cell.bg.rgb.green == 0 && cell.bg.rgb.blue == 0)) {
                    cairo_set_source_rgb(cr, cell.bg.rgb.red / 255.0,
                                         cell.bg.rgb.green / 255.0,
                                         cell.bg.rgb.blue / 255.0);
                    cairo_rectangle(cr, x, y, TERM_CHAR_WIDTH, TERM_CHAR_HEIGHT);
                    cairo_fill(cr);
                }

                // Cell text color (#F2F3F7 default)
                if (VTERM_COLOR_IS_RGB(&cell.fg)) {
                    cairo_set_source_rgb(cr, cell.fg.rgb.red / 255.0,
                                         cell.fg.rgb.green / 255.0,
                                         cell.fg.rgb.blue / 255.0);
                } else {
                    cairo_set_source_rgb(cr, 242.0 / 255.0, 243.0 / 255.0, 247.0 / 255.0);
                }

                char utf8[16] = {0};
                int len = g_unichar_to_utf8(cell.chars[0], utf8);
                utf8[len] = '\0';

                cairo_move_to(cr, x + 1.0, y + TERM_CHAR_HEIGHT - 4.0);
                cairo_show_text(cr, utf8);
            }
        }
    }

    // Draw cursor (#7D95FF AxisOS One UI Dark accent)
    if (tab->cursor_visible) {
        double cx = tab->cursor_col * TERM_CHAR_WIDTH;
        double cy = tab->cursor_row * TERM_CHAR_HEIGHT;
        cairo_set_source_rgba(cr, 125.0 / 255.0, 149.0 / 255.0, 255.0 / 255.0, 0.80);
        cairo_rectangle(cr, cx, cy, TERM_CHAR_WIDTH, TERM_CHAR_HEIGHT);
        cairo_fill(cr);
    }

    return FALSE;
}

static void send_to_pty(TerminalTab *tab, const char *data, size_t len) {
    if (tab && tab->pty_fd >= 0 && data && len > 0) {
        ssize_t ret = write(tab->pty_fd, data, len);
        (void)ret;
    }
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    TerminalTab *tab = (TerminalTab *)user_data;
    if (!tab) return FALSE;

    guint mod = event->state & gtk_accelerator_get_default_mod_mask();

    if ((mod & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
        if (event->keyval == GDK_KEY_T || event->keyval == GDK_KEY_t ||
            event->keyval == GDK_KEY_W || event->keyval == GDK_KEY_w) {
            // New tab / close tab handled at window level
            return FALSE;
        }
        if (event->keyval == GDK_KEY_V || event->keyval == GDK_KEY_v) {
            GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
            gchar *text = gtk_clipboard_wait_for_text(cb);
            if (text && *text) {
                send_to_pty(tab, text, strlen(text));
                g_free(text);
            }
            return TRUE;
        }
    }

    switch (event->keyval) {
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
            send_to_pty(tab, "\r", 1);
            return TRUE;
        case GDK_KEY_BackSpace:
            send_to_pty(tab, "\x7f", 1);
            return TRUE;
        case GDK_KEY_Delete:
        case GDK_KEY_KP_Delete:
            send_to_pty(tab, "\x1b[3~", 4);
            return TRUE;
        case GDK_KEY_Home:
        case GDK_KEY_KP_Home:
            send_to_pty(tab, "\x1b[H", 3);
            return TRUE;
        case GDK_KEY_End:
        case GDK_KEY_KP_End:
            send_to_pty(tab, "\x1b[F", 3);
            return TRUE;
        case GDK_KEY_Page_Up:
        case GDK_KEY_KP_Page_Up:
            send_to_pty(tab, "\x1b[5~", 4);
            return TRUE;
        case GDK_KEY_Page_Down:
        case GDK_KEY_KP_Page_Down:
            send_to_pty(tab, "\x1b[6~", 4);
            return TRUE;
        case GDK_KEY_Tab:
            send_to_pty(tab, "\t", 1);
            return TRUE;
        case GDK_KEY_Escape:
            send_to_pty(tab, "\x1b", 1);
            return TRUE;
        case GDK_KEY_Up:
        case GDK_KEY_KP_Up:
            send_to_pty(tab, "\x1b[A", 3);
            return TRUE;
        case GDK_KEY_Down:
        case GDK_KEY_KP_Down:
            send_to_pty(tab, "\x1b[B", 3);
            return TRUE;
        case GDK_KEY_Right:
        case GDK_KEY_KP_Right:
            send_to_pty(tab, "\x1b[C", 3);
            return TRUE;
        case GDK_KEY_Left:
        case GDK_KEY_KP_Left:
            send_to_pty(tab, "\x1b[D", 3);
            return TRUE;
        default:
            if (mod & GDK_CONTROL_MASK) {
                if (event->keyval >= 'a' && event->keyval <= 'z') {
                    char c = (char)(event->keyval - 'a' + 1);
                    send_to_pty(tab, &c, 1);
                    return TRUE;
                } else if (event->keyval >= 'A' && event->keyval <= 'Z') {
                    char c = (char)(event->keyval - 'A' + 1);
                    send_to_pty(tab, &c, 1);
                    return TRUE;
                }
            } else {
                guint32 uc = gdk_keyval_to_unicode(event->keyval);
                if (uc >= 32) {
                    char utf8[8] = {0};
                    int len = g_unichar_to_utf8(uc, utf8);
                    if (len > 0) {
                        send_to_pty(tab, utf8, (size_t)len);
                        return TRUE;
                    }
                } else if (event->string && event->string[0]) {
                    send_to_pty(tab, event->string, strlen(event->string));
                    return TRUE;
                }
            }
            break;
    }

    return FALSE;
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)event;
    (void)user_data;
    gtk_widget_grab_focus(widget);
    return FALSE;
}

static void on_tab_size_allocate(GtkWidget *widget, GdkRectangle *allocation, gpointer user_data) {
    (void)widget;
    TerminalTab *tab = (TerminalTab *)user_data;
    if (!tab) return;

    int new_cols = (int)(allocation->width / TERM_CHAR_WIDTH);
    int new_rows = (int)(allocation->height / TERM_CHAR_HEIGHT);

    if (new_cols < 10) new_cols = 10;
    if (new_rows < 5) new_rows = 5;

    if (new_cols != tab->cols || new_rows != tab->rows) {
        tab->cols = new_cols;
        tab->rows = new_rows;
        vterm_set_size(tab->vt, tab->rows, tab->cols);

        struct winsize ws = {
            .ws_row = (unsigned short)tab->rows,
            .ws_col = (unsigned short)tab->cols,
            .ws_xpixel = (unsigned short)allocation->width,
            .ws_ypixel = (unsigned short)allocation->height
        };
        ioctl(tab->pty_fd, TIOCSWINSZ, &ws);
    }
}

static void tab_destroy(TerminalTab *tab) {
    if (!tab) return;
    if (tab->io_watch_id) g_source_remove(tab->io_watch_id);
    if (tab->pty_fd >= 0) close(tab->pty_fd);
    if (tab->child_pid > 0) {
        kill(tab->child_pid, SIGTERM);
        waitpid(tab->child_pid, NULL, WNOHANG);
    }
    if (tab->vt) vterm_free(tab->vt);
    g_free(tab);
}

static void on_tab_close_btn_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    TerminalTab *tab = (TerminalTab *)user_data;
    TerminalApp *app = (TerminalApp *)tab->app;

    gint page = gtk_notebook_page_num(GTK_NOTEBOOK(app->notebook), tab->drawing_area);
    if (page >= 0) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(app->notebook), page);
        app->tabs = g_list_remove(app->tabs, tab);
        tab_destroy(tab);
    }
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(app->notebook)) == 0) {
        gtk_window_close(GTK_WINDOW(app->window));
    }
}

static TerminalTab *create_new_tab(TerminalApp *app) {
    TerminalTab *tab = g_new0(TerminalTab, 1);
    tab->app = app;
    tab->rows = DEFAULT_ROWS;
    tab->cols = DEFAULT_COLS;
    tab->cursor_visible = TRUE;

    tab->vt = vterm_new(tab->rows, tab->cols);
    vterm_set_utf8(tab->vt, 1);
    tab->vts = vterm_obtain_screen(tab->vt);
    vterm_screen_set_callbacks(tab->vts, &screen_cbs, tab);
    vterm_screen_reset(tab->vts, 1);

    struct winsize ws = {
        .ws_row = (unsigned short)tab->rows,
        .ws_col = (unsigned short)tab->cols,
        .ws_xpixel = (unsigned short)(tab->cols * TERM_CHAR_WIDTH),
        .ws_ypixel = (unsigned short)(tab->rows * TERM_CHAR_HEIGHT)
    };

    tab->child_pid = forkpty(&tab->pty_fd, NULL, NULL, &ws);
    if (tab->child_pid == 0) {
        // Child process: restore default SIGCHLD and launch user's preferred shell
        signal(SIGCHLD, SIG_DFL);
        const char *sh = getenv("SHELL");
        if (!sh || !*sh) sh = "/bin/bash";

        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);
        setenv("AXIS_TERMINAL", "1", 1);

        /* Print AxisOS Logo & Live System Details on every new terminal session */
        if (access("/usr/local/bin/axis-fetch", X_OK) == 0) {
            int rc = system("/usr/local/bin/axis-fetch 2>/dev/null");
            (void)rc;
            setenv("AXIS_FETCH_SHOWN", "1", 1);
        }

        execlp(sh, sh, "-l", NULL);
        _exit(1);
    }

    // Set non-blocking on master PTY
    int flags = fcntl(tab->pty_fd, F_GETFL, 0);
    fcntl(tab->pty_fd, F_SETFL, flags | O_NONBLOCK);

    GIOChannel *chan = g_io_channel_unix_new(tab->pty_fd);
    g_io_channel_set_encoding(chan, NULL, NULL);
    g_io_channel_set_buffered(chan, FALSE);
    tab->io_watch_id = g_io_add_watch(chan, G_IO_IN | G_IO_PRI | G_IO_HUP | G_IO_ERR, on_pty_read, tab);
    g_io_channel_unref(chan);

    tab->drawing_area = gtk_drawing_area_new();
    gtk_widget_set_can_focus(tab->drawing_area, TRUE);
    gtk_widget_set_events(tab->drawing_area, GDK_EXPOSURE_MASK | GDK_KEY_PRESS_MASK | GDK_BUTTON_PRESS_MASK);

    g_signal_connect(tab->drawing_area, "draw", G_CALLBACK(on_draw_terminal), tab);
    g_signal_connect(tab->drawing_area, "key-press-event", G_CALLBACK(on_key_press), tab);
    g_signal_connect(tab->drawing_area, "button-press-event", G_CALLBACK(on_button_press), tab);
    g_signal_connect(tab->drawing_area, "size-allocate", G_CALLBACK(on_tab_size_allocate), tab);

    // Tab title
    tab->tab_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    tab->tab_label = gtk_label_new("Terminal");
    GtkWidget *close_btn = gtk_button_new_from_icon_name("window-close-symbolic", GTK_ICON_SIZE_MENU);
    gtk_button_set_relief(GTK_BUTTON(close_btn), GTK_RELIEF_NONE);
    g_signal_connect(close_btn, "clicked", G_CALLBACK(on_tab_close_btn_clicked), tab);

    gtk_box_pack_start(GTK_BOX(tab->tab_box), tab->tab_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(tab->tab_box), close_btn, FALSE, FALSE, 0);
    gtk_widget_show_all(tab->tab_box);

    gint page = gtk_notebook_append_page(GTK_NOTEBOOK(app->notebook), tab->drawing_area, tab->tab_box);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(app->notebook), tab->drawing_area, TRUE);
    gtk_widget_show_all(tab->drawing_area);
    gtk_notebook_set_current_page(GTK_NOTEBOOK(app->notebook), page);

    app->tabs = g_list_append(app->tabs, tab);
    gtk_widget_grab_focus(tab->drawing_area);
    return tab;
}

static void on_new_tab_btn_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    TerminalApp *app = (TerminalApp *)user_data;
    create_new_tab(app);
}

static gboolean on_window_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    TerminalApp *app = (TerminalApp *)user_data;
    guint mod = event->state & gtk_accelerator_get_default_mod_mask();

    if ((mod & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
        if (event->keyval == GDK_KEY_T || event->keyval == GDK_KEY_t) {
            create_new_tab(app);
            return TRUE;
        } else if (event->keyval == GDK_KEY_W || event->keyval == GDK_KEY_w) {
            gint cur = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
            if (cur >= 0) {
                GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), cur);
                for (GList *l = app->tabs; l != NULL; l = l->next) {
                    TerminalTab *t = (TerminalTab *)l->data;
                    if (t->drawing_area == page) {
                        gtk_notebook_remove_page(GTK_NOTEBOOK(app->notebook), cur);
                        app->tabs = g_list_remove(app->tabs, t);
                        tab_destroy(t);
                        break;
                    }
                }
                if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(app->notebook)) == 0) {
                    gtk_window_close(GTK_WINDOW(app->window));
                }
            }
            return TRUE;
        }
    }
    return FALSE;
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
    gtk_css_provider_load_from_data(extra_prov, CSS_TERMINAL, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(extra_prov),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(extra_prov);
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("AxisOS Terminal Emulator (Pure C11 / libvterm)\n");
        printf("Usage: axis-terminal [OPTION] [-e COMMAND]\n");
        printf("Options:\n");
        printf("  --help, -h       Show this help message and exit\n");
        printf("  --version, -v    Print version information\n");
        return 0;
    }
    if (argc > 1 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        printf("axis-terminal 2.0.0 (Pure C11, GTK 3, libvterm)\n");
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

    TerminalApp app = {0};

    app.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size(GTK_WINDOW(app.window), 940, 580);
    gtk_window_set_position(GTK_WINDOW(app.window), GTK_WIN_POS_CENTER);
    gtk_window_set_title(GTK_WINDOW(app.window), "AxisOS Terminal");
    gtk_window_set_icon_name(GTK_WINDOW(app.window), "utilities-terminal");

    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app.window), root_box);

    /* Sleek Internal Toolbar (KWin handles native titlebar cleanly) */
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_container_set_border_width(GTK_CONTAINER(top_bar), 4);
    GtkWidget *new_tab_btn = gtk_button_new_from_icon_name("list-add", GTK_ICON_SIZE_MENU);
    gtk_widget_set_tooltip_text(new_tab_btn, "New Tab (Ctrl+Shift+T)");
    g_signal_connect(new_tab_btn, "clicked", G_CALLBACK(on_new_tab_btn_clicked), &app);
    gtk_box_pack_start(GTK_BOX(top_bar), new_tab_btn, FALSE, FALSE, 4);

    GtkWidget *bar_lbl = gtk_label_new("AxisOS Terminal  •  Obsidian Pure C11");
    gtk_box_pack_start(GTK_BOX(top_bar), bar_lbl, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(root_box), top_bar, FALSE, FALSE, 0);

    app.notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(app.notebook), TRUE);
    gtk_box_pack_start(GTK_BOX(root_box), app.notebook, TRUE, TRUE, 0);

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(app.window, "key-press-event", G_CALLBACK(on_window_key_press), &app);

    create_new_tab(&app);

    gtk_widget_show_all(app.window);
    gtk_main();

    return 0;
}
