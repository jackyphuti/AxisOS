#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    const char *target = "/usr/local/bin/axisos-settings-panel";
    if (access(target, X_OK) != 0) {
        target = "/usr/bin/axisos-settings-panel";
    }

    char **new_argv = malloc((argc + 2) * sizeof(char *));
    if (!new_argv) return 1;

    new_argv[0] = (char *)target;
    int idx = 1;

    if (argc > 1) {
        if (strcmp(argv[1], "about") == 0 || strcmp(argv[1], "--about") == 0 || strcmp(argv[1], "-a") == 0) {
            new_argv[idx++] = "about";
        } else if (strcmp(argv[1], "updates") == 0 || strcmp(argv[1], "--updates") == 0 || strcmp(argv[1], "-u") == 0) {
            new_argv[idx++] = "updates";
        } else {
            for (int i = 1; i < argc; i++) {
                new_argv[idx++] = argv[i];
            }
        }
    }
    new_argv[idx] = NULL;

    execv(target, new_argv);
    execvp("axisos-settings-panel", new_argv);

    perror("Failed to launch axisos-settings-panel");
    free(new_argv);
    return 1;
}
