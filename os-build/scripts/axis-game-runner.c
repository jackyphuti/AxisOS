#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <glob.h>
#include <ctype.h>

static void ensure_emulation_directories(void) {
    const char *home = getenv("HOME");
    if (!home) return;

    char cmd[1536];
    snprintf(cmd, sizeof(cmd),
        "mkdir -p \"%s/Emulation/bios/ps1\" \"%s/Emulation/bios/ps2\" \"%s/Emulation/bios/ps3\" "
        "\"%s/Emulation/bios/psp\" \"%s/Emulation/bios/psvita\" \"%s/Emulation/bios/ps4\" "
        "\"%s/Emulation/games/ps1\" \"%s/Emulation/games/ps2\" \"%s/Emulation/games/ps3\" "
        "\"%s/Emulation/games/psp\" \"%s/Emulation/games/psvita\" \"%s/Emulation/games/ps4\" 2>/dev/null",
        home, home, home, home, home, home,
        home, home, home, home, home, home);
    system(cmd);
}

static void send_notification(const char *title, const char *msg) {
    pid_t pid = fork();
    if (pid == 0) {
        char *argv[] = {
            "notify-send",
            "-i", "input-gaming",
            "-a", "AxisOS Game Station",
            (char *)title,
            (char *)msg,
            NULL
        };
        execvp("notify-send", argv);
        _exit(0);
    }
    if (pid > 0) {
        waitpid(pid, NULL, 0);
    }
}

/*
 * BIOS Policy: Users must provide legitimate BIOS dumps from physical hardware they own.
 * AxisOS does NOT distribute or bundle proprietary Sony PlayStation BIOS images.
 */
static int has_bios(const char *sys_name) {
    const char *home = getenv("HOME");
    if (!home) return 1;

    char pattern[512];
    glob_t g;
    int found = 0;

    if (strcmp(sys_name, "ps2") == 0) {
        snprintf(pattern, sizeof(pattern), "%s/Emulation/bios/ps2/*.[bB][iI][nN]", home);
        if (glob(pattern, 0, NULL, &g) == 0) {
            if (g.gl_pathc > 0) found = 1;
            globfree(&g);
        }
        if (!found) {
            snprintf(pattern, sizeof(pattern), "%s/.var/app/net.pcsx2.PCSX2/config/PCSX2/bios/*.[bB][iI][nN]", home);
            if (glob(pattern, 0, NULL, &g) == 0) {
                if (g.gl_pathc > 0) found = 1;
                globfree(&g);
            }
        }
    } else if (strcmp(sys_name, "ps1") == 0) {
        snprintf(pattern, sizeof(pattern), "%s/Emulation/bios/ps1/*.[bB][iI][nN]", home);
        if (glob(pattern, 0, NULL, &g) == 0) {
            if (g.gl_pathc > 0) found = 1;
            globfree(&g);
        }
        if (!found) {
            snprintf(pattern, sizeof(pattern), "%s/.var/app/org.duckstation.DuckStation/config/duckstation/bios/*.[bB][iI][nN]", home);
            if (glob(pattern, 0, NULL, &g) == 0) {
                if (g.gl_pathc > 0) found = 1;
                globfree(&g);
            }
        }
    } else {
        found = 1;
    }
    return found;
}

