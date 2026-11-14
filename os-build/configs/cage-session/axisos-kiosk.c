#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    setenv("XDG_SESSION_TYPE", "wayland", 1);
    setenv("XDG_CURRENT_DESKTOP", "AxisOS", 1);
    setenv("GDK_BACKEND", "wayland", 1);
    setenv("WLR_RENDERER_ALLOW_SOFTWARE", "1", 1);
    setenv("WLR_NO_HARDWARE_CURSORS", "1", 1);
    setenv("MOZ_ENABLE_WAYLAND", "1", 1);
    setenv("OZONE_PLATFORM", "wayland", 1);

    unsetenv("WAYLAND_DISPLAY");
    unsetenv("DISPLAY");

    const char *candidates[] = {
        "/usr/local/bin/axis-shell",
        "/usr/local/bin/axisos-shell",
        "/usr/local/bin/axis-compositor",
        NULL
    };

    for (int i = 0; candidates[i] != NULL; i++) {
        if (access(candidates[i], X_OK) == 0) {
            execl(candidates[i], candidates[i], NULL);
        }
    }
    return 1;
}
