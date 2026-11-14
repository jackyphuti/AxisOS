#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("[AxisOS GPU] Autodetecting graphics hardware...\n");
    FILE *fp = popen("lspci 2>/dev/null | grep -iE 'vga|3d|display'", "r");
    char line[256];
    int detected = 0;
    if (fp) {
        while (fgets(line, sizeof(line), fp)) {
            printf("  -> %s", line);
            detected = 1;
        }
        pclose(fp);
    }
    if (!detected) {
        printf("  -> Virtualized / DRM KMS Wayland display device\n");
    }
    return 0;
}
