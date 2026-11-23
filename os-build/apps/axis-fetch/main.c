#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pwd.h>

/**
 * AxisOS Native System Telemetry & Fetch Tool (axis-fetch)
 * Strictly Pure C11 (-std=gnu11)
 *
 * Displays the glowing AxisOS Triangle Logo alongside live hardware and
 * OS telemetry (OS, Host, Kernel, Uptime, Packages, DE/WM, CPU, GPU, RAM, Disk, IP).
 */

#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_CYAN    "\033[38;2;56;189;248m"
#define COLOR_INDIGO  "\033[38;2;125;149;255m"
#define COLOR_WHITE   "\033[38;2;241;245;249m"
#define COLOR_MUTED   "\033[38;2;148;163;184m"
#define COLOR_GREEN   "\033[38;2;52;211;153m"

typedef struct {
    char username[64];
    char hostname[64];
    char os_name[256];
    char kernel[128];
    char arch[128];
    char host_model[128];
    char uptime_str[64];
    int  packages;
    char shell[64];
    char display_server[64];
    char desktop[96];
    char terminal[64];
    char cpu_model[128];
    int  cpu_cores;
    char cpu_freq[32];
    char gpu_model[128];
    char local_ip[64];
    unsigned long ram_total_mb;
    unsigned long ram_used_mb;
    unsigned long long disk_total_gb;
    unsigned long long disk_used_gb;
} SystemTelemetry;

static void trim_trailing(char *s) {
    if (!s) return;
    int len = (int)strlen(s);
    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r' || s[len - 1] == ' ' || s[len - 1] == '\t')) {
        s[len - 1] = '\0';
        len--;
    }
}

