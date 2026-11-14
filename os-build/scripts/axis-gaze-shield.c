#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(void) {
    printf("[AxisOS GazeShield] Initializing camera privacy shield daemon...\n");
    if (access("/dev/video0", F_OK) != 0) {
        printf("[AxisOS GazeShield] No camera device found at /dev/video0.\n");
        return 0;
    }
    printf("[AxisOS GazeShield] Active camera monitor running.\n");
    return 0;
}
