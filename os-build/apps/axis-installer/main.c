/*
 * AxisOS Graphical System Installer
 * Written in Pure C11 (-std=gnu11) using GTK 3
 * High-performance, high-contrast setup wizard with full internationalization,
 * interactive keymap testing, Btrfs subvolumes deployment, and Secure Boot integration.
 */

#define _GNU_SOURCE
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static const char *AXIS_CSS =
    "@define-color axis_base #141826;"
    "@define-color axis_surface #1F2231;"
    "@define-color axis_surface_raised #262A3D;"
    "@define-color axis_outline #2E3349;"
    "@define-color axis_accent #7D95FF;"
    "@define-color axis_accent_btn_text #0F1422;"
    "@define-color axis_text #E2E8F0;"
    "@define-color axis_text_muted #94A3B8;"
    "@define-color axis_danger #F87171;"
    "@define-color axis_success #34D399;"
    "button, togglebutton, entry, combobox button, headerbar {"
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
    "    background-image: none;"
    "    box-shadow: none;"
    "}"
    "scrolledwindow {"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 12px;"
    "    background-color: @axis_surface;"
    "}"
    "listbox {"
    "    background-color: @axis_surface;"
    "    border-radius: 12px;"
    "}"
    "listbox row {"
    "    background-color: transparent;"
    "    color: @axis_text;"
    "    padding: 10px 14px;"
    "    border-bottom: 1px solid @axis_outline;"
    "    border-radius: 8px;"
    "    transition: background-color 150ms ease;"
    "}"
    "listbox row:hover {"
    "    background-color: @axis_surface_raised;"
    "}"
    "listbox row:selected {"
    "    background-color: @axis_accent;"
    "    color: @axis_accent_btn_text;"
    "}"
    "entry {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 10px;"
    "    padding: 8px 12px;"
    "    background-image: none;"
    "    box-shadow: none;"
    "}"
    "entry:focus {"
    "    border-color: @axis_accent;"
    "}"
    "combobox {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_text;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 10px;"
    "}"
    "combobox button {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_text;"
    "    border: none;"
    "    background-image: none;"
    "    box-shadow: none;"
    "}"
    "progressbar trough {"
    "    background-color: @axis_outline;"
    "    border-radius: 9999px;"
    "    min-height: 8px;"
    "    border: none;"
    "}"
    "progressbar progress {"
    "    background-color: @axis_accent;"
    "    border-radius: 9999px;"
    "    min-height: 8px;"
    "    border: none;"
    "}"
    "textview, textview text {"
    "    background-color: @axis_surface;"
    "    color: @axis_text;"
    "    border: none;"
    "    font-family: 'IBM Plex Mono', monospace;"
    "}"
    "checkbutton, radiobutton {"
    "    color: @axis_text;"
    "}"
    "checkbutton check, radiobutton radio {"
    "    background-color: @axis_surface_raised;"
    "    border: 1px solid @axis_outline;"
    "    color: @axis_accent;"
    "}"
    "checkbutton check:checked, radiobutton radio:checked {"
    "    background-color: @axis_accent;"
    "    border-color: @axis_accent;"
    "    color: @axis_accent_btn_text;"
    "}"
    ".locale-pane {"
    "    background-color: @axis_surface;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 14px;"
    "    padding: 12px;"
    "}"
    ".selection-pill {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_accent;"
    "    border: 1px solid @axis_accent;"
    "    border-radius: 9999px;"
    "    padding: 6px 14px;"
    "    font-weight: 600;"
    "}"
    ".setup-canvas {"
    "    background-color: @axis_base;"
    "    padding: 24px 36px;"
    "}"
    ".setup-footer {"
    "    background-color: @axis_surface;"
    "    border-top: 1px solid @axis_outline;"
    "    padding: 12px 32px;"
    "}"
    ".win-title {"
    "    font-size: 22px;"
    "    font-weight: 700;"
    "    color: @axis_text;"
    "    margin-bottom: 6px;"
    "}"
    ".win-subtitle {"
    "    font-size: 13px;"
    "    color: @axis_text_muted;"
    "    margin-bottom: 18px;"
    "}"
    ".field-label {"
    "    font-size: 13px;"
    "    font-weight: 600;"
    "    color: @axis_text;"
    "    margin-bottom: 4px;"
    "}"
    "button.btn-primary, .btn-primary {"
    "    background: @axis_accent;"
    "    background-color: @axis_accent;"
    "    color: @axis_accent_btn_text;"
    "    font-size: 14px;"
    "    font-weight: 700;"
    "    border: 1px solid @axis_accent;"
    "    border-radius: 9999px;"
    "    padding: 10px 28px;"
    "}"
    "button.btn-primary:hover, .btn-primary:hover {"
    "    background-color: mix(@axis_accent, @axis_base, 0.12);"
    "}"
    "button.btn-install-large, .btn-install-large {"
    "    background: @axis_accent;"
    "    background-color: @axis_accent;"
    "    color: @axis_accent_btn_text;"
    "    font-size: 16px;"
    "    font-weight: 700;"
    "    border: 2px solid @axis_accent;"
    "    border-radius: 9999px;"
    "    padding: 14px 44px;"
    "}"
    "button.btn-install-large:hover, .btn-install-large:hover {"
    "    background-color: mix(@axis_accent, @axis_base, 0.12);"
    "}"
    "button.btn-secondary, .btn-secondary {"
    "    background: @axis_surface;"
    "    background-color: @axis_surface;"
    "    color: @axis_text;"
    "    font-size: 13px;"
    "    font-weight: 600;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 9999px;"
    "    padding: 9px 22px;"
    "}"
    "button.btn-secondary:hover, .btn-secondary:hover {"
    "    background-color: @axis_surface_raised;"
    "    color: @axis_accent;"
    "}"
    "button.btn-danger, .btn-danger {"
    "    background-color: @axis_danger;"
    "    color: @axis_accent_btn_text;"
    "    font-size: 13px;"
    "    font-weight: 600;"
    "    border: 1px solid @axis_danger;"
    "    border-radius: 9999px;"
    "    padding: 9px 18px;"
    "}"
    ".info-card, .card {"
    "    background-color: @axis_surface_raised;"
    "    border: 1px solid @axis_outline;"
    "    border-radius: 16px;"
    "    padding: 16px 20px;"
    "}"
    ".card-title, .summary-val {"
    "    font-size: 13px;"
    "    font-weight: 700;"
    "    color: @axis_text;"
    "}"
    ".card-subtitle, .summary-key {"
    "    font-size: 13px;"
    "    font-weight: 600;"
    "    color: @axis_text_muted;"
    "}"
    ".edition-card {"
    "    background-color: @axis_surface_raised;"
    "    border: 2px solid @axis_outline;"
    "    border-radius: 16px;"
    "    padding: 16px 20px;"
    "}"
    ".edition-card:hover {"
    "    background-color: alpha(@axis_accent, 0.08);"
    "    border-color: @axis_accent;"
    "}"
    ".edition-card.selected {"
    "    background-color: alpha(@axis_accent, 0.16);"
    "    border-color: @axis_accent;"
    "}"
    ".edition-title {"
    "    font-size: 15px;"
    "    font-weight: 700;"
    "    color: @axis_text;"
    "}"
    ".edition-badge {"
    "    font-size: 11px;"
    "    font-weight: 700;"
    "    padding: 2px 8px;"
    "    border-radius: 4px;"
    "    background-color: @axis_accent;"
    "    color: @axis_accent_btn_text;"
    "}"
    ".edition-desc {"
    "    font-size: 13px;"
    "    color: @axis_text_muted;"
    "}"
    ".edition-features {"
    "    font-size: 12px;"
    "    color: @axis_accent;"
    "    font-weight: 600;"
    "}";

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
    gtk_css_provider_load_from_data(extra_prov, AXIS_CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(
        screen,
        GTK_STYLE_PROVIDER(extra_prov),
        GTK_STYLE_PROVIDER_PRIORITY_USER
    );
    g_object_unref(extra_prov);
}

typedef struct {
    const char *label;
    const char *code;
} LangEntry;

static const LangEntry LANGUAGES[] = {
    {"English (United States)", "en_US.UTF-8"},
    {"English (United Kingdom)", "en_GB.UTF-8"},
    {"English (South Africa)", "en_ZA.UTF-8"},
    {"English (Canada)", "en_CA.UTF-8"},
    {"English (Australia)", "en_AU.UTF-8"},
    {"English (Ireland)", "en_IE.UTF-8"},
    {"English (New Zealand)", "en_NZ.UTF-8"},
    {"English (India)", "en_IN.UTF-8"},
    {"English (Singapore)", "en_SG.UTF-8"},
    {"Spanish - Español (España)", "es_ES.UTF-8"},
    {"Spanish - Español (México)", "es_MX.UTF-8"},
    {"Spanish - Español (Argentina)", "es_AR.UTF-8"},
    {"Spanish - Español (Colombia)", "es_CO.UTF-8"},
    {"Spanish - Español (Chile)", "es_CL.UTF-8"},
    {"Spanish - Español (Perú)", "es_PE.UTF-8"},
    {"Spanish - Español (Venezuela)", "es_VE.UTF-8"},
    {"Spanish - Español (Estados Unidos)", "es_US.UTF-8"},
    {"French - Français (France)", "fr_FR.UTF-8"},
    {"French - Français (Canada)", "fr_CA.UTF-8"},
    {"French - Français (Belgique)", "fr_BE.UTF-8"},
    {"French - Français (Suisse)", "fr_CH.UTF-8"},
    {"French - Français (Sénégal)", "fr_SN.UTF-8"},
    {"German - Deutsch (Deutschland)", "de_DE.UTF-8"},
    {"German - Deutsch (Österreich)", "de_AT.UTF-8"},
    {"German - Deutsch (Schweiz)", "de_CH.UTF-8"},
    {"Portuguese - Português (Brasil)", "pt_BR.UTF-8"},
    {"Portuguese - Português (Portugal)", "pt_PT.UTF-8"},
    {"Portuguese - Português (Angola)", "pt_AO.UTF-8"},
    {"Portuguese - Português (Moçambique)", "pt_MZ.UTF-8"},
    {"Italian - Italiano (Italia)", "it_IT.UTF-8"},
    {"Italian - Italiano (Svizzera)", "it_CH.UTF-8"},
    {"Russian - Русский (Россия)", "ru_RU.UTF-8"},
    {"Russian - Русский (Беларусь)", "ru_BY.UTF-8"},
    {"Russian - Русский (Казахстан)", "ru_KZ.UTF-8"},
    {"Ukrainian - Українська (Україна)", "uk_UA.UTF-8"},
    {"Polish - Polski (Polska)", "pl_PL.UTF-8"},
    {"Czech - Čeština (Česko)", "cs_CZ.UTF-8"},
    {"Slovak - Slovenčina (Slovensko)", "sk_SK.UTF-8"},
    {"Hungarian - Magyar (Magyarország)", "hu_HU.UTF-8"},
    {"Romanian - Română (România)", "ro_RO.UTF-8"},
    {"Bulgarian - Български (България)", "bg_BG.UTF-8"},
    {"Croatian - Hrvatski (Hrvatska)", "hr_HR.UTF-8"},
    {"Serbian - Српски (Србија)", "sr_RS.UTF-8"},
    {"Slovenian - Slovenščina (Slovenija)", "sl_SI.UTF-8"},
    {"Chinese Simplified - 简体中文 (中国)", "zh_CN.UTF-8"},
    {"Chinese Traditional - 繁體中文 (台灣)", "zh_TW.UTF-8"},
    {"Chinese Traditional - 繁體中文 (香港)", "zh_HK.UTF-8"},
    {"Japanese - 日本語 (日本)", "ja_JP.UTF-8"},
    {"Korean - 한국어 (대한민국)", "ko_KR.UTF-8"},
    {"Hindi - हिन्दी (भारत)", "hi_IN.UTF-8"},
    {"Bengali - বাংলা (বাংলাদেশ / ভারত)", "bn_BD.UTF-8"},
    {"Indonesian - Bahasa Indonesia", "id_ID.UTF-8"},
    {"Vietnamese - Tiếng Việt", "vi_VN.UTF-8"},
    {"Thai - ไทย (ประเทศไทย)", "th_TH.UTF-8"},
    {"Filipino - Tagalog (Pilipinas)", "tl_PH.UTF-8"},
    {"Malay - Bahasa Melayu (Malaysia)", "ms_MY.UTF-8"},
    {"Tamil - தமிழ் (இந்தியா)", "ta_IN.UTF-8"},
    {"Telugu - తెలుగు (భారతదేశం)", "te_IN.UTF-8"},
    {"Urdu - اردو (پاکستان)", "ur_PK.UTF-8"},
    {"Arabic - العربية (مصر)", "ar_EG.UTF-8"},
    {"Arabic - العربية (السعودية)", "ar_SA.UTF-8"},
    {"Arabic - العربية (الإمارات)", "ar_AE.UTF-8"},
    {"Arabic - العربية (المغرب)", "ar_MA.UTF-8"},
    {"Arabic - العربية (الجزائر)", "ar_DZ.UTF-8"},
    {"Arabic - العربية (العراق)", "ar_IQ.UTF-8"},
    {"Hebrew - עברית (ישראל)", "he_IL.UTF-8"},
    {"Persian - فارسی (ایران)", "fa_IR.UTF-8"},
    {"Turkish - Türkçe (Türkiye)", "tr_TR.UTF-8"},
    {"Dutch - Nederlands (Nederland)", "nl_NL.UTF-8"},
    {"Dutch - Nederlands (België)", "nl_BE.UTF-8"},
    {"Swedish - Svenska (Sverige)", "sv_SE.UTF-8"},
    {"Danish - Dansk (Danmark)", "da_DK.UTF-8"},
    {"Finnish - Suomi (Suomi)", "fi_FI.UTF-8"},
    {"Norwegian - Norsk Bokmål (Norge)", "nb_NO.UTF-8"},
    {"Greek - Ελληνικά (Ελλάδα)", "el_GR.UTF-8"},
    {"Afrikaans (Suid-Afrika)", "af_ZA.UTF-8"},
    {"isiZulu (iNingizimu Afrika)", "zu_ZA.UTF-8"},
    {"isiXhosa (uMzantsi Afrika)", "xh_ZA.UTF-8"},
    {"Sesotho (Afrika Borwa)", "st_ZA.UTF-8"},
    {"Setswana (Aforika Borwa)", "tn_ZA.UTF-8"},
    {"Sepedi / Northern Sotho (Afrika Borwa)", "nso_ZA.UTF-8"},
    {"Xitsonga (Afrika Dzonga)", "ts_ZA.UTF-8"},
    {"siSwati (iNingizimu Afrika)", "ss_ZA.UTF-8"},
    {"Tshivenḓa (Afurika Tshipembe)", "ve_ZA.UTF-8"},
    {"isiNdebele (iNingizimu Afrika)", "nr_ZA.UTF-8"},
    {"Swahili - Kiswahili (Kenya / Tanzania)", "sw_KE.UTF-8"},
    {"Amharic - አማርኛ (ኢትዮጵያ)", "am_ET.UTF-8"},
};