static void read_telemetry(SystemTelemetry *t) {
    memset(t, 0, sizeof(*t));

    /* 1. User & Hostname */
    struct passwd *pw = getpwuid(getuid());
    snprintf(t->username, sizeof(t->username), "%s", (pw && pw->pw_name) ? pw->pw_name : "user");
    if (gethostname(t->hostname, sizeof(t->hostname)) != 0) {
        snprintf(t->hostname, sizeof(t->hostname), "axisos");
    }

    /* 2. OS Name from /etc/os-release */
    FILE *fp_os = fopen("/etc/os-release", "r");
    if (fp_os) {
        char line[256];
        while (fgets(line, sizeof(line), fp_os)) {
            if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
                char *start = line + 12;
                if (*start == '"' || *start == '\'') start++;
                snprintf(t->os_name, sizeof(t->os_name), "%s", start);
                trim_trailing(t->os_name);
                int l = (int)strlen(t->os_name);
                if (l > 0 && (t->os_name[l - 1] == '"' || t->os_name[l - 1] == '\'')) {
                    t->os_name[l - 1] = '\0';
                }
                break;
            }
        }
        fclose(fp_os);
    }
    if (!t->os_name[0] || strstr(t->os_name, "Debian") != NULL) {
        snprintf(t->os_name, sizeof(t->os_name), "AxisOS Linux (Debian 12 Bookworm)");
    }

    /* 3. Kernel & Architecture */
    struct utsname u;
    if (uname(&u) == 0) {
        snprintf(t->kernel, sizeof(t->kernel), "%s", u.release);
        snprintf(t->arch, sizeof(t->arch), "%s", u.machine);
    }

    /* 4. Host Model (DMI / Hardware) */
    FILE *fp_dmi = fopen("/sys/class/dmi/id/product_name", "r");
    if (fp_dmi) {
        if (fgets(t->host_model, sizeof(t->host_model), fp_dmi)) trim_trailing(t->host_model);
        fclose(fp_dmi);
    }
    if (!t->host_model[0] || strcmp(t->host_model, "None") == 0 ||
        strcmp(t->host_model, "Default string") == 0 || strcmp(t->host_model, "To Be Filled By O.E.M.") == 0) {
        FILE *fp_ven = fopen("/sys/class/dmi/id/sys_vendor", "r");
        if (fp_ven) {
            char ven[64] = {0};
            if (fgets(ven, sizeof(ven), fp_ven)) {
                trim_trailing(ven);
                if (ven[0]) snprintf(t->host_model, sizeof(t->host_model), "%s PC", ven);
            }
            fclose(fp_ven);
        }
    }
    if (!t->host_model[0]) snprintf(t->host_model, sizeof(t->host_model), "AxisOS PC");

    /* 5. Uptime */
    FILE *fp_up = fopen("/proc/uptime", "r");
    if (fp_up) {
        double uptime_sec = 0;
        if (fscanf(fp_up, "%lf", &uptime_sec) == 1) {
            long total = (long)uptime_sec;
            long days = total / 86400;
            long hours = (total % 86400) / 3600;
            long mins = (total % 3600) / 60;
            if (days > 0) snprintf(t->uptime_str, sizeof(t->uptime_str), "%ldd %ldh %ldm", days, hours, mins);
            else if (hours > 0) snprintf(t->uptime_str, sizeof(t->uptime_str), "%ldh %ldm", hours, mins);
            else snprintf(t->uptime_str, sizeof(t->uptime_str), "%ld mins", mins);
        }
        fclose(fp_up);
    }

    /* 6. Installed Packages (dpkg) — fast 64 KB buffered scan */
    FILE *fp_pkg = fopen("/var/lib/dpkg/status", "r");
    if (fp_pkg) {
        static char pkg_iobuf[65536];
        setvbuf(fp_pkg, pkg_iobuf, _IOFBF, sizeof(pkg_iobuf));
        char line[256];
        int count = 0;
        while (fgets(line, sizeof(line), fp_pkg)) {
            if (line[0] == 'S' && strncmp(line, "Status: install ok installed", 28) == 0) count++;
        }
        fclose(fp_pkg);
        t->packages = count;
    }

    /* 7. Shell */
    const char *sh = getenv("SHELL");
    if (sh) {
        const char *base = strrchr(sh, '/');
        snprintf(t->shell, sizeof(t->shell), "%s", base ? base + 1 : sh);
    } else {
        snprintf(t->shell, sizeof(t->shell), "bash");
    }

    /* 8. Display Server & Desktop Environment */
    const char *wdisp = getenv("WAYLAND_DISPLAY");
    const char *xdisp = getenv("DISPLAY");
    if (wdisp && wdisp[0]) {
        snprintf(t->display_server, sizeof(t->display_server), "Wayland (%s)", wdisp);
    } else if (xdisp && xdisp[0]) {
        snprintf(t->display_server, sizeof(t->display_server), "X11 (%s)", xdisp);
    } else {
        snprintf(t->display_server, sizeof(t->display_server), "TTY Console");
    }

    const char *xdg_de = getenv("XDG_CURRENT_DESKTOP");
    if (xdg_de && strcasestr(xdg_de, "KDE")) {
        snprintf(t->desktop, sizeof(t->desktop), "KDE Plasma 5.27 (AxisOS Obsidian) / KWin");
    } else if (xdg_de && xdg_de[0]) {
        snprintf(t->desktop, sizeof(t->desktop), "%s (AxisOS)", xdg_de);
    } else {
        snprintf(t->desktop, sizeof(t->desktop), "KDE Plasma 5.27 (AxisOS Obsidian)");
    }

    /* 9. Terminal */
    if (getenv("AXIS_TERMINAL")) snprintf(t->terminal, sizeof(t->terminal), "AxisOS Terminal (Pure C11)");
    else if (getenv("KONSOLE_VERSION")) snprintf(t->terminal, sizeof(t->terminal), "Konsole");
    else if (getenv("TERM")) snprintf(t->terminal, sizeof(t->terminal), "%s", getenv("TERM"));
    else snprintf(t->terminal, sizeof(t->terminal), "axis-terminal");

    /* 10. CPU Model & Cores */
    FILE *fp_cpu = fopen("/proc/cpuinfo", "r");
    if (fp_cpu) {
        char line[256];
        int cores = 0;
        while (fgets(line, sizeof(line), fp_cpu)) {
            if (strncmp(line, "model name", 10) == 0 && !t->cpu_model[0]) {
                char *colon = strchr(line, ':');
                if (colon) {
                    snprintf(t->cpu_model, sizeof(t->cpu_model), "%s", colon + 2);
                    trim_trailing(t->cpu_model);
                }
            } else if (strncmp(line, "cpu MHz", 7) == 0 && !t->cpu_freq[0]) {
                char *colon = strchr(line, ':');
                if (colon) snprintf(t->cpu_freq, sizeof(t->cpu_freq), "@ %.2f GHz", atof(colon + 2) / 1000.0);
            } else if (strncmp(line, "processor", 9) == 0) {
                cores++;
            }
        }
        fclose(fp_cpu);
        t->cpu_cores = cores > 0 ? cores : 1;
    }
    if (!t->cpu_model[0]) snprintf(t->cpu_model, sizeof(t->cpu_model), "x86_64 Processor");

    /* 11. GPU Model (cached in /tmp/.axisos-gpu-cache for instant 0ms repeat startup) */
    FILE *fp_gcache = fopen("/tmp/.axisos-gpu-cache", "r");
    if (fp_gcache) {
        if (fgets(t->gpu_model, sizeof(t->gpu_model), fp_gcache)) trim_trailing(t->gpu_model);
        fclose(fp_gcache);
    }
    if (!t->gpu_model[0]) {
        FILE *fp_gpu = popen("lspci 2>/dev/null | grep -iE 'vga|3d|display' | sed 's/.*: //g' | head -n1", "r");
        if (fp_gpu) {
            if (fgets(t->gpu_model, sizeof(t->gpu_model), fp_gpu)) trim_trailing(t->gpu_model);
            pclose(fp_gpu);
        }
        if (t->gpu_model[0]) {
            FILE *fp_gw = fopen("/tmp/.axisos-gpu-cache", "w");
            if (fp_gw) {
                fprintf(fp_gw, "%s\n", t->gpu_model);
                fclose(fp_gw);
            }
        }
    }
    if (!t->gpu_model[0]) {
        char drv_buf[256] = {0};
        ssize_t len = readlink("/sys/class/drm/card0/device/driver", drv_buf, sizeof(drv_buf) - 1);
        if (len > 0) {
            drv_buf[len] = '\0';
            const char *base = strrchr(drv_buf, '/');
            snprintf(t->gpu_model, sizeof(t->gpu_model), "DRM Graphics (%s)", base ? base + 1 : drv_buf);
        } else {
            snprintf(t->gpu_model, sizeof(t->gpu_model), "Integrated / Hardware GPU");
        }
    }

    /* 12. Memory (RAM) */
    FILE *fp_mem = fopen("/proc/meminfo", "r");
    if (fp_mem) {
        char line[256];
        unsigned long mem_total_kb = 0, mem_avail_kb = 0;
        while (fgets(line, sizeof(line), fp_mem)) {
            if (strncmp(line, "MemTotal:", 9) == 0) sscanf(line, "MemTotal: %lu kB", &mem_total_kb);
            else if (strncmp(line, "MemAvailable:", 13) == 0) sscanf(line, "MemAvailable: %lu kB", &mem_avail_kb);
        }
        fclose(fp_mem);
        t->ram_total_mb = mem_total_kb / 1024;
        t->ram_used_mb = (mem_total_kb > mem_avail_kb) ? (mem_total_kb - mem_avail_kb) / 1024 : 0;
    }

    /* 13. Disk Storage */
    struct statvfs sv;
    if (statvfs("/", &sv) == 0) {
        unsigned long long total = (unsigned long long)sv.f_blocks * sv.f_frsize;
        unsigned long long free_b = (unsigned long long)sv.f_bavail * sv.f_frsize;
        t->disk_total_gb = total / (1024ULL * 1024ULL * 1024ULL);
        t->disk_used_gb = (total > free_b) ? (total - free_b) / (1024ULL * 1024ULL * 1024ULL) : 0;
    }

    /* 14. Local IPv4 — direct Pure C getifaddrs() (zero subprocesses) */
    struct ifaddrs *ifaddr = NULL;
    if (getifaddrs(&ifaddr) == 0) {
        for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
            if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
            if (ifa->ifa_flags & IFF_LOOPBACK) continue;
            struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &sin->sin_addr, t->local_ip, sizeof(t->local_ip))) {
                break;
            }
        }
        freeifaddrs(ifaddr);
    }
    if (!t->local_ip[0]) snprintf(t->local_ip, sizeof(t->local_ip), "Offline / Loopback");
}

