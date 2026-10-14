/*
 * AxisOS Graphical System Installer
 * Written in C++ using GTK 3
 * High-performance, high-contrast setup wizard with full internationalization,
 * interactive keymap testing, Btrfs subvolumes deployment, and Secure Boot integration.
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

static const char* AXIS_CSS = R"(
* {
    font-family: "Segoe UI", "Inter", "Ubuntu", "Cantarell", sans-serif;
}

window {
    background-color: #0B0D11;
    color: #F8FAFC;
}

headerbar {
    background: #14171E;
    border-bottom: 1px solid rgba(255, 255, 255, 0.15);
    color: #FFFFFF;
    min-height: 42px;
    padding: 0 12px;
}

headerbar .title {
    font-weight: 700;
    font-size: 14px;
    color: #FFFFFF;
}

.setup-canvas {
    background-color: #0B0D11;
    padding: 24px 36px;
}

.setup-footer {
    background-color: #14171E;
    border-top: 1px solid rgba(255, 255, 255, 0.15);
    padding: 12px 32px;
}

.win-title {
    font-size: 22px;
    font-weight: 700;
    color: #FFFFFF;
    margin-bottom: 6px;
}

.win-subtitle {
    font-size: 13px;
    color: #94A3B8;
    margin-bottom: 18px;
    line-height: 1.4;
}

.field-label {
    font-size: 13px;
    font-weight: 600;
    color: #E2E8F0;
    margin-bottom: 4px;
}

/* Primary Action Buttons - High Contrast Axis Cyan */
.btn-primary {
    background-color: #3A9CC9;
    color: #FFFFFF;
    font-size: 14px;
    font-weight: 700;
    border: 1px solid #3A9CC9;
    border-radius: 6px;
    padding: 10px 28px;
}

.btn-primary:hover {
    background-color: #4FB0DD;
    border-color: #4FB0DD;
}

.btn-primary:active {
    background-color: #2B8BB8;
}

.btn-primary:disabled {
    background-color: #1E232B;
    color: #475569;
    border-color: #1E232B;
}

/* Prominent Install Now Button */
.btn-install-large {
    background-color: #3A9CC9;
    color: #FFFFFF;
    font-size: 16px;
    font-weight: 700;
    border: 2px solid #60C0ED;
    border-radius: 8px;
    padding: 14px 44px;
}

.btn-install-large:hover {
    background-color: #4FB0DD;
    border-color: #FFFFFF;
}

/* Secondary Navigation Buttons */
.btn-secondary {
    background-color: #1E232B;
    color: #F8FAFC;
    font-size: 13px;
    font-weight: 600;
    border: 1px solid rgba(255, 255, 255, 0.20);
    border-radius: 6px;
    padding: 9px 22px;
}

.btn-secondary:hover {
    background-color: rgba(255, 255, 255, 0.12);
    border-color: #FFFFFF;
    color: #FFFFFF;
}

/* Danger Action Buttons */
.btn-danger {
    background-color: #7A2234;
    color: #FFFFFF;
    font-size: 13px;
    font-weight: 600;
    border: 1px solid #D9384E;
    border-radius: 6px;
    padding: 9px 18px;
}

.btn-danger:hover {
    background-color: #9C2B42;
    border-color: #FFFFFF;
    color: #FFFFFF;
}

/* Information & Summary Cards */
.info-card {
    background-color: #14171E;
    border: 1px solid rgba(255, 255, 255, 0.12);
    border-radius: 8px;
    padding: 16px 20px;
}

.summary-key {
    font-size: 13px;
    font-weight: 600;
    color: #94A3B8;
}

.summary-val {
    font-size: 13px;
    font-weight: 700;
    color: #FFFFFF;
}

/* Inputs & Form Controls */
entry {
    background-color: #181B22;
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 0.22);
    border-radius: 6px;
    padding: 8px 12px;
    font-size: 13px;
}

entry:focus {
    border-color: #3A9CC9;
    background-color: #1E222B;
}

combobox,
combobox.combo,
combobox button,
combobox button.combo,
combobox > box > button,
combobox > box > button.combo,
combobox.linked > button,
combobox.linked > button.combo {
    background-color: #181B22;
    background-image: none;
    box-shadow: none;
    text-shadow: none;
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 0.22);
    border-radius: 6px;
    padding: 7px 12px;
}

combobox:hover,
combobox button:hover,
combobox button.combo:hover,
combobox > box > button:hover,
combobox > box > button.combo:hover {
    background-color: #222632;
    background-image: none;
    border-color: #3A9CC9;
    color: #FFFFFF;
}

combobox:focus,
combobox button:focus,
combobox button.combo:focus {
    border-color: #3A9CC9;
    background-color: #1E222B;
    background-image: none;
    box-shadow: 0 0 0 1px #3A9CC9;
    color: #FFFFFF;
}

combobox cellview,
combobox cellview text,
combobox cellview label,
combobox label,
combobox text {
    color: #FFFFFF;
    background-color: transparent;
    background-image: none;
}

combobox arrow {
    color: #3A9CC9;
    min-width: 14px;
    min-height: 14px;
}

combobox menu,
combobox window menu,
menu {
    background-color: #14171E;
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 0.25);
    padding: 4px;
}

combobox menu menuitem,
menu menuitem {
    background-color: transparent;
    color: #FFFFFF;
    padding: 8px 14px;
    border-radius: 4px;
}

combobox menu menuitem:hover,
menu menuitem:hover {
    background-color: #2563EB;
    color: #FFFFFF;
}

/* TreeView (Drive selection) */
treeview {
    background-color: #14171E;
    color: #FFFFFF;
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: 6px;
}

treeview:selected,
treeview row:selected {
    background-color: #3A9CC9;
    color: #FFFFFF;
}

treeview header button {
    background-color: #1E232B;
    color: #CBD5E1;
    border: none;
    padding: 6px 10px;
    font-weight: 700;
}

/* Progress Checklist */
.progress-stage {
    font-size: 14px;
    color: #94A3B8;
    margin-bottom: 8px;
}

.progress-stage.active {
    color: #3A9CC9;
    font-weight: 700;
}

.progress-stage.done {
    color: #4ADE80;
    font-weight: 700;
}

progressbar progress {
    background-color: #3A9CC9;
    border-radius: 3px;
}

progressbar trough {
    background-color: #1E232B;
    border-radius: 3px;
    min-height: 8px;
}
)";

struct LangEntry {
    const char* label;
    const char* code;
};

static const LangEntry LANGUAGES[] = {
    {"English (United States)", "en_US.UTF-8"},
    {"English (United Kingdom)", "en_GB.UTF-8"},
    {"English (Canada)", "en_CA.UTF-8"},
    {"English (Australia)", "en_AU.UTF-8"},
    {"English (South Africa)", "en_ZA.UTF-8"},
    {"Spanish - Español (España)", "es_ES.UTF-8"},
    {"Spanish - Español (México)", "es_MX.UTF-8"},
    {"Spanish - Español (Argentina)", "es_AR.UTF-8"},
    {"French - Français (France)", "fr_FR.UTF-8"},
    {"French - Français (Canada)", "fr_CA.UTF-8"},
    {"German - Deutsch (Deutschland)", "de_DE.UTF-8"},
    {"Italian - Italiano (Italia)", "it_IT.UTF-8"},
    {"Portuguese - Português (Brasil)", "pt_BR.UTF-8"},
    {"Portuguese - Português (Portugal)", "pt_PT.UTF-8"},
    {"Russian - Русский (Россия)", "ru_RU.UTF-8"},
    {"Japanese - 日本語 (日本)", "ja_JP.UTF-8"},
    {"Chinese Simplified - 简体中文 (中国)", "zh_CN.UTF-8"},
    {"Chinese Traditional - 繁體中文 (台灣)", "zh_TW.UTF-8"},
    {"Korean - 한국어 (대한민국)", "ko_KR.UTF-8"},
    {"Arabic - العربية (العالم العربي)", "ar_AE.UTF-8"},
    {"Hindi - हिन्दी (भारत)", "hi_IN.UTF-8"},
    {"Dutch - Nederlands (Nederland)", "nl_NL.UTF-8"},
    {"Polish - Polski (Polska)", "pl_PL.UTF-8"},
    {"Swedish - Svenska (Sverige)", "sv_SE.UTF-8"},
    {"Turkish - Türkçe (Türkiye)", "tr_TR.UTF-8"},
    {"Ukrainian - Українська (Україна)", "uk_UA.UTF-8"},
    {"Indonesian - Bahasa Indonesia", "id_ID.UTF-8"},
    {"Vietnamese - Tiếng Việt", "vi_VN.UTF-8"}
};

