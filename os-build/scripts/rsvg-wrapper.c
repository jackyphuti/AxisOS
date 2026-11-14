#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: rsvg-wrapper <input.svg> <output.png> [width] [height]\n");
        return 1;
    }
    char cmd[512];
    if (argc >= 5) {
        snprintf(cmd, sizeof(cmd), "rsvg-convert -w %s -h %s \"%s\" -o \"%s\" 2>/dev/null", argv[3], argv[4], argv[1], argv[2]);
    } else {
        snprintf(cmd, sizeof(cmd), "rsvg-convert \"%s\" -o \"%s\" 2>/dev/null", argv[1], argv[2]);
    }
    return system(cmd);
}
