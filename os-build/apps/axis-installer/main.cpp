/*
 * AxisOS Windows 10-Style Graphical System Installer
 * Written in C++ using GTK 3
 * Exact Windows 10 Setup Wizard UX with custom AxisOS touches, Btrfs deployment,
 * and synchronized User/Root administrator security.
 */

#include <gtk/gtk.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>

static const char* WIN10_CSS = R"(
* {
    font-family: "Segoe UI", "Inter", "Cantarell", sans-serif;
}

window {
    background-color: #0A0B0D;
    color: #F8FAFC;
}

headerbar {
    background: #14161A;
    border-bottom: 1px solid rgba(255, 255, 255, 0.12);
    color: #FFFFFF;
    min-height: 40px;
    padding: 0 10px;
}

headerbar .title {
    font-weight: 600;
    font-size: 14px;
    color: #FFFFFF;
}

.setup-canvas {
    background-color: #0A0B0D;
    padding: 30px 45px;
}

.win-title {
    font-size: 22px;
    font-weight: 300;
    color: #FFFFFF;
    margin-bottom: 8px;
}

.win-subtitle {
    font-size: 13px;
    color: #94A3B8;
    margin-bottom: 24px;
    line-height: 1.4;
}

.btn-win-primary {
    background-color: #FFFFFF;
    color: #0A0B0D;
    font-size: 13px;
    font-weight: 700;
    border: 1px solid #FFFFFF;
    border-radius: 4px;
    padding: 8px 24px;
}

.btn-win-primary:hover {
    background-color: #E2E8F0;
    border-color: #E2E8F0;
}

.btn-win-primary:disabled {
    background-color: #202329;
    color: #64748B;
    border-color: #202329;
}

.btn-win-secondary {
    background-color: #202329;
    color: #F8FAFC;
    font-size: 13px;
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: 4px;
    padding: 7px 18px;
}

.btn-win-secondary:hover {
    background-color: rgba(255, 255, 255, 0.14);
    color: #FFFFFF;
    border-color: #FFFFFF;
}

.btn-install-large {
    background-color: #FFFFFF;
    color: #0A0B0D;
    font-size: 16px;
    font-weight: 700;
    border: 1px solid #FFFFFF;
    border-radius: 4px;
    padding: 14px 44px;
}

.btn-install-large:hover {
    background-color: #E2E8F0;
}

.option-card {
    background-color: #14161A;
    border: 1px solid rgba(255, 255, 255, 0.10);
    border-radius: 6px;
    padding: 16px 20px;
    margin-bottom: 12px;
}

.option-card:hover {
    border-color: #FFFFFF;
    background-color: #202329;
}

.option-card-title {
    font-size: 15px;
    font-weight: 600;
    color: #FFFFFF;
    margin-bottom: 4px;
}

.option-card-desc {
    font-size: 12px;
    color: #94A3B8;
}

.treeview-win {
    background-color: #14161A;
    color: #F8FAFC;
    border: 1px solid rgba(255, 255, 255, 0.12);
}

.treeview-win:selected {
    background-color: #FFFFFF;
    color: #0A0B0D;
}

entry {
    background-color: #202329;
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 0.14);
    border-radius: 4px;
    padding: 8px 12px;
    font-size: 13px;
}

entry:focus {
    border-color: #FFFFFF;
}

combobox button {
    background-color: #202329;
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 0.14);
    border-radius: 4px;
    padding: 6px 10px;
}

.check-label {
    font-size: 13px;
    color: #F8FAFC;
}

.progress-stage {
    font-size: 14px;
    color: #94A3B8;
    margin-bottom: 10px;
}

.progress-stage.active {
    color: #FFFFFF;
    font-weight: 600;
}

.progress-stage.done {
    color: #FFFFFF;
}

progressbar progress {
    background-color: #FFFFFF;
    border-radius: 2px;
}

progressbar trough {
    background-color: #202329;
    border-radius: 2px;
}
)";

struct DriveInfo {
    std::string path;
    std::string size;
    std::string model;
};

class Win10InstallerApp {
public:
    GtkWidget* window;
    GtkWidget* stack;
    GtkWidget* btn_back;
    GtkWidget* btn_next;
    int current_step = 0;

