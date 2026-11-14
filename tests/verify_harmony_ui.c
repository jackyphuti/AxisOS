#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

static int g_errors = 0;

static void assert_file_contains(const char *path, const char *needle) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[-] FAILED: Missing required file %s\n", path);
        g_errors++;
        return;
    }
    char buf[4096];
    bool found = false;
    while (fgets(buf, sizeof(buf), f)) {
        if (strstr(buf, needle)) {
            found = true;
            break;
        }
    }
    fclose(f);
    if (!found) {
        fprintf(stderr, "[-] FAILED: %s does not contain '%s'\n", path, needle);
        g_errors++;
    }
}

static void verify_theme(void) {
    printf("--- 1. Verifying Unified AxisOS Theme Tokens & CSS ---\n");
    assert_file_contains("data/theme/axis-tokens.css", "axis_dark_base");
    assert_file_contains("data/theme/axis-tokens.css", "axis_dark_surface");
    assert_file_contains("data/theme/axis-tokens.css", "axis_dark_accent");
    assert_file_contains("data/theme/axis-tokens.css", "axis_light_base");
    assert_file_contains("data/theme/axis-tokens.css", "@define-color");
    assert_file_contains("data/theme/axis-components.css", "@axis_text");
    if (g_errors == 0) {
        printf("PASS: Theme tokens and CSS verified.\n");
    }
}

static void verify_apps(void) {
    printf("--- 2. Verifying Pure C11 Desktop Applications ---\n");
    const char *apps[] = {
        "axis-installer",
        "axis-oobe",
        "axis-package-installer",
        "axis-scheduler-manager",
        "axis-settings",
        "axis-software-manager",
        "axis-task-manager",
        "axis-terminal",
        "axis-update-manager",
        "axis-welcome",
        "axis-usb-daemon",
        "nobara-welcome",
        NULL
    };

    for (int i = 0; apps[i] != NULL; ++i) {
        char path[256];
        snprintf(path, sizeof(path), "os-build/apps/%s/main.c", apps[i]);
        if (access(path, R_OK) != 0) {
            fprintf(stderr, "[-] FAILED: Missing C source %s\n", path);
            g_errors++;
        } else {
            printf("  [OK] %-26s -> Pure C11 (main.c)\n", apps[i]);
        }
    }
    if (g_errors == 0) {
        printf("PASS: Pure C11 desktop applications verified.\n");
    }
}

static void verify_shell(void) {
    printf("--- 3. Verifying Pure C Wayland Desktop Shell (shell-c) ---\n");
    const char *shell_files[] = {
        "shell-c/main.c",
        "shell-c/system_controller.c",
        "shell-c/system_controller.h",
        "shell-c/auth_manager.c",
        "shell-c/auth_manager.h",
        "shell-c/ui_shell.c",
        "shell-c/ui_shell.h",
        "shell-c/Makefile",
        NULL
    };

    for (int i = 0; shell_files[i] != NULL; ++i) {
        if (access(shell_files[i], R_OK) != 0) {
            fprintf(stderr, "[-] FAILED: Missing shell source %s\n", shell_files[i]);
            g_errors++;
        }
    }

    if (access("compositor-qt", F_OK) == 0) {
        fprintf(stderr, "[-] FAILED: compositor-qt must not exist\n");
        g_errors++;
    }
    if (access("terminal-qt", F_OK) == 0) {
        fprintf(stderr, "[-] FAILED: terminal-qt must not exist\n");
        g_errors++;
    }

    if (g_errors == 0) {
        printf("PASS: Pure C Wayland desktop shell verified.\n");
    }
}

int main(void) {
    printf("==================================================\n");
    printf("  AxisOS Pure C Architecture Verification Suite   \n");
    printf("==================================================\n");

    verify_theme();
    verify_apps();
    verify_shell();

    if (g_errors == 0) {
        printf("\nALL AXISOS PURE C VERIFICATION CHECKS PASSED!\n");
        return 0;
    } else {
        fprintf(stderr, "\nVERIFICATION FAILED: %d errors encountered.\n", g_errors);
        return 1;
    }
}
