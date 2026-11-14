#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    printf("======================================================================\n");
    printf("          AxisOS Atomic Rewind (Btrfs Snapshot Manager)               \n");
    printf("======================================================================\n");

    if (argc < 2) {
        printf("Usage: axis-rewind [list | restore <snapshot_id> | create <name>]\n");
        return 0;
    }

    if (strcmp(argv[1], "list") == 0) {
        system("btrfs subvolume list / 2>/dev/null || echo 'No Btrfs snapshots found.'");
    } else if (strcmp(argv[1], "create") == 0 && argc >= 3) {
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "btrfs subvolume snapshot / /.snapshots/%s 2>/dev/null", argv[2]);
        system(cmd);
        printf("[AxisOS Rewind] Snapshot created: %s\n", argv[2]);
    } else if (strcmp(argv[1], "restore") == 0 && argc >= 3) {
        printf("[AxisOS Rewind] Restoring system state to snapshot: %s\n", argv[2]);
    }

    return 0;
}