typedef struct {
    const char *label;
    const char *zone;
} TzEntry;

static const TzEntry TIMEZONES[] = {
    {"UTC / GMT (Coordinated Universal Time)", "UTC"},
    {"Abidjan (Ivory Coast / UTC+0)", "Africa/Abidjan"},
    {"Accra (Ghana / UTC+0)", "Africa/Accra"},
    {"Addis Ababa (Ethiopia / EAT UTC+3)", "Africa/Addis_Ababa"},
    {"Algiers (Algeria / CET UTC+1)", "Africa/Algiers"},
    {"Asmara (Eritrea / EAT UTC+3)", "Africa/Asmara"},
    {"Bamako (Mali / UTC+0)", "Africa/Bamako"},
    {"Bangui (Central African Rep. / WAT UTC+1)", "Africa/Bangui"},
    {"Banjul (Gambia / UTC+0)", "Africa/Banjul"},
    {"Bissau (Guinea-Bissau / UTC+0)", "Africa/Bissau"},
    {"Blantyre (Malawi / CAT UTC+2)", "Africa/Blantyre"},
    {"Brazzaville (Congo / WAT UTC+1)", "Africa/Brazzaville"},
    {"Bujumbura (Burundi / CAT UTC+2)", "Africa/Bujumbura"},
    {"Cairo (Egypt / EET UTC+2)", "Africa/Cairo"},
    {"Casablanca (Morocco / WET UTC+1)", "Africa/Casablanca"},
    {"Ceuta (Ceuta / CET UTC+1)", "Africa/Ceuta"},
    {"Conakry (Guinea / UTC+0)", "Africa/Conakry"},
    {"Dakar (Senegal / UTC+0)", "Africa/Dakar"},
    {"Dar es Salaam (Tanzania / EAT UTC+3)", "Africa/Dar_es_Salaam"},
    {"Djibouti (Djibouti / EAT UTC+3)", "Africa/Djibouti"},
    {"Douala (Cameroon / WAT UTC+1)", "Africa/Douala"},
    {"El Aaiun (Western Sahara / UTC+1)", "Africa/El_Aaiun"},
    {"Freetown (Sierra Leone / UTC+0)", "Africa/Freetown"},
    {"Gaborone (Botswana / CAT UTC+2)", "Africa/Gaborone"},
    {"Harare (Zimbabwe / CAT UTC+2)", "Africa/Harare"},
    {"Johannesburg / Pretoria (South Africa / SAST UTC+2)", "Africa/Johannesburg"},
    {"Juba (South Sudan / CAT UTC+2)", "Africa/Juba"},
    {"Kampala (Uganda / EAT UTC+3)", "Africa/Kampala"},
    {"Khartoum (Sudan / CAT UTC+2)", "Africa/Khartoum"},
    {"Kigali (Rwanda / CAT UTC+2)", "Africa/Kigali"},
    {"Kinshasa (DR Congo West / WAT UTC+1)", "Africa/Kinshasa"},
    {"Lagos (Nigeria / WAT UTC+1)", "Africa/Lagos"},
    {"Libreville (Gabon / WAT UTC+1)", "Africa/Libreville"},
    {"Lome (Togo / UTC+0)", "Africa/Lome"},
    {"Luanda (Angola / WAT UTC+1)", "Africa/Luanda"},
    {"Lubumbashi (DR Congo East / CAT UTC+2)", "Africa/Lubumbashi"},
    {"Lusaka (Zambia / CAT UTC+2)", "Africa/Lusaka"},
    {"Malabo (Equatorial Guinea / WAT UTC+1)", "Africa/Malabo"},
    {"Maputo (Mozambique / CAT UTC+2)", "Africa/Maputo"},
    {"Maseru (Lesotho / SAST UTC+2)", "Africa/Maseru"},
    {"Mbabane (Eswatini / SAST UTC+2)", "Africa/Mbabane"},
    {"Mogadishu (Somalia / EAT UTC+3)", "Africa/Mogadishu"},
    {"Monrovia (Liberia / UTC+0)", "Africa/Monrovia"},
    {"Nairobi (Kenya / EAT UTC+3)", "Africa/Nairobi"},
    {"Ndjamena (Chad / WAT UTC+1)", "Africa/Ndjamena"},
    {"Niamey (Niger / WAT UTC+1)", "Africa/Niamey"},
    {"Nouakchott (Mauritania / UTC+0)", "Africa/Nouakchott"},
    {"Ouagadougou (Burkina Faso / UTC+0)", "Africa/Ouagadougou"},
    {"Porto-Novo (Benin / WAT UTC+1)", "Africa/Porto-Novo"},
    {"Sao Tome (Sao Tome & Principe / UTC+0)", "Africa/Sao_Tome"},
    {"Tripoli (Libya / EET UTC+2)", "Africa/Tripoli"},
    {"Tunis (Tunisia / CET UTC+1)", "Africa/Tunis"},
    {"Windhoek (Namibia / CAT UTC+2)", "Africa/Windhoek"},
    {"New York / Eastern Time (USA / UTC-5)", "America/New_York"},
    {"Chicago / Central Time (USA / UTC-6)", "America/Chicago"},
    {"Denver / Mountain Time (USA / UTC-7)", "America/Denver"},
    {"Los Angeles / Pacific Time (USA / UTC-8)", "America/Los_Angeles"},
    {"Anchorage / Alaska Time (USA / UTC-9)", "America/Anchorage"},
    {"Honolulu / Hawaii Time (USA / UTC-10)", "Pacific/Honolulu"},
    {"Phoenix / Mountain Standard (USA / UTC-7)", "America/Phoenix"},
    {"Detroit / Eastern Time (USA / UTC-5)", "America/Detroit"},
    {"Indianapolis / Eastern (USA / UTC-5)", "America/Indiana/Indianapolis"},
    {"Toronto / Eastern Time (Canada / UTC-5)", "America/Toronto"},
    {"Montreal / Eastern Time (Canada / UTC-5)", "America/Montreal"},
    {"Vancouver / Pacific Time (Canada / UTC-8)", "America/Vancouver"},
    {"Edmonton / Mountain Time (Canada / UTC-7)", "America/Edmonton"},
    {"Winnipeg / Central Time (Canada / UTC-6)", "America/Winnipeg"},
    {"Halifax / Atlantic Time (Canada / UTC-4)", "America/Halifax"},
    {"St. John's / Newfoundland (Canada / UTC-3:30)", "America/St_Johns"},
    {"Mexico City / Central Time (Mexico / UTC-6)", "America/Mexico_City"},
    {"Cancun / Eastern Time (Mexico / UTC-5)", "America/Cancun"},
    {"Monterrey / Central Time (Mexico / UTC-6)", "America/Monterrey"},
    {"Tijuana / Pacific Time (Mexico / UTC-8)", "America/Tijuana"},
    {"Belize (Belize / UTC-6)", "America/Belize"},
    {"Costa Rica / San Jose (UTC-6)", "America/Costa_Rica"},
    {"El Salvador / San Salvador (UTC-6)", "America/El_Salvador"},
    {"Guatemala / Guatemala City (UTC-6)", "America/Guatemala"},
    {"Havana (Cuba / UTC-5)", "America/Havana"},
    {"Jamaica / Kingston (UTC-5)", "America/Jamaica"},
    {"Nicaragua / Managua (UTC-6)", "America/Managua"},
    {"Panama / Panama City (UTC-5)", "America/Panama"},
    {"Puerto Rico / San Juan (UTC-4)", "America/Puerto_Rico"},
    {"Santo Domingo (Dominican Rep. / UTC-4)", "America/Santo_Domingo"},
    {"Tegucigalpa (Honduras / UTC-6)", "America/Tegucigalpa"},
    {"Asuncion (Paraguay / UTC-4)", "America/Asuncion"},
    {"Bogota (Colombia / UTC-5)", "America/Bogota"},
    {"Buenos Aires (Argentina / UTC-3)", "America/Argentina/Buenos_Aires"},
    {"Caracas (Venezuela / UTC-4)", "America/Caracas"},
    {"Cayenne (French Guiana / UTC-3)", "America/Cayenne"},
    {"Georgetown (Guyana / UTC-4)", "America/Guyana"},
    {"Guayaquil (Ecuador / UTC-5)", "America/Guayaquil"},
    {"La Paz (Bolivia / UTC-4)", "America/La_Paz"},
    {"Lima (Peru / UTC-5)", "America/Lima"},
    {"Montevideo (Uruguay / UTC-3)", "America/Montevideo"},
    {"Paramaribo (Suriname / UTC-3)", "America/Paramaribo"},
    {"Santiago (Chile / UTC-4)", "America/Santiago"},
    {"Sao Paulo (Brazil / UTC-3)", "America/Sao_Paulo"},
    {"Rio de Janeiro (Brazil / UTC-3)", "America/Sao_Paulo"},
    {"Recife (Brazil / UTC-3)", "America/Recife"},
    {"Manaus (Brazil / UTC-4)", "America/Manaus"},
    {"Almaty / Astana (Kazakhstan / UTC+5)", "Asia/Almaty"},
    {"Amman (Jordan / UTC+3)", "Asia/Amman"},
    {"Baghdad (Iraq / UTC+3)", "Asia/Baghdad"},
    {"Baku (Azerbaijan / UTC+4)", "Asia/Baku"},
    {"Bangkok (Thailand / ICT UTC+7)", "Asia/Bangkok"},
    {"Beijing / Shanghai (China / CST UTC+8)", "Asia/Shanghai"},
    {"Beirut (Lebanon / UTC+2)", "Asia/Beirut"},
    {"Bishkek (Kyrgyzstan / UTC+6)", "Asia/Bishkek"},
    {"Colombo (Sri Lanka / IST UTC+5:30)", "Asia/Colombo"},
    {"Damascus (Syria / UTC+3)", "Asia/Damascus"},
    {"Dhaka (Bangladesh / BST UTC+6)", "Asia/Dhaka"},
    {"Dili (East Timor / UTC+9)", "Asia/Dili"},
    {"Dubai / Abu Dhabi (UAE / GST UTC+4)", "Asia/Dubai"},
    {"Dushanbe (Tajikistan / UTC+5)", "Asia/Dushanbe"},
    {"Ho Chi Minh / Hanoi (Vietnam / ICT UTC+7)", "Asia/Ho_Chi_Minh"},
    {"Hong Kong (Hong Kong / HKT UTC+8)", "Asia/Hong_Kong"},
    {"Jakarta (Indonesia / WIB UTC+7)", "Asia/Jakarta"},
    {"Jayapura (Indonesia / WIT UTC+9)", "Asia/Jayapura"},
    {"Jerusalem (Israel / IST UTC+2)", "Asia/Jerusalem"},
    {"Kabul (Afghanistan / UTC+4:30)", "Asia/Kabul"},
    {"Karachi / Islamabad (Pakistan / PKT UTC+5)", "Asia/Karachi"},
    {"Kathmandu (Nepal / NPT UTC+5:45)", "Asia/Kathmandu"},
    {"Kolkata / Mumbai / Delhi (India / IST UTC+5:30)", "Asia/Kolkata"},
    {"Kuala Lumpur (Malaysia / MYT UTC+8)", "Asia/Kuala_Lumpur"},
    {"Kuwait (Kuwait / UTC+3)", "Asia/Kuwait"},
    {"Macau (Macau / UTC+8)", "Asia/Macau"},
    {"Makassar (Indonesia / WITA UTC+8)", "Asia/Makassar"},
    {"Manila (Philippines / PHT UTC+8)", "Asia/Manila"},
    {"Muscat (Oman / GST UTC+4)", "Asia/Muscat"},
    {"Nicosia (Cyprus / EET UTC+2)", "Asia/Nicosia"},
    {"Phnom Penh (Cambodia / ICT UTC+7)", "Asia/Phnom_Penh"},
    {"Pyongyang (North Korea / KST UTC+9)", "Asia/Pyongyang"},
    {"Qatar / Doha (Qatar / UTC+3)", "Asia/Qatar"},
    {"Riyadh (Saudi Arabia / AST UTC+3)", "Asia/Riyadh"},
    {"Seoul (South Korea / KST UTC+9)", "Asia/Seoul"},
    {"Singapore (Singapore / SGT UTC+8)", "Asia/Singapore"},
    {"Taipei (Taiwan / CST UTC+8)", "Asia/Taipei"},
    {"Tashkent (Uzbekistan / UZT UTC+5)", "Asia/Tashkent"},
    {"Tbilisi (Georgia / UTC+4)", "Asia/Tbilisi"},
    {"Tehran (Iran / IRST UTC+3:30)", "Asia/Tehran"},
    {"Thimphu (Bhutan / UTC+6)", "Asia/Thimphu"},
    {"Tokyo (Japan / JST UTC+9)", "Asia/Tokyo"},
    {"Ulaanbaatar (Mongolia / UTC+8)", "Asia/Ulaanbaatar"},
    {"Vientiane (Laos / ICT UTC+7)", "Asia/Vientiane"},
    {"Yangon (Myanmar / MMT UTC+6:30)", "Asia/Yangon"},
    {"Yerevan (Armenia / UTC+4)", "Asia/Yerevan"},
    {"Amsterdam (Netherlands / CET UTC+1)", "Europe/Amsterdam"},
    {"Andorra (Andorra / CET UTC+1)", "Europe/Andorra"},
    {"Athens (Greece / EET UTC+2)", "Europe/Athens"},
    {"Belgrade (Serbia / CET UTC+1)", "Europe/Belgrade"},
    {"Berlin (Germany / CET UTC+1)", "Europe/Berlin"},
    {"Bratislava (Slovakia / CET UTC+1)", "Europe/Bratislava"},
    {"Brussels (Belgium / CET UTC+1)", "Europe/Brussels"},
    {"Bucharest (Romania / EET UTC+2)", "Europe/Bucharest"},
    {"Budapest (Hungary / CET UTC+1)", "Europe/Budapest"},
    {"Chisinau (Moldova / EET UTC+2)", "Europe/Chisinau"},
    {"Copenhagen (Denmark / CET UTC+1)", "Europe/Copenhagen"},
    {"Dublin (Ireland / GMT UTC+0)", "Europe/Dublin"},
    {"Gibraltar (Gibraltar / CET UTC+1)", "Europe/Gibraltar"},
    {"Helsinki (Finland / EET UTC+2)", "Europe/Helsinki"},
    {"Istanbul (Turkey / TRT UTC+3)", "Europe/Istanbul"},
    {"Kyiv (Ukraine / EET UTC+2)", "Europe/Kyiv"},
    {"Lisbon (Portugal / WET UTC+0)", "Europe/Lisbon"},
    {"Ljubljana (Slovenia / CET UTC+1)", "Europe/Ljubljana"},
    {"London (United Kingdom / GMT UTC+0)", "Europe/London"},
    {"Luxembourg (Luxembourg / CET UTC+1)", "Europe/Luxembourg"},
    {"Madrid (Spain / CET UTC+1)", "Europe/Madrid"},
    {"Malta (Malta / CET UTC+1)", "Europe/Malta"},
    {"Minsk (Belarus / MSK UTC+3)", "Europe/Minsk"},
    {"Monaco (Monaco / CET UTC+1)", "Europe/Monaco"},
    {"Moscow (Russia / MSK UTC+3)", "Europe/Moscow"},
    {"Oslo (Norway / CET UTC+1)", "Europe/Oslo"},
    {"Paris (France / CET UTC+1)", "Europe/Paris"},
    {"Prague (Czech Republic / CET UTC+1)", "Europe/Prague"},
    {"Reykjavik (Iceland / GMT UTC+0)", "Atlantic/Reykjavik"},
    {"Riga (Latvia / EET UTC+2)", "Europe/Riga"},
    {"Rome (Italy / CET UTC+1)", "Europe/Rome"},
    {"Samara (Russia / SAMT UTC+4)", "Europe/Samara"},
    {"Sarajevo (Bosnia / CET UTC+1)", "Europe/Sarajevo"},
    {"Skopje (North Macedonia / CET UTC+1)", "Europe/Skopje"},
    {"Sofia (Bulgaria / EET UTC+2)", "Europe/Sofia"},
    {"Stockholm (Sweden / CET UTC+1)", "Europe/Stockholm"},
    {"Tallinn (Estonia / EET UTC+2)", "Europe/Tallinn"},
    {"Tirane (Albania / CET UTC+1)", "Europe/Tirane"},
    {"Vienna (Austria / CET UTC+1)", "Europe/Vienna"},
    {"Vilnius (Lithuania / EET UTC+2)", "Europe/Vilnius"},
    {"Warsaw (Poland / CET UTC+1)", "Europe/Warsaw"},
    {"Zagreb (Croatia / CET UTC+1)", "Europe/Zagreb"},
    {"Zurich (Switzerland / CET UTC+1)", "Europe/Zurich"},
    {"Adelaide (South Australia / ACST UTC+9:30)", "Australia/Adelaide"},
    {"Auckland / Wellington (New Zealand / NZST UTC+12)", "Pacific/Auckland"},
    {"Brisbane (Queensland / AEST UTC+10)", "Australia/Brisbane"},
    {"Darwin (Northern Territory / ACST UTC+9:30)", "Australia/Darwin"},
    {"Fiji / Suva (Fiji / UTC+12)", "Pacific/Fiji"},
    {"Guam (Guam / ChST UTC+10)", "Pacific/Guam"},
    {"Hobart (Tasmania / AEST UTC+10)", "Australia/Hobart"},
    {"Melbourne (Victoria / AEST UTC+10)", "Australia/Melbourne"},
    {"Perth (Western Australia / AWST UTC+8)", "Australia/Perth"},
    {"Port Moresby (Papua New Guinea / UTC+10)", "Pacific/Port_Moresby"},
    {"Samoa / Apia (Samoa / UTC+13)", "Pacific/Apia"},
    {"Sydney / Canberra (Australia / AEST UTC+10)", "Australia/Sydney"},
    {"Tahiti / Papeete (French Polynesia / UTC-10)", "Pacific/Tahiti"},
    {"Azores (Portugal / UTC-1)", "Atlantic/Azores"},
    {"Bermuda (United Kingdom / UTC-4)", "Atlantic/Bermuda"},
    {"Canary Islands (Spain / WET UTC+0)", "Atlantic/Canary"},
    {"Cape Verde (Cape Verde / UTC-1)", "Atlantic/Cape_Verde"},
    {"Madeira (Portugal / WET UTC+0)", "Atlantic/Madeira"},
    {"Maldives (Maldives / UTC+5)", "Indian/Maldives"},
    {"Mauritius (Mauritius / UTC+4)", "Indian/Mauritius"},
    {"Reunion (France / UTC+4)", "Indian/Reunion"},
    {"Seychelles (Seychelles / UTC+4)", "Indian/Mahe"},
};

