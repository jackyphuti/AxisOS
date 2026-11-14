#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("[AxisOS Chroot] Compiling pure C native applications in chroot environment...\n");
    system("os-build/scripts/compile-native-apps 2>/dev/null || true");
    return 0;
}
