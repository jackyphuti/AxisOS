#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    setenv("WLR_RENDERER_ALLOW_SOFTWARE", "1", 1);
    setenv("WLR_NO_HARDWARE_CURSORS", "1", 1);
    setenv("GDK_BACKEND", "wayland", 1);
    setenv("MOZ_ENABLE_WAYLAND", "1", 1);
    setenv("OZONE_PLATFORM", "wayland", 1);
    setenv("XDG_SESSION_TYPE", "wayland", 1);
    setenv("XDG_CURRENT_DESKTOP", "AxisOS", 1);

    const char *runtime_dir = getenv("XDG_RUNTIME_DIR");
    if (!runtime_dir || !*runtime_dir) {
        char runtime_buf[128];
        snprintf(runtime_buf, sizeof(runtime_buf), "/run/user/%d", getuid());
        mkdir(runtime_buf, 0700);
        setenv("XDG_RUNTIME_DIR", runtime_buf, 1);
    }

    const char *candidates[] = {
        "/usr/local/bin/axis-compositor",
        "/usr/bin/axis-compositor",
        "/usr/local/bin/axisos-shell",
        "/usr/bin/axisos-shell",
        "/usr/local/bin/LiquidNodesOS",
        "/usr/bin/LiquidNodesOS",
        NULL
    };

    for (int i = 0; candidates[i] != NULL; i++) {
        if (access(candidates[i], X_OK) == 0) {
            execv(candidates[i], argv);
        }
    }

    fprintf(stderr, "Error: Neither axis-compositor nor axisos-shell could be found.\n");
    return 1;
}
