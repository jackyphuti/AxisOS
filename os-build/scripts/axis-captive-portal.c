#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void launch_browser(const char *url) {
    printf("[AxisOS Network] Opening web browser for Wi-Fi portal login -> %s\n", url);
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "chromium --new-window --no-sandbox \"%s\" 2>/dev/null || firefox --new-window \"%s\" 2>/dev/null || xdg-open \"%s\" 2>/dev/null", url, url, url);
    system(cmd);
}

int main(void) {
    const char *probe_urls[] = {
        "http://connectivitycheck.gstatic.com/generate_204",
        "http://detectportal.firefox.com/canonical.html",
        "http://www.msftconnecttest.com/connecttest.txt",
        "http://neverssl.com",
        NULL
    };

    char target_url[256] = {0};

    for (int i = 0; probe_urls[i] != NULL; i++) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "curl -s -I -m 3 \"%s\" 2>/dev/null | grep -i '^Location:' | awk '{print $2}' | tr -d '\\r\\n'", probe_urls[i]);
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char loc[256] = {0};
            if (fgets(loc, sizeof(loc), fp)) {
                if (strncmp(loc, "http://", 7) == 0 || strncmp(loc, "https://", 8) == 0) {
                    strncpy(target_url, loc, sizeof(target_url) - 1);
                    pclose(fp);
                    break;
                }
            }
            pclose(fp);
        }
    }

    if (strlen(target_url) == 0) {
        strncpy(target_url, "http://neverssl.com", sizeof(target_url) - 1);
    }

    const char *disp = getenv("DISPLAY");
    const char *wdisp = getenv("WAYLAND_DISPLAY");
    if (disp || wdisp) {
        launch_browser(target_url);
        return 0;
    }

    sleep(2);
    FILE *nfp = popen("nmcli networking connectivity check 2>/dev/null || echo \"unknown\"", "r");
    char conn[64] = {0};
    if (nfp) {
        if (fgets(conn, sizeof(conn), nfp)) {
            char *nl = strchr(conn, '\n');
            if (nl) *nl = '\0';
        }
        pclose(nfp);
    }

    if (strstr(conn, "portal") != NULL) {
        launch_browser(target_url);
    }

    return 0;
}