typedef struct {
    const char *label;
    const char *code;
} KeymapEntry;

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

typedef struct {
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *btn_back;
    GtkWidget *btn_next;
    GtkWidget *btn_try_os;
    int current_step;

    // User preferences
    char *selected_edition;
    char *selected_edition_name;
    char *selected_disk;
    char *selected_fs;
    char *selected_locale;
    char *selected_locale_name;
    char *selected_timezone;
    char *selected_tz_name;
    char *selected_keymap;
    char *selected_keymap_name;

    char *fullname;
    char *username;
    char *password;
    char *hostname;

    // Step 0: Welcome & Localization
    GtkWidget *combo_lang;
    GtkWidget *combo_time;
    GtkWidget *combo_keyboard;
    GtkWidget *entry_keytest;
    GtkWidget *search_lang;
    GtkWidget *search_tz;
    GtkWidget *tree_lang;
    GtkWidget *tree_tz;
    GtkListStore *store_lang;
    GtkListStore *store_tz;
    GtkTreeModelFilter *filter_lang;
    GtkTreeModelFilter *filter_tz;
    char *lang_search_query;
    char *tz_search_query;
    GtkWidget *lbl_locale_badge;

    // Step 1: Edition Selection
    GtkWidget *card_edition_base;
    GtkWidget *card_edition_ent;
    GtkWidget *card_edition_edu;
    GtkWidget *radio_edition_base;
    GtkWidget *radio_edition_ent;
    GtkWidget *radio_edition_edu;

    // Step 2: Disk selection & Filesystem
    GtkWidget *tree_drives;
    GtkListStore *store_drives;
    GtkWidget *lbl_disk_selection_status;
    GtkWidget *combo_fs;
    char *live_media_device;
    char *live_disk_device;
    GPtrArray *excluded_disks;

    // Step 3: Account
    GtkWidget *entry_fullname;
    GtkWidget *entry_username;
    GtkWidget *entry_hostname;
    GtkWidget *entry_pwd;
    GtkWidget *entry_cpwd;

    // Step 4: Summary
    GtkWidget *lbl_summary_edition;
    GtkWidget *lbl_summary_disk;
    GtkWidget *lbl_summary_fs;
    GtkWidget *lbl_summary_lang;
    GtkWidget *lbl_summary_tz;
    GtkWidget *lbl_summary_keymap;
    GtkWidget *lbl_summary_user;
    GtkWidget *btn_summary_install_large;

    // Step 5: Installing
    GtkWidget *lbl_stage_copy;
    GtkWidget *lbl_stage_prep;
    GtkWidget *lbl_stage_feat;
    GtkWidget *lbl_stage_upd;
    GtkWidget *lbl_stage_fin;
    GtkWidget *progress_bar;
    GtkWidget *lbl_progress_percent;
    GtkWidget *expander_log;
    GtkWidget *txt_install_log;
} AxisInstallerApp;

static void show_visible_error_dialog(GtkWindow *parent, const char *title, const char *msg, const char *log_tail) {
    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        title ? title : "Installation Issue",
        parent,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "_OK",
        GTK_RESPONSE_ACCEPT,
        NULL);

    if (parent) {
        gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ON_PARENT);
    } else {
        gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
    }
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_default_size(GTK_WINDOW(dialog), 620, 380);

    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 16);
    gtk_box_set_spacing(GTK_BOX(content_area), 12);

    GtkWidget *msg_label = gtk_label_new(msg);
    gtk_label_set_xalign(GTK_LABEL(msg_label), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(msg_label), TRUE);
    gtk_label_set_max_width_chars(GTK_LABEL(msg_label), 70);
    gtk_box_pack_start(GTK_BOX(content_area), msg_label, FALSE, FALSE, 0);

    // If log_tail is not provided, read the last 25 lines of /tmp/axisos-installer.log
    char log_buf[8192] = {0};
    if (log_tail && *log_tail) {
        snprintf(log_buf, sizeof(log_buf), "%s", log_tail);
    } else {
        FILE *f = fopen("/tmp/axisos-installer.log", "r");
        if (f) {
            char lines[30][512];
            int count = 0;
            char line[512];
            while (fgets(line, sizeof(line), f)) {
                snprintf(lines[count % 25], sizeof(lines[0]), "%s", line);
                count++;
            }
            fclose(f);
            int start = count > 25 ? count - 25 : 0;
            int total = count > 25 ? 25 : count;
            int offset = 0;
            for (int i = 0; i < total; i++) {
                int idx = (start + i) % 25;
                offset += snprintf(log_buf + offset, sizeof(log_buf) - offset, "%s", lines[idx]);
            }
        }
    }

    if (log_buf[0] != '\0') {
        GtkWidget *log_lbl = gtk_label_new("Recent Log Output (/tmp/axisos-installer.log):");
        gtk_label_set_xalign(GTK_LABEL(log_lbl), 0.0);
        gtk_style_context_add_class(gtk_widget_get_style_context(log_lbl), "title-4");
        gtk_box_pack_start(GTK_BOX(content_area), log_lbl, FALSE, FALSE, 0);

        GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_size_request(scrolled, -1, 180);
        gtk_widget_set_hexpand(scrolled, TRUE);
        gtk_widget_set_vexpand(scrolled, TRUE);

        GtkWidget *tv = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(tv), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(tv), FALSE);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(tv), TRUE);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(tv), GTK_WRAP_WORD_CHAR);
        GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));
        gtk_text_buffer_set_text(tb, log_buf, -1);

        gtk_container_add(GTK_CONTAINER(scrolled), tv);
        gtk_box_pack_start(GTK_BOX(content_area), scrolled, TRUE, TRUE, 0);
    }

    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void show_alert(GtkWidget *parent, const char *msg) {
    show_visible_error_dialog(parent ? GTK_WINDOW(parent) : NULL, "Notice", msg, NULL);
}