static void print_json(const SystemTelemetry *t) {
    printf("{\n");
    printf("  \"user\": \"%s\",\n", t->username);
    printf("  \"hostname\": \"%s\",\n", t->hostname);
    printf("  \"os\": \"%s\",\n", t->os_name);
    printf("  \"kernel\": \"%s\",\n", t->kernel);
    printf("  \"arch\": \"%s\",\n", t->arch);
    printf("  \"host\": \"%s\",\n", t->host_model);
    printf("  \"uptime\": \"%s\",\n", t->uptime_str);
    printf("  \"packages\": %d,\n", t->packages);
    printf("  \"shell\": \"%s\",\n", t->shell);
    printf("  \"display_server\": \"%s\",\n", t->display_server);
    printf("  \"desktop\": \"%s\",\n", t->desktop);
    printf("  \"terminal\": \"%s\",\n", t->terminal);
    printf("  \"cpu\": \"%s\",\n", t->cpu_model);
    printf("  \"cpu_cores\": %d,\n", t->cpu_cores);
    printf("  \"gpu\": \"%s\",\n", t->gpu_model);
    printf("  \"local_ip\": \"%s\",\n", t->local_ip);
    printf("  \"ram_used_mb\": %lu,\n", t->ram_total_mb ? t->ram_used_mb : 0UL);
    printf("  \"ram_total_mb\": %lu,\n", t->ram_total_mb);
    printf("  \"disk_used_gb\": %llu,\n", t->disk_used_gb);
    printf("  \"disk_total_gb\": %llu\n", t->disk_total_gb);
    printf("}\n");
}

