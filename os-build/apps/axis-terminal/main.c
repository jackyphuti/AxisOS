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

    gboolean has_selection;
    gboolean selecting;
    int sel_start_row;
    int sel_start_col;
    int sel_end_row;
    int sel_end_col;

    gpointer app;
} TerminalTab;

typedef struct {
    GtkWidget *window;
    GtkWidget *header_bar;
    GtkWidget *notebook;
    GtkWidget *status_label;
    GtkWidget *popup_menu;
    guint status_timeout_id;
    GList *tabs;
} TerminalApp;

static TerminalTab *create_new_tab(TerminalApp *app);
static void send_to_pty(TerminalTab *tab, const char *data, size_t len);

static gboolean on_clear_status_timeout(gpointer user_data) {
    TerminalApp *app = (TerminalApp *)user_data;
    if (app) {
        if (app->status_label) {
            gtk_label_set_text(GTK_LABEL(app->status_label), "");
        }
        app->status_timeout_id = 0;
    }
    return G_SOURCE_REMOVE;
}

static void show_status_hint(TerminalApp *app, const char *msg) {
    if (!app || !app->status_label) return;
    if (app->status_timeout_id) {
        g_source_remove(app->status_timeout_id);
        app->status_timeout_id = 0;
    }
    gtk_label_set_text(GTK_LABEL(app->status_label), msg ? msg : "");
    if (msg && *msg) {
        app->status_timeout_id = g_timeout_add(1600, on_clear_status_timeout, app);
    }
}

static TerminalTab *get_active_tab(TerminalApp *app) {
    if (!app || !app->notebook) return NULL;
    gint cur = gtk_notebook_get_current_page(GTK_NOTEBOOK(app->notebook));
    if (cur < 0) return NULL;
    GtkWidget *page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(app->notebook), cur);
    for (GList *l = app->tabs; l != NULL; l = l->next) {
        TerminalTab *t = (TerminalTab *)l->data;
        if (t && t->drawing_area == page) {
            return t;
        }
    }
    return NULL;
}

static void clamp_cell(const TerminalTab *tab, double x, double y, int *out_row, int *out_col) {
    int c = (int)(x / TERM_CHAR_WIDTH);
    int r = (int)(y / TERM_CHAR_HEIGHT);
    if (c < 0) c = 0;
    if (c >= tab->cols) c = tab->cols - 1;
    if (r < 0) r = 0;
    if (r >= tab->rows) r = tab->rows - 1;
    *out_row = r;
    *out_col = c;
}

static void normalize_selection(const TerminalTab *tab, int *r1, int *c1, int *r2, int *c2) {
    if (tab->sel_start_row < tab->sel_end_row ||
        (tab->sel_start_row == tab->sel_end_row && tab->sel_start_col <= tab->sel_end_col)) {
        *r1 = tab->sel_start_row;
        *c1 = tab->sel_start_col;
        *r2 = tab->sel_end_row;
        *c2 = tab->sel_end_col;
    } else {
        *r1 = tab->sel_end_row;
        *c1 = tab->sel_end_col;
        *r2 = tab->sel_start_row;
        *c2 = tab->sel_start_col;
    }
    if (*r1 < 0) *r1 = 0;
    if (*c1 < 0) *c1 = 0;
    if (*r2 >= tab->rows) *r2 = tab->rows - 1;
    if (*c2 >= tab->cols) *c2 = tab->cols - 1;
}

static gboolean cell_is_selected(const TerminalTab *tab, int row, int col) {
    if (!tab || !tab->has_selection) return FALSE;
    int r1, c1, r2, c2;
    normalize_selection(tab, &r1, &c1, &r2, &c2);
    if (row < r1 || row > r2) return FALSE;
    if (row == r1 && col < c1) return FALSE;
    if (row == r2 && col > c2) return FALSE;
    return TRUE;
}

