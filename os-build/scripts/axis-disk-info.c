#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/statvfs.h>

/**
 * AxisOS Storage & Disk Partition Telemetry (axis-disk-info)
 * Strictly Pure C11 (-std=gnu11 / -std=c17)
 *
 * Scans physical disks, NVMe, SATA, SSD/HDD rotational status,
 * and formats telemetry for axis-installer and system diagnostics.
 */

static void trim_trailing(char *s) {
    if (!s) return;
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' || s[len - 1] == ' ' || s[len - 1] == '\t')) {
        s[len - 1] = '\0';
        len--;
    }
}

int main(int argc, char **argv) {
    int human_mode = (argc > 1 && (strcmp(argv[1], "--human") == 0 || strcmp(argv[1], "-h") == 0));

    if (human_mode) {
        printf("======================================================================\n");
        printf("          AxisOS Storage & Disk Partition Telemetry                   \n");
        printf("======================================================================\n");
    }

    FILE *fp = popen("lsblk -d -p -n -l -o NAME,SIZE,ROTA,TYPE,MODEL 2>/dev/null", "r");
    if (fp) {
        char line[512];
        while (fgets(line, sizeof(line), fp)) {
            trim_trailing(line);
            if (!line[0]) continue;

            char dev[64] = {0};
            char size[32] = {0};
            int rota = 0;
            char type[32] = {0};
            char model[128] = {0};

            // Parse columns: NAME SIZE ROTA TYPE [MODEL...]
            int matched = sscanf(line, "%63s %31s %d %31s %127[^\n]", dev, size, &rota, type, model);
            if (matched >= 4) {
                // Filter out non-disk block devices (loop, zram, ram)
                if (strstr(dev, "/dev/loop") || strstr(dev, "/dev/zram") || strstr(dev, "/dev/ram")) {
                    continue;
                }
                if (strcmp(type, "disk") != 0 && strcmp(type, "rom") == 0) {
                    continue;
                }

                trim_trailing(model);
                if (!model[0]) {
                    snprintf(model, sizeof(model), "Generic Storage Device");
                }

                const char *drive_tech = (rota == 0) ? "SSD/NVMe" : "HDD";

                if (human_mode) {
                    printf("  Drive: %-16s Size: %-8s Type: %-8s Tech: %-8s Model: %s\n",
                           dev, size, type, drive_tech, model);
                } else {
                    // Pipe-separated format expected by axis-installer:
                    // DEV|CAPACITY|FREE_SPACE|DESCRIPTION
                    printf("%s|%s|Unallocated|%s (%s)\n", dev, size, model, drive_tech);
                }
            }
        }
        pclose(fp);
    }

    if (human_mode) {
        struct statvfs sv;
        if (statvfs("/", &sv) == 0) {
            unsigned long long total = (unsigned long long)sv.f_blocks * sv.f_frsize;
            unsigned long long avail = (unsigned long long)sv.f_bavail * sv.f_frsize;
            printf("----------------------------------------------------------------------\n");
            printf("  Root (/) Total: %.1f GB | Available: %.1f GB\n",
                   (double)total / (1024.0 * 1024.0 * 1024.0),
                   (double)avail / (1024.0 * 1024.0 * 1024.0));
        }
    }

    return 0;
}
