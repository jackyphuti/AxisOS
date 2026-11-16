#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void print_usage(const char *prog) {
    printf("AxisOS Session Switcher (Pure C11)\n");
    printf("Usage: %s [desktop | gamemode | status]\n\n", prog);
    printf("Commands:\n");
    printf("  desktop     Switch and launch native LiquidNodes Pure C Desktop Shell\n");
    printf("  gamemode    Switch and launch gamescope Steam Big Picture console session\n");
    printf("  status      Check currently configured default session\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *target = argv[1];

    if (strcmp(target, "desktop") == 0) {
        printf("[AxisOS Session] Switching session to: Desktop (LiquidNodes wlroots)\n");
        system("systemctl disable axisos-gamemode.service 2>/dev/null || true");
        system("systemctl enable axisos.service 2>/dev/null || true");
        if (access("/run/systemd/system", F_OK) == 0) {
            system("systemctl stop axisos-gamemode.service 2>/dev/null || true");
            system("systemctl restart axisos.service 2>/dev/null || true");
        }
        return 0;
    } else if (strcmp(target, "gamemode") == 0) {
        printf("[AxisOS Session] Switching session to: Game Mode (gamescope)\n");
        system("systemctl disable axisos.service 2>/dev/null || true");
        system("systemctl enable axisos-gamemode.service 2>/dev/null || true");
        if (access("/run/systemd/system", F_OK) == 0) {
            system("systemctl stop axisos.service 2>/dev/null || true");
            system("systemctl restart axisos-gamemode.service 2>/dev/null || true");
        }
        return 0;
    } else if (strcmp(target, "status") == 0) {
        printf("[AxisOS Session Status]\n");
        int ret = system("systemctl is-enabled axisos-gamemode.service 2>/dev/null | grep -q enabled");
        if (ret == 0) {
            printf("  Active Session: Game Mode (gamescope)\n");
        } else {
            printf("  Active Session: Desktop Shell (LiquidNodes)\n");
        }
        return 0;
    } else {
        fprintf(stderr, "Unknown session target: %s\n", target);
        print_usage(argv[0]);
        return 1;
    }
}