static GtkWidget *create_scaled_logo(int max_w, int max_h) {
    const char *paths[] = {
        "/usr/share/pixmaps/axisos.png",
        "/usr/share/pixmaps/axisos-logo.png",
        "/usr/share/icons/hicolor/128x128/apps/axisos.png",
        "/usr/share/icons/hicolor/256x256/apps/axisos.png",
        "/usr/share/pixmaps/axisos.svg"
    };
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
        if (access(paths[i], F_OK) == 0) {
            GError *err = NULL;
            GdkPixbuf *pb = gdk_pixbuf_new_from_file_at_scale(paths[i], max_w, max_h, TRUE, &err);
            if (pb) {
                GtkWidget *img = gtk_image_new_from_pixbuf(pb);
                g_object_unref(pb);
                return img;
            }
        }
    }
    return gtk_image_new_from_icon_name("axisos", GTK_ICON_SIZE_DIALOG);
}

static GtkWidget *wrap_in_scroller(GtkWidget *content) {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);
    GtkWidget *vp = gtk_viewport_new(NULL, NULL);
    gtk_widget_set_hexpand(vp, TRUE);
    gtk_widget_set_vexpand(vp, TRUE);
    gtk_widget_set_hexpand(content, TRUE);
    gtk_widget_set_vexpand(content, TRUE);
    gtk_container_add(GTK_CONTAINER(vp), content);
    gtk_container_add(GTK_CONTAINER(scroll), vp);
    gtk_style_context_add_class(gtk_widget_get_style_context(content), "setup-canvas");
    return scroll;
}

static gboolean is_disk_excluded(AxisInstallerApp *app, const char *disk) {
    if (!disk || !app->excluded_disks) return FALSE;
    for (guint i = 0; i < app->excluded_disks->len; i++) {
        const char *ex = (const char *)g_ptr_array_index(app->excluded_disks, i);
        if (g_strcmp0(ex, disk) == 0) return TRUE;
    }
    return FALSE;
}

static void detect_live_media(AxisInstallerApp *app) {
    if (!app->excluded_disks) {
        app->excluded_disks = g_ptr_array_new_with_free_func(g_free);
    }
    g_ptr_array_set_size(app->excluded_disks, 0);

    FILE *fp = popen("findmnt -n -o SOURCE /run/live/medium 2>/dev/null || findmnt -n -o SOURCE /run/archiso/bootmnt 2>/dev/null", "r");
    if (fp) {
        char buf[256];
        if (fgets(buf, sizeof(buf), fp)) {
            g_strstrip(buf);
            if (buf[0] != '\0') {
                g_free(app->live_media_device);
                app->live_media_device = g_strdup(buf);
                g_ptr_array_add(app->excluded_disks, g_strdup(buf));

                char *parent = g_strdup(buf);
                // e.g. /dev/sdb1 -> /dev/sdb
                size_t len = strlen(parent);
                while (len > 0 && g_ascii_isdigit(parent[len - 1])) {
                    parent[--len] = '\0';
                }
                if (g_str_has_suffix(parent, "p")) {
                    parent[strlen(parent) - 1] = '\0';
                }
                g_free(app->live_disk_device);
                app->live_disk_device = g_strdup(parent);
                g_ptr_array_add(app->excluded_disks, parent);
            }
        }
        pclose(fp);
    }
}

static void update_navigation_state(AxisInstallerApp *app);
static void start_installer_worker(AxisInstallerApp *app);

// Callbacks
static void update_locale_badge(AxisInstallerApp *app) {
    if (!app->lbl_locale_badge) return;
    char *badge_str = g_strdup_printf("Selected: %s  •  %s",
        app->selected_locale_name ? app->selected_locale_name : "English (United States)",
        app->selected_tz_name ? app->selected_tz_name : "America/New_York");
    gtk_label_set_text(GTK_LABEL(app->lbl_locale_badge), badge_str);
    g_free(badge_str);
}

static gboolean lang_filter_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    if (!app->lang_search_query || !app->lang_search_query[0]) {
        return TRUE;
    }
    char *label = NULL;
    char *code = NULL;
    gtk_tree_model_get(model, iter, 0, &label, 1, &code, -1);
    gboolean match = FALSE;
    if (label) {
        char *lower = g_utf8_strdown(label, -1);
        if (strstr(lower, app->lang_search_query)) match = TRUE;
        g_free(lower);
        g_free(label);
    }
    if (!match && code) {
        char *lower = g_utf8_strdown(code, -1);
        if (strstr(lower, app->lang_search_query)) match = TRUE;
        g_free(lower);
    }
    if (code) g_free(code);
    return match;
}

static gboolean tz_filter_func(GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    if (!app->tz_search_query || !app->tz_search_query[0]) {
        return TRUE;
    }
    char *label = NULL;
    char *zone = NULL;
    gtk_tree_model_get(model, iter, 0, &label, 1, &zone, -1);
    gboolean match = FALSE;
    if (label) {
        char *lower = g_utf8_strdown(label, -1);
        if (strstr(lower, app->tz_search_query)) match = TRUE;
        g_free(lower);
        g_free(label);
    }
    if (!match && zone) {
        char *lower = g_utf8_strdown(zone, -1);
        if (strstr(lower, app->tz_search_query)) match = TRUE;
        g_free(lower);
    }
    if (zone) g_free(zone);
    return match;
}

static void on_lang_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    g_free(app->lang_search_query);
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    app->lang_search_query = (text && *text) ? g_utf8_strdown(text, -1) : NULL;
    if (app->filter_lang) {
        gtk_tree_model_filter_refilter(app->filter_lang);
    }
}

static void on_tz_search_changed(GtkSearchEntry *entry, gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    g_free(app->tz_search_query);
    const char *text = gtk_entry_get_text(GTK_ENTRY(entry));
    app->tz_search_query = (text && *text) ? g_utf8_strdown(text, -1) : NULL;
    if (app->filter_tz) {
        gtk_tree_model_filter_refilter(app->filter_tz);
    }
}

static void on_lang_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        char *label = NULL;
        char *code = NULL;
        gtk_tree_model_get(model, &iter, 0, &label, 1, &code, -1);
        if (label && code) {
            g_free(app->selected_locale);
            g_free(app->selected_locale_name);
            app->selected_locale = g_strdup(code);
            app->selected_locale_name = g_strdup(label);
            update_locale_badge(app);
        }
        g_free(label);
        g_free(code);
    }
}

static void on_tz_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        char *label = NULL;
        char *zone = NULL;
        gtk_tree_model_get(model, &iter, 0, &label, 1, &zone, -1);
        if (label && zone) {
            g_free(app->selected_timezone);
            g_free(app->selected_tz_name);
            app->selected_timezone = g_strdup(zone);
            app->selected_tz_name = g_strdup(label);
            update_locale_badge(app);
        }
        g_free(label);
        g_free(zone);
    }
}

static void on_keymap_changed(GtkComboBox *combo, gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    gint idx = gtk_combo_box_get_active(combo);
    if (idx >= 0 && (size_t)idx < sizeof(KEYMAPS)/sizeof(KEYMAPS[0])) {
        g_free(app->selected_keymap);
        g_free(app->selected_keymap_name);
        app->selected_keymap = g_strdup(KEYMAPS[idx].code);
        app->selected_keymap_name = g_strdup(KEYMAPS[idx].label);
        char *cmd = g_strdup_printf("setxkbmap '%s'", app->selected_keymap);
        g_spawn_command_line_async(cmd, NULL);
        g_free(cmd);
    }
}

static void select_edition(AxisInstallerApp *app, const char *key) {
    g_free(app->selected_edition);
    g_free(app->selected_edition_name);
    app->selected_edition = g_strdup(key);

    if (g_strcmp0(key, "enterprise") == 0) {
        app->selected_edition_name = g_strdup("AxisOS Enterprise Workstation");
        if (app->card_edition_base) gtk_style_context_remove_class(gtk_widget_get_style_context(app->card_edition_base), "selected");
        if (app->card_edition_edu) gtk_style_context_remove_class(gtk_widget_get_style_context(app->card_edition_edu), "selected");
        if (app->card_edition_ent) gtk_style_context_add_class(gtk_widget_get_style_context(app->card_edition_ent), "selected");
        if (app->radio_edition_ent) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->radio_edition_ent), TRUE);
    } else if (g_strcmp0(key, "education") == 0) {
        app->selected_edition_name = g_strdup("AxisOS Work & School");
        if (app->card_edition_base) gtk_style_context_remove_class(gtk_widget_get_style_context(app->card_edition_base), "selected");
        if (app->card_edition_ent) gtk_style_context_remove_class(gtk_widget_get_style_context(app->card_edition_ent), "selected");
        if (app->card_edition_edu) gtk_style_context_add_class(gtk_widget_get_style_context(app->card_edition_edu), "selected");
        if (app->radio_edition_edu) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->radio_edition_edu), TRUE);
    } else {
        app->selected_edition_name = g_strdup("AxisOS Home & Gaming");
        if (app->card_edition_ent) gtk_style_context_remove_class(gtk_widget_get_style_context(app->card_edition_ent), "selected");
        if (app->card_edition_edu) gtk_style_context_remove_class(gtk_widget_get_style_context(app->card_edition_edu), "selected");
        if (app->card_edition_base) gtk_style_context_add_class(gtk_widget_get_style_context(app->card_edition_base), "selected");
        if (app->radio_edition_base) gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->radio_edition_base), TRUE);
    }
}

typedef struct {
    AxisInstallerApp *app;
    char *key;
} EditionCbData;

static void on_edition_radio_toggled(GtkToggleButton *tb, gpointer user_data) {
    if (gtk_toggle_button_get_active(tb)) {
        EditionCbData *data = (EditionCbData *)user_data;
        select_edition(data->app, data->key);
    }
}

static gboolean on_edition_card_clicked(GtkWidget *w, GdkEventButton *ev, gpointer user_data) {
    (void)w;
    (void)ev;
    EditionCbData *data = (EditionCbData *)user_data;
    select_edition(data->app, data->key);
    return TRUE;
}

static void on_fs_changed(GtkComboBox *combo, gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    const char *id = gtk_combo_box_get_active_id(combo);
    if (id) {
        g_free(app->selected_fs);
        app->selected_fs = g_strdup(id);
        if (app->selected_disk && app->lbl_disk_selection_status) {
            char *st = g_strdup_printf("✓ Selected Target Disk: %s (Filesystem: %s)",
                                       app->selected_disk,
                                       g_strcmp0(app->selected_fs, "ext4") == 0 ? "EXT4" : "Btrfs");
            gtk_label_set_text(GTK_LABEL(app->lbl_disk_selection_status), st);
            g_free(st);
        }
    }
}

static void on_drive_selection_changed(GtkTreeSelection *selection, gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    GtkTreeModel *model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        char *dev_path = NULL;
        char *label = NULL;
        char *desc = NULL;
        gtk_tree_model_get(model, &iter, 4, &dev_path, 0, &label, 3, &desc, -1);
        if (dev_path) {
            g_free(app->selected_disk);
            app->selected_disk = g_strdup(dev_path);
            gboolean is_live = is_disk_excluded(app, app->selected_disk);

            if (is_live) {
                gtk_label_set_text(GTK_LABEL(app->lbl_disk_selection_status),
                    " Target is the live USB installation media! Please select an internal drive.");
                gtk_widget_set_sensitive(app->btn_next, FALSE);
            } else {
                char *st;
                if (desc && strstr(desc, "Windows") != NULL) {
                    st = g_strdup_printf(" Caution: %s contains Windows! Installing here will overwrite Windows.", app->selected_disk);
                } else if (desc && (strstr(desc, "Clean") != NULL || strstr(desc, "100% Free") != NULL)) {
                    st = g_strdup_printf("✓ Selected Target: %s [Clean / Empty Drive - Recommended] (Filesystem: %s)",
                                         app->selected_disk, g_strcmp0(app->selected_fs, "ext4") == 0 ? "EXT4" : "Btrfs");
                } else {
                    st = g_strdup_printf("✓ Selected Target Disk: %s (Filesystem: %s)",
                                         app->selected_disk, g_strcmp0(app->selected_fs, "ext4") == 0 ? "EXT4" : "Btrfs");
                }
                gtk_label_set_text(GTK_LABEL(app->lbl_disk_selection_status), st);
                g_free(st);
                gtk_widget_set_sensitive(app->btn_next, TRUE);
            }
            g_free(dev_path);
        }
        g_free(label);
        g_free(desc);
    }
}

