#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    (void)argc;
    const char *candidates[] = {
        "/usr/local/bin/axisos-settings-panel",
        "/usr/bin/axisos-settings-panel",
        NULL
    };
    for (int i = 0; candidates[i] != NULL; i++) {
        if (access(candidates[i], X_OK) == 0) {
            execv(candidates[i], argv);
        }
    }
    execvp("axisos-settings-panel", argv);
    fprintf(stderr, "Error: axisos-settings-panel binary not found.\n");
    return 1;
}
