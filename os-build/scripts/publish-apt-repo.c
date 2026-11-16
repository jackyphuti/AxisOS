#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void) {
    printf("======================================================\n");
    printf("     Publishing AxisOS Remote APT Repository (Pure C) \n");
    printf("======================================================\n");

    int ret = system("bash os-build/scripts/publish-apt-repo.sh");
    if (ret != 0) {
        fprintf(stderr, "[-] Failed to publish APT repository (exit code %d)\n", ret);
        return 1;
    }

    printf("Repository published under public_apt/\n");
    return 0;
}