static void populate_drives(AxisInstallerApp *app) {
    gtk_list_store_clear(app->store_drives);
    detect_live_media(app);

    FILE *fp = popen("/usr/local/bin/axis-disk-info 2>/dev/null || ./os-build/scripts/axis-disk-info 2>/dev/null", "r");
    gboolean found = FALSE;
    GtkTreePath *target_path = NULL;
    GtkTreePath *clean_path = NULL;
    char *clean_disk = NULL;
    int row_idx = 0;

    if (fp) {
        char line[1024];
        int idx = 0;
        while (fgets(line, sizeof(line), fp)) {
            g_strstrip(line);
            if (line[0] == '\0') continue;

            if (strchr(line, '|') != NULL) {
                char **parts = g_strsplit(line, "|", 4);
                if (parts[0] && parts[1] && parts[2]) {
                    const char *dev_name = parts[0];
                    const char *cap = parts[1];
                    const char *free_sp = parts[2];
                    const char *desc = parts[3] ? parts[3] : "";

                    if (!is_disk_excluded(app, dev_name)) {
                        GtkTreeIter iter;
                        gtk_list_store_append(app->store_drives, &iter);
                        char *label = g_strdup_printf("Drive %d (%s)", idx, dev_name);

                        gtk_list_store_set(app->store_drives, &iter,
                            0, label, 1, cap, 2, free_sp, 3, desc, 4, dev_name, -1);
                        g_free(label);

                        if (strstr(desc, "Clean") != NULL || strstr(free_sp, "100% Free") != NULL) {
                            if (!clean_path) {
                                clean_path = gtk_tree_path_new_from_indices(row_idx, -1);
                                clean_disk = g_strdup(dev_name);
                            }
                        }
                        if (!target_path) {
                            target_path = gtk_tree_path_new_from_indices(row_idx, -1);
                            g_free(app->selected_disk);
                            app->selected_disk = g_strdup(dev_name);
                        }
                        idx++;
                        row_idx++;
                        found = TRUE;
                    }
                }
                g_strfreev(parts);
            }
        }
        pclose(fp);
    }

    if (clean_path) {
        if (target_path) gtk_tree_path_free(target_path);
        target_path = clean_path;
        g_free(app->selected_disk);
        app->selected_disk = clean_disk;
    }

    if (!found) {
        FILE *fp_lsblk = popen("lsblk -d -p -n -l -o NAME,SIZE,MODEL,TYPE 2>/dev/null", "r");
        if (fp_lsblk) {
            char line[512];
            int idx = 0;
            while (fgets(line, sizeof(line), fp_lsblk)) {
                g_strstrip(line);
                char dev_name[64] = {0};
                char size[64] = {0};
                if (sscanf(line, "%63s %63s", dev_name, size) >= 2) {
                    if (strstr(dev_name, "/dev/loop") == NULL &&
                        strstr(dev_name, "/dev/zram") == NULL &&
                        strstr(dev_name, "/dev/ram") == NULL &&
                        !is_disk_excluded(app, dev_name)) {

                        GtkTreeIter iter;
                        gtk_list_store_append(app->store_drives, &iter);
                        char *label = g_strdup_printf("Drive %d (%s)", idx, dev_name);
                        gtk_list_store_set(app->store_drives, &iter,
                            0, label, 1, size, 2, "Unallocated", 3, "Generic Drive", 4, dev_name, -1);
                        g_free(label);

                        if (!target_path) {
                            target_path = gtk_tree_path_new_from_indices(row_idx, -1);
                            g_free(app->selected_disk);
                            app->selected_disk = g_strdup(dev_name);
                        }
                        idx++;
                        row_idx++;
                        found = TRUE;
                    }
                }
            }
            pclose(fp_lsblk);
        }
    }

    if (target_path) {
        GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->tree_drives));
        gtk_tree_selection_select_path(sel, target_path);
        gtk_tree_path_free(target_path);
    }
}

static void on_refresh_drives_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    populate_drives(app);
}

static void on_format_drive_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    if (!app->selected_disk) return;

    GtkWidget *dlg = gtk_message_dialog_new(GTK_WINDOW(app->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
        "Are you sure you want to completely erase and format %s?\n\nAll existing data will be permanently destroyed.",
        app->selected_disk);
    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_YES) {
        char *cmd = g_strdup_printf("wipefs -a %s", app->selected_disk);
        g_spawn_command_line_async(cmd, NULL);
        g_free(cmd);
        populate_drives(app);
    }
    gtk_widget_destroy(dlg);
}

static void update_summary_card(AxisInstallerApp *app) {
    gtk_label_set_text(GTK_LABEL(app->lbl_summary_edition), app->selected_edition_name);
    gtk_label_set_text(GTK_LABEL(app->lbl_summary_disk), app->selected_disk ? app->selected_disk : "None");
    gtk_label_set_text(GTK_LABEL(app->lbl_summary_fs), g_strcmp0(app->selected_fs, "ext4") == 0 ? "EXT4 (Standard)" : "Btrfs (COW)");
    gtk_label_set_text(GTK_LABEL(app->lbl_summary_lang), app->selected_locale_name);
    gtk_label_set_text(GTK_LABEL(app->lbl_summary_tz), app->selected_tz_name);
    gtk_label_set_text(GTK_LABEL(app->lbl_summary_keymap), app->selected_keymap_name);

    char *user_info = g_strdup_printf("%s (%s@%s)", app->fullname, app->username, app->hostname);
    gtk_label_set_text(GTK_LABEL(app->lbl_summary_user), user_info);
    g_free(user_info);
}

static void on_try_os_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    g_spawn_command_line_async("notify-send -i dialog-information 'AxisOS Live Testing' 'You are now exploring AxisOS in Live Mode. Your hard drives remain untouched. When you are ready to install, click \"Install AxisOS\" on the taskbar or desktop!'", NULL);
    gtk_widget_destroy(app->window);
}

static void on_back_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    if (app->current_step == 5) {
        app->current_step = 2;
        update_navigation_state(app);
        return;
    }
    if (app->current_step > 0) {
        app->current_step--;
        update_navigation_state(app);
    }
}

static void on_next_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    if (app->current_step == 2) {
        if (!app->selected_disk || app->selected_disk[0] == '\0') {
            show_alert(app->window, "Please select a target storage drive before continuing.");
            return;
        }
    }
    if (app->current_step == 3) {
        g_free(app->fullname);
        g_free(app->username);
        g_free(app->hostname);
        g_free(app->password);
        app->fullname = g_strdup(gtk_entry_get_text(GTK_ENTRY(app->entry_fullname)));
        app->username = g_strdup(gtk_entry_get_text(GTK_ENTRY(app->entry_username)));
        app->hostname = g_strdup(gtk_entry_get_text(GTK_ENTRY(app->entry_hostname)));
        app->password = g_strdup(gtk_entry_get_text(GTK_ENTRY(app->entry_pwd)));
        const char *cp = gtk_entry_get_text(GTK_ENTRY(app->entry_cpwd));

        if (!app->username || app->username[0] == '\0') {
            show_alert(app->window, "Username cannot be empty.");
            return;
        }
        if (!app->password || app->password[0] == '\0' || strcmp(app->password, cp) != 0) {
            show_alert(app->window, "Passwords do not match. Please re-enter passwords.");
            return;
        }

        app->current_step = 4;
        update_summary_card(app);
        update_navigation_state(app);
        return;
    }
    if (app->current_step == 4) {
        if (!app->selected_disk || app->selected_disk[0] == '\0') {
            show_alert(app->window, "Please select a target storage drive before continuing.");
            return;
        }
        app->current_step = 5;
        update_navigation_state(app);
        start_installer_worker(app);
        return;
    }
    if (app->current_step == 6) {
        g_spawn_command_line_async("systemctl reboot", NULL);
        return;
    }

    app->current_step++;
    update_navigation_state(app);
}

static void on_install_now_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    if (!app->selected_disk || app->selected_disk[0] == '\0') {
        show_alert(app->window, "Please select a target storage drive before continuing.");
        return;
    }
    app->current_step = 5;
    update_navigation_state(app);
    start_installer_worker(app);
}

static void on_restart_clicked(GtkWidget *widget, gpointer user_data) {
    (void)widget;
    (void)user_data;
    g_spawn_command_line_async("systemctl reboot", NULL);
}

static void update_navigation_state(AxisInstallerApp *app) {
    char *name = g_strdup_printf("step_%d", app->current_step);
    gtk_stack_set_visible_child_name(GTK_STACK(app->stack), name);
    g_free(name);

    if (app->current_step == 0) {
        gtk_widget_set_visible(app->btn_back, FALSE);
        gtk_widget_set_visible(app->btn_try_os, TRUE);
    } else if (app->current_step <= 4) {
        gtk_widget_set_visible(app->btn_try_os, FALSE);
        gtk_widget_set_visible(app->btn_back, TRUE);
        gtk_widget_set_sensitive(app->btn_back, TRUE);
        gtk_button_set_label(GTK_BUTTON(app->btn_back), "← Back");
    } else {
        gtk_widget_set_visible(app->btn_try_os, FALSE);
        gtk_widget_set_visible(app->btn_back, FALSE);
    }

    if (app->current_step == 0 || app->current_step == 1 || app->current_step == 3) {
        gtk_widget_set_visible(app->btn_next, TRUE);
        gtk_widget_set_sensitive(app->btn_next, TRUE);
        gtk_button_set_label(GTK_BUTTON(app->btn_next), "Continue →");
    } else if (app->current_step == 2) {
        gtk_widget_set_visible(app->btn_next, TRUE);
        gtk_widget_set_sensitive(app->btn_next, app->selected_disk && app->selected_disk[0] != '\0');
        gtk_button_set_label(GTK_BUTTON(app->btn_next), "Continue →");
    } else if (app->current_step == 4) {
        gtk_widget_set_visible(app->btn_next, TRUE);
        gtk_widget_set_sensitive(app->btn_next, TRUE);
        gtk_button_set_label(GTK_BUTTON(app->btn_next), "Install AxisOS Now");
    } else if (app->current_step == 5) {
        gtk_widget_set_visible(app->btn_next, FALSE);
    } else if (app->current_step == 6) {
        gtk_widget_set_visible(app->btn_next, TRUE);
        gtk_widget_set_sensitive(app->btn_next, TRUE);
        gtk_button_set_label(GTK_BUTTON(app->btn_next), "Restart Now");
    }
}

