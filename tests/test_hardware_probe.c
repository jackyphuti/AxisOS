#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>

int main(void) {
    printf("=== Probing Hardware (Pure C11) ===\n");

    // 1. Device Model & Vendor
    char model[128] = "PC Device";
    char vendor[128] = "";

    const char *model_paths[] = {
        "/sys/class/dmi/id/product_name",
        "/sys/devices/virtual/dmi/id/product_name",
        NULL
    };
    for (int i = 0; model_paths[i] != NULL; ++i) {
        FILE *f = fopen(model_paths[i], "r");
        if (f) {
            char buf[128];
            if (fgets(buf, sizeof(buf), f)) {
                char *trim = strtok(buf, "\r\n");
                if (trim && *trim && strcmp(trim, "None") != 0) {
                    strncpy(model, trim, sizeof(model) - 1);
                    fclose(f);
                    break;
                }
            }
            fclose(f);
        }
    }

    const char *vendor_paths[] = {
        "/sys/class/dmi/id/sys_vendor",
        "/sys/devices/virtual/dmi/id/sys_vendor",
        NULL
    };
    for (int i = 0; vendor_paths[i] != NULL; ++i) {
        FILE *f = fopen(vendor_paths[i], "r");
        if (f) {
            char buf[128];
            if (fgets(buf, sizeof(buf), f)) {
                char *trim = strtok(buf, "\r\n");
                if (trim && *trim && strcmp(trim, "None") != 0) {
                    strncpy(vendor, trim, sizeof(vendor) - 1);
                    fclose(f);
                    break;
                }
            }
            fclose(f);
        }
    }

    if (vendor[0] && !strstr(model, vendor)) {
        printf("Device: %s %s\n", vendor, model);
    } else {
        printf("Device: %s\n", model);
    }

    // 2. CPU
    char cpu_model[128] = "x86_64 Processor";
    FILE *f_cpu = fopen("/proc/cpuinfo", "r");
    if (f_cpu) {
        char line[256];
        while (fgets(line, sizeof(line), f_cpu)) {
            if (strncmp(line, "model name", 10) == 0) {
                char *col = strchr(line, ':');
                if (col) {
                    char *trim = col + 1;
                    while (*trim == ' ' || *trim == '\t') trim++;
                    char *end = trim + strlen(trim) - 1;
                    while (end > trim && (*end == '\n' || *end == '\r' || *end == ' ')) {
                        *end = '\0';
                        end--;
                    }
                    strncpy(cpu_model, trim, sizeof(cpu_model) - 1);
                    break;
                }
            }
        }
        fclose(f_cpu);
    }
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0) cores = 1;
    printf("Processor: %s (%ld cores)\n", cpu_model, cores);

    // 3. RAM
    char ram_str[64] = "Unknown";
    FILE *f_mem = fopen("/proc/meminfo", "r");
    if (f_mem) {
        char line[256];
        while (fgets(line, sizeof(line), f_mem)) {
            if (strncmp(line, "MemTotal:", 9) == 0) {
                unsigned long kb = 0;
                if (sscanf(line + 9, "%lu", &kb) == 1) {
                    snprintf(ram_str, sizeof(ram_str), "%.1f GB", kb / (1024.0 * 1024.0));
                }
                break;
            }
        }
        fclose(f_mem);
    }
    printf("Memory (RAM): %s\n", ram_str);

    // 4. Graphics (GPU)
    printf("Graphics (GPU): Hardware Accelerated Wayland Graphics (wlroots / DRM KMS)\n");

    // 5. Storage
    struct statvfs sv;
    if (statvfs("/", &sv) == 0) {
        double total_gb = (double)(sv.f_blocks * sv.f_frsize) / (1024.0 * 1024.0 * 1024.0);
        double avail_gb = (double)(sv.f_bavail * sv.f_frsize) / (1024.0 * 1024.0 * 1024.0);
        printf("Storage: %.0f GB Storage (%.0f GB Available)\n", total_gb, avail_gb);
    } else {
        printf("Storage: Internal System Storage\n");
    }

    // 6. Kernel & OS
    struct utsname u;
    if (uname(&u) == 0) {
        printf("Kernel: %s (%s)\n", u.release, u.machine);
    } else {
        printf("Kernel: Linux XanMod (x86_64)\n");
    }

    return 0;
}
