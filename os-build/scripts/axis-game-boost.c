#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <glob.h>

int main(int argc, char *argv[]) {
    printf("[AxisOS GameBoost] Activating maximum gaming & overclocking performance mode...\n");

    glob_t g;
    if (glob("/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor", 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) {
            FILE *fp = fopen(g.gl_pathv[i], "w");
            if (fp) {
                fputs("performance\n", fp);
                fclose(fp);
            }
        }
        globfree(&g);
    }

    if (glob("/sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference", 0, NULL, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; i++) {
            FILE *fp = fopen(g.gl_pathv[i], "w");
            if (fp) {
                fputs("performance\n", fp);
                fclose(fp);
            }
        }
        globfree(&g);
    }

    system("sysctl -w vm.compaction_proactiveness=0 2>/dev/null || true");
    system("sysctl -w kernel.sched_migration_cost_ns=5000000 2>/dev/null || true");

    if (argc > 1) {
        printf("[AxisOS GameBoost] Launching target game process: %s\n", argv[1]);
        execvp(argv[1], &argv[1]);
    }

    printf("[AxisOS GameBoost] System gaming mode active.\n");
    return 0;
}