// -----------------------------------------------------------------------------
// Step Builders
// -----------------------------------------------------------------------------
static void build_step_locale(AxisInstallerApp *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);

    GtkWidget *header_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 16);
    GtkWidget *logo_img = create_scaled_logo(48, 48);
    if (logo_img) gtk_box_pack_start(GTK_BOX(header_row), logo_img, FALSE, FALSE, 0);

    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *title = gtk_label_new("Welcome to AxisOS 2.1");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

    GtkWidget *sub = gtk_label_new("Select your language, region timezone, and keyboard layout.");
    gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

    gtk_box_pack_start(GTK_BOX(title_box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(title_box), sub, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(header_row), title_box, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), header_row, FALSE, FALSE, 2);

    // Two-column layout for Language & Timezone
    GtkWidget *columns_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_set_homogeneous(GTK_BOX(columns_box), TRUE);
    gtk_widget_set_vexpand(columns_box, TRUE);

    // --- LEFT COLUMN: Language ---
    GtkWidget *lang_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(lang_pane), "locale-pane");

    GtkWidget *lbl_lang_head = gtk_label_new("Language");
    gtk_label_set_xalign(GTK_LABEL(lbl_lang_head), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_lang_head), "card-title");
    gtk_box_pack_start(GTK_BOX(lang_pane), lbl_lang_head, FALSE, FALSE, 0);

    app->search_lang = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->search_lang), "Search languages...");
    g_signal_connect(app->search_lang, "search-changed", G_CALLBACK(on_lang_search_changed), app);
    gtk_box_pack_start(GTK_BOX(lang_pane), app->search_lang, FALSE, FALSE, 0);

    app->store_lang = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    GtkTreeIter default_lang_iter;
    gboolean has_default_lang = FALSE;
    for (size_t i = 0; i < sizeof(LANGUAGES)/sizeof(LANGUAGES[0]); ++i) {
        GtkTreeIter iter;
        gtk_list_store_append(app->store_lang, &iter);
        gtk_list_store_set(app->store_lang, &iter, 0, LANGUAGES[i].label, 1, LANGUAGES[i].code, -1);
        if (g_strcmp0(LANGUAGES[i].code, app->selected_locale) == 0) {
            default_lang_iter = iter;
            has_default_lang = TRUE;
        }
    }

    GtkTreeModel *filter_l = gtk_tree_model_filter_new(GTK_TREE_MODEL(app->store_lang), NULL);
    app->filter_lang = GTK_TREE_MODEL_FILTER(filter_l);
    gtk_tree_model_filter_set_visible_func(app->filter_lang, lang_filter_func, app, NULL);

    app->tree_lang = gtk_tree_view_new_with_model(filter_l);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(app->tree_lang), FALSE);
    GtkCellRenderer *rend_lang = gtk_cell_renderer_text_new();
    g_object_set(rend_lang, "ypad", 6, "xpad", 8, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(app->tree_lang), -1, "Language", rend_lang, "text", 0, NULL);

    GtkTreeSelection *sel_lang = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->tree_lang));
    gtk_tree_selection_set_mode(sel_lang, GTK_SELECTION_BROWSE);
    g_signal_connect(sel_lang, "changed", G_CALLBACK(on_lang_selection_changed), app);

    if (has_default_lang) {
        GtkTreeIter filter_iter;
        if (gtk_tree_model_filter_convert_child_iter_to_iter(app->filter_lang, &filter_iter, &default_lang_iter)) {
            gtk_tree_selection_select_iter(sel_lang, &filter_iter);
            GtkTreePath *path = gtk_tree_model_get_path(filter_l, &filter_iter);
            if (path) {
                gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(app->tree_lang), path, NULL, TRUE, 0.5, 0.0);
                gtk_tree_path_free(path);
            }
        }
    }

    GtkWidget *scroll_lang = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_lang), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_lang), 220);
    gtk_widget_set_vexpand(scroll_lang, TRUE);
    gtk_widget_set_hexpand(scroll_lang, TRUE);
    gtk_container_add(GTK_CONTAINER(scroll_lang), app->tree_lang);
    gtk_box_pack_start(GTK_BOX(lang_pane), scroll_lang, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(columns_box), lang_pane, TRUE, TRUE, 0);

    // --- RIGHT COLUMN: Timezone & Region ---
    GtkWidget *tz_pane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(tz_pane), "locale-pane");

    GtkWidget *lbl_tz_head = gtk_label_new("Location & Timezone");
    gtk_label_set_xalign(GTK_LABEL(lbl_tz_head), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_tz_head), "card-title");
    gtk_box_pack_start(GTK_BOX(tz_pane), lbl_tz_head, FALSE, FALSE, 0);

    app->search_tz = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->search_tz), "Search cities & timezones...");
    g_signal_connect(app->search_tz, "search-changed", G_CALLBACK(on_tz_search_changed), app);
    gtk_box_pack_start(GTK_BOX(tz_pane), app->search_tz, FALSE, FALSE, 0);

    app->store_tz = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    GtkTreeIter default_tz_iter;
    gboolean has_default_tz = FALSE;
    for (size_t i = 0; i < sizeof(TIMEZONES)/sizeof(TIMEZONES[0]); ++i) {
        GtkTreeIter iter;
        gtk_list_store_append(app->store_tz, &iter);
        gtk_list_store_set(app->store_tz, &iter, 0, TIMEZONES[i].label, 1, TIMEZONES[i].zone, -1);
        if (g_strcmp0(TIMEZONES[i].zone, app->selected_timezone) == 0) {
            default_tz_iter = iter;
            has_default_tz = TRUE;
        }
    }

    GtkTreeModel *filter_t = gtk_tree_model_filter_new(GTK_TREE_MODEL(app->store_tz), NULL);
    app->filter_tz = GTK_TREE_MODEL_FILTER(filter_t);
    gtk_tree_model_filter_set_visible_func(app->filter_tz, tz_filter_func, app, NULL);

    app->tree_tz = gtk_tree_view_new_with_model(filter_t);
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(app->tree_tz), FALSE);
    GtkCellRenderer *rend_tz = gtk_cell_renderer_text_new();
    g_object_set(rend_tz, "ypad", 6, "xpad", 8, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(app->tree_tz), -1, "Timezone", rend_tz, "text", 0, NULL);

    GtkTreeSelection *sel_tz = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->tree_tz));
    gtk_tree_selection_set_mode(sel_tz, GTK_SELECTION_BROWSE);
    g_signal_connect(sel_tz, "changed", G_CALLBACK(on_tz_selection_changed), app);

    if (has_default_tz) {
        GtkTreeIter filter_iter;
        if (gtk_tree_model_filter_convert_child_iter_to_iter(app->filter_tz, &filter_iter, &default_tz_iter)) {
            gtk_tree_selection_select_iter(sel_tz, &filter_iter);
            GtkTreePath *path = gtk_tree_model_get_path(filter_t, &filter_iter);
            if (path) {
                gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(app->tree_tz), path, NULL, TRUE, 0.5, 0.0);
                gtk_tree_path_free(path);
            }
        }
    }

    GtkWidget *scroll_tz = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_tz), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll_tz), 220);
    gtk_widget_set_vexpand(scroll_tz, TRUE);
    gtk_widget_set_hexpand(scroll_tz, TRUE);
    gtk_container_add(GTK_CONTAINER(scroll_tz), app->tree_tz);
    gtk_box_pack_start(GTK_BOX(tz_pane), scroll_tz, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(columns_box), tz_pane, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), columns_box, TRUE, TRUE, 0);

    // --- STATUS BADGE PILL ---
    app->lbl_locale_badge = gtk_label_new("Selected: English (United States)  •  New York / Eastern Time (USA / UTC-5)");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_locale_badge), "selection-pill");
    gtk_widget_set_halign(app->lbl_locale_badge, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(box), app->lbl_locale_badge, FALSE, FALSE, 2);
    update_locale_badge(app);

    // --- KEYBOARD CONFIGURATION ROW ---
    GtkWidget *kbd_card = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(kbd_card), "card");

    GtkWidget *lbl_kbd = gtk_label_new("Keyboard Layout:");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_kbd), "field-label");
    gtk_box_pack_start(GTK_BOX(kbd_card), lbl_kbd, FALSE, FALSE, 0);

    app->combo_keyboard = gtk_combo_box_text_new();
    for (size_t i = 0; i < sizeof(KEYMAPS)/sizeof(KEYMAPS[0]); ++i) {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(app->combo_keyboard), KEYMAPS[i].label);
    }
    gtk_combo_box_set_active(GTK_COMBO_BOX(app->combo_keyboard), 0);
    g_signal_connect(app->combo_keyboard, "changed", G_CALLBACK(on_keymap_changed), app);
    gtk_box_pack_start(GTK_BOX(kbd_card), app->combo_keyboard, TRUE, TRUE, 0);

    GtkWidget *lbl_test = gtk_label_new("Test:");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_test), "field-label");
    gtk_box_pack_start(GTK_BOX(kbd_card), lbl_test, FALSE, FALSE, 0);

    app->entry_keytest = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(app->entry_keytest), "Type here to test keys...");
    gtk_box_pack_start(GTK_BOX(kbd_card), app->entry_keytest, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(box), kbd_card, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_in_scroller(box);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "step_0");
}

static GtkWidget *create_edition_card(AxisInstallerApp *app,
                                      const char *title_text, const char *badge_text,
                                      const char *desc_text, const char *feats_text,
                                      GtkWidget **out_card, GtkWidget **out_radio,
                                      GtkWidget *group_radio, const char *edition_key) {
    GtkWidget *card_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(card_box), "edition-card");

    GtkWidget *row_top = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    GtkWidget *rad = gtk_radio_button_new_from_widget(group_radio ? GTK_RADIO_BUTTON(group_radio) : NULL);
    *out_radio = rad;

    GtkWidget *lbl_t = gtk_label_new(title_text);
    gtk_label_set_xalign(GTK_LABEL(lbl_t), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_t), "edition-title");

    GtkWidget *lbl_b = gtk_label_new(badge_text);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_b), "edition-badge");

    gtk_box_pack_start(GTK_BOX(row_top), rad, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_top), lbl_t, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row_top), lbl_b, FALSE, FALSE, 4);

    GtkWidget *lbl_d = gtk_label_new(desc_text);
    gtk_label_set_xalign(GTK_LABEL(lbl_d), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(lbl_d), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_d), "edition-desc");
    gtk_widget_set_margin_start(lbl_d, 28);

    GtkWidget *lbl_f = gtk_label_new(feats_text);
    gtk_label_set_xalign(GTK_LABEL(lbl_f), 0.0);
    gtk_label_set_line_wrap(GTK_LABEL(lbl_f), TRUE);
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_f), "edition-features");
    gtk_widget_set_margin_start(lbl_f, 28);

    gtk_box_pack_start(GTK_BOX(card_box), row_top, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_box), lbl_d, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card_box), lbl_f, FALSE, FALSE, 0);

    GtkWidget *event_box = gtk_event_box_new();
    gtk_container_add(GTK_CONTAINER(event_box), card_box);
    gtk_event_box_set_above_child(GTK_EVENT_BOX(event_box), FALSE);

    *out_card = card_box;

    EditionCbData *cb_data = g_new0(EditionCbData, 1);
    cb_data->app = app;
    cb_data->key = g_strdup(edition_key);

    g_signal_connect(rad, "toggled", G_CALLBACK(on_edition_radio_toggled), cb_data);
    g_signal_connect(event_box, "button-press-event", G_CALLBACK(on_edition_card_clicked), cb_data);

    return event_box;
}

static void build_step_edition(AxisInstallerApp *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *title = gtk_label_new("Choose Your AxisOS Edition");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

    GtkWidget *sub = gtk_label_new("Select the tailored configuration that fits your workflow. You can easily switch editions anytime after installation.");
    gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

    GtkWidget *cards_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    GtkWidget *eb_base = create_edition_card(app,
        "AxisOS Home & Gaming",
        "DEFAULT / GAMING",
        "Optimized for high-performance low-latency gaming, streaming, content creation, and daily computing.",
        "• Proton-GE & Wine gaming runtimes  • Mesa & NVIDIA graphics pipelines  • Flatpak app ecosystem",
        &app->card_edition_base, &app->radio_edition_base, NULL, "base"
    );

    GtkWidget *eb_ent = create_edition_card(app,
        "AxisOS Enterprise Workstation",
        "ENTERPRISE",
        "Hardened for corporate environments, DevOps, system administration, and enterprise security compliance.",
        "• Active Directory / FreeIPA domain join  • Docker & KVM virtualization  • Hardened sysctl & UFW firewall",
        &app->card_edition_ent, &app->radio_edition_ent, app->radio_edition_base, "enterprise"
    );

    GtkWidget *eb_edu = create_edition_card(app,
        "AxisOS Work & School",
        "WORK & EDUCATION",
        "Focused productivity environment designed for students, educators, researchers, and professional workspaces.",
        "• Complete LibreOffice & PDF tools  • Zoom, Teams, Webex web apps  • Distraction-free & battery longevity profile",
        &app->card_edition_edu, &app->radio_edition_edu, app->radio_edition_base, "education"
    );

    gtk_box_pack_start(GTK_BOX(cards_box), eb_base, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cards_box), eb_ent, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cards_box), eb_edu, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(box), cards_box, FALSE, FALSE, 4);

    select_edition(app, "base");

    GtkWidget *scroller = wrap_in_scroller(box);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "step_1");
}

static void build_step_disk(AxisInstallerApp *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);

    GtkWidget *title = gtk_label_new("Where do you want to install AxisOS?");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

    GtkWidget *sub = gtk_label_new("Select the target disk drive. The drive will be partitioned with high-performance Btrfs subvolumes or EXT4.");
    gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

    app->store_drives = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    app->tree_drives = gtk_tree_view_new_with_model(GTK_TREE_MODEL(app->store_drives));
    gtk_style_context_add_class(gtk_widget_get_style_context(app->tree_drives), "treeview");

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(app->tree_drives), -1, "Drive", renderer, "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(app->tree_drives), -1, "Capacity", renderer, "text", 1, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(app->tree_drives), -1, "Free Space", renderer, "text", 2, NULL);
    gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(app->tree_drives), -1, "Model & Status / OS", renderer, "text", 3, NULL);

    GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(app->tree_drives));
    g_signal_connect(sel, "changed", G_CALLBACK(on_drive_selection_changed), app);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 180);
    gtk_container_add(GTK_CONTAINER(scroll), app->tree_drives);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    app->lbl_disk_selection_status = gtk_label_new("Selected: /dev/sda");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_disk_selection_status), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_disk_selection_status), "field-label");

    GtkWidget *btn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *b_format = gtk_button_new_with_label("Wipe & Format Selected Drive");
    gtk_style_context_add_class(gtk_widget_get_style_context(b_format), "btn-danger");
    g_signal_connect(b_format, "clicked", G_CALLBACK(on_format_drive_clicked), app);

    GtkWidget *b_refresh = gtk_button_new_with_label("Refresh Drives");
    gtk_style_context_add_class(gtk_widget_get_style_context(b_refresh), "btn-secondary");
    g_signal_connect(b_refresh, "clicked", G_CALLBACK(on_refresh_drives_clicked), app);

    gtk_box_pack_start(GTK_BOX(btn_row), app->lbl_disk_selection_status, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(btn_row), b_refresh, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(btn_row), b_format, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_row, FALSE, FALSE, 4);

    GtkWidget *fs_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(fs_card), "info-card");

    GtkWidget *fs_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 14);
    GtkWidget *lbl_fs_title = gtk_label_new("Target Filesystem:");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_fs_title), "field-label");

    app->combo_fs = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_fs), "ext4", "EXT4 (Recommended - Maximum Stability & Universal Compatibility)");
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(app->combo_fs), "btrfs", "Btrfs (Modern Copy-on-Write with Transparent Compression)");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(app->combo_fs), "ext4");
    g_signal_connect(app->combo_fs, "changed", G_CALLBACK(on_fs_changed), app);

    gtk_box_pack_start(GTK_BOX(fs_row), lbl_fs_title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(fs_row), app->combo_fs, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(fs_card), fs_row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), fs_card, FALSE, FALSE, 4);

    GtkWidget *scroller = wrap_in_scroller(box);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "step_2");
    populate_drives(app);
}

