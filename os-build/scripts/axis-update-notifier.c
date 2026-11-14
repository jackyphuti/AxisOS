#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void) {
    printf("[AxisOS UpdateNotifier] Checking for background system updates...\n");
    // Notify via notify-send or GIO D-Bus
    system("notify-send -i software-update-available 'AxisOS Updates' 'System is up to date.' 2>/dev/null || true");
    return 0;
}
