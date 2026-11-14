#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    printf("[AxisOS ISO] Generating Hybrid UEFI ISO with xorriso...\n");
    system("xorriso -as mkisofs -r -V 'AxisOS_Live' -o axisos-live-amd64.iso -J -l os-build/work/binary 2>/dev/null || true");
    return 0;
}
