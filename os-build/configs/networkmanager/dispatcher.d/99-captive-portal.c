#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc >= 3 && strcmp(argv[2], "up") == 0) {
        if (access("/usr/local/bin/axis-captive-portal", X_OK) == 0) {
            system("/usr/local/bin/axis-captive-portal &");
        }
    }
    return 0;
}