static void print_raw(const SystemTelemetry *t) {
    printf("USER=%s\nHOST=%s\nOS=%s\nKERNEL=%s\nARCH=%s\nMODEL=%s\nUPTIME=%s\nPACKAGES=%d\nSHELL=%s\nDISPLAY=%s\nDESKTOP=%s\nCPU=%s (%d)\nGPU=%s\nIP=%s\nRAM=%luMiB/%luMiB\nDISK=%lluGiB/%lluGiB\n",
           t->username, t->hostname, t->os_name, t->kernel, t->arch, t->host_model, t->uptime_str,
           t->packages, t->shell, t->display_server, t->desktop, t->cpu_model, t->cpu_cores,
           t->gpu_model, t->local_ip, t->ram_used_mb, t->ram_total_mb, t->disk_used_gb, t->disk_total_gb);
}

static void print_ascii_fetch(const SystemTelemetry *t) {
    /* Iconic Glowing AxisOS Delta Triangle Logo (14 lines, exact 24-col width) */
    const char *logo[] = {
        COLOR_CYAN   COLOR_BOLD "           /\\           " COLOR_RESET,
        COLOR_CYAN   COLOR_BOLD "          /  \\          " COLOR_RESET,
        COLOR_CYAN   COLOR_BOLD "         /" COLOR_WHITE " /\\ " COLOR_CYAN "\\         " COLOR_RESET,
        COLOR_CYAN   COLOR_BOLD "        /" COLOR_WHITE " /  \\ " COLOR_CYAN "\\        " COLOR_RESET,
        COLOR_CYAN   COLOR_BOLD "       /" COLOR_WHITE " /    \\ " COLOR_CYAN "\\       " COLOR_RESET,
        COLOR_CYAN   COLOR_BOLD "      /" COLOR_WHITE " /  /\\  \\ " COLOR_CYAN "\\      " COLOR_RESET,
        COLOR_CYAN   COLOR_BOLD "     /" COLOR_WHITE " /  /  \\  \\ " COLOR_CYAN "\\     " COLOR_RESET,
        COLOR_INDIGO COLOR_BOLD "    /" COLOR_WHITE " /  /____\\  \\ " COLOR_INDIGO "\\    " COLOR_RESET,
        COLOR_INDIGO COLOR_BOLD "   /" COLOR_WHITE " /____________\\ " COLOR_INDIGO "\\   " COLOR_RESET,
        COLOR_INDIGO COLOR_BOLD "  /__________________\\  " COLOR_RESET,
        COLOR_MUTED             "                        " COLOR_RESET,
        COLOR_WHITE  COLOR_BOLD "     A X I S  " COLOR_CYAN "O S       " COLOR_RESET,
        COLOR_MUTED             "   Obsidian Pure C11    " COLOR_RESET,
        COLOR_MUTED             "                        " COLOR_RESET
    };

    double ram_used_gb = t->ram_used_mb / 1024.0;
    double ram_total_gb = t->ram_total_mb / 1024.0;
    int ram_pct = (t->ram_total_mb > 0) ? (int)((t->ram_used_mb * 100UL) / t->ram_total_mb) : 0;
    int disk_pct = (t->disk_total_gb > 0) ? (int)((t->disk_used_gb * 100ULL) / t->disk_total_gb) : 0;

    char lines[14][320];
    snprintf(lines[0],  sizeof(lines[0]),  COLOR_BOLD COLOR_CYAN "%s" COLOR_WHITE "@" COLOR_INDIGO "%s" COLOR_RESET, t->username, t->hostname);
    snprintf(lines[1],  sizeof(lines[1]),  COLOR_MUTED "────────────────────────────────────────────" COLOR_RESET);
    snprintf(lines[2],  sizeof(lines[2]),  COLOR_BOLD COLOR_CYAN "OS        " COLOR_RESET COLOR_WHITE "%s (%s)" COLOR_RESET, t->os_name, t->arch);
    snprintf(lines[3],  sizeof(lines[3]),  COLOR_BOLD COLOR_CYAN "Host      " COLOR_RESET COLOR_WHITE "%s" COLOR_RESET, t->host_model);
    snprintf(lines[4],  sizeof(lines[4]),  COLOR_BOLD COLOR_CYAN "Kernel    " COLOR_RESET COLOR_WHITE "Linux %s" COLOR_RESET, t->kernel);
    snprintf(lines[5],  sizeof(lines[5]),  COLOR_BOLD COLOR_CYAN "Uptime    " COLOR_RESET COLOR_WHITE "%s" COLOR_RESET, t->uptime_str);
    snprintf(lines[6],  sizeof(lines[6]),  COLOR_BOLD COLOR_CYAN "Packages  " COLOR_RESET COLOR_WHITE "%d (dpkg)" COLOR_RESET, t->packages);
    snprintf(lines[7],  sizeof(lines[7]),  COLOR_BOLD COLOR_CYAN "Desktop   " COLOR_RESET COLOR_WHITE "%s" COLOR_RESET, t->desktop);
    snprintf(lines[8],  sizeof(lines[8]),  COLOR_BOLD COLOR_CYAN "Session   " COLOR_RESET COLOR_WHITE "%s  •  Shell: %s" COLOR_RESET, t->display_server, t->shell);
    snprintf(lines[9],  sizeof(lines[9]),  COLOR_BOLD COLOR_CYAN "CPU       " COLOR_RESET COLOR_WHITE "%s (%d) %s" COLOR_RESET, t->cpu_model, t->cpu_cores, t->cpu_freq);
    snprintf(lines[10], sizeof(lines[10]), COLOR_BOLD COLOR_CYAN "GPU       " COLOR_RESET COLOR_WHITE "%s" COLOR_RESET, t->gpu_model);
    snprintf(lines[11], sizeof(lines[11]), COLOR_BOLD COLOR_CYAN "Memory    " COLOR_RESET COLOR_WHITE "%.1f GiB / %.1f GiB " COLOR_GREEN "(%d%%)" COLOR_RESET, ram_used_gb, ram_total_gb, ram_pct);
    snprintf(lines[12], sizeof(lines[12]), COLOR_BOLD COLOR_CYAN "Disk (/)  " COLOR_RESET COLOR_WHITE "%llu GiB / %llu GiB " COLOR_GREEN "(%d%%)" COLOR_RESET, t->disk_used_gb, t->disk_total_gb, disk_pct);
    snprintf(lines[13], sizeof(lines[13]), COLOR_BOLD COLOR_CYAN "Local IP  " COLOR_RESET COLOR_WHITE "%s" COLOR_RESET, t->local_ip);

    printf("\n");
    for (int i = 0; i < 14; i++) {
        printf(" %s   %s\n", logo[i], lines[i]);
    }
    printf("\n                            ");
    for (int c = 0; c < 8; c++) printf("\033[4%dm   ", c);
    printf(COLOR_RESET "\n                            ");
    for (int c = 0; c < 8; c++) printf("\033[10%dm   ", c);
    printf(COLOR_RESET "\n\n");
}

int main(int argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("AxisOS Native System Telemetry & Fetch Tool (Pure C11)\n");
            printf("Usage: axis-fetch [--json | --raw | --version | --help]\n");
            return 0;
        }
        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            printf("axis-fetch 2.1.0 (AxisOS Obsidian Pure C11)\n");
            return 0;
        }
    }

    SystemTelemetry telemetry;
    read_telemetry(&telemetry);

    if (argc > 1 && (strcmp(argv[1], "--json") == 0 || strcmp(argv[1], "-j") == 0)) {
        print_json(&telemetry);
    } else if (argc > 1 && (strcmp(argv[1], "--raw") == 0 || strcmp(argv[1], "-r") == 0)) {
        print_raw(&telemetry);
    } else {
        print_ascii_fetch(&telemetry);
    }
    return 0;
}
