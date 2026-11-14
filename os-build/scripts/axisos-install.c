#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("======================================================================\n");
    printf("          AxisOS Pure C CLI System Installer Engine                   \n");
    printf("======================================================================\n");

    if (argc < 2) {
        printf("Usage: axisos-install <target_disk> [hostname] [username]\n");
        printf("Example: axisos-install /dev/nvme0n1 axis-pc user\n");
        return 0;
    }

    const char *disk = argv[1];
    printf("[AxisOS Install] Target drive selected: %s\n", disk);
    printf("[AxisOS Install] Ready to launch system installation.\n");

    return 0;
}