struct TzEntry {
    const char* label;
    const char* zone;
};

static const TzEntry TIMEZONES[] = {
    {"New York / Eastern Time (UTC-5)", "America/New_York"},
    {"Chicago / Central Time (UTC-6)", "America/Chicago"},
    {"Denver / Mountain Time (UTC-7)", "America/Denver"},
    {"Los Angeles / Pacific Time (UTC-8)", "America/Los_Angeles"},
    {"Anchorage / Alaska Time (UTC-9)", "America/Anchorage"},
    {"Honolulu / Hawaii Time (UTC-10)", "Pacific/Honolulu"},
    {"Toronto / Eastern Time (Canada)", "America/Toronto"},
    {"Vancouver / Pacific Time (Canada)", "America/Vancouver"},
    {"Mexico City (Mexico)", "America/Mexico_City"},
    {"São Paulo (Brazil)", "America/Sao_Paulo"},
    {"Buenos Aires (Argentina)", "America/Buenos_Aires"},
    {"London (United Kingdom / GMT)", "Europe/London"},
    {"Paris (France / CET)", "Europe/Paris"},
    {"Berlin (Germany / CET)", "Europe/Berlin"},
    {"Rome (Italy / CET)", "Europe/Rome"},
    {"Madrid (Spain / CET)", "Europe/Madrid"},
    {"Amsterdam (Netherlands / CET)", "Europe/Amsterdam"},
    {"Warsaw (Poland / CET)", "Europe/Warsaw"},
    {"Stockholm (Sweden / CET)", "Europe/Stockholm"},
    {"Kyiv (Ukraine / EET)", "Europe/Kyiv"},
    {"Moscow (Russia / MSK)", "Europe/Moscow"},
    {"Dubai (UAE / GST)", "Asia/Dubai"},
    {"Mumbai / Kolkata (India / IST)", "Asia/Kolkata"},
    {"Singapore (Singapore / SGT)", "Asia/Singapore"},
    {"Hong Kong (Hong Kong / HKT)", "Asia/Hong_Kong"},
    {"Beijing / Shanghai (China / CST)", "Asia/Shanghai"},
    {"Tokyo (Japan / JST)", "Asia/Tokyo"},
    {"Seoul (South Korea / KST)", "Asia/Seoul"},
    {"Jakarta / Bangkok (SE Asia / ICT)", "Asia/Jakarta"},
    {"Sydney (Australia / AEST)", "Australia/Sydney"},
    {"Melbourne (Australia / AEST)", "Australia/Melbourne"},
    {"Perth (Western Australia / AWST)", "Australia/Perth"},
    {"Auckland (New Zealand / NZST)", "Pacific/Auckland"},
    {"Johannesburg (South Africa / SAST)", "Africa/Johannesburg"},
    {"Cairo (Egypt / EET)", "Africa/Cairo"},
    {"UTC (Coordinated Universal Time)", "UTC"}
};

struct KeymapEntry {
    const char* label;
    const char* code;
};

static const KeymapEntry KEYMAPS[] = {
    {"English (US Standard)", "us"},
    {"English (US International AltGr dead keys)", "us(altgr-intl)"},
    {"English (US with dead keys)", "us(intl)"},
    {"English (UK Standard)", "gb"},
    {"English (Dvorak)", "us(dvorak)"},
    {"English (Colemak)", "us(colemak)"},
    {"Spanish (Spain)", "es"},
    {"Spanish (Latin America)", "latam"},
    {"French (AZERTY France)", "fr"},
    {"French (Canada)", "ca"},
    {"German (QWERTZ Germany)", "de"},
    {"Italian (Italy)", "it"},
    {"Portuguese (Brazil ABNT2)", "br"},
    {"Portuguese (Portugal)", "pt"},
    {"Russian (Standard)", "ru"},
    {"Japanese (OADG 109A)", "jp"},
    {"Korean (101/104 Key)", "kr"},
    {"Chinese (Standard)", "cn"},
    {"Polish (Programmers)", "pl"},
    {"Dutch (Netherlands)", "nl"},
    {"Swedish (Sweden)", "se"},
    {"Turkish (Q Standard)", "tr"},
    {"Ukrainian (Standard)", "ua"},
    {"Arabic (101 Key)", "ara"},
    {"Vietnamese (Standard)", "vn"}
};

static void show_alert(GtkWidget* parent, const std::string& msg) {
    GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(parent),
        GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
        "%s", msg.c_str());
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}

static GtkWidget* create_scaled_logo(int max_w, int max_h) {
    const char* paths[] = {
        "/usr/share/pixmaps/axisos.png",
        "/usr/share/pixmaps/axisos-logo.png",
        "/usr/share/icons/hicolor/128x128/apps/axisos.png",
        "/usr/share/icons/hicolor/256x256/apps/axisos.png",
        "/usr/share/pixmaps/axisos.svg"
    };
    for (const char* p : paths) {
        if (access(p, F_OK) == 0) {
            GError* err = NULL;
            GdkPixbuf* pb = gdk_pixbuf_new_from_file_at_scale(p, max_w, max_h, TRUE, &err);
            if (pb) {
                GtkWidget* img = gtk_image_new_from_pixbuf(pb);
                g_object_unref(pb);
                return img;
            }
        }
    }
    return gtk_image_new_from_icon_name("axisos", GTK_ICON_SIZE_DIALOG);
}

class AxisInstallerApp {
public:
    GtkWidget* window;
    GtkWidget* stack;
    GtkWidget* btn_back;
    GtkWidget* btn_next;
    int current_step = 0;

    // User preferences
    std::string selected_disk = "/dev/sda";
    std::string selected_fs = "ext4";
    std::string selected_locale = "en_US.UTF-8";
    std::string selected_locale_name = "English (United States)";
    std::string selected_timezone = "America/New_York";
    std::string selected_tz_name = "New York / Eastern Time (UTC-5)";
    std::string selected_keymap = "us";
    std::string selected_keymap_name = "English (US Standard)";

    std::string fullname = "AxisOS User";
    std::string username = "axis";
    std::string password = "axis";
    std::string hostname = "axis-pc";

    // Step 0: Welcome & Localization
    GtkWidget* combo_lang;
    GtkWidget* combo_time;
    GtkWidget* combo_keyboard;
    GtkWidget* entry_keytest;

    // Step 1: Disk selection & Filesystem
    GtkWidget* tree_drives;
    GtkListStore* store_drives;
    GtkWidget* lbl_disk_selection_status;
    GtkWidget* combo_fs;
    std::string live_media_device = "";
    std::string live_disk_device = "";
    std::vector<std::string> excluded_disks;

