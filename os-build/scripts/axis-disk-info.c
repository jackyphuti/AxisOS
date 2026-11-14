#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/statvfs.h>

int main(void) {
    printf("======================================================================\n");
    printf("          AxisOS Storage & Disk Partition Telemetry                   \n");
    printf("======================================================================\n");

    FILE *fp = popen("lsblk -ndo NAME,SIZE,TYPE,MOUNTPOINT 2>/dev/null", "r");
    if (fp) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            printf("  %s", line);
        }
        pclose(fp);
    }

    struct statvfs sv;
    if (statvfs("/", &sv) == 0) {
        unsigned long long total = (unsigned long long)sv.f_blocks * sv.f_frsize;
        unsigned long long avail = (unsigned long long)sv.f_bavail * sv.f_frsize;
        printf("Root Filesystem (/) Total: %.1f GB | Available: %.1f GB\n",
               (double)total / (1024.0 * 1024.0 * 1024.0),
               (double)avail / (1024.0 * 1024.0 * 1024.0));
    }

    return 0;
}
