#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    printf("======================================================================\n");
    printf("          AxisOS Active Directory / Domain Join                       \n");
    printf("======================================================================\n");

    if (argc < 2) {
        printf("Usage: axis-domain-join <domain_name> [user]\n");
        return 0;
    }

    printf("[AxisOS Domain] Joining realm: %s\n", argv[1]);
    return 0;
}
