#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("[AxisOS Hardware] Checking for NVIDIA graphics adapters...\n");
    int res = system("lspci 2>/dev/null | grep -qi nvidia");
    if (res == 0) {
        printf("[AxisOS Hardware] NVIDIA GPU detected. Configuring drivers.\n");
    } else {
        printf("[AxisOS Hardware] No NVIDIA adapter found.\n");
    }
    return 0;
}
