#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

static void trim_whitespace(char *str) {
    if (!str) return;
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
}

static void launch_browser_async(const char *url) {
    printf("[AxisOS Network] Opening dedicated window for Wi-Fi portal login -> %s\n", url);

    // Send a desktop notification via notify-send
    pid_t notif_pid = fork();
    if (notif_pid == 0) {
        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        char *notif_argv[] = {
            "notify-send",
            "-i", "network-wireless-symbolic",
            "-a", "AxisOS Network",
            "Captive Portal Detected",
            "Please log in via the browser to access the internet.",
            NULL
        };
        execvp("notify-send", notif_argv);
        _exit(0);
    }

    // Launch browser asynchronously
    pid_t pid = fork();
    if (pid == 0) {
        setenv("GDK_BACKEND", "wayland", 1);
        setenv("OZONE_PLATFORM", "wayland", 1);
        setenv("MOZ_ENABLE_WAYLAND", "1", 1);

        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }

        char app_arg[512];
        snprintf(app_arg, sizeof(app_arg), "--app=%s", url);

        // 1. Try Chromium / Google Chrome with dedicated app window
        char *chrom_argv[] = {
            "chromium",
            app_arg,
            "--window-size=900,680",
            "--no-first-run",
            "--no-default-browser-check",
            NULL
        };
        execvp("chromium", chrom_argv);

        char *chrome_argv[] = {
            "google-chrome",
            app_arg,
            "--window-size=900,680",
            NULL
        };
        execvp("google-chrome", chrome_argv);

        // 2. Try Firefox new window
        char *fox_argv[] = {
            "firefox",
            "--new-window",
            (char *)url,
            NULL
        };
        execvp("firefox", fox_argv);

        // 3. Fallback to xdg-open
        char *xdg_argv[] = {
            "xdg-open",
            (char *)url,
            NULL
        };
        execvp("xdg-open", xdg_argv);

        _exit(1);
    }
}

int main(void) {
    const char *probe_urls[] = {
        "http://connectivitycheck.gstatic.com/generate_204",
        "http://nmcheck.gnome.org/check_network_status.txt",
        "http://detectportal.firefox.com/canonical.html",
        "http://neverssl.com",
        NULL
    };

    char target_url[512] = {0};

    // 1. Check effective redirection URL with curl -L -w "%{url_effective}"
    for (int i = 0; probe_urls[i] != NULL; i++) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "curl -s -L -o /dev/null -w \"%%{url_effective}\" -m 4 \"%s\" 2>/dev/null", probe_urls[i]);
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char eff_url[512] = {0};
            if (fgets(eff_url, sizeof(eff_url), fp)) {
                trim_whitespace(eff_url);
                if (strncmp(eff_url, "http://", 7) == 0 || strncmp(eff_url, "https://", 8) == 0) {
                    // Check if it redirected away from the original test URL
                    if (strstr(eff_url, "generate_204") == NULL &&
                        strstr(eff_url, "check_network_status") == NULL &&
                        strstr(eff_url, "canonical.html") == NULL &&
                        strstr(eff_url, "neverssl.com") == NULL) {
                        snprintf(target_url, sizeof(target_url), "%s", eff_url);
                        pclose(fp);
                        break;
                    }
                }
            }
            pclose(fp);
        }

        // Also check HTTP 302 Location header
        snprintf(cmd, sizeof(cmd), "curl -s -I -m 3 \"%s\" 2>/dev/null | grep -i '^Location:' | awk '{print $2}' | tr -d '\\r\\n'", probe_urls[i]);
        fp = popen(cmd, "r");
        if (fp) {
            char loc[512] = {0};
            if (fgets(loc, sizeof(loc), fp)) {
                if (strncmp(loc, "http://", 7) == 0 || strncmp(loc, "https://", 8) == 0) {
                    snprintf(target_url, sizeof(target_url), "%s", loc);
                    pclose(fp);
                    break;
                }
            }
            pclose(fp);
        }
    }

    // 2. Fallback to default gateway IP if probe didn't capture a distinct URL
    if (strlen(target_url) == 0) {
        FILE *gfp = popen("ip route 2>/dev/null | awk '/default/ {print $3}'", "r");
        if (gfp) {
            char gw[128] = {0};
            if (fgets(gw, sizeof(gw), gfp)) {
                char *nl = strchr(gw, '\n');
                if (nl) *nl = '\0';
                if (strlen(gw) > 6) {
                    snprintf(target_url, sizeof(target_url), "http://%s", gw);
                }
            }
            pclose(gfp);
        }
    }

    if (strlen(target_url) == 0) {
        snprintf(target_url, sizeof(target_url), "%s", "http://neverssl.com");
    }

    const char *disp = getenv("DISPLAY");
    const char *wdisp = getenv("WAYLAND_DISPLAY");
    if (disp || wdisp) {
        launch_browser_async(target_url);
        return 0;
    }

    sleep(1);
    FILE *nfp = popen("nmcli networking connectivity check 2>/dev/null || echo \"unknown\"", "r");
    char conn[64] = {0};
    if (nfp) {
        if (fgets(conn, sizeof(conn), nfp)) {
            char *nl = strchr(conn, '\n');
            if (nl) *nl = '\0';
        }
        pclose(nfp);
    }

    if (strstr(conn, "portal") != NULL || strstr(conn, "limited") != NULL) {
        launch_browser_async(target_url);
    }

    return 0;
}