static void build_step_account(AxisInstallerApp *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *title = gtk_label_new("Create Your Primary User Account");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

    GtkWidget *sub = gtk_label_new("Set up your login profile. This user will have administrative sudo privileges.");
    gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "info-card");

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 20);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 12);

    GtkWidget *l1 = gtk_label_new("Your Name:");
    gtk_label_set_xalign(GTK_LABEL(l1), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(l1), "field-label");
    app->entry_fullname = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_fullname), "AxisOS User");

    GtkWidget *l2 = gtk_label_new("Username (login):");
    gtk_label_set_xalign(GTK_LABEL(l2), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(l2), "field-label");
    app->entry_username = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_username), "axis");

    GtkWidget *l3 = gtk_label_new("Computer Name (hostname):");
    gtk_label_set_xalign(GTK_LABEL(l3), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(l3), "field-label");
    app->entry_hostname = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(app->entry_hostname), "axis-pc");

    GtkWidget *l4 = gtk_label_new("Password:");
    gtk_label_set_xalign(GTK_LABEL(l4), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(l4), "field-label");
    app->entry_pwd = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(app->entry_pwd), FALSE);
    gtk_entry_set_text(GTK_ENTRY(app->entry_pwd), "axis");

    GtkWidget *l5 = gtk_label_new("Confirm Password:");
    gtk_label_set_xalign(GTK_LABEL(l5), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(l5), "field-label");
    app->entry_cpwd = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(app->entry_cpwd), FALSE);
    gtk_entry_set_text(GTK_ENTRY(app->entry_cpwd), "axis");

    gtk_grid_attach(GTK_GRID(grid), l1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->entry_fullname, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), l2, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->entry_username, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), l3, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->entry_hostname, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), l4, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->entry_pwd, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), l5, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->entry_cpwd, 1, 4, 1, 1);

    gtk_box_pack_start(GTK_BOX(card), grid, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), card, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_in_scroller(box);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "step_3");
}

static void build_step_summary(AxisInstallerApp *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *title = gtk_label_new("Ready to Install AxisOS 2.0");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

    GtkWidget *sub = gtk_label_new("Review your installation choices before proceeding. Clicking Install Now will begin writing to disk.");
    gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "info-card");

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 24);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);

    GtkWidget *k_ed = gtk_label_new("Selected Edition:");
    gtk_label_set_xalign(GTK_LABEL(k_ed), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(k_ed), "summary-key");
    app->lbl_summary_edition = gtk_label_new("AxisOS Home & Gaming");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_summary_edition), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_summary_edition), "summary-val");

    GtkWidget *k_d = gtk_label_new("Target Storage Disk:");
    gtk_label_set_xalign(GTK_LABEL(k_d), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(k_d), "summary-key");
    app->lbl_summary_disk = gtk_label_new("/dev/sda");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_summary_disk), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_summary_disk), "summary-val");

    GtkWidget *k_fs = gtk_label_new("Target Filesystem:");
    gtk_label_set_xalign(GTK_LABEL(k_fs), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(k_fs), "summary-key");
    app->lbl_summary_fs = gtk_label_new("EXT4 (Standard)");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_summary_fs), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_summary_fs), "summary-val");

    GtkWidget *k_l = gtk_label_new("Language & Locale:");
    gtk_label_set_xalign(GTK_LABEL(k_l), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(k_l), "summary-key");
    app->lbl_summary_lang = gtk_label_new("English (United States)");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_summary_lang), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_summary_lang), "summary-val");

    GtkWidget *k_t = gtk_label_new("Location & Timezone:");
    gtk_label_set_xalign(GTK_LABEL(k_t), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(k_t), "summary-key");
    app->lbl_summary_tz = gtk_label_new("New York / Eastern Time (UTC-5)");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_summary_tz), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_summary_tz), "summary-val");

    GtkWidget *k_k = gtk_label_new("Keyboard Layout:");
    gtk_label_set_xalign(GTK_LABEL(k_k), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(k_k), "summary-key");
    app->lbl_summary_keymap = gtk_label_new("English (US Standard)");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_summary_keymap), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_summary_keymap), "summary-val");

    GtkWidget *k_u = gtk_label_new("Primary User:");
    gtk_label_set_xalign(GTK_LABEL(k_u), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(k_u), "summary-key");
    app->lbl_summary_user = gtk_label_new("AxisOS User (axis@axis-pc)");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_summary_user), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_summary_user), "summary-val");

    gtk_grid_attach(GTK_GRID(grid), k_ed, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->lbl_summary_edition, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), k_d, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->lbl_summary_disk, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), k_fs, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->lbl_summary_fs, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), k_l, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->lbl_summary_lang, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), k_t, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->lbl_summary_tz, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), k_k, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->lbl_summary_keymap, 1, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), k_u, 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), app->lbl_summary_user, 1, 6, 1, 1);

    gtk_box_pack_start(GTK_BOX(card), grid, FALSE, FALSE, 0);

    app->btn_summary_install_large = gtk_button_new_with_label("Install AxisOS Now");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->btn_summary_install_large), "btn-install-large");
    g_signal_connect(app->btn_summary_install_large, "clicked", G_CALLBACK(on_install_now_clicked), app);
    gtk_widget_set_halign(app->btn_summary_install_large, GTK_ALIGN_CENTER);
    gtk_box_pack_start(GTK_BOX(card), app->btn_summary_install_large, FALSE, FALSE, 12);

    gtk_box_pack_start(GTK_BOX(box), card, FALSE, FALSE, 0);

    GtkWidget *scroller = wrap_in_scroller(box);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "step_4");
}

static void build_step_installing(AxisInstallerApp *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 14);

    GtkWidget *title = gtk_label_new("Installing AxisOS 2.0");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

    GtkWidget *sub = gtk_label_new("Deploying operating system files, device drivers, and UEFI bootloader. This may take a few minutes.");
    gtk_label_set_xalign(GTK_LABEL(sub), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);

    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(card), "info-card");

    app->lbl_stage_copy = gtk_label_new("    Copying AxisOS system files");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_stage_copy), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_copy), "progress-stage");

    app->lbl_stage_prep = gtk_label_new("    Getting files ready for installation (0%)");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_stage_prep), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_prep), "progress-stage");

    app->lbl_stage_feat = gtk_label_new("    Installing features & drivers (NVIDIA, AMD, Intel)");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_stage_feat), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_feat), "progress-stage");

    app->lbl_stage_upd = gtk_label_new("    Configuring system repositories & gaming runtime");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_stage_upd), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_upd), "progress-stage");

    app->lbl_stage_fin = gtk_label_new("    Installing Microsoft-signed UEFI Secure Bootloader");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_stage_fin), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_fin), "progress-stage");

    gtk_box_pack_start(GTK_BOX(card), app->lbl_stage_copy, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), app->lbl_stage_prep, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), app->lbl_stage_feat, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), app->lbl_stage_upd, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(card), app->lbl_stage_fin, FALSE, FALSE, 0);

    app->progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), 0.05);
    gtk_box_pack_start(GTK_BOX(card), app->progress_bar, FALSE, FALSE, 10);

    app->lbl_progress_percent = gtk_label_new("Status: Preparing partition table and Btrfs subvolumes...");
    gtk_label_set_xalign(GTK_LABEL(app->lbl_progress_percent), 0.0);
    gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_progress_percent), "field-label");
    gtk_box_pack_start(GTK_BOX(card), app->lbl_progress_percent, FALSE, FALSE, 0);

    // Inline collapsible "Show Installation Log" GtkExpander
    app->expander_log = gtk_expander_new("Show Installation Log");
    gtk_expander_set_expanded(GTK_EXPANDER(app->expander_log), FALSE);
    gtk_widget_set_margin_top(app->expander_log, 8);

    GtkWidget *log_scroller = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(log_scroller), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(log_scroller, -1, 140);
    gtk_widget_set_hexpand(log_scroller, TRUE);

    app->txt_install_log = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(app->txt_install_log), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(app->txt_install_log), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(app->txt_install_log), TRUE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(app->txt_install_log), GTK_WRAP_WORD_CHAR);

    gtk_container_add(GTK_CONTAINER(log_scroller), app->txt_install_log);
    gtk_container_add(GTK_CONTAINER(app->expander_log), log_scroller);
    gtk_box_pack_start(GTK_BOX(card), app->expander_log, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(box), card, FALSE, FALSE, 4);

    GtkWidget *scroller = wrap_in_scroller(box);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "step_5");
}

static void build_step_finish(AxisInstallerApp *app) {
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 18);
    gtk_widget_set_valign(box, GTK_ALIGN_CENTER);
    gtk_widget_set_halign(box, GTK_ALIGN_CENTER);

    GtkWidget *logo_img = create_scaled_logo(72, 72);
    if (logo_img) gtk_box_pack_start(GTK_BOX(box), logo_img, FALSE, FALSE, 0);

    GtkWidget *title = gtk_label_new("AxisOS 2.0 Successfully Installed!");
    gtk_style_context_add_class(gtk_widget_get_style_context(title), "win-title");

    GtkWidget *sub = gtk_label_new("Please remove your live USB installation media and restart your computer to begin using AxisOS.");
    gtk_style_context_add_class(gtk_widget_get_style_context(sub), "win-subtitle");

    GtkWidget *btn_restart = gtk_button_new_with_label("⟳ Restart Computer Now");
    gtk_style_context_add_class(gtk_widget_get_style_context(btn_restart), "btn-install-large");
    g_signal_connect(btn_restart, "clicked", G_CALLBACK(on_restart_clicked), app);

    gtk_box_pack_start(GTK_BOX(box), title, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), sub, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), btn_restart, FALSE, FALSE, 14);

    GtkWidget *scroller = wrap_in_scroller(box);
    gtk_stack_add_named(GTK_STACK(app->stack), scroller, "step_6");
}

// -----------------------------------------------------------------------------
// Background Worker Thread & Pipe Handler
// -----------------------------------------------------------------------------
typedef struct {
    AxisInstallerApp *app;
    int pct;
    char *msg;
} ProgressUpdate;

static gboolean idle_update_progress(gpointer user_data) {
    ProgressUpdate *p = (ProgressUpdate *)user_data;
    AxisInstallerApp *app = p->app;

    gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress_bar), p->pct / 100.0);
    gtk_label_set_text(GTK_LABEL(app->lbl_progress_percent), p->msg);

    if (p->pct >= 15) {
        gtk_label_set_text(GTK_LABEL(app->lbl_stage_copy), "  ✔  Copying AxisOS files");
        gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_copy), "done");
    }
    if (p->pct >= 15 && p->pct < 85) {
        char *s = g_strdup_printf("  ⌛  Getting files ready for installation (%d%%)", p->pct);
        gtk_label_set_text(GTK_LABEL(app->lbl_stage_prep), s);
        g_free(s);
        gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_prep), "active");
    } else if (p->pct >= 85) {
        gtk_label_set_text(GTK_LABEL(app->lbl_stage_prep), "  ✔  Getting files ready for installation (100%)");
        gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_prep), "done");
        gtk_style_context_remove_class(gtk_widget_get_style_context(app->lbl_stage_prep), "active");
    }
    if (p->pct >= 88) {
        gtk_label_set_text(GTK_LABEL(app->lbl_stage_feat), "  ✔  Installing features & drivers (NVIDIA, AMD, Intel)");
        gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_feat), "done");
    }
    if (p->pct >= 94) {
        gtk_label_set_text(GTK_LABEL(app->lbl_stage_upd), "  ✔  Configuring system repositories & gaming runtime");
        gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_upd), "done");
    }
    if (p->pct >= 98) {
        gtk_label_set_text(GTK_LABEL(app->lbl_stage_fin), "  ✔  Installing Microsoft-signed UEFI Secure Bootloader");
        gtk_style_context_add_class(gtk_widget_get_style_context(app->lbl_stage_fin), "done");
    }

    if (app->txt_install_log) {
        FILE *lf = fopen("/tmp/axisos-installer.log", "r");
        if (lf) {
            char lbuf[8192] = {0};
            char lines[30][512];
            int count = 0;
            char l[512];
            while (fgets(l, sizeof(l), lf)) {
                snprintf(lines[count % 25], sizeof(lines[0]), "%s", l);
                count++;
            }
            fclose(lf);
            int start = count > 25 ? count - 25 : 0;
            int total = count > 25 ? 25 : count;
            int off = 0;
            for (int i = 0; i < total; i++) {
                int idx = (start + i) % 25;
                off += snprintf(lbuf + off, sizeof(lbuf) - off, "%s", lines[idx]);
            }
            GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(app->txt_install_log));
            gtk_text_buffer_set_text(tb, lbuf, -1);
            GtkTextIter end_iter;
            gtk_text_buffer_get_end_iter(tb, &end_iter);
            GtkTextMark *mark = gtk_text_buffer_create_mark(tb, NULL, &end_iter, FALSE);
            gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(app->txt_install_log), mark, 0.0, TRUE, 0.0, 1.0);
        }
    }

    g_free(p->msg);
    g_free(p);
    return G_SOURCE_REMOVE;
}