static void clear_selection(TerminalTab *tab) {
    if (!tab) return;
    if (tab->has_selection || tab->selecting) {
        tab->has_selection = FALSE;
        tab->selecting = FALSE;
        if (tab->drawing_area) {
            gtk_widget_queue_draw(tab->drawing_area);
        }
    }
}

static gboolean is_word_char(uint32_t ch) {
    if (ch == 0) return FALSE;
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9')) {
        return TRUE;
    }
    switch (ch) {
        case '_': case '-': case '.': case '/': case '~':
        case ':': case '@': case '+': case '=': case '%':
            return TRUE;
        default:
            return ch >= 128;
    }
}

static gchar *extract_selection_text(TerminalTab *tab) {
    if (!tab || !tab->vts || !tab->has_selection) return NULL;

    int r1, c1, r2, c2;
    normalize_selection(tab, &r1, &c1, &r2, &c2);

    GString *out = g_string_new(NULL);
    GString *line = g_string_new(NULL);

    for (int r = r1; r <= r2; ++r) {
        g_string_truncate(line, 0);
        int c_start = (r == r1) ? c1 : 0;
        int c_end = (r == r2) ? c2 : (tab->cols - 1);

        for (int c = c_start; c <= c_end; ++c) {
            VTermPos pos = { .row = r, .col = c };
            VTermScreenCell cell;
            memset(&cell, 0, sizeof(cell));
            if (vterm_screen_get_cell(tab->vts, pos, &cell) && cell.chars[0] != 0) {
                for (int i = 0; i < VTERM_MAX_CHARS_PER_CELL && cell.chars[i] != 0; ++i) {
                    char utf8[8] = {0};
                    int len = g_unichar_to_utf8(cell.chars[i], utf8);
                    if (len > 0) {
                        g_string_append_len(line, utf8, len);
                    }
                }
                if (cell.width > 1) {
                    c += (cell.width - 1);
                }
            } else {
                g_string_append_c(line, ' ');
            }
        }

        while (line->len > 0 && line->str[line->len - 1] == ' ') {
            g_string_truncate(line, line->len - 1);
        }

        g_string_append_len(out, line->str, (gssize)line->len);
        if (r < r2) {
            g_string_append_c(out, '\n');
        }
    }

    g_string_free(line, TRUE);
    if (out->len == 0) {
        g_string_free(out, TRUE);
        return NULL;
    }
    return g_string_free(out, FALSE);
}

static void copy_selection_to_clipboard(TerminalTab *tab, gboolean copy_to_clipboard) {
    if (!tab) return;
    gchar *text = extract_selection_text(tab);
    if (!text) return;

    GtkClipboard *primary = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    if (primary) {
        gtk_clipboard_set_text(primary, text, -1);
    }

    if (copy_to_clipboard) {
        GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        if (cb) {
            gtk_clipboard_set_text(cb, text, -1);
            gtk_clipboard_store(cb);
        }
        show_status_hint((TerminalApp *)tab->app, "Copied to clipboard");
    }

    g_free(text);
}

static void paste_from_clipboard(TerminalTab *tab, GdkAtom selection) {
    if (!tab) return;

    GtkClipboard *cb = gtk_clipboard_get(selection);
    gchar *text = cb ? gtk_clipboard_wait_for_text(cb) : NULL;

    if ((!text || !*text) && selection == GDK_SELECTION_PRIMARY) {
        g_free(text);
        GtkClipboard *fallback = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        text = fallback ? gtk_clipboard_wait_for_text(fallback) : NULL;
    }

    if (text && *text) {
        /* Normalize CRLF -> LF so pasted Windows/web text does not double-enter */
        GString *clean = g_string_sized_new(strlen(text));
        for (const char *p = text; *p; ++p) {
            if (p[0] == '\r' && p[1] == '\n') {
                g_string_append_c(clean, '\n');
                ++p;
            } else {
                g_string_append_c(clean, *p);
            }
        }
        clear_selection(tab);
        send_to_pty(tab, clean->str, clean->len);
        g_string_free(clean, TRUE);
        show_status_hint((TerminalApp *)tab->app, "Pasted");
    }

    g_free(text);
    if (tab->drawing_area) {
        gtk_widget_grab_focus(tab->drawing_area);
    }
}