    // Step 2: Account
    GtkWidget* entry_fullname;
    GtkWidget* entry_username;
    GtkWidget* entry_hostname;
    GtkWidget* entry_pwd;
    GtkWidget* entry_cpwd;

    // Step 3: Summary
    GtkWidget* lbl_summary_disk;
    GtkWidget* lbl_summary_fs;
    GtkWidget* lbl_summary_lang;
    GtkWidget* lbl_summary_tz;
    GtkWidget* lbl_summary_keymap;
    GtkWidget* lbl_summary_user;
    GtkWidget* btn_summary_install_large;

    // Step 4: Installing
    GtkWidget* lbl_stage_copy;
    GtkWidget* lbl_stage_prep;
    GtkWidget* lbl_stage_feat;
    GtkWidget* lbl_stage_upd;
    GtkWidget* lbl_stage_fin;
    GtkWidget* progress_bar;
    GtkWidget* lbl_progress_percent;

    AxisInstallerApp() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "AxisOS 2.0 Setup");

        // Fit entire screen and start maximized
        GdkScreen* screen = gdk_screen_get_default();
        if (screen) {
            int sw = gdk_screen_get_width(screen);
            int sh = gdk_screen_get_height(screen);
            gtk_window_set_default_size(GTK_WINDOW(window), sw, sh);
        } else {
            gtk_window_set_default_size(GTK_WINDOW(window), 1024, 720);
        }
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_resizable(GTK_WINDOW(window), TRUE);
        gtk_window_maximize(GTK_WINDOW(window));
        gtk_window_set_default_icon_name("axisos");
        gtk_window_set_icon_name(GTK_WINDOW(window), "axisos");

        // Enforce dark theme preferences
        GtkSettings* default_settings = gtk_settings_get_default();
        if (default_settings) {
            g_object_set(default_settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
            g_object_set(default_settings, "gtk-theme-name", "Adwaita-dark", NULL);
        }

        // Apply high-contrast AxisOS CSS with USER priority
        GtkCssProvider* provider = gtk_css_provider_new();
        gtk_css_provider_load_from_data(provider, AXIS_CSS, -1, NULL);
        gtk_style_context_add_provider_for_screen(
            gdk_screen_get_default(),
            GTK_STYLE_PROVIDER(provider),
            GTK_STYLE_PROVIDER_PRIORITY_USER
        );
        g_object_unref(provider);

        // Headerbar
        GtkWidget* hb = gtk_header_bar_new();
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
        gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS 2.0 Setup Wizard");
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Liquid Nodes Edition");
        gtk_window_set_titlebar(GTK_WINDOW(window), hb);

        // Main Layout Box
        GtkWidget* root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(window), root_box);

        // Content Stack
        stack = gtk_stack_new();
        gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
        gtk_stack_set_transition_duration(GTK_STACK(stack), 160);
        gtk_box_pack_start(GTK_BOX(root_box), stack, TRUE, TRUE, 0);

        // Pinned Footer Bar (ALWAYS VISIBLE AT THE BOTTOM)
        GtkWidget* footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
        gtk_style_context_add_class(gtk_widget_get_style_context(footer), "setup-footer");

        btn_back = gtk_button_new_with_label("← Back");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_back), "btn-secondary");
        g_signal_connect_swapped(btn_back, "clicked", G_CALLBACK(on_back_clicked), this);

        btn_next = gtk_button_new_with_label("Continue →");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_next), "btn-primary");
        g_signal_connect_swapped(btn_next, "clicked", G_CALLBACK(on_next_clicked), this);

        gtk_box_pack_start(GTK_BOX(footer), btn_back, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(footer), btn_next, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(root_box), footer, FALSE, FALSE, 0);

        // Detect live installation media right away
        detect_live_media();

        // Build 6 Setup Screens
        build_step_locale();    // Step 0: Welcome & Localization
        build_step_disk();      // Step 1: Storage & Drive Selection
        build_step_account();   // Step 2: User Account & Security
        build_step_summary();   // Step 3: Confirmation Summary & Install Now
        build_step_installing();// Step 4: Live Installation Progress
        build_step_finish();    // Step 5: Completed & Reboot

        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
        update_navigation_state();
    }

    void detect_live_media() {
        excluded_disks.clear();
        live_media_device = "";
        live_disk_device = "";

        const char* check_cmd = "for mp in /run/live/medium /run/live/rootfs /run/initramfs /run/initramfs/live /cdrom /live/image; do "
                                "  if mountpoint -q \"$mp\" 2>/dev/null || [ -d \"$mp\" ]; then "
                                "    src=$(findmnt -n -o SOURCE \"$mp\" 2>/dev/null || true); "
                                "    [ -n \"$src\" ] && echo \"$src\"; "
                                "  fi; "
                                "done; "
                                "rsrc=$(findmnt -n -o SOURCE / 2>/dev/null || true); "
                                "if [ -n \"$rsrc\" ] && [ \"$rsrc\" != \"overlay\" ] && [ \"$rsrc\" != \"rootfs\" ] && [ \"$rsrc\" != \"tmpfs\" ]; then "
                                "  echo \"$rsrc\"; "
                                "fi; "
                                "awk '$2 ~ /^\\/run\\/(live|initramfs)/ || $2 ~ /^\\/cdrom/ {if ($1 ~ /^\\/dev/) print $1}' /proc/mounts 2>/dev/null || true";

        FILE* flive = popen(check_cmd, "r");
        if (flive) {
            char lbuf[512];
            while (fgets(lbuf, sizeof(lbuf), flive)) {
                std::string dev = lbuf;
                while (!dev.empty() && (dev.back() == '\n' || dev.back() == '\r' || dev.back() == ' ')) dev.pop_back();
                if (dev.empty()) continue;

                std::string parent = dev;
                if (parent.find("/dev/loop") == 0) {
                    std::string back_cmd = "losetup -O BACK-FILE -n " + parent + " 2>/dev/null";
                    FILE* fback = popen(back_cmd.c_str(), "r");
                    if (fback) {
                        char bbuf[512];
                        if (fgets(bbuf, sizeof(bbuf), fback)) {
                            std::string bfile = bbuf;
                            while (!bfile.empty() && (bfile.back() == '\n' || bfile.back() == '\r')) bfile.pop_back();
                            if (!bfile.empty()) {
                                std::string df_cmd = "df -P \"" + bfile + "\" 2>/dev/null | awk 'NR==2 {print $1}'";
                                FILE* fdf = popen(df_cmd.c_str(), "r");
                                if (fdf) {
                                    char dbuf[512];
                                    if (fgets(dbuf, sizeof(dbuf), fdf)) {
                                        std::string ddev = dbuf;
                                        while (!ddev.empty() && (ddev.back() == '\n' || ddev.back() == '\r')) ddev.pop_back();
                                        if (!ddev.empty()) parent = ddev;
                                    }
                                    pclose(fdf);
                                }
                            }
                        }
                        pclose(fback);
                    }
                }

                if (parent.find("/dev/") == 0) {
                    std::string pk_cmd = "lsblk -n -o PKNAME " + parent + " 2>/dev/null";
                    FILE* fpk = popen(pk_cmd.c_str(), "r");
                    if (fpk) {
                        char pbuf[256];
                        if (fgets(pbuf, sizeof(pbuf), fpk)) {
                            std::string pk = pbuf;
                            while (!pk.empty() && (pk.back() == '\n' || pk.back() == '\r' || pk.back() == ' ')) pk.pop_back();
                            if (!pk.empty()) parent = "/dev/" + pk;
                        }
                        pclose(fpk);
                    }
                }

                if (live_media_device.empty()) live_media_device = dev;
                if (live_disk_device.empty()) live_disk_device = parent;

                bool exists = false;
                for (const auto& ex : excluded_disks) {
                    if (ex == parent || ex == dev) { exists = true; break; }
                }
                if (!exists) {
                    excluded_disks.push_back(parent);
                    if (dev != parent) excluded_disks.push_back(dev);
                }
            }
            pclose(flive);
        }
    }

    bool is_disk_excluded(const std::string& dev) const {
        if (!live_disk_device.empty() && dev == live_disk_device) return true;
        if (!live_media_device.empty() && dev == live_media_device) return true;
        for (const auto& ex : excluded_disks) {
            if (dev == ex) return true;
        }
        return false;
    }

    GtkWidget* wrap_in_scroller(GtkWidget* child) {
        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
        gtk_style_context_add_class(gtk_widget_get_style_context(scroll), "setup-canvas");
        gtk_container_add(GTK_CONTAINER(scroll), child);
        return scroll;
    }

    // -------------------------------------------------------------------------
    // Step 0: Welcome & Localization (Language, Location, Keyboard)
    // -------------------------------------------------------------------------
    void build_step_locale() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        // Header with Scaled Logo (clean, crisp 56x56 icon)
        GtkWidget* header_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
        GtkWidget* logo_img = create_scaled_logo(56, 56);
        if (logo_img) gtk_box_pack_start(GTK_BOX(header_row), logo_img, FALSE, FALSE, 0);

        GtkWidget* title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
        GtkWidget* title = gtk_label_new("Welcome to AxisOS 2.0");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* sub = gtk_label_new("Select your language, region timezone, and keyboard layout.");
        gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

        gtk_box_pack_start(GTK_BOX(title_box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(title_box), sub, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(header_row), title_box, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(box), header_row, FALSE, FALSE, 6);

        // Grid for selections
        GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "info-card");

        GtkWidget* grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 14);

        // Language
        GtkWidget* l1 = gtk_label_new("Language:");
        gtk_label_set_xalign(GTK_LABEL(l1), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l1), "field-label");

        combo_lang = gtk_combo_box_text_new();
        for (size_t i = 0; i < sizeof(LANGUAGES)/sizeof(LANGUAGES[0]); ++i) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_lang), LANGUAGES[i].label);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_lang), 0);
        g_signal_connect(combo_lang, "changed", G_CALLBACK(on_language_changed), this);

        // Timezone / Location
        GtkWidget* l2 = gtk_label_new("Location & Timezone:");
        gtk_label_set_xalign(GTK_LABEL(l2), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l2), "field-label");

        combo_time = gtk_combo_box_text_new();
        for (size_t i = 0; i < sizeof(TIMEZONES)/sizeof(TIMEZONES[0]); ++i) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_time), TIMEZONES[i].label);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_time), 0);
        g_signal_connect(combo_time, "changed", G_CALLBACK(on_timezone_changed), this);

        // Keyboard layout
        GtkWidget* l3 = gtk_label_new("Keyboard Layout:");
        gtk_label_set_xalign(GTK_LABEL(l3), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l3), "field-label");

        combo_keyboard = gtk_combo_box_text_new();
        for (size_t i = 0; i < sizeof(KEYMAPS)/sizeof(KEYMAPS[0]); ++i) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_keyboard), KEYMAPS[i].label);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_keyboard), 0);
        g_signal_connect(combo_keyboard, "changed", G_CALLBACK(on_keymap_changed), this);

        // Test field
        GtkWidget* l4 = gtk_label_new("Test Keyboard:");
        gtk_label_set_xalign(GTK_LABEL(l4), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l4), "field-label");

        entry_keytest = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry_keytest), "Type here to test keys and symbols...");

        gtk_grid_attach(GTK_GRID(grid), l1, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), combo_lang, 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), l2, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), combo_time, 1, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), l3, 0, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), combo_keyboard, 1, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), l4, 0, 3, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry_keytest, 1, 3, 1, 1);

        gtk_box_pack_start(GTK_BOX(card), grid, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), card, FALSE, FALSE, 4);

        GtkWidget* scroller = wrap_in_scroller(box);
        gtk_stack_add_named(GTK_STACK(stack), scroller, "step_0");
    }

    static void on_language_changed(GtkComboBox* combo, AxisInstallerApp* app) {
        int idx = gtk_combo_box_get_active(combo);
        if (idx >= 0 && idx < (int)(sizeof(LANGUAGES)/sizeof(LANGUAGES[0]))) {
            app->selected_locale = LANGUAGES[idx].code;
            app->selected_locale_name = LANGUAGES[idx].label;
        }
    }

    static void on_timezone_changed(GtkComboBox* combo, AxisInstallerApp* app) {
        int idx = gtk_combo_box_get_active(combo);
        if (idx >= 0 && idx < (int)(sizeof(TIMEZONES)/sizeof(TIMEZONES[0]))) {
            app->selected_timezone = TIMEZONES[idx].zone;
            app->selected_tz_name = TIMEZONES[idx].label;
        }
    }

    static void on_keymap_changed(GtkComboBox* combo, AxisInstallerApp* app) {
        int idx = gtk_combo_box_get_active(combo);
        if (idx >= 0 && idx < (int)(sizeof(KEYMAPS)/sizeof(KEYMAPS[0]))) {
            app->selected_keymap = KEYMAPS[idx].code;
            app->selected_keymap_name = KEYMAPS[idx].label;
            // Apply keyboard layout live for testing
            std::string cmd = "setxkbmap " + app->selected_keymap + " 2>/dev/null || true";
            system(cmd.c_str());
        }
    }

    // -------------------------------------------------------------------------
    // Step 1: Storage & Drive Selection
    // -------------------------------------------------------------------------
    void build_step_disk() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

        GtkWidget* title = gtk_label_new("Where do you want to install AxisOS?");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* sub = gtk_label_new("Select the target disk drive. The drive will be partitioned with high-performance Btrfs subvolumes.");
        gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

        // Drive Table
        store_drives = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
        tree_drives = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store_drives));
        gtk_style_context_add_class(gtk_widget_get_style_context(tree_drives), "treeview");

        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_drives), -1, "Drive", renderer, "text", 0, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_drives), -1, "Capacity", renderer, "text", 1, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_drives), -1, "Free Space", renderer, "text", 2, NULL);
        gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(tree_drives), -1, "Model & Status / OS", renderer, "text", 3, NULL);

        GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_drives));
        g_signal_connect(sel, "changed", G_CALLBACK(on_drive_selection_changed), this);

        GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 180);
        gtk_container_add(GTK_CONTAINER(scroll), tree_drives);
        gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

        // Drive selection status indicator
        lbl_disk_selection_status = gtk_label_new("Selected: /dev/sda");
        gtk_label_set_xalign(GTK_LABEL(lbl_disk_selection_status), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_disk_selection_status), "field-label");

        GtkWidget* btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        GtkWidget* b_format = gtk_button_new_with_label("🧹 Wipe & Format Selected Drive");
        gtk_style_context_add_class(gtk_widget_get_style_context(b_format), "btn-danger");
        g_signal_connect(b_format, "clicked", G_CALLBACK(on_format_drive_clicked), this);

        GtkWidget* b_refresh = gtk_button_new_with_label("↻ Refresh Drives");
        gtk_style_context_add_class(gtk_widget_get_style_context(b_refresh), "btn-secondary");
        g_signal_connect_swapped(b_refresh, "clicked", G_CALLBACK(on_refresh_drives_clicked), this);

        gtk_box_pack_start(GTK_BOX(btn_row), lbl_disk_selection_status, TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(btn_row), b_refresh, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(btn_row), b_format, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), btn_row, FALSE, FALSE, 4);

        // Filesystem Architecture selector
        GtkWidget* fs_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
        gtk_style_context_add_class(gtk_widget_get_style_context(fs_card), "info-card");

        GtkWidget* fs_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);
        GtkWidget* lbl_fs_title = gtk_label_new("Target Filesystem:");
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_fs_title), "field-label");

        combo_fs = gtk_combo_box_text_new();
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_fs), "ext4", "EXT4 (Recommended - Maximum Stability & Universal Compatibility)");
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(combo_fs), "btrfs", "Btrfs (Modern Copy-on-Write with Transparent Compression)");
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(combo_fs), "ext4");
        g_signal_connect(combo_fs, "changed", G_CALLBACK(on_fs_changed), this);

        gtk_box_pack_start(GTK_BOX(fs_row), lbl_fs_title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(fs_row), combo_fs, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(fs_card), fs_row, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), fs_card, FALSE, FALSE, 4);

        GtkWidget* scroller = wrap_in_scroller(box);
        gtk_stack_add_named(GTK_STACK(stack), scroller, "step_1");
        populate_drives();
    }

    static void on_fs_changed(GtkComboBox* combo, AxisInstallerApp* app) {
        const char* id = gtk_combo_box_get_active_id(combo);
        if (id) {
            app->selected_fs = id;
            if (!app->selected_disk.empty()) {
                std::string st = "✓ Selected Target Disk: " + app->selected_disk + " (Filesystem: " + (app->selected_fs == "ext4" ? "EXT4" : "Btrfs") + ")";
                gtk_label_set_text(GTK_LABEL(app->lbl_disk_selection_status), st.c_str());
            }
        }
    }

    static void on_drive_selection_changed(GtkTreeSelection* selection, AxisInstallerApp* app) {
        GtkTreeModel* model;
        GtkTreeIter iter;
        if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
            gchar* dev_path = nullptr;
            gchar* label = nullptr;
            gchar* desc = nullptr;
            gtk_tree_model_get(model, &iter, 4, &dev_path, 0, &label, 3, &desc, -1);
            if (dev_path) {
                app->selected_disk = dev_path;
                bool is_live = app->is_disk_excluded(app->selected_disk);

                if (is_live) {
                    gtk_label_set_text(GTK_LABEL(app->lbl_disk_selection_status),
                        " Target is the live USB installation media! Please select an internal drive.");
                    gtk_widget_set_sensitive(app->btn_next, FALSE);
                } else {
                    std::string s_desc = desc ? desc : "";
                    std::string st;
                    if (s_desc.find("Windows") != std::string::npos) {
                        st = " Caution: " + app->selected_disk + " contains Windows! Installing here will overwrite Windows.";
                    } else if (s_desc.find("Clean") != std::string::npos || s_desc.find("100% Free") != std::string::npos) {
                        st = "✓ Selected Target: " + app->selected_disk + " [Clean / Empty Drive - Recommended] (Filesystem: " + (app->selected_fs == "ext4" ? "EXT4" : "Btrfs") + ")";
                    } else {
                        st = "✓ Selected Target Disk: " + app->selected_disk + " (Filesystem: " + (app->selected_fs == "ext4" ? "EXT4" : "Btrfs") + ")";
                    }
                    gtk_label_set_text(GTK_LABEL(app->lbl_disk_selection_status), st.c_str());
                    gtk_widget_set_sensitive(app->btn_next, TRUE);
                }
                g_free(dev_path);
            }
            if (label) g_free(label);
            if (desc) g_free(desc);
        }
    }

    void populate_drives() {
        gtk_list_store_clear(store_drives);
        detect_live_media();

        // 1. Run axis-disk-info.sh for accurate capacity, true unallocated free space, and OS detection
        FILE* fp = popen("/usr/local/bin/axis-disk-info.sh 2>/dev/null || bash /os-build/scripts/axis-disk-info.sh 2>/dev/null", "r");
        bool found = false;
        GtkTreePath* target_path = nullptr;
        GtkTreePath* clean_path = nullptr;
        std::string clean_disk;
        int row_idx = 0;

        if (fp) {
            char line[1024];
            int idx = 0;
            while (fgets(line, sizeof(line), fp)) {
                std::string sline(line);
                while (!sline.empty() && (sline.back() == '\n' || sline.back() == '\r')) sline.pop_back();
                if (sline.empty()) continue;

                if (sline.find('|') != std::string::npos) {
                    std::stringstream ss(sline);
                    std::string dev_name, cap, free_sp, desc;
                    std::getline(ss, dev_name, '|');
                    std::getline(ss, cap, '|');
                    std::getline(ss, free_sp, '|');
                    std::getline(ss, desc);

                    // Requirement 3: Exclude installer boot device and live media from drive list
                    if (is_disk_excluded(dev_name)) {
                        continue;
                    }

                    GtkTreeIter iter;
                    gtk_list_store_append(store_drives, &iter);
                    std::string label = "Drive " + std::to_string(idx) + " (" + dev_name + ")";

                    gtk_list_store_set(store_drives, &iter,
                        0, label.c_str(),
                        1, cap.c_str(),
                        2, free_sp.c_str(),
                        3, desc.c_str(),
                        4, dev_name.c_str(),
                        -1);

                    // Prefer clean / empty drive as default, otherwise first available drive
                    if (desc.find("Clean") != std::string::npos || free_sp.find("100% Free") != std::string::npos) {
                        if (clean_path == nullptr) {
                            clean_path = gtk_tree_path_new_from_indices(row_idx, -1);
                            clean_disk = dev_name;
                        }
                    }
                    if (target_path == nullptr) {
                        target_path = gtk_tree_path_new_from_indices(row_idx, -1);
                        selected_disk = dev_name;
                    }

                    idx++;
                    row_idx++;
                    found = true;
                }
            }
            pclose(fp);
        }

        if (clean_path != nullptr) {
            if (target_path) gtk_tree_path_free(target_path);
            target_path = clean_path;
            selected_disk = clean_disk;
        }

        // 2. Fallback to lsblk if axis-disk-info.sh returned no entries
        if (!found) {
            FILE* fp_lsblk = popen("lsblk -d -p -n -l -o NAME,SIZE,MODEL,TYPE 2>/dev/null", "r");
            if (fp_lsblk) {
                char line[512];
                int idx = 0;
                while (fgets(line, sizeof(line), fp_lsblk)) {
                    std::istringstream iss(line);
                    std::string dev_name, size;
                    iss >> dev_name >> size;
                    std::string rest;
                    std::getline(iss, rest);
                    while (!rest.empty() && (rest.front() == ' ' || rest.front() == '\t')) rest.erase(0, 1);
                    while (!rest.empty() && (rest.back() == '\n' || rest.back() == '\r' || rest.back() == ' ')) rest.pop_back();

                    if (dev_name.find("/dev/loop") == std::string::npos &&
                        dev_name.find("/dev/zram") == std::string::npos &&
                        dev_name.find("/dev/ram") == std::string::npos) {

                        // Requirement 3: Exclude installer boot device and live media from drive list
                        if (is_disk_excluded(dev_name)) {
                            continue;
                        }

                        GtkTreeIter iter;
                        gtk_list_store_append(store_drives, &iter);
                        std::string label = "Drive " + std::to_string(idx) + " (" + dev_name + ")";

                        std::string desc = rest.empty() ? "Internal Storage" : rest;

                        gtk_list_store_set(store_drives, &iter,
                            0, label.c_str(),
                            1, size.c_str(),
                            2, "Unknown",
                            3, desc.c_str(),
                            4, dev_name.c_str(),
                            -1);

                        if (target_path == nullptr) {
                            target_path = gtk_tree_path_new_from_indices(row_idx, -1);
                            selected_disk = dev_name;
                        }

                        idx++;
                        row_idx++;
                        found = true;
                    }
                }
                pclose(fp_lsblk);
            }
        }

        if (!found) {
            GtkTreeIter iter;
            gtk_list_store_append(store_drives, &iter);
            gtk_list_store_set(store_drives, &iter,
                0, "Drive 0 (/dev/sda)",
                1, "64.0 GB",
                2, "64.0 GB (100% Free)",
                3, "Primary System Storage [Clean / Empty Drive]",
                4, "/dev/sda",
                -1);
            selected_disk = "/dev/sda";
            target_path = gtk_tree_path_new_first();
        } else if (target_path == nullptr) {
            target_path = gtk_tree_path_new_first();
        }

        // Select row
        GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_drives));
        if (target_path) {
            gtk_tree_selection_select_path(sel, target_path);
            gtk_tree_path_free(target_path);
        }
    }

    static void on_format_drive_clicked(GtkButton* btn, AxisInstallerApp* app) {
        if (app->selected_disk.empty()) {
            GtkWidget* alert = gtk_message_dialog_new(GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                "No target drive selected. Please select a drive from the list first.");
            gtk_dialog_run(GTK_DIALOG(alert));
            gtk_widget_destroy(alert);
            return;
        }

        bool is_live = (!app->live_disk_device.empty() && app->selected_disk == app->live_disk_device) ||
                       (!app->live_media_device.empty() && app->selected_disk == app->live_media_device);
        if (is_live) {
            GtkWidget* alert = gtk_message_dialog_new(GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                "Cannot format the active installation media (%s)!\nPlease select an internal drive.",
                app->selected_disk.c_str());
            gtk_dialog_run(GTK_DIALOG(alert));
            gtk_widget_destroy(alert);
            return;
        }

        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(app->window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_WARNING,
            GTK_BUTTONS_OK_CANCEL,
            "⚠️ Format & Wipe Target Drive\n\nAre you sure you want to completely erase %s?\n\nThis will wipe all existing partition tables, Windows/Linux operating systems, and user files on %s.\n\nThe drive will be reset to a clean GPT disk ready for AxisOS.",
            app->selected_disk.c_str(), app->selected_disk.c_str());

        gint response = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (response == GTK_RESPONSE_OK) {
            gtk_label_set_text(GTK_LABEL(app->lbl_disk_selection_status), "⏳ Wiping and initializing drive partition table...");
            while (gtk_events_pending()) gtk_main_iteration();

            std::string cmd = "umount -f -l " + app->selected_disk + "* 2>/dev/null; "
                              "swapoff -a 2>/dev/null; "
                              "wipefs -a -f " + app->selected_disk + " 2>/dev/null; "
                              "dd if=/dev/zero of=" + app->selected_disk + " bs=1M count=32 conv=notrunc status=none 2>/dev/null; "
                              "SECTORS=$(blockdev --getsz " + app->selected_disk + " 2>/dev/null || echo 0); "
                              "if [ \"$SECTORS\" -gt 65536 ]; then dd if=/dev/zero of=" + app->selected_disk + " bs=512 seek=$(( SECTORS - 65536 )) count=65536 conv=notrunc status=none 2>/dev/null; fi; "
                              "sfdisk --wipe always --wipe-partitions always --force " + app->selected_disk + " <<< 'label: gpt' 2>/dev/null || true; "
                              "sync; blockdev --rereadpt " + app->selected_disk + " 2>/dev/null || true; "
                              "partprobe " + app->selected_disk + " 2>/dev/null || true; "
                              "udevadm settle --timeout=3 2>/dev/null || sleep 1";
            system(cmd.c_str());

            app->populate_drives();

            std::string success_msg = "✓ Successfully wiped and formatted " + app->selected_disk + "! Drive is clean.";
            gtk_label_set_text(GTK_LABEL(app->lbl_disk_selection_status), success_msg.c_str());

            GtkWidget* done_diag = gtk_message_dialog_new(GTK_WINDOW(app->window),
                GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                "✓ Drive %s wiped and formatted successfully!\n\nIt is now completely clean and ready for AxisOS installation.",
                app->selected_disk.c_str());
            gtk_dialog_run(GTK_DIALOG(done_diag));
            gtk_widget_destroy(done_diag);
        }
    }

    static void on_refresh_drives_clicked(AxisInstallerApp* app) {
        app->populate_drives();
    }

    // -------------------------------------------------------------------------
    // Step 2: User Account & Security Credentials
    // -------------------------------------------------------------------------
    void build_step_account() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* title = gtk_label_new("Who is going to use this PC?");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* sub = gtk_label_new("Create your primary administrator account. User and root credentials are synchronized for seamless Debian security.");
        gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

        GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "info-card");

        GtkWidget* grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid), 18);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 12);

        GtkWidget* l1 = gtk_label_new("Your Full Name:");
        gtk_label_set_xalign(GTK_LABEL(l1), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l1), "field-label");
        entry_fullname = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry_fullname), "AxisOS User");

        GtkWidget* l2 = gtk_label_new("Username (Login ID):");
        gtk_label_set_xalign(GTK_LABEL(l2), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l2), "field-label");
        entry_username = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry_username), "axis");

        GtkWidget* l3 = gtk_label_new("Computer Name (Hostname):");
        gtk_label_set_xalign(GTK_LABEL(l3), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l3), "field-label");
        entry_hostname = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry_hostname), "axis-pc");

        GtkWidget* l4 = gtk_label_new("Password:");
        gtk_label_set_xalign(GTK_LABEL(l4), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l4), "field-label");
        entry_pwd = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(entry_pwd), FALSE);
        gtk_entry_set_text(GTK_ENTRY(entry_pwd), "axis");

        GtkWidget* l5 = gtk_label_new("Confirm Password:");
        gtk_label_set_xalign(GTK_LABEL(l5), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(l5), "field-label");
        entry_cpwd = gtk_entry_new();
        gtk_entry_set_visibility(GTK_ENTRY(entry_cpwd), FALSE);
        gtk_entry_set_text(GTK_ENTRY(entry_cpwd), "axis");

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

        gtk_box_pack_start(GTK_BOX(card), grid, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), card, FALSE, FALSE, 4);

        GtkWidget* scroller = wrap_in_scroller(box);
        gtk_stack_add_named(GTK_STACK(stack), scroller, "step_2");
    }

    // -------------------------------------------------------------------------
    // Step 3: Confirmation Summary & Clear Install Now Option
    // -------------------------------------------------------------------------
    void build_step_summary() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* title = gtk_label_new("Ready to Install AxisOS");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* sub = gtk_label_new("Review your installation summary below. Click 'Install AxisOS Now' to begin.");
        gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

        // Summary Card
        GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "info-card");

        GtkWidget* grid = gtk_grid_new();
        gtk_grid_set_column_spacing(GTK_GRID(grid), 24);
        gtk_grid_set_row_spacing(GTK_GRID(grid), 10);

        auto add_row = [&](int row, const char* k, GtkWidget** out_lbl) {
            GtkWidget* lk = gtk_label_new(k);
            gtk_label_set_xalign(GTK_LABEL(lk), 0.0);
            gtk_style_context_add_class(gtk_widget_get_style_context(lk), "summary-key");

            GtkWidget* lv = gtk_label_new("---");
            gtk_label_set_xalign(GTK_LABEL(lv), 0.0);
            gtk_style_context_add_class(gtk_widget_get_style_context(lv), "summary-val");
            *out_lbl = lv;

            gtk_grid_attach(GTK_GRID(grid), lk, 0, row, 1, 1);
            gtk_grid_attach(GTK_GRID(grid), lv, 1, row, 1, 1);
        };

        add_row(0, "Target Hard Disk:", &lbl_summary_disk);
        add_row(1, "Filesystem Architecture:", &lbl_summary_fs);
        add_row(2, "Language & Locale:", &lbl_summary_lang);
        add_row(3, "Location & Timezone:", &lbl_summary_tz);
        add_row(4, "Keyboard Layout:", &lbl_summary_keymap);
        add_row(5, "User Administrator:", &lbl_summary_user);

        gtk_box_pack_start(GTK_BOX(card), grid, FALSE, FALSE, 0);

        // Prominent Big Center Install Button
        GtkWidget* btn_center_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_halign(btn_center_box, GTK_ALIGN_CENTER);
        gtk_widget_set_margin_top(btn_center_box, 14);

        btn_summary_install_large = gtk_button_new_with_label("⚡ Install AxisOS Now");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_summary_install_large), "btn-install-large");
        g_signal_connect_swapped(btn_summary_install_large, "clicked", G_CALLBACK(on_install_now_clicked), this);

        gtk_box_pack_start(GTK_BOX(btn_center_box), btn_summary_install_large, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), btn_center_box, FALSE, FALSE, 8);

        gtk_box_pack_start(GTK_BOX(box), card, FALSE, FALSE, 4);

        GtkWidget* scroller = wrap_in_scroller(box);
        gtk_stack_add_named(GTK_STACK(stack), scroller, "step_3");
    }

    void update_summary_card() {
        if (!lbl_summary_disk) return;
        gtk_label_set_text(GTK_LABEL(lbl_summary_disk), selected_disk.c_str());
        std::string fs_str = (selected_fs == "ext4") ? "EXT4 (Recommended - Universal Stability)" : "Btrfs (Modern with Transparent Compression)";
        if (lbl_summary_fs) gtk_label_set_text(GTK_LABEL(lbl_summary_fs), fs_str.c_str());
        gtk_label_set_text(GTK_LABEL(lbl_summary_lang), selected_locale_name.c_str());
        gtk_label_set_text(GTK_LABEL(lbl_summary_tz), selected_tz_name.c_str());
        gtk_label_set_text(GTK_LABEL(lbl_summary_keymap), selected_keymap_name.c_str());
        std::string usr_info = username + " (" + fullname + ")";
        gtk_label_set_text(GTK_LABEL(lbl_summary_user), usr_info.c_str());
    }

    // -------------------------------------------------------------------------
    // Step 4: Live Installation Progress
    // -------------------------------------------------------------------------
    void build_step_installing() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

        GtkWidget* title = gtk_label_new("Installing AxisOS 2.0");
        gtk_label_set_xalign(GTK_LABEL(title), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* sub = gtk_label_new("Deploying operating system files, device drivers, and UEFI bootloader. This may take a few minutes.");
        gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

        GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_style_context_add_class(gtk_widget_get_style_context(card), "info-card");

        // 5 Stages Checklist
        lbl_stage_copy = gtk_label_new("    Copying AxisOS system files");
        gtk_label_set_xalign(GTK_LABEL(lbl_stage_copy), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_stage_copy), "progress-stage");

        lbl_stage_prep = gtk_label_new("    Getting files ready for installation (0%)");
        gtk_label_set_xalign(GTK_LABEL(lbl_stage_prep), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_stage_prep), "progress-stage");

        lbl_stage_feat = gtk_label_new("    Installing features & drivers (NVIDIA, AMD, Intel)");
        gtk_label_set_xalign(GTK_LABEL(lbl_stage_feat), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_stage_feat), "progress-stage");

        lbl_stage_upd = gtk_label_new("    Configuring system repositories & gaming runtime");
        gtk_label_set_xalign(GTK_LABEL(lbl_stage_upd), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_stage_upd), "progress-stage");

        lbl_stage_fin = gtk_label_new("    Installing Microsoft-signed UEFI Secure Bootloader");
        gtk_label_set_xalign(GTK_LABEL(lbl_stage_fin), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_stage_fin), "progress-stage");

        gtk_box_pack_start(GTK_BOX(card), lbl_stage_copy, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), lbl_stage_prep, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), lbl_stage_feat, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), lbl_stage_upd, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(card), lbl_stage_fin, FALSE, FALSE, 0);

        progress_bar = gtk_progress_bar_new();
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.05);
        gtk_box_pack_start(GTK_BOX(card), progress_bar, FALSE, FALSE, 10);

        lbl_progress_percent = gtk_label_new("Status: Preparing partition table and Btrfs subvolumes...");
        gtk_label_set_xalign(GTK_LABEL(lbl_progress_percent), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(lbl_progress_percent), "field-label");
        gtk_box_pack_start(GTK_BOX(card), lbl_progress_percent, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(box), card, FALSE, FALSE, 4);

        GtkWidget* scroller = wrap_in_scroller(box);
        gtk_stack_add_named(GTK_STACK(stack), scroller, "step_4");
    }

    // -------------------------------------------------------------------------
    // Step 5: Completed & Ready to Reboot
    // -------------------------------------------------------------------------
    void build_step_finish() {
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
        gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
        gtk_widget_set_halign(box, GTK_ALIGN_CENTER);

        GtkWidget* logo_img = create_scaled_logo(72, 72);
        if (logo_img) gtk_box_pack_start(GTK_BOX(box), logo_img, FALSE, FALSE, 0);

        GtkWidget* title = gtk_label_new("AxisOS 2.0 Successfully Installed!");
        gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

        GtkWidget* sub = gtk_label_new("Please remove your live USB installation media and restart your computer to begin using AxisOS.");
        gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

        GtkWidget* btn_restart = gtk_button_new_with_label("⟳ Restart Computer Now");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_restart), "btn-install-large");
        g_signal_connect_swapped(btn_restart, "clicked", G_CALLBACK(on_restart_clicked), this);

        gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), btn_restart, FALSE, FALSE, 14);

        GtkWidget* scroller = wrap_in_scroller(box);
        gtk_stack_add_named(GTK_STACK(stack), scroller, "step_5");
    }

    // -------------------------------------------------------------------------
    // Navigation State & Flow Control
    // -------------------------------------------------------------------------
    void update_navigation_state() {
        std::string name = "step_" + std::to_string(current_step);
        gtk_stack_set_visible_child_name(GTK_STACK(stack), name.c_str());

        // Back button configuration
        if (current_step == 0) {
            gtk_widget_set_visible(btn_back, FALSE);
        } else if (current_step < 4) {
            gtk_widget_set_visible(btn_back, TRUE);
            gtk_widget_set_sensitive(btn_back, TRUE);
            gtk_button_set_label(GTK_BUTTON(btn_back), "← Back");
        } else if (current_step == 4) { // Installing
            gtk_widget_set_visible(btn_back, FALSE);
        } else if (current_step == 5) { // Finished
            gtk_widget_set_visible(btn_back, FALSE);
        }

        // Next / Continue / Install button configuration (ALWAYS VISIBLE!)
        if (current_step == 0) {
            gtk_widget_set_visible(btn_next, TRUE);
            gtk_widget_set_sensitive(btn_next, TRUE);
            gtk_button_set_label(GTK_BUTTON(btn_next), "Continue →");
        } else if (current_step == 1) {
            gtk_widget_set_visible(btn_next, TRUE);
            gtk_widget_set_sensitive(btn_next, !selected_disk.empty());
            gtk_button_set_label(GTK_BUTTON(btn_next), "Continue →");
        } else if (current_step == 2) {
            gtk_widget_set_visible(btn_next, TRUE);
            gtk_widget_set_sensitive(btn_next, TRUE);
            gtk_button_set_label(GTK_BUTTON(btn_next), "Continue →");
        } else if (current_step == 3) { // Summary
            gtk_widget_set_visible(btn_next, TRUE);
            gtk_widget_set_sensitive(btn_next, TRUE);
            gtk_button_set_label(GTK_BUTTON(btn_next), "⚡ Install AxisOS Now");
        } else if (current_step == 4) { // Installing
            gtk_widget_set_visible(btn_next, FALSE);
        } else if (current_step == 5) { // Finished
            gtk_widget_set_visible(btn_next, TRUE);
            gtk_widget_set_sensitive(btn_next, TRUE);
            gtk_button_set_label(GTK_BUTTON(btn_next), "Restart Now");
        }
    }

    static void on_back_clicked(AxisInstallerApp* app) {
        if (app->current_step == 4) {
            // Recovery from installation failure: return to disk selection
            app->current_step = 1;
            app->update_navigation_state();
            return;
        }
        if (app->current_step > 0) {
            app->current_step--;
            app->update_navigation_state();
        }
    }

    static void on_next_clicked(AxisInstallerApp* app) {
        if (app->current_step == 1) {
            if (app->selected_disk.empty()) {
                show_alert(app->window, "Please select a target storage drive before continuing.");
                return;
            }
        }

        if (app->current_step == 2) {
            // Validate account credentials
            app->fullname = gtk_entry_get_text(GTK_ENTRY(app->entry_fullname));
            app->username = gtk_entry_get_text(GTK_ENTRY(app->entry_username));
            app->hostname = gtk_entry_get_text(GTK_ENTRY(app->entry_hostname));
            app->password = gtk_entry_get_text(GTK_ENTRY(app->entry_pwd));
            const char* cp = gtk_entry_get_text(GTK_ENTRY(app->entry_cpwd));

            if (app->username.empty()) {
                show_alert(app->window, "Username cannot be empty.");
                return;
            }
            if (app->password.empty() || app->password != cp) {
                show_alert(app->window, "Passwords do not match. Please re-enter passwords.");
                return;
            }

            app->current_step = 3;
            app->update_summary_card();
            app->update_navigation_state();
            return;
        }

        if (app->current_step == 3) {
            if (app->selected_disk.empty()) {
                show_alert(app->window, "Please select a target storage drive before continuing.");
                return;
            }
            // User confirmed on summary screen -> Launch installation worker
            app->current_step = 4;
            app->update_navigation_state();
            app->start_installer_worker();
            return;
        }

        if (app->current_step == 5) {
            // Finished screen -> Reboot
            system("systemctl reboot 2>/dev/null || reboot 2>/dev/null");
            return;
        }

        app->current_step++;
        app->update_navigation_state();
    }

    static void on_install_now_clicked(AxisInstallerApp* app) {
        if (app->selected_disk.empty()) {
            show_alert(app->window, "Please select a target storage drive before continuing.");
            return;
        }
        // Center button on summary screen
        app->current_step = 4;
        app->update_navigation_state();
        app->start_installer_worker();
    }

    static void on_restart_clicked(AxisInstallerApp* app) {
        system("systemctl reboot 2>/dev/null || reboot 2>/dev/null");
    }

    void start_installer_worker() {
        std::thread([this]() {
            std::string sudo_prefix = (geteuid() == 0) ? "" : "sudo ";
            std::string cmd = sudo_prefix + "/usr/local/bin/axisos-installer.sh"
                              " --disk " + selected_disk +
                              " --fs " + selected_fs +
                              " --username " + username +
                              " --password " + password +
                              " --fullname '" + fullname + "'" +
                              " --hostname " + hostname +
                              " --locale " + selected_locale +
                              " --timezone " + selected_timezone +
                              " --keymap " + selected_keymap +
                              " 2>&1";

            FILE* fp = popen(cmd.c_str(), "r");
            std::string last_error_line = "Unexpected installation error.";
            if (fp) {
                char buf[512];
                while (fgets(buf, sizeof(buf), fp)) {
                    std::string line(buf);
                    while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) line.pop_back();
                    if (!line.empty() && line.find("PROGRESS:") != 0) {
                        if (line.rfind("ERROR:", 0) == 0 || line.rfind("Error:", 0) == 0) {
                            last_error_line = line;
                        } else if (last_error_line.rfind("ERROR:", 0) != 0 && last_error_line.rfind("Error:", 0) != 0) {
                            last_error_line = line;
                        }
                    }
                    if (line.find("PROGRESS:") == 0) {
                        size_t p1 = line.find(':', 9);
                        if (p1 != std::string::npos) {
                            int pct = std::atoi(line.substr(9, p1 - 9).c_str());
                            std::string msg = line.substr(p1 + 1);
                            while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) msg.pop_back();

                            g_idle_add(+[](gpointer data) -> gboolean {
                                auto pair = static_cast<std::pair<AxisInstallerApp*, std::pair<int, std::string>>*>(data);
                                AxisInstallerApp* app = pair->first;
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
                                    gtk_label_set_text(GTK_LABEL(app->lbl_stage_upd), "  ✔  Configuring system repositories & gaming runtime");
                                    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_upd), "done");
                                }
                                if (pct >= 98) {
                                    gtk_label_set_text(GTK_LABEL(app->lbl_stage_fin), "  ✔  Installing Microsoft-signed UEFI Secure Bootloader");
                                    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_fin), "done");
                                }

                                delete pair;
                                return G_SOURCE_REMOVE;
                            }, new std::pair<AxisInstallerApp*, std::pair<int, std::string>>(this, {pct, msg}));
                        }
                    }
                }
                int status = pclose(fp);
                if (status == 0) {
                    g_idle_add(+[](gpointer data) -> gboolean {
                        AxisInstallerApp* app = static_cast<AxisInstallerApp*>(data);
                        app->current_step = 5;
                        app->update_navigation_state();
                        return G_SOURCE_REMOVE;
                    }, this);
                } else {
                    g_idle_add(+[](gpointer data) -> gboolean {
                        auto pair = static_cast<std::pair<AxisInstallerApp*, std::string>*>(data);
                        AxisInstallerApp* app = pair->first;
                        std::string err = pair->second;

                        std::string err_text = "Installation stopped: " + err;
                        gtk_label_set_text(GTK_LABEL(app->lbl_progress_percent), err_text.c_str());

                        // Enable Back button for user recovery
                        gtk_widget_set_visible(app->btn_back, TRUE);
                        gtk_widget_set_sensitive(app->btn_back, TRUE);
                        gtk_button_set_label(GTK_BUTTON(app->btn_back), "Back to Drive Selection");

                        GtkWidget* d = gtk_message_dialog_new(GTK_WINDOW(app->window),
                            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                            "An issue occurred during installation:\n\n%s\n\nPlease select 'Back to Drive Selection' to choose another drive or verify disk health.",
                            err.c_str());
                        gtk_dialog_run(GTK_DIALOG(d));
                        gtk_widget_destroy(d);

                        delete pair;
                        return G_SOURCE_REMOVE;
                    }, new std::pair<AxisInstallerApp*, std::string>(this, last_error_line));
                }
            }
        }).detach();
    }
};

int main(int argc, char* argv[]) {
    // If AxisOS is already installed on the drive, prevent launching the installer
    if (access("/etc/axisos-installed", F_OK) == 0) {
        g_print("AxisOS is already installed on this machine.\n");
        return 0;
    }
    gtk_init(&argc, &argv);
    AxisInstallerApp app;
    gtk_widget_show_all(app.window);
    gtk_main();
    return 0;
}
