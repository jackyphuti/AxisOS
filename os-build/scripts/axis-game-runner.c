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

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "mkdir -p \"%s/Emulation/bios/ps1\" \"%s/Emulation/bios/ps2\" \"%s/Emulation/bios/ps3\" \"%s/Emulation/bios/psp\" "
        "\"%s/Emulation/games/ps1\" \"%s/Emulation/games/ps2\" \"%s/Emulation/games/ps3\" \"%s/Emulation/games/psp\" 2>/dev/null",
        home, home, home, home, home, home, home, home);
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

    send_notification("PlayStation 2 BIOS Required",
        "Place your PS2 BIOS (.bin) into ~/Emulation/bios/ps2 to start playing.");

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
        "zenity --question --title='AxisOS Game Station' "
        "--text='<b>PlayStation 2 BIOS Required</b>\\n\\nTo play PlayStation 2 games, please place your legitimate PS2 BIOS dump (.bin) into:\\n<tt>%s</tt>\\n\\nWould you like to open the BIOS folder now?' "
        "--ok-label='Open BIOS Folder' --cancel-label='Continue Anyway' 2>/dev/null",
        bios_path);
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
    for (char *p = lower; *p; p++) *p = (char)tolower((unsigned char)*p);

    if (strstr(lower, ".pkg") || strstr(lower, ".sfb") || strstr(lower, "ps3_game") || strstr(lower, "/ps3/")) {
        return "ps3";
    }
    if (strstr(lower, ".pbp") || strstr(lower, "/psp/")) {
        return "psp";
    }
    if (strstr(lower, ".cue") || strstr(lower, ".mdf") || strstr(lower, "/ps1/")) {
        return "ps1";
    }
    // Default disc image (.iso, .chd, .cso) is PS2
    return "ps2";
}

int main(int argc, char *argv[]) {
    ensure_emulation_directories();

    const char *system_type = NULL;
    const char *rom_path = NULL;

    if (argc == 1) {
        // No args: open games directory or general hub
        const char *home = getenv("HOME");
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "xdg-open \"%s/Emulation/games\" 2>/dev/null || true", home ? home : "/home/axis");
        system(cmd);
        return 0;
    }

    if (argc >= 2) {
        if (strcmp(argv[1], "ps1") == 0 || strcmp(argv[1], "ps2") == 0 ||
            strcmp(argv[1], "ps3") == 0 || strcmp(argv[1], "psp") == 0) {
            system_type = argv[1];
            if (argc >= 3) {
                rom_path = argv[2];
            }
        } else {
            rom_path = argv[1];
            system_type = detect_system(rom_path);
        }
    }

    printf("[AxisOS Game Station] System: %s | ROM: %s\n", system_type, rom_path ? rom_path : "(GUI Launcher)");

    const char *app_id = "net.pcsx2.PCSX2";
    const char *name = "PlayStation 2";

    if (strcmp(system_type, "ps3") == 0) {
        app_id = "net.rpcs3.RPCS3";
        name = "PlayStation 3";
    } else if (strcmp(system_type, "ps1") == 0) {
        app_id = "org.duckstation.DuckStation";
        name = "PlayStation 1";
    } else if (strcmp(system_type, "psp") == 0) {
        app_id = "org.ppsspp.PPSSPP";
        name = "PlayStation Portable";
    }

    // 1. Hardware Tuning & Kernel Drivers
    system("modprobe -q uinput hid-sony hid-playstation 2>/dev/null || true");
    system("axis-game-boost >/dev/null 2>&1 &");

    // 2. Ensure Emulator is Ready
    ensure_emulator_installed(app_id, name);

    // 3. BIOS Check for PS2
    if (strcmp(system_type, "ps2") == 0 && !has_bios("ps2")) {
        prompt_missing_bios("ps2");
    }

    // 4. Configure Wayland Environment
    setenv("GDK_BACKEND", "wayland", 1);
    setenv("QT_QPA_PLATFORM", "wayland", 1);
    setenv("SDL_VIDEODRIVER", "wayland", 1);
    setenv("MOZ_ENABLE_WAYLAND", "1", 1);

    char notif_start[256];
    snprintf(notif_start, sizeof(notif_start), "Starting %s...", name);
    send_notification("AxisOS Game Station", notif_start);

    // 5. Build Execution Arguments
    char *exec_args[16];
    int arg_idx = 0;
    exec_args[arg_idx++] = "flatpak";
    exec_args[arg_idx++] = "run";
    exec_args[arg_idx++] = (char *)app_id;

    if (strcmp(system_type, "ps2") == 0) {
        exec_args[arg_idx++] = "-fullscreen";
        if (rom_path) {
            exec_args[arg_idx++] = "-batch";
            exec_args[arg_idx++] = (char *)rom_path;
        }
    } else if (strcmp(system_type, "ps3") == 0) {
        exec_args[arg_idx++] = "--fullscreen";
        if (rom_path) {
            exec_args[arg_idx++] = (char *)rom_path;
        }
    } else if (strcmp(system_type, "ps1") == 0) {
        exec_args[arg_idx++] = "-fullscreen";
        if (rom_path) {
            exec_args[arg_idx++] = "-batch";
            exec_args[arg_idx++] = (char *)rom_path;
        }
    } else if (strcmp(system_type, "psp") == 0) {
        if (rom_path) {
            exec_args[arg_idx++] = (char *)rom_path;
        }
    }
    exec_args[arg_idx] = NULL;

    // 6. Launch Emulator Process
    pid_t pid = fork();
    if (pid == 0) {
        execvp("flatpak", exec_args);
        _exit(1);
    }

    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        printf("[AxisOS Game Station] Game exited. Restoring desktop.\n");
        send_notification("AxisOS Game Station", "Returned to desktop.");
    }

    return 0;
}