static void select_word_at(TerminalTab *tab, int row, int col) {
    if (!tab || !tab->vts) return;
    VTermPos pos = { .row = row, .col = col };
    VTermScreenCell cell;
    memset(&cell, 0, sizeof(cell));
    vterm_screen_get_cell(tab->vts, pos, &cell);

    gboolean target_word = is_word_char(cell.chars[0]);
    int left = col;
    int right = col;

    while (left > 0) {
        VTermPos lp = { .row = row, .col = left - 1 };
        VTermScreenCell lc;
        memset(&lc, 0, sizeof(lc));
        vterm_screen_get_cell(tab->vts, lp, &lc);
        if (is_word_char(lc.chars[0]) != target_word) break;
        left--;
    }
    while (right + 1 < tab->cols) {
        VTermPos rp = { .row = row, .col = right + 1 };
        VTermScreenCell rc;
        memset(&rc, 0, sizeof(rc));
        vterm_screen_get_cell(tab->vts, rp, &rc);
        if (is_word_char(rc.chars[0]) != target_word) break;
        right++;
    }

    tab->sel_start_row = row;
    tab->sel_start_col = left;
    tab->sel_end_row = row;
    tab->sel_end_col = right;
    tab->has_selection = TRUE;
    tab->selecting = FALSE;
    copy_selection_to_clipboard(tab, FALSE);
    if (tab->drawing_area) {
        gtk_widget_queue_draw(tab->drawing_area);
    }
}

static void select_line_at(TerminalTab *tab, int row) {
    if (!tab || !tab->vts) return;
    int last_col = tab->cols - 1;
    while (last_col > 0) {
        VTermPos p = { .row = row, .col = last_col };
        VTermScreenCell c;
        memset(&c, 0, sizeof(c));
        if (vterm_screen_get_cell(tab->vts, p, &c) && c.chars[0] != 0 && c.chars[0] != ' ') {
            break;
        }
        last_col--;
    }

    tab->sel_start_row = row;
    tab->sel_start_col = 0;
    tab->sel_end_row = row;
    tab->sel_end_col = last_col;
    tab->has_selection = TRUE;
    tab->selecting = FALSE;
    copy_selection_to_clipboard(tab, FALSE);
    if (tab->drawing_area) {
        gtk_widget_queue_draw(tab->drawing_area);
    }
}

