#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("======================================================================\n");
    printf("          AxisOS Pure C Biometrics & Fprint Engine                    \n");
    printf("======================================================================\n");

    if (argc < 2) {
        printf("Usage: axis-biometrics [status | enroll <user> | verify <user>]\n");
        return 0;
    }

    if (strcmp(argv[1], "status") == 0) {
        printf("[AxisOS Biometrics] Querying Fprint daemon...\n");
        system("fprintd-list \"$USER\" 2>/dev/null || echo 'No enrolled biometric devices found.'");
    } else if (strcmp(argv[1], "enroll") == 0) {
        const char *user = (argc >= 3) ? argv[2] : getenv("USER");
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "fprintd-enroll \"%s\" 2>/dev/null", user ? user : "root");
        system(cmd);
    } else if (strcmp(argv[1], "verify") == 0) {
        const char *user = (argc >= 3) ? argv[2] : getenv("USER");
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "fprintd-verify \"%s\" 2>/dev/null", user ? user : "root");
        system(cmd);
    }

    return 0;
}
