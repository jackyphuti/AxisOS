#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    (void)argc;
    const char *candidates[] = {
        "/usr/local/bin/axis-installer",
        "/usr/bin/axis-installer",
        NULL
    };
    for (int i = 0; candidates[i] != NULL; i++) {
        if (access(candidates[i], X_OK) == 0) {
            execv(candidates[i], argv);
        }
    }
    execvp("axis-installer", argv);
    fprintf(stderr, "Error: axis-installer binary not found.\n");
    return 1;
}