static void select_all_screen(TerminalTab *tab) {
    if (!tab || !tab->vts) return;
    int last_row = 0;
    for (int r = tab->rows - 1; r >= 0; --r) {
        gboolean row_non_empty = FALSE;
        for (int c = 0; c < tab->cols; ++c) {
            VTermPos p = { .row = r, .col = c };
            VTermScreenCell cell;
            memset(&cell, 0, sizeof(cell));
            if (vterm_screen_get_cell(tab->vts, p, &cell) && cell.chars[0] != 0 && cell.chars[0] != ' ') {
                row_non_empty = TRUE;
                break;
            }
        }
        if (row_non_empty) {
            last_row = r;
            break;
        }
    }

    tab->sel_start_row = 0;
    tab->sel_start_col = 0;
    tab->sel_end_row = last_row;
    tab->sel_end_col = tab->cols - 1;
    tab->has_selection = TRUE;
    tab->selecting = FALSE;
    copy_selection_to_clipboard(tab, FALSE);
    if (tab->drawing_area) {
        gtk_widget_queue_draw(tab->drawing_area);
    }
}

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
    TerminalTab *tab = (TerminalTab *)user;
    if (!tab) return 1;

    tab->cursor_row = pos.row;
    tab->cursor_col = pos.col;
    tab->cursor_visible = visible;

    if (tab->drawing_area && gtk_widget_get_window(tab->drawing_area)) {
        GdkWindow *win = gtk_widget_get_window(tab->drawing_area);
        GdkRectangle r_old = {
            .x = (int)(oldpos.col * TERM_CHAR_WIDTH),
            .y = (int)(oldpos.row * TERM_CHAR_HEIGHT),
            .width = (int)(TERM_CHAR_WIDTH + 2),
            .height = (int)(TERM_CHAR_HEIGHT + 2)
        };
        GdkRectangle r_new = {
            .x = (int)(pos.col * TERM_CHAR_WIDTH),
            .y = (int)(pos.row * TERM_CHAR_HEIGHT),
            .width = (int)(TERM_CHAR_WIDTH + 2),
            .height = (int)(TERM_CHAR_HEIGHT + 2)
        };
        gdk_window_invalidate_rect(win, &r_old, FALSE);
        gdk_window_invalidate_rect(win, &r_new, FALSE);
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
        char buf[32768];
        int chunks = 0;
        ssize_t bytes = 0;
        while (chunks < 4 && (bytes = read(tab->pty_fd, buf, sizeof(buf))) > 0) {
            vterm_input_write(tab->vt, buf, (size_t)bytes);
            chunks++;
        }
        if (chunks == 0 && bytes == 0) {
            return G_SOURCE_REMOVE;
        }
    }

    if (condition & (G_IO_HUP | G_IO_ERR)) {
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

static const char *CSS_TERMINAL =
    "@define-color axis_base #0E1320;"
    "@define-color axis_surface #171D2F;"
    "@define-color axis_surface_raised #222B45;"
    "@define-color axis_outline #2E3856;"
    "@define-color axis_accent #38BDF8;"
    "@define-color axis_accent_alt #7D95FF;"
    "@define-color axis_text #F2F3F7;"
    "@define-color axis_text_muted #94A3B8;"
    "window, headerbar, notebook, notebook header, button, menu, menuitem {"
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
    ".axis-term-toolbar {"
    "    background-color: @axis_surface;"
    "    border-bottom: 1px solid @axis_outline;"
    "    padding: 4px 8px;"
    "}"
    ".axis-term-btn {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 6px;"
    "    padding: 4px 10px;"
    "    font-size: 12px;"
    "    font-weight: 500;"
    "}"
    ".axis-term-btn:hover {"
    "    background-color: #2B3657;"
    "    border-color: @axis_accent;"
    "    color: #FFFFFF;"
    "}"
    ".axis-term-btn:active {"
    "    background-color: #1A243F;"
    "}"
    ".axis-term-status {"
    "    color: @axis_accent;"
    "    font-size: 12px;"
    "    font-weight: 600;"
    "}"
    ".axis-term-title {"
    "    color: @axis_text_muted;"
    "    font-size: 12px;"
    "    font-weight: 600;"
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
    "}"
    "menu {"
    "    background-color: #171D2F;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 8px;"
    "    padding: 4px;"
    "}"
    "menuitem {"
    "    background-color: transparent;"
    "    color: @axis_text;"
    "    padding: 6px 14px;"
    "    border-radius: 5px;"
    "}"
    "menuitem:hover {"
    "    background-color: #263455;"
    "    color: #FFFFFF;"
    "}"
    "menuitem:disabled {"
    "    color: #64748B;"
    "}"
    "menu separator {"
    "    background-color: @axis_outline;"
    "    margin: 4px 6px;"
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

    /* Dirty-rectangle clip bounds: only render cells intersecting the damaged region */
    double cx1 = 0, cy1 = 0, cx2 = alloc.width, cy2 = alloc.height;
    cairo_clip_extents(cr, &cx1, &cy1, &cx2, &cy2);
    int r_start = (int)(cy1 / TERM_CHAR_HEIGHT) - 1;
    int r_end   = (int)(cy2 / TERM_CHAR_HEIGHT) + 1;
    int c_start = (int)(cx1 / TERM_CHAR_WIDTH) - 1;
    int c_end   = (int)(cx2 / TERM_CHAR_WIDTH) + 1;
    if (r_start < 0) r_start = 0;
    if (r_end >= tab->rows) r_end = tab->rows - 1;
    if (c_start < 0) c_start = 0;
    if (c_end >= tab->cols) c_end = tab->cols - 1;

    VTermPos pos;
    for (pos.row = r_start; pos.row <= r_end; ++pos.row) {
        for (pos.col = c_start; pos.col <= c_end; ++pos.col) {
            double x = pos.col * TERM_CHAR_WIDTH;
            double y = pos.row * TERM_CHAR_HEIGHT;
            gboolean selected = cell_is_selected(tab, pos.row, pos.col);

            VTermScreenCell cell;
            memset(&cell, 0, sizeof(cell));
            gboolean has_cell = vterm_screen_get_cell(tab->vts, pos, &cell) != 0;

            if (has_cell && cell.chars[0] != 0) {
                vterm_screen_convert_color_to_rgb(tab->vts, &cell.bg);
                vterm_screen_convert_color_to_rgb(tab->vts, &cell.fg);

                if (selected) {
                    cairo_set_source_rgba(cr, 56.0 / 255.0, 189.0 / 255.0, 248.0 / 255.0, 0.36);
                    cairo_rectangle(cr, x, y, TERM_CHAR_WIDTH, TERM_CHAR_HEIGHT);
                    cairo_fill(cr);
                } else if (VTERM_COLOR_IS_RGB(&cell.bg) &&
                           !(cell.bg.rgb.red == 0 && cell.bg.rgb.green == 0 && cell.bg.rgb.blue == 0)) {
                    cairo_set_source_rgb(cr, cell.bg.rgb.red / 255.0,
                                         cell.bg.rgb.green / 255.0,
                                         cell.bg.rgb.blue / 255.0);
                    cairo_rectangle(cr, x, y, TERM_CHAR_WIDTH, TERM_CHAR_HEIGHT);
                    cairo_fill(cr);
                }

                if (selected) {
                    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
                } else if (VTERM_COLOR_IS_RGB(&cell.fg)) {
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
            } else if (selected) {
                cairo_set_source_rgba(cr, 56.0 / 255.0, 189.0 / 255.0, 248.0 / 255.0, 0.36);
                cairo_rectangle(cr, x, y, TERM_CHAR_WIDTH, TERM_CHAR_HEIGHT);
                cairo_fill(cr);
            }
        }
    }

    // Draw cursor (#7D95FF AxisOS accent)
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

static gboolean is_modifier_only_key(guint keyval) {
    switch (keyval) {
        case GDK_KEY_Shift_L: case GDK_KEY_Shift_R:
        case GDK_KEY_Control_L: case GDK_KEY_Control_R:
        case GDK_KEY_Alt_L: case GDK_KEY_Alt_R:
        case GDK_KEY_Super_L: case GDK_KEY_Super_R:
        case GDK_KEY_Meta_L: case GDK_KEY_Meta_R:
        case GDK_KEY_Caps_Lock: case GDK_KEY_Num_Lock:
            return TRUE;
        default:
            return FALSE;
    }
}

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    (void)widget;
    TerminalTab *tab = (TerminalTab *)user_data;
    if (!tab) return FALSE;

    if (is_modifier_only_key(event->keyval)) {
        return FALSE;
    }

    guint mod = event->state & gtk_accelerator_get_default_mod_mask();

    /* Ctrl+Shift shortcuts */
    if ((mod & (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) {
        if (event->keyval == GDK_KEY_T || event->keyval == GDK_KEY_t ||
            event->keyval == GDK_KEY_W || event->keyval == GDK_KEY_w) {
            return FALSE;
        }
        if (event->keyval == GDK_KEY_C || event->keyval == GDK_KEY_c) {
            copy_selection_to_clipboard(tab, TRUE);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_V || event->keyval == GDK_KEY_v) {
            paste_from_clipboard(tab, GDK_SELECTION_CLIPBOARD);
            return TRUE;
        }
        if (event->keyval == GDK_KEY_A || event->keyval == GDK_KEY_a) {
            select_all_screen(tab);
            return TRUE;
        }
    }

    /* Classic X11/Linux shortcuts: Ctrl+Insert (Copy) & Shift+Insert (Paste) */
    if ((mod & GDK_CONTROL_MASK) && !(mod & GDK_SHIFT_MASK) &&
        (event->keyval == GDK_KEY_Insert || event->keyval == GDK_KEY_KP_Insert)) {
        copy_selection_to_clipboard(tab, TRUE);
        return TRUE;
    }
    if ((mod & GDK_SHIFT_MASK) && !(mod & GDK_CONTROL_MASK) &&
        (event->keyval == GDK_KEY_Insert || event->keyval == GDK_KEY_KP_Insert)) {
        paste_from_clipboard(tab, GDK_SELECTION_CLIPBOARD);
        return TRUE;
    }

    clear_selection(tab);

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

static void on_menu_copy(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    TerminalTab *tab = (TerminalTab *)user_data;
    copy_selection_to_clipboard(tab, TRUE);
    if (tab && tab->drawing_area) gtk_widget_grab_focus(tab->drawing_area);
}

static void on_menu_paste(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    TerminalTab *tab = (TerminalTab *)user_data;
    paste_from_clipboard(tab, GDK_SELECTION_CLIPBOARD);
}

static void on_menu_select_all(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    TerminalTab *tab = (TerminalTab *)user_data;
    select_all_screen(tab);
    if (tab && tab->drawing_area) gtk_widget_grab_focus(tab->drawing_area);
}

static void on_menu_clear(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    TerminalTab *tab = (TerminalTab *)user_data;
    if (!tab) return;
    clear_selection(tab);
    send_to_pty(tab, "\x0c", 1);
    if (tab->drawing_area) gtk_widget_grab_focus(tab->drawing_area);
}

static void on_menu_new_tab(GtkMenuItem *item, gpointer user_data) {
    (void)item;
    TerminalTab *tab = (TerminalTab *)user_data;
    if (tab && tab->app) {
        create_new_tab((TerminalApp *)tab->app);
    }
}

static void show_context_menu(TerminalTab *tab, GdkEventButton *event) {
    if (!tab || !tab->app) return;
    TerminalApp *app = (TerminalApp *)tab->app;

    if (app->popup_menu) {
        gtk_widget_destroy(app->popup_menu);
        app->popup_menu = NULL;
    }

    GtkWidget *menu = gtk_menu_new();
    app->popup_menu = menu;

    GtkWidget *mi_copy = gtk_menu_item_new_with_label("Copy   (Ctrl+Shift+C)");
    gtk_widget_set_sensitive(mi_copy, tab->has_selection);
    g_signal_connect(mi_copy, "activate", G_CALLBACK(on_menu_copy), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_copy);

    GtkWidget *mi_paste = gtk_menu_item_new_with_label("Paste   (Ctrl+Shift+V)");
    g_signal_connect(mi_paste, "activate", G_CALLBACK(on_menu_paste), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_paste);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget *mi_selall = gtk_menu_item_new_with_label("Select All   (Ctrl+Shift+A)");
    g_signal_connect(mi_selall, "activate", G_CALLBACK(on_menu_select_all), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_selall);

    GtkWidget *mi_clear = gtk_menu_item_new_with_label("Clear   (Ctrl+L)");
    g_signal_connect(mi_clear, "activate", G_CALLBACK(on_menu_clear), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_clear);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());

    GtkWidget *mi_newtab = gtk_menu_item_new_with_label("New Tab   (Ctrl+Shift+T)");
    g_signal_connect(mi_newtab, "activate", G_CALLBACK(on_menu_new_tab), tab);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_newtab);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_pointer(GTK_MENU(menu), (const GdkEvent *)event);
}

static gboolean on_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    TerminalTab *tab = (TerminalTab *)user_data;
    gtk_widget_grab_focus(widget);
    if (!tab) return FALSE;

    int row = 0, col = 0;
    clamp_cell(tab, event->x, event->y, &row, &col);

    if (event->button == 1) {
        if (event->type == GDK_3BUTTON_PRESS) {
            select_line_at(tab, row);
            return TRUE;
        }
        if (event->type == GDK_2BUTTON_PRESS) {
            select_word_at(tab, row, col);
            return TRUE;
        }
        if (event->type == GDK_BUTTON_PRESS) {
            tab->selecting = TRUE;
            tab->has_selection = FALSE;
            tab->sel_start_row = row;
            tab->sel_start_col = col;
            tab->sel_end_row = row;
            tab->sel_end_col = col;
            gtk_widget_queue_draw(widget);
            return TRUE;
        }
    } else if (event->button == 2 && event->type == GDK_BUTTON_PRESS) {
        paste_from_clipboard(tab, GDK_SELECTION_PRIMARY);
        return TRUE;
    } else if (event->button == 3 && event->type == GDK_BUTTON_PRESS) {
        show_context_menu(tab, event);
        return TRUE;
    }

    return FALSE;
}

static gboolean on_motion_notify(GtkWidget *widget, GdkEventMotion *event, gpointer user_data) {
    TerminalTab *tab = (TerminalTab *)user_data;
    if (!tab || !tab->selecting) return FALSE;

    int row = 0, col = 0;
    clamp_cell(tab, event->x, event->y, &row, &col);

    if (row != tab->sel_end_row || col != tab->sel_end_col || !tab->has_selection) {
        tab->sel_end_row = row;
        tab->sel_end_col = col;
        tab->has_selection = TRUE;
        gtk_widget_queue_draw(widget);
    }
    return TRUE;
}

static gboolean on_button_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data) {
    (void)widget;
    TerminalTab *tab = (TerminalTab *)user_data;
    if (!tab) return FALSE;

    if (event->button == 1 && tab->selecting) {
        tab->selecting = FALSE;
        if (tab->has_selection) {
            copy_selection_to_clipboard(tab, FALSE);
        }
        return TRUE;
    }
    return FALSE;
}