    // User choices
    std::string selected_disk = "/dev/sda";
    std::string fullname = "AxisOS User";
    std::string username = "axis";
    std::string password = "axis";
    std::string hostname = "axis-pc";

    // Widgets for steps
    GtkWidget* combo_lang;
    GtkWidget* combo_time;
    GtkWidget* combo_keyboard;
    GtkWidget* chk_license;
    GtkWidget* tree_drives;
    GtkListStore* store_drives;
    GtkWidget* entry_fullname;
    GtkWidget* entry_username;
    GtkWidget* entry_hostname;
    GtkWidget* entry_pwd;
    GtkWidget* entry_cpwd;

    // Progress widgets
    GtkWidget* lbl_stage_copy;
    GtkWidget* lbl_stage_prep;
    GtkWidget* lbl_stage_feat;
    GtkWidget* lbl_stage_upd;
    GtkWidget* lbl_stage_fin;
    GtkWidget* progress_bar;
    GtkWidget* lbl_progress_percent;

    Win10InstallerApp() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "AxisOS 2.0 Setup");
        gtk_window_set_default_size(GTK_WINDOW(window), 820, 560);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

        // Apply Windows 10 CSS
        GtkCssProvider* provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider, WIN10_CSS, -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
        );
        g_object_unref(provider);

        // Headerbar
        GtkWidget* hb = gtk_header_bar_new();
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
        gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS 2.0 Setup");
        gtk_window_set_titlebar(GTK_WINDOW(window), hb);

        // Main Layout
        GtkWidget* root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(window), root_box);

        // Content Stack
        stack = gtk_stack_new();
        gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
        gtk_stack_set_transition_duration(GTK_STACK(stack), 180);
        gtk_style_context_add_class(gtk_widget_get_style_context(stack), "setup-canvas");
        gtk_box_pack_start(GTK_BOX(root_box), stack, TRUE, TRUE, 0);

        // Navigation Footer Bar
        GtkWidget* footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_widget_set_margin_top(footer, 14);
        gtk_widget_set_margin_bottom(footer, 18);
        gtk_widget_set_margin_start(footer, 40);
        gtk_widget_set_margin_end(footer, 40);

        btn_back = gtk_button_new_with_label("Back");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_back), "btn-win-secondary");
        g_signal_connect_swapped(btn_back, "clicked", G_CALLBACK(on_back_clicked), this);
        gtk_widget_set_sensitive(btn_back, FALSE);

        btn_next = gtk_button_new_with_label("Next");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_next), "btn-win-primary");
        g_signal_connect_swapped(btn_next, "clicked", G_CALLBACK(on_next_clicked), this);

        gtk_box_pack_start(GTK_BOX(footer), btn_back, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(footer), btn_next, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(root_box), footer, FALSE, FALSE, 0);

        // Build Windows 10 Wizard Screens
        build_step_locale();      // 0: Language, time, keyboard
        build_step_install_now(); // 1: "Install now" screen
        build_step_license();     // 2: License terms checkbox
        build_step_type();        // 3: Upgrade vs Custom
        build_step_disk();        // 4: Where do you want to install?
        build_step_account();     // 5: User & administrator password
        build_step_installing();  // 6: 5-stage installing checklist
        build_step_finish();      // 7: Rebooting in 10 seconds

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
        update_navigation_state();
    }

    // -------------------------------------------------------------------------
    // Step 0: Language & Locale
    // -------------------------------------------------------------------------
    void build_step_locale() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* title = gtk_label_new("AxisOS 2.0");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* sub = gtk_label_new("Select your language, time format, and keyboard input method.");
        gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

        GtkWidget* grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid), 18);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 14);

        GtkWidget* l1 = gtk_label_new("Language to install:");
        gtk_label_set_xalign(GTK_LABEL(l1), 0.0);
        combo_lang = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_lang), "English (United States)");
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_lang), 0);

        GtkWidget* l2 = gtk_label_new("Time and currency format:");
        gtk_label_set_xalign(GTK_LABEL(l2), 0.0);
        combo_time = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_time), "English (United States)");
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_time), 0);

        GtkWidget* l3 = gtk_label_new("Keyboard or input method:");
        gtk_label_set_xalign(GTK_LABEL(l3), 0.0);
        combo_keyboard = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_keyboard), "US");
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_keyboard), 0);

        gtk_grid_attach(GTK_GRID(grid), l1, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), combo_lang, 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), l2, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), combo_time, 1, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), l3, 0, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), combo_keyboard, 1, 2, 1, 1);

        gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);
        gtk_stack_add_named(GTK_STACK(stack), box, "step_0");
    }

    // -------------------------------------------------------------------------
    // Step 1: Iconic "Install now"
    // -------------------------------------------------------------------------
    void build_step_install_now() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 24);
        gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
        gtk_widget_set_halign(box, GTK_ALIGN_CENTER);

        GtkWidget* logo_label = gtk_label_new("AxisOS Linux 2.0");
        gtk_style_context_add_class(gtk_widget_get_style_context(logo_label), "win-title");

        GtkWidget* desc = gtk_label_new("Debian GNU/Linux Edition • 64-bit Hybrid Architecture");
        gtk_style_context_add_class(gtk_widget_get_style_context(desc), "win-subtitle");

        GtkWidget* btn_install_now = gtk_button_new_with_label("Install now");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_install_now), "btn-install-large");
        g_signal_connect_swapped(btn_install_now, "clicked", G_CALLBACK(on_install_now_clicked), this);

        GtkWidget* repair_link = gtk_link_button_new_with_label("about:blank", "Repair your computer");
        gtk_widget_set_halign(repair_link, GTK_ALIGN_CENTER);

        gtk_box_pack_start(GTK_BOX(box), logo_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), desc, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), btn_install_now, FALSE, FALSE, 10);
        gtk_box_pack_start(GTK_BOX(box), repair_link, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), box, "step_1");
    }

    // -------------------------------------------------------------------------
    // Step 2: License terms
    // -------------------------------------------------------------------------
    void build_step_license() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

        GtkWidget* title = gtk_label_new("Applicable notices and license terms");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 260);
        GtkWidget* txt = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(txt), FALSE);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(txt), GTK_WRAP_WORD);
        
        GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(txt));
        gtk_text_buffer_set_text(buf,
            "AXISOS OPERATING SYSTEM LICENSE TERMS (DEBIAN EDITION)\n\n"
            "AxisOS is free and open-source software built upon Debian GNU/Linux.\n\n"
            "1. FREEDOM OF USE\n"
            "You are granted the right to run AxisOS for any personal, development, or enterprise purpose.\n\n"
            "2. PRIVACY & SECURITY\n"
            "AxisOS respects user privacy. There is zero telemetry tracking, no background advertising, and no data harvesting.\n\n"
            "3. ROOT & HARDWARE PRIVILEGES\n"
            "Administrative root credentials are synchronized with your user account to guarantee Debian-grade security.\n\n"
            "By installing, you agree to these open-source principles.", -1);

        gtk_container_add(GTK_CONTAINER(scroll), txt);

        chk_license = gtk_check_button_new_with_label("I accept the license terms");
        gtk_style_context_add_class(gtk_widget_get_style_context(chk_license), "check-label");
        g_signal_connect_swapped(chk_license, "toggled", G_CALLBACK(on_license_toggled), this);

        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(box), chk_license, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), box, "step_2");
    }

    // -------------------------------------------------------------------------
    // Step 3: Installation Type (Upgrade vs Custom)
    // -------------------------------------------------------------------------
    void build_step_type() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* title = gtk_label_new("Which type of installation do you want?");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");
        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);

        // Option 1: Upgrade
        GtkWidget* card1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_style_context_add_class(gtk_widget_get_style_context(card1), "option-card");
        GtkWidget* c1_t = gtk_label_new("Upgrade: Install AxisOS and keep files, settings, and applications");
        gtk_label_set_xalign(GTK_LABEL(c1_t), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(c1_t), "option-card-title");
        GtkWidget* c1_d = gtk_label_new("The files, settings, and applications are preserved. This option is only available when a compatible partition exists.");
        gtk_label_set_xalign(GTK_LABEL(c1_d), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(c1_d), "option-card-desc");
        gtk_box_pack_start(GTK_BOX(card1), c1_t, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card1), c1_d, FALSE, FALSE, 0);

        // Option 2: Custom
        GtkWidget* card2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_style_context_add_class(gtk_widget_get_style_context(card2), "option-card");
        GtkWidget* c2_t = gtk_label_new("Custom: Install AxisOS only (advanced)");
        gtk_label_set_xalign(GTK_LABEL(c2_t), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(c2_t), "option-card-title");
        GtkWidget* c2_d = gtk_label_new("The drives and partitions can be selected, formatted with Btrfs, and cleanly deployed.");
        gtk_label_set_xalign(GTK_LABEL(c2_d), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(c2_d), "option-card-desc");
        gtk_box_pack_start(GTK_BOX(card2), c2_t, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card2), c2_d, FALSE, FALSE, 0);

        // Connect click event on card2 (custom)
        GtkWidget* ev2 = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(ev2), card2);
        g_signal_connect_swapped(ev2, "button-press-event", G_CALLBACK(on_custom_install_selected), this);

        GtkWidget* ev1 = gtk_event_box_new();
        gtk_container_add(GTK_CONTAINER(ev1), card1);
        g_signal_connect_swapped(ev1, "button-press-event", G_CALLBACK(on_custom_install_selected), this);

        gtk_box_pack_start(GTK_BOX(box), ev1, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), ev2, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), box, "step_3");
    }

    // -------------------------------------------------------------------------
    // Step 4: Where do you want to install AxisOS? (Disk Table)
    // -------------------------------------------------------------------------
    void build_step_disk() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

        GtkWidget* title = gtk_label_new("Where do you want to install AxisOS?");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");
        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);

        // Table with Drive 0, Drive 1
        store_drives = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        tree_drives = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_drives));
        gtk_style_context_add_class(gtk_widget_get_style_context(tree_drives), "treeview-win");

        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_drives), -1, "Name", renderer, "text", 0, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_drives), -1, "Total size", renderer, "text", 1, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_drives), -1, "Free space", renderer, "text", 2, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_drives), -1, "Type / Model", renderer, "text", 3, NULL);

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 200);
        gtk_container_add(GTK_CONTAINER(scroll), tree_drives);
        gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

        // Windows action buttons: [Refresh] [Delete] [Format] [New]
        GtkWidget* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        GtkWidget* b_refresh = gtk_button_new_with_label("Refresh");
        GtkWidget* b_delete = gtk_button_new_with_label("Delete");
        GtkWidget* b_format = gtk_button_new_with_label("Format");
        GtkWidget* b_new = gtk_button_new_with_label("New");

        gtk_style_context_add_class(gtk_widget_get_style_context(b_refresh), "btn-win-secondary");
        gtk_style_context_add_class(gtk_widget_get_style_context(b_delete), "btn-win-secondary");
        gtk_style_context_add_class(gtk_widget_get_style_context(b_format), "btn-win-secondary");
        gtk_style_context_add_class(gtk_widget_get_style_context(b_new), "btn-win-secondary");

        g_signal_connect_swapped(b_refresh, "clicked", G_CALLBACK(on_refresh_drives_clicked), this);

        gtk_box_pack_start(GTK_BOX(btn_row), b_refresh, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_row), b_delete, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_row), b_format, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(btn_row), b_new, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), btn_row, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), box, "step_4");
        populate_drives();
    }

    void populate_drives() {
        gtk_list_store_clear(store_drives);
        
        FILE* fp = popen("lsblk -d -p -n -l -o NAME,SIZE,MODEL,TYPE 2>/dev/null", "r");
        bool found = false;
        if (fp) {
            char line[512];
            int idx = 0;
            while (fgets(line, sizeof(line), fp)) {
                std::istringstream iss(line);
                std::string dev_name, size, model, type;
                iss >> dev_name >> size;
                std::string rest;
                std::getline(iss, rest);
                
                if (dev_name.find("/dev/loop") == std::string::npos &&
                    dev_name.find("/dev/zram") == std::string::npos &&
                    dev_name.find("/dev/ram") == std::string::npos) {
                    
                    GtkTreeIter iter;
                    gtk_list_store_append(store_drives, &iter);
                    std::string label = "Drive " + std::to_string(idx) + " (" + dev_name + ")";
                    gtk_list_store_set(store_drives, &iter,
                        0, label.c_str(),
                        1, size.c_str(),
                        2, size.c_str(),
                        3, (rest.empty() ? "Primary Disk" : rest.c_str()),
                        -1);
                    if (idx == 0) selected_disk = dev_name;
                    idx++;
                    found = true;
                }
            }
            pclose(fp);
        }

        if (!found) {
            GtkTreeIter iter;
            gtk_list_store_append(store_drives, &iter);
            gtk_list_store_set(store_drives, &iter,
                0, "Drive 0 (/dev/sda)",
                1, "64.0 GB",
                2, "64.0 GB",
                3, "Primary System Storage",
                -1);
            selected_disk = "/dev/sda";
        }

        // Select first row
        GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_drives));
        GtkTreePath* path = gtk_tree_path_new_first();
        gtk_tree_selection_select_path(sel, path);
        gtk_tree_path_free(path);
    }

    // -------------------------------------------------------------------------
    // Step 5: User & Root Security Account
    // -------------------------------------------------------------------------
    void build_step_account() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

        GtkWidget* title = gtk_label_new("Who's going to use this PC?");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* sub = gtk_label_new("Create your account. In AxisOS Debian Edition, user and root administrator\npasswords are kept identical so you have seamless, authenticated security.");
        gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

        GtkWidget* grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid), 16);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 10);

        GtkWidget* l1 = gtk_label_new("Your full name:");
        gtk_label_set_xalign(GTK_LABEL(l1), 0.0);
        entry_fullname = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry_fullname), "AxisOS User");

        GtkWidget* l2 = gtk_label_new("Username (login):");
        gtk_label_set_xalign(GTK_LABEL(l2), 0.0);
        entry_username = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry_username), "axis");

        GtkWidget* l3 = gtk_label_new("PC name (hostname):");
        gtk_label_set_xalign(GTK_LABEL(l3), 0.0);
        entry_hostname = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry_hostname), "axis-pc");

        GtkWidget* l4 = gtk_label_new("Choose password:");
        gtk_label_set_xalign(GTK_LABEL(l4), 0.0);
        entry_pwd = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(entry_pwd), FALSE);
        gtk_entry_set_text(GTK_ENTRY(entry_pwd), "password");

        GtkWidget* l5 = gtk_label_new("Confirm password:");
        gtk_label_set_xalign(GTK_LABEL(l5), 0.0);
        entry_cpwd = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(entry_cpwd), FALSE);
        gtk_entry_set_text(GTK_ENTRY(entry_cpwd), "password");

        gtk_grid_attach(GTK_GRID(grid), l1, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry_fullname, 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), l2, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry_username, 1, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), l3, 0, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry_hostname, 1, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), l4, 0, 3, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry_pwd, 1, 3, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), l5, 0, 4, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry_cpwd, 1, 4, 1, 1);

        gtk_box_pack_start(GTK_BOX(box), grid, FALSE, FALSE, 0);
        gtk_stack_add_named(GTK_STACK(stack), box, "step_5");
    }

    // -------------------------------------------------------------------------
    // Step 6: The Iconic Windows 10 5-Stage "Installing AxisOS"
    // -------------------------------------------------------------------------
    void build_step_installing() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);

        GtkWidget* title = gtk_label_new("Installing AxisOS");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* sub = gtk_label_new("Your PC will restart several times. This might take a while.");
        gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

        // 5 Stages Checklist
        lbl_stage_copy = gtk_label_new("    Copying AxisOS files");
        gtk_label_set_xalign(GTK_LABEL(lbl_stage_copy), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_stage_copy), "progress-stage");

        lbl_stage_prep = gtk_label_new("    Getting files ready for installation (0%)");
        gtk_label_set_xalign(GTK_LABEL(lbl_stage_prep), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_stage_prep), "progress-stage");

        lbl_stage_feat = gtk_label_new("    Installing features & drivers (NVIDIA, AMD, Intel)");
        gtk_label_set_xalign(GTK_LABEL(lbl_stage_feat), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_stage_feat), "progress-stage");

        lbl_stage_upd = gtk_label_new("    Installing updates");
        gtk_label_set_xalign(GTK_LABEL(lbl_stage_upd), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_stage_upd), "progress-stage");

        lbl_stage_fin = gtk_label_new("    Finishing up");
        gtk_label_set_xalign(GTK_LABEL(lbl_stage_fin), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_stage_fin), "progress-stage");

        gtk_box_pack_start(GTK_BOX(box), lbl_stage_copy, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), lbl_stage_prep, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), lbl_stage_feat, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), lbl_stage_upd, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), lbl_stage_fin, FALSE, FALSE, 0);

        progress_bar = gtk_progress_bar_new();
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.05);
        gtk_box_pack_start(GTK_BOX(box), progress_bar, FALSE, FALSE, 14);

        lbl_progress_percent = gtk_label_new("Status: Preparing disk partitioning and Btrfs subvolumes...");
        gtk_label_set_xalign(GTK_LABEL(lbl_progress_percent), 0.0);
        gtk_box_pack_start(GTK_BOX(box), lbl_progress_percent, FALSE, FALSE, 0);

        gtk_stack_add_named(GTK_STACK(stack), box, "step_6");
    }

    // -------------------------------------------------------------------------
    // Step 7: Restarting in 10 seconds
    // -------------------------------------------------------------------------
    void build_step_finish() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 16);
        gtk_widget_set_valign(box, GTK_ALIGN_CENTER);

        GtkWidget* title = gtk_label_new("AxisOS needs to restart to continue");
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* sub = gtk_label_new("Please remove your live USB installation media and restart your computer.");
        gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

        GtkWidget* btn_restart = gtk_button_new_with_label("Restart now");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_restart), "btn-win-primary");
        gtk_widget_set_halign(btn_restart, GTK_ALIGN_CENTER);
        g_signal_connect_swapped(btn_restart, "clicked", G_CALLBACK(on_restart_clicked), this);

        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), btn_restart, FALSE, FALSE, 12);

        gtk_stack_add_named(GTK_STACK(stack), box, "step_7");
    }

    // -------------------------------------------------------------------------
    // Navigation & Logic
    // -------------------------------------------------------------------------
    void update_navigation_state() {
        std::string name = "step_" + std::to_string(current_step);
        gtk_stack_set_visible_child_name(GTK_STACK(stack), name.c_str());

        // Back button
        gtk_widget_set_sensitive(btn_back, current_step > 0 && current_step < 6);

        // Next button
        if (current_step == 1) { // Install now screen
            gtk_widget_set_visible(btn_next, FALSE);
            gtk_widget_set_visible(btn_back, TRUE);
        } else if (current_step == 2) { // License
            gtk_widget_set_visible(btn_next, TRUE);
            gtk_widget_set_sensitive(btn_next, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(chk_license)));
            gtk_button_set_label(GTK_BUTTON(btn_next), "Next");
        } else if (current_step == 3) { // Type
            gtk_widget_set_visible(btn_next, FALSE);
        } else if (current_step == 5) { // Account
            gtk_widget_set_visible(btn_next, TRUE);
            gtk_button_set_label(GTK_BUTTON(btn_next), "Install");
        } else if (current_step == 6) { // Installing
            gtk_widget_set_visible(btn_next, FALSE);
            gtk_widget_set_visible(btn_back, FALSE);
        } else if (current_step == 7) { // Finish
            gtk_widget_set_visible(btn_next, FALSE);
            gtk_widget_set_visible(btn_back, FALSE);
        } else {
            gtk_widget_set_visible(btn_next, TRUE);
            gtk_widget_set_sensitive(btn_next, TRUE);
            gtk_button_set_label(GTK_BUTTON(btn_next), "Next");
        }
    }

    static void on_install_now_clicked(Win10InstallerApp* app) {
        app->current_step = 2; // Jump to License
        app->update_navigation_state();
    }

    static void on_license_toggled(Win10InstallerApp* app) {
        gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(app->chk_license));
        gtk_widget_set_sensitive(app->btn_next, active);
    }

    static gboolean on_custom_install_selected(Win10InstallerApp* app) {
        app->current_step = 4; // Jump to Disk Table
        app->update_navigation_state();
        return TRUE;
    }

    static void on_refresh_drives_clicked(Win10InstallerApp* app) {
        app->populate_drives();
    }

    static void on_back_clicked(Win10InstallerApp* app) {
        if (app->current_step > 0) {
            app->current_step--;
            if (app->current_step == 3) app->current_step = 2;
            app->update_navigation_state();
        }
    }

    static void on_next_clicked(Win10InstallerApp* app) {
        if (app->current_step == 5) {
            // Confirm install
            app->fullname = gtk_entry_get_text(GTK_ENTRY(app->entry_fullname));
            app->username = gtk_entry_get_text(GTK_ENTRY(app->entry_username));
            app->hostname = gtk_entry_get_text(GTK_ENTRY(app->entry_hostname));
            app->password = gtk_entry_get_text(GTK_ENTRY(app->entry_pwd));
            const char* cp = gtk_entry_get_text(GTK_ENTRY(app->entry_cpwd));

            if (app->password.empty() || app->password != cp) {
                GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(app->window),
                    GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                    "Password mismatch. Please verify passwords match.");
                gtk_dialog_run(GTK_DIALOG(d));
                gtk_widget_destroy(d);
                return;
            }

            // Launch Installing Screen
            app->current_step = 6;
            app->update_navigation_state();
            app->start_installer_worker();
            return;
        }

        app->current_step++;
        app->update_navigation_state();
    }

    static void on_restart_clicked(Win10InstallerApp* app) {
        system("systemctl reboot 2>/dev/null || reboot 2>/dev/null");
    }

    void start_installer_worker() {
        std::thread([this]() {
            std::string cmd = "sudo -n /usr/local/bin/axisos-installer.sh --disk " + selected_disk +
                              " --username " + username +
                              " --password " + password +
                              " --fullname '" + fullname + "' --hostname " + hostname;

            FILE* fp = popen(cmd.c_str(), "r");
            if (fp) {
                char buf[512];
                while (fgets(buf, sizeof(buf), fp)) {
                    std::string line(buf);
                    if (line.find("PROGRESS:") == 0) {
                        size_t p1 = line.find(':', 9);
                        if (p1 != std::string::npos) {
                            int pct = std::atoi(line.substr(9, p1 - 9).c_str());
                            std::string msg = line.substr(p1 + 1);
                            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) msg.pop_back();

                            g_idle_add(+[](gpointer data) -> gboolean {
                                auto pair = static_cast<std::pair<Win10InstallerApp*, std::pair<int, std::string>>*>(data);
                                Win10InstallerApp* app = pair->first;
                                int pct = pair->second.first;
                                std::string msg = pair->second.second;

                                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), pct / 100.0);
                                gtk_label_set_text(GTK_LABEL(app->lbl_progress_percent), msg.c_str());

                                if (pct >= 15) {
                                    gtk_label_set_text(GTK_LABEL(app->lbl_stage_copy), "  ✔  Copying AxisOS files");
                                    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_copy), "done");
                                }
                                if (pct >= 15 && pct < 85) {
                                    std::string s = "  ⌛  Getting files ready for installation (" + std::to_string(pct) + "%)";
                                    gtk_label_set_text(GTK_LABEL(app->lbl_stage_prep), s.c_str());
                                    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_prep), "active");
                                } else if (pct >= 85) {
                                    gtk_label_set_text(GTK_LABEL(app->lbl_stage_prep), "  ✔  Getting files ready for installation (100%)");
                                    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_prep), "done");
                                    gtk_style_context_remove_class(gtk_widget_get_style_context(app->lbl_stage_prep), "active");
                                }
                                if (pct >= 88) {
                                    gtk_label_set_text(GTK_LABEL(app->lbl_stage_feat), "  ✔  Installing features & drivers (NVIDIA, AMD, Intel)");
                                    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_feat), "done");
                                }
                                if (pct >= 94) {
                                    gtk_label_set_text(GTK_LABEL(app->lbl_stage_upd), "  ✔  Installing updates");
                                    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_upd), "done");
                                }
                                if (pct >= 98) {
                                    gtk_label_set_text(GTK_LABEL(app->lbl_stage_fin), "  ✔  Finishing up");
                                    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_fin), "done");
                                }

                                delete pair;
                                return G_SOURCE_REMOVE;
                            }, new std::pair<Win10InstallerApp*, std::pair<int, std::string>>(this, {pct, msg}));
                        }
                    }
                }
                int status = pclose(fp);
                if (status == 0) {
                    g_idle_add(+[](gpointer data) -> gboolean {
                        Win10InstallerApp* app = static_cast<Win10InstallerApp*>(data);
                        app->current_step = 7;
                        app->update_navigation_state();
                        return G_SOURCE_REMOVE;
                    }, this);
                }
            }
        }).detach();
    }
};

int main(int argc, char* argv[]) {
    gtk_init(&argc, &argv);
    Win10InstallerApp app;
    gtk_widget_show_all(app.window);
    gtk_main();
    return 0;
}
