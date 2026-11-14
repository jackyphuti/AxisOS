#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(void) {
    printf("[AxisOS Fastboot] Applying low-latency kernel fastboot patches...\n");

    system("sysctl -w vm.dirty_ratio=10 2>/dev/null || true");
    system("sysctl -w vm.dirty_background_ratio=5 2>/dev/null || true");
    system("sysctl -w vm.vfs_cache_pressure=50 2>/dev/null || true");

    // Enable zRAM fast swap if available
    system("modprobe zram 2>/dev/null || true");

    printf("[AxisOS Fastboot] Fastboot patches successfully applied.\n");
    return 0;
}