static void on_drawing_area_realize(GtkWidget *widget, gpointer user_data) {
    (void)user_data;
    GdkWindow *win = gtk_widget_get_window(widget);
    if (win) {
        GdkDisplay *disp = gtk_widget_get_display(widget);
        GdkCursor *cursor = gdk_cursor_new_from_name(disp, "text");
        if (cursor) {
            gdk_window_set_cursor(win, cursor);
            g_object_unref(cursor);
        }
    }
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
        clear_selection(tab);
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
    gtk_widget_set_events(tab->drawing_area,
                          GDK_EXPOSURE_MASK |
                          GDK_KEY_PRESS_MASK |
                          GDK_BUTTON_PRESS_MASK |
                          GDK_BUTTON_RELEASE_MASK |
                          GDK_POINTER_MOTION_MASK);

    g_signal_connect(tab->drawing_area, "realize", G_CALLBACK(on_drawing_area_realize), tab);
    g_signal_connect(tab->drawing_area, "draw", G_CALLBACK(on_draw_terminal), tab);
    g_signal_connect(tab->drawing_area, "key-press-event", G_CALLBACK(on_key_press), tab);
    g_signal_connect(tab->drawing_area, "button-press-event", G_CALLBACK(on_button_press), tab);
    g_signal_connect(tab->drawing_area, "motion-notify-event", G_CALLBACK(on_motion_notify), tab);
    g_signal_connect(tab->drawing_area, "button-release-event", G_CALLBACK(on_button_release), tab);
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

static void on_copy_btn_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    TerminalApp *app = (TerminalApp *)user_data;
    TerminalTab *tab = get_active_tab(app);
    if (tab) {
        copy_selection_to_clipboard(tab, TRUE);
        if (tab->drawing_area) gtk_widget_grab_focus(tab->drawing_area);
    }
}

