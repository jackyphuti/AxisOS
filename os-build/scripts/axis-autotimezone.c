#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    char tz[64] = {0};
    FILE *fp = popen("curl -s --max-time 4 https://ipapi.co/timezone 2>/dev/null", "r");
    if (fp) {
        if (fgets(tz, sizeof(tz), fp)) {
            char *nl = strchr(tz, '\n');
            if (nl) *nl = '\0';
        }
        pclose(fp);
    }

    if (strlen(tz) == 0 || strlen(tz) > 40) {
        fp = popen("curl -s --max-time 4 http://worldtimeapi.org/api/ip 2>/dev/null | grep -o '\"timezone\":\"[^\"]*\"' | cut -d'\"' -f4", "r");
        if (fp) {
            if (fgets(tz, sizeof(tz), fp)) {
                char *nl = strchr(tz, '\n');
                if (nl) *nl = '\0';
            }
            pclose(fp);
        }
    }

    if (strlen(tz) > 0) {
        char check_path[128];
        snprintf(check_path, sizeof(check_path), "/usr/share/zoneinfo/%s", tz);
        if (access(check_path, R_OK) == 0) {
            printf("[AxisOS Timezone] Auto-detected timezone: %s\n", tz);
            char cmd[128];
            snprintf(cmd, sizeof(cmd), "timedatectl set-timezone \"%s\" 2>/dev/null || true", tz);
            system(cmd);
        }
    }

    system("timedatectl set-ntp true 2>/dev/null || true");
    printf("[AxisOS Timezone] NTP clock synchronization enabled.\n");
    return 0;
}
