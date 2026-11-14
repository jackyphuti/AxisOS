#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    printf("==================================================\n");
    printf("      AxisOS Chroot & Bootable ISO Generator      \n");
    printf("==================================================\n");

    printf("[1/3] Preparing live-build workspace...\n");
    system("mkdir -p os-build/work 2>/dev/null || true");

    printf("[2/3] Configuring Debian live chroot...\n");
    system("lb config --distribution bookworm --architecture amd64 2>/dev/null || true");

    printf("[3/3] Building live ISO system image...\n");
    system("lb build 2>/dev/null || true");

    return 0;
}
