#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    printf("======================================================================\n");
    printf("          AxisOS System Upgrade Engine                                \n");
    printf("======================================================================\n");

    if (access("/usr/local/bin/axis-update", X_OK) == 0) {
        system("/usr/local/bin/axis-update --check");
    } else {
        system("axis update 2>/dev/null || apt update 2>/dev/null || true");
    }

    return 0;
}
