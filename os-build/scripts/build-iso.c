#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void) {
    printf("==================================================\n");
    printf("       AxisOS Pure C Bootable ISO Builder         \n");
    printf("==================================================\n");

    mkdir("os-build/work", 0755);

    printf("[1/3] Compiling Native Pure C Applications & Shell...\n");
    system("os-build/scripts/compile-native-apps 2>/dev/null || true");

    printf("[2/3] Preparing live-build workspace...\n");

    const char *tool = NULL;
    if (system("command -v podman >/dev/null 2>&1") == 0) {
        tool = "podman";
    } else if (system("command -v docker >/dev/null 2>&1") == 0) {
        tool = "docker";
    }

    if (tool) {
        printf("[3/3] Launching ISO generation in %s container...\n", tool);
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "%s run --privileged --rm -v \"$PWD:/build\" -w /build debian:bookworm bash os-build/scripts/inner-build.sh 2>/dev/null || true", tool);
        system(cmd);
    } else {
        printf("[3/3] Building native ISO via lb build...\n");
        system("bash os-build/scripts/inner-build.sh 2>/dev/null || true");
    }

    printf("ISO generation process finished.\n");
    return 0;
}
