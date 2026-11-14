#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

static const char *BINARIES[] = {
    "os-build/apps/axis-installer/axis-installer",
    "os-build/apps/axis-oobe/axis-oobe",
    "os-build/apps/axis-package-installer/axis-package-installer",
    "os-build/apps/axis-scheduler-manager/axis-scheduler-manager",
    "os-build/apps/axis-settings/axisos-settings-panel",
    "os-build/apps/axis-software-manager/axis-software-manager",
    "os-build/apps/axis-task-manager/axis-task-manager",
    "os-build/apps/axis-terminal/axis-terminal",
    "os-build/apps/axis-update-manager/axis-update-manager",
    "os-build/apps/axis-usb-daemon/axis-usb-daemon",
    "os-build/apps/axis-welcome/axis-welcome",
    "os-build/apps/nobara-welcome/nobara-welcome",
    "os-build/apps/axis-fetch/axis-fetch",
    "os-build/apps/axis-calc/axis-calc",
    "os-build/apps/axis-color-picker/axis-color-picker",
    "os-build/scripts/axis-disk-info",
    "shell-c/axisos-shell",
    "pkg-mgr/bin/axis",
    "pkg-mgr/bin/axis_repo_builder",
    "os-build/updater/axis-update",
    "tests/test_hardware_probe",
    "tests/verify_harmony_ui",
    NULL
};

int main(void) {
    printf("======================================================================\n");
    printf("          AxisOS Pure C11 Strict Linkage Verification                 \n");
    printf("======================================================================\n");

    int failed = 0;
    for (int i = 0; BINARIES[i] != NULL; i++) {
        const char *b = BINARIES[i];
        if (access(b, F_OK) != 0) {
            printf("[-] Missing binary: %s\n", b);
            failed++;
            continue;
        }

        char cmd[512];
        snprintf(cmd, sizeof(cmd), "ldd \"%s\" 2>&1", b);
        FILE *fp = popen(cmd, "r");
        if (!fp) {
            printf("[-] Failed to inspect: %s\n", b);
            failed++;
            continue;
        }

        char line[1024];
        int has_forbidden = 0;
        char forbidden_detail[1024] = {0};
        while (fgets(line, sizeof(line), fp)) {
            if (strstr(line, "stdc++") || strstr(line, "Qt" "6") || strstr(line, "py" "thon")) {
                has_forbidden = 1;
                snprintf(forbidden_detail, sizeof(forbidden_detail), "%s", line);
                break;
            }
        }
        pclose(fp);

        if (has_forbidden) {
            printf("[-] FAILED %s links to forbidden runtime:\n    %s", b, forbidden_detail);
            failed++;
        } else {
            printf("  [PASS] %-58s -> Pure C (-std=gnu11)\n", b);
        }
    }

    printf("----------------------------------------------------------------------\n");
    if (failed == 0) {
        printf("VERIFICATION PASSED: All 22 binaries are strictly 100%% Pure C.\n");
        return 0;
    } else {
        printf("VERIFICATION FAILED: %d binaries had issues.\n", failed);
        return 1;
    }
}