static void prompt_missing_bios(const char *sys_name) {
    const char *home = getenv("HOME");
    char bios_path[512] = {0};
    if (home) {
        snprintf(bios_path, sizeof(bios_path), "%s/Emulation/bios/%s", home, sys_name);
    }

    const char *title = strcmp(sys_name, "ps1") == 0 ? "PlayStation 1 BIOS Required" : "PlayStation 2 BIOS Required";
    char msg[512];
    snprintf(msg, sizeof(msg), "Place your %s BIOS dump (.bin) into ~/Emulation/bios/%s to play.",
             strcmp(sys_name, "ps1") == 0 ? "PS1" : "PS2", sys_name);
    send_notification(title, msg);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "zenity --question --title='AxisOS Game Station' "
        "--text='<b>%s</b>\\n\\nTo play %s games, please place your legitimate console BIOS dump (.bin) into:\\n<tt>%s</tt>\\n\\nAxisOS does not bundle proprietary BIOS files.\\nWould you like to open the BIOS directory now?' "
        "--ok-label='Open BIOS Folder' --cancel-label='Continue Anyway' 2>/dev/null",
        title, strcmp(sys_name, "ps1") == 0 ? "PlayStation 1" : "PlayStation 2", bios_path);
    int ret = system(cmd);
    if (WIFEXITED(ret) && WEXITSTATUS(ret) == 0) {
        char open_cmd[512];
        snprintf(open_cmd, sizeof(open_cmd), "xdg-open \"%s\" 2>/dev/null &", bios_path);
        system(open_cmd);
    }
}

static void ensure_emulator_installed(const char *app_id, const char *friendly_name) {
    char check_cmd[256];
    snprintf(check_cmd, sizeof(check_cmd), "flatpak info %s >/dev/null 2>&1", app_id);
    if (system(check_cmd) != 0) {
        char notif_msg[256];
        snprintf(notif_msg, sizeof(notif_msg), "Setting up %s for first run...", friendly_name);
        send_notification("AxisOS Game Station", notif_msg);

        system("flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo 2>/dev/null || true");
        
        char install_cmd[512];
        snprintf(install_cmd, sizeof(install_cmd), "flatpak install -y --user flathub %s", app_id);
        system(install_cmd);

        char override_cmd[512];
        snprintf(override_cmd, sizeof(override_cmd),
            "flatpak override --user --socket=wayland --socket=fallback-x11 --device=dri --socket=pulseaudio --filesystem=host %s 2>/dev/null || true",
            app_id);
        system(override_cmd);
    }
}

static const char *detect_system(const char *rom_path) {
    if (!rom_path) return "ps2";

    char lower[512];
    strncpy(lower, rom_path, sizeof(lower) - 1);
    lower[sizeof(lower) - 1] = '\0';
    for (int i = 0; lower[i]; i++) lower[i] = (char)tolower(lower[i]);

    if (strstr(lower, ".pbp") || strstr(lower, "/psp/")) return "psp";
    if (strstr(lower, ".pkg") || strstr(lower, ".sfb") || strstr(lower, "/ps3/")) return "ps3";
    if (strstr(lower, ".vpk") || strstr(lower, "/psvita/")) return "psvita";
    if (strstr(lower, "/ps4/")) return "ps4";
    if (strstr(lower, ".cue") || strstr(lower, "/ps1/")) return "ps1";
    if (strstr(lower, ".cso") || strstr(lower, ".chd") || strstr(lower, "/ps2/")) return "ps2";

    if (strstr(lower, ".iso") || strstr(lower, ".bin")) {
        if (strstr(lower, "ps1") || strstr(lower, "playstation 1")) return "ps1";
        if (strstr(lower, "ps3") || strstr(lower, "playstation 3")) return "ps3";
        if (strstr(lower, "psp")) return "psp";
        if (strstr(lower, "ps4")) return "ps4";
        return "ps2";
    }

    return "ps2";
}