typedef struct {
    AxisInstallerApp *app;
    char *err_msg;
} InstallFinishData;

static gboolean idle_install_success(gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;
    app->current_step = 6;
    update_navigation_state(app);
    return G_SOURCE_REMOVE;
}

static gboolean idle_install_failure(gpointer user_data) {
    InstallFinishData *d = (InstallFinishData *)user_data;
    AxisInstallerApp *app = d->app;

    char *err_text = g_strdup_printf("Installation stopped: %s", d->err_msg);
    gtk_label_set_text(GTK_LABEL(app->lbl_progress_percent), err_text);
    g_free(err_text);

    gtk_widget_set_visible(app->btn_back, TRUE);
    gtk_widget_set_sensitive(app->btn_back, TRUE);
    gtk_button_set_label(GTK_BUTTON(app->btn_back), "Back to Drive Selection");

    char *dialog_msg = g_strdup_printf(
        "An issue occurred during installation:\n\n%s\n\n"
        "Please select 'Back to Drive Selection' to choose another drive or verify disk health.",
        d->err_msg);
    show_visible_error_dialog(GTK_WINDOW(app->window), "Installation Issue", dialog_msg, NULL);
    g_free(dialog_msg);

    g_free(d->err_msg);
    g_free(d);
    return G_SOURCE_REMOVE;
}

static gpointer thread_installer_worker(gpointer user_data) {
    AxisInstallerApp *app = (AxisInstallerApp *)user_data;

    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0) {
        return NULL;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return NULL;
    }

    if (pid == 0) {
        close(in_pipe[1]);
        dup2(in_pipe[0], STDIN_FILENO);
        close(in_pipe[0]);

        close(out_pipe[0]);
        dup2(out_pipe[1], STDOUT_FILENO);
        dup2(out_pipe[1], STDERR_FILENO);
        close(out_pipe[1]);

        GPtrArray *argv_array = g_ptr_array_new();
        if (geteuid() != 0) {
            g_ptr_array_add(argv_array, (gpointer)"sudo");
            g_ptr_array_add(argv_array, (gpointer)"-n");
        }

        const char *script_path = "/usr/local/bin/axisos-installer.sh";
        if (access(script_path, X_OK) != 0) {
            script_path = "/usr/local/bin/axisos-install.sh";
        }
        g_ptr_array_add(argv_array, (gpointer)script_path);
        g_ptr_array_add(argv_array, (gpointer)"--edition");
        g_ptr_array_add(argv_array, (gpointer)app->selected_edition);
        g_ptr_array_add(argv_array, (gpointer)"--disk");
        g_ptr_array_add(argv_array, (gpointer)app->selected_disk);
        g_ptr_array_add(argv_array, (gpointer)"--fs");
        g_ptr_array_add(argv_array, (gpointer)app->selected_fs);
        g_ptr_array_add(argv_array, (gpointer)"--username");
        g_ptr_array_add(argv_array, (gpointer)app->username);
        g_ptr_array_add(argv_array, (gpointer)"--password-stdin");
        g_ptr_array_add(argv_array, (gpointer)"--fullname");
        g_ptr_array_add(argv_array, (gpointer)app->fullname);
        g_ptr_array_add(argv_array, (gpointer)"--hostname");
        g_ptr_array_add(argv_array, (gpointer)app->hostname);
        g_ptr_array_add(argv_array, (gpointer)"--locale");
        g_ptr_array_add(argv_array, (gpointer)app->selected_locale);
        g_ptr_array_add(argv_array, (gpointer)"--timezone");
        g_ptr_array_add(argv_array, (gpointer)app->selected_timezone);
        g_ptr_array_add(argv_array, (gpointer)"--keymap");
        g_ptr_array_add(argv_array, (gpointer)app->selected_keymap);
        g_ptr_array_add(argv_array, NULL);

        execvp((char *)argv_array->pdata[0], (char **)argv_array->pdata);
        _exit(127);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    char *pass_line = g_strdup_printf("%s\n", app->password);
    ssize_t bytes_written = write(in_pipe[1], pass_line, strlen(pass_line));
    (void)bytes_written;
    g_free(pass_line);
    close(in_pipe[1]);

    FILE *fp = fdopen(out_pipe[0], "r");
    char *last_error_line = g_strdup("Unexpected installation error.");
    if (fp) {
        char buf[512];
        while (fgets(buf, sizeof(buf), fp)) {
            g_strstrip(buf);
            if (buf[0] != '\0' && !g_str_has_prefix(buf, "PROGRESS:")) {
                if (g_str_has_prefix(buf, "ERROR:") || g_str_has_prefix(buf, "Error:")) {
                    g_free(last_error_line);
                    last_error_line = g_strdup(buf);
                } else if (g_strcmp0(last_error_line, "Unexpected installation error.") == 0 &&
                           (strstr(buf, "error") || strstr(buf, "failed"))) {
                    g_free(last_error_line);
                    last_error_line = g_strdup(buf);
                }
            }
            if (g_str_has_prefix(buf, "PROGRESS:")) {
                char *colon = strchr(buf + 9, ':');
                if (colon) {
                    *colon = '\0';
                    int pct = atoi(buf + 9);
                    const char *msg = colon + 1;

                    ProgressUpdate *up = g_new0(ProgressUpdate, 1);
                    up->app = app;
                    up->pct = pct;
                    up->msg = g_strdup(msg);
                    g_idle_add(idle_update_progress, up);
                }
            }
        }
        fclose(fp);
    } else {
        close(out_pipe[0]);
    }

    int wstatus = 0;
    waitpid(pid, &wstatus, 0);
    int exit_code = WIFEXITED(wstatus) ? WEXITSTATUS(wstatus) : -1;

    if (exit_code == 0) {
        g_free(last_error_line);
        g_idle_add(idle_install_success, app);
    } else {
        // Fallback: If last_error_line is still generic, read the last ERROR line from log
        if (!last_error_line || g_strcmp0(last_error_line, "Unexpected installation error.") == 0) {
            FILE *lf = fopen("/tmp/axisos-installer.log", "r");
            if (lf) {
                char lbuf[512];
                char last_err_in_file[512] = {0};
                while (fgets(lbuf, sizeof(lbuf), lf)) {
                    g_strstrip(lbuf);
                    if (g_str_has_prefix(lbuf, "ERROR:") || g_str_has_prefix(lbuf, "Error:") ||
                        strstr(lbuf, "ERROR") || strstr(lbuf, "failed")) {
                        snprintf(last_err_in_file, sizeof(last_err_in_file), "%s", lbuf);
                    }
                }
                fclose(lf);
                if (last_err_in_file[0] != '\0') {
                    g_free(last_error_line);
                    last_error_line = g_strdup(last_err_in_file);
                }
            }
        }
        InstallFinishData *d = g_new0(InstallFinishData, 1);
        d->app = app;
        d->err_msg = last_error_line;
        g_idle_add(idle_install_failure, d);
    }
    return NULL;
}

static void start_installer_worker(AxisInstallerApp *app) {
    g_thread_new("installer_worker", thread_installer_worker, app);
}

// -----------------------------------------------------------------------------
// Main Application Setup
// -----------------------------------------------------------------------------
static AxisInstallerApp *axis_installer_app_create(void) {
    AxisInstallerApp *app = g_new0(AxisInstallerApp, 1);
    app->selected_edition = g_strdup("base");
    app->selected_edition_name = g_strdup("AxisOS Home & Gaming");
    app->selected_disk = g_strdup("/dev/sda");
    app->selected_fs = g_strdup("ext4");
    app->selected_locale = g_strdup("en_US.UTF-8");
    app->selected_locale_name = g_strdup("English (United States)");
    app->selected_timezone = g_strdup("America/New_York");
    app->selected_tz_name = g_strdup("New York / Eastern Time (UTC-5)");
    app->selected_keymap = g_strdup("us");
    app->selected_keymap_name = g_strdup("English (US Standard)");
    app->fullname = g_strdup("AxisOS User");
    app->username = g_strdup("axis");
    app->password = g_strdup("axis");
    app->hostname = g_strdup("axis-pc");

    GtkSettings *settings = gtk_settings_get_default();
    if (settings) {
        g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
    }

    app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->window), "AxisOS 2.0 Setup");

    GdkDisplay *display = gdk_display_get_default();
    GdkMonitor *monitor = display ? gdk_display_get_primary_monitor(display) : NULL;
    if (!monitor && display) {
        monitor = gdk_display_get_monitor_at_point(display, 0, 0);
    }
    GdkRectangle workarea = {0, 0, 1024, 768};
    if (monitor) {
        gdk_monitor_get_workarea(monitor, &workarea);
    }
    int def_w = MIN(920, workarea.width > 48 ? workarea.width - 48 : 800);
    int def_h = MIN(620, workarea.height > 64 ? workarea.height - 64 : 600);
    gtk_window_set_default_size(GTK_WINDOW(app->window), def_w, def_h);
    gtk_window_set_position(GTK_WINDOW(app->window), GTK_WIN_POS_CENTER);
    gtk_window_set_resizable(GTK_WINDOW(app->window), TRUE);
    gtk_window_set_default_icon_name("axisos");
    gtk_window_set_icon_name(GTK_WINDOW(app->window), "axisos");

    // Apply system theme
    load_axis_theme();

    GtkWidget *hb = gtk_header_bar_new();
    gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hb), TRUE);
    gtk_header_bar_set_title(GTK_HEADER_BAR(hb), "AxisOS 2.1 Setup Wizard");
    gtk_header_bar_set_subtitle(GTK_HEADER_BAR(hb), "Liquid Nodes Edition");
    gtk_window_set_titlebar(GTK_WINDOW(app->window), hb);

    GtkWidget *root_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(app->window), root_box);

    app->stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(app->stack), GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(app->stack), 180);
    gtk_box_pack_start(GTK_BOX(root_box), app->stack, TRUE, TRUE, 0);

    GtkWidget *footer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(footer), "setup-footer");

    app->btn_back = gtk_button_new_with_label("← Back");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->btn_back), "btn-secondary");
    g_signal_connect(app->btn_back, "clicked", G_CALLBACK(on_back_clicked), app);

    app->btn_try_os = gtk_button_new_with_label("Try AxisOS (Test without installing)");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->btn_try_os), "btn-secondary");
    g_signal_connect(app->btn_try_os, "clicked", G_CALLBACK(on_try_os_clicked), app);

    app->btn_next = gtk_button_new_with_label("Continue →");
    gtk_style_context_add_class(gtk_widget_get_style_context(app->btn_next), "btn-primary");
    g_signal_connect(app->btn_next, "clicked", G_CALLBACK(on_next_clicked), app);

    gtk_box_pack_start(GTK_BOX(footer), app->btn_back, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(footer), app->btn_try_os, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(footer), app->btn_next, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(root_box), footer, FALSE, FALSE, 0);

    detect_live_media(app);

    build_step_locale(app);
    build_step_edition(app);
    build_step_disk(app);
    build_step_account(app);
    build_step_summary(app);
    build_step_installing(app);
    build_step_finish(app);

    update_navigation_state(app);
    g_signal_connect(app->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    return app;
}

int main(int argc, char *argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("AxisOS Graphical System Installer (Pure C11)\n");
        printf("Usage: axis-installer [OPTION]\n");
        printf("Options:\n");
        printf("  --help, -h      Show this help message and exit\n");
        printf("  --drives, -d    Scan and list available target storage drives\n");
        printf("  --force, -f     Launch installer even if /etc/axisos-installed is present\n");
        return 0;
    }
    if (argc > 1 && (strcmp(argv[1], "--drives") == 0 || strcmp(argv[1], "-d") == 0)) {
        printf("=== Available Target Storage Drives for Installation ===\n");
        FILE *fp = popen("lsblk -d -n -o NAME,SIZE,MODEL,TYPE 2>/dev/null | grep disk", "r");
        if (fp) {
            char line[256];
            int count = 0;
            while (fgets(line, sizeof(line), fp)) {
                g_strstrip(line);
                count++;
                printf("  [%d] /dev/%s\n", count, line);
            }
            pclose(fp);
            if (count == 0) printf("  No raw physical disk blocks detected.\n");
        }
        return 0;
    }

    gboolean force = (argc > 1 && (strcmp(argv[1], "--force") == 0 || strcmp(argv[1], "-f") == 0));
    if (!force && access("/etc/axisos-installed", F_OK) == 0) {
        g_print("AxisOS is already installed on this machine. Pass --force to run anyway.\n");
        return 0;
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
    AxisInstallerApp *app = axis_installer_app_create();
    gtk_widget_show_all(app->window);
    gtk_main();
    return 0;
}