static void on_paste_btn_clicked(GtkButton *btn, gpointer user_data) {
    (void)btn;
    TerminalApp *app = (TerminalApp *)user_data;
    TerminalTab *tab = get_active_tab(app);
    if (tab) {
        paste_from_clipboard(tab, GDK_SELECTION_CLIPBOARD);
    }
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
    GError *css_err = NULL;
    if (!gtk_css_provider_load_from_data(extra_prov, CSS_TERMINAL, -1, &css_err)) {
        if (css_err) {
            fprintf(stderr, "axis-terminal CSS warning: %s\n", css_err->message);
            g_error_free(css_err);
        }
    }
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(extra_prov),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(extra_prov);
}

static GtkWidget *make_toolbar_btn(const char *icon_name, const char *label_text, const char *tooltip) {
    GtkWidget *btn = gtk_button_new();
    gtk_widget_set_can_focus(btn, FALSE);
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "axis-term-btn");
    if (tooltip) {
        gtk_widget_set_tooltip_text(btn, tooltip);
    }

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    if (icon_name) {
        GtkWidget *img = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
        gtk_box_pack_start(GTK_BOX(box), img, FALSE, FALSE, 0);
    }
    if (label_text) {
        GtkWidget *lbl = gtk_label_new(label_text);
        gtk_box_pack_start(GTK_BOX(box), lbl, FALSE, FALSE, 0);
    }
    gtk_container_add(GTK_CONTAINER(btn), box);
    return btn;
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
        printf("axis-terminal 2.1.0 (Pure C11, GTK 3, libvterm, Copy/Paste)\n");
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
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(top_bar), "axis-term-toolbar");

    GtkWidget *new_tab_btn = make_toolbar_btn("list-add-symbolic", "New Tab", "New Tab (Ctrl+Shift+T)");
    g_signal_connect(new_tab_btn, "clicked", G_CALLBACK(on_new_tab_btn_clicked), &app);
    gtk_box_pack_start(GTK_BOX(top_bar), new_tab_btn, FALSE, FALSE, 0);

    GtkWidget *copy_btn = make_toolbar_btn("edit-copy-symbolic", "Copy", "Copy Selection (Ctrl+Shift+C or Ctrl+Insert)");
    g_signal_connect(copy_btn, "clicked", G_CALLBACK(on_copy_btn_clicked), &app);
    gtk_box_pack_start(GTK_BOX(top_bar), copy_btn, FALSE, FALSE, 0);

    GtkWidget *paste_btn = make_toolbar_btn("edit-paste-symbolic", "Paste", "Paste from Clipboard (Ctrl+Shift+V, Shift+Insert, or Middle-Click)");
    g_signal_connect(paste_btn, "clicked", G_CALLBACK(on_paste_btn_clicked), &app);
    gtk_box_pack_start(GTK_BOX(top_bar), paste_btn, FALSE, FALSE, 0);

    GtkWidget *bar_lbl = gtk_label_new("AxisOS Terminal  •  Obsidian Pure C11");
    gtk_style_context_add_class(gtk_widget_get_style_context(bar_lbl), "axis-term-title");
    gtk_box_pack_start(GTK_BOX(top_bar), bar_lbl, FALSE, FALSE, 10);

    app.status_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(app.status_label), "axis-term-status");
    gtk_box_pack_end(GTK_BOX(top_bar), app.status_label, FALSE, FALSE, 6);

    gtk_box_pack_start(GTK_BOX(root_box), top_bar, FALSE, FALSE, 0);

    app.notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(app.notebook), TRUE);
    gtk_box_pack_start(GTK_BOX(root_box), app.notebook, TRUE, TRUE, 0);

    g_signal_connect(app.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(app.window, "key-press-event", G_CALLBACK(on_window_key_press), &app);

    create_new_tab(&app);

    gtk_widget_show_all(app.window);
    gtk_main();

    if (app.status_timeout_id) {
        g_source_remove(app.status_timeout_id);
        app.status_timeout_id = 0;
    }
    if (app.popup_menu) {
        gtk_widget_destroy(app.popup_menu);
        app.popup_menu = NULL;
    }

    return 0;
}