int main(int argc, char *argv[]) {
    ensure_emulation_directories();

    // Trigger GameBoost (performance governor & sysctl)
    if (access("/usr/local/bin/axis-game-boost", X_OK) == 0) {
        system("/usr/local/bin/axis-game-boost >/dev/null 2>&1 &");
    }

    // Ensure udev & controller kernel drivers are loaded
    system("modprobe uinput 2>/dev/null || true");
    system("modprobe hid-sony 2>/dev/null || true");
    system("modprobe hid-playstation 2>/dev/null || true");

    if (argc < 2) {
        printf("AxisOS Universal PlayStation Game Runner (Pure C11)\n");
        printf("Usage: axis-game-runner [ps1|ps2|ps3|psp|psvita|ps4] [game_path]\n");
        printf("       axis-game-runner <game_file_path>\n");
        return 0;
    }

    const char *sys_name = NULL;
    const char *game_path = NULL;

    if (strcmp(argv[1], "ps1") == 0 || strcmp(argv[1], "ps2") == 0 ||
        strcmp(argv[1], "ps3") == 0 || strcmp(argv[1], "psp") == 0 ||
        strcmp(argv[1], "psvita") == 0 || strcmp(argv[1], "ps4") == 0) {
        sys_name = argv[1];
        if (argc >= 3) {
            game_path = argv[2];
        }
    } else {
        game_path = argv[1];
        sys_name = detect_system(game_path);
    }

    if ((strcmp(sys_name, "ps1") == 0 || strcmp(sys_name, "ps2") == 0) && !has_bios(sys_name)) {
        prompt_missing_bios(sys_name);
    }

    const char *app_id = NULL;
    const char *friendly_name = NULL;
    char launch_cmd[1024];

    if (strcmp(sys_name, "ps1") == 0) {
        app_id = "org.duckstation.DuckStation";
        friendly_name = "DuckStation (PS1)";
        ensure_emulator_installed(app_id, friendly_name);
        if (game_path) {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland --env=GDK_BACKEND=wayland %s -fullscreen \"%s\"",
                app_id, game_path);
        } else {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland --env=GDK_BACKEND=wayland %s",
                app_id);
        }
    } else if (strcmp(sys_name, "ps2") == 0) {
        app_id = "net.pcsx2.PCSX2";
        friendly_name = "PCSX2 (PS2)";
        ensure_emulator_installed(app_id, friendly_name);
        if (game_path) {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland --env=GDK_BACKEND=wayland %s -fullscreen -nogui \"%s\"",
                app_id, game_path);
        } else {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland --env=GDK_BACKEND=wayland %s",
                app_id);
        }
    } else if (strcmp(sys_name, "ps3") == 0) {
        app_id = "net.rpcs3.RPCS3";
        friendly_name = "RPCS3 (PS3)";
        ensure_emulator_installed(app_id, friendly_name);
        if (game_path) {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland --env=GDK_BACKEND=wayland %s --no-gui \"%s\"",
                app_id, game_path);
        } else {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland --env=GDK_BACKEND=wayland %s",
                app_id);
        }
    } else if (strcmp(sys_name, "psp") == 0) {
        app_id = "org.ppsspp.PPSSPP";
        friendly_name = "PPSSPP (PSP)";
        ensure_emulator_installed(app_id, friendly_name);
        if (game_path) {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland --env=SDL_VIDEODRIVER=wayland %s --fullscreen \"%s\"",
                app_id, game_path);
        } else {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland --env=SDL_VIDEODRIVER=wayland %s",
                app_id);
        }
    } else if (strcmp(sys_name, "psvita") == 0) {
        app_id = "org.vita3k.Vita3K";
        friendly_name = "Vita3K (PS Vita)";
        ensure_emulator_installed(app_id, friendly_name);
        if (game_path) {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland %s -r \"%s\"",
                app_id, game_path);
        } else {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland %s",
                app_id);
        }
    } else if (strcmp(sys_name, "ps4") == 0) {
        app_id = "net.shadps4.shadPS4";
        friendly_name = "shadPS4 (PS4)";
        ensure_emulator_installed(app_id, friendly_name);
        if (game_path) {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland %s -g \"%s\"",
                app_id, game_path);
        } else {
            snprintf(launch_cmd, sizeof(launch_cmd),
                "flatpak run --env=QT_QPA_PLATFORM=wayland %s",
                app_id);
        }
    }

    printf("[AxisOS Game Station] Launching %s (%s)...\n", friendly_name, sys_name);
    int ret = system(launch_cmd);

    // After exit: restore desktop compositor focus
    system("axis-compositor --wake 2>/dev/null || true");

    return WEXITSTATUS(ret);
}
