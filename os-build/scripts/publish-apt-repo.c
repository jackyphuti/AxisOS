#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void) {
    printf("======================================================\n");
    printf("     Publishing AxisOS Remote APT Repository (Pure C) \n");
    printf("======================================================\n");

    mkdir("public_apt", 0755);
    mkdir("public_apt/dists", 0755);
    mkdir("public_apt/dists/bookworm", 0755);
    mkdir("public_apt/dists/bookworm/main", 0755);
    mkdir("public_apt/dists/bookworm/main/binary-amd64", 0755);
    mkdir("public_apt/pool", 0755);
    mkdir("public_apt/pool/main", 0755);

    printf("Indexing Debian packages...\n");
    system("dpkg-scanpackages public_apt/pool/main /dev/null > public_apt/dists/bookworm/main/binary-amd64/Packages 2>/dev/null || true");
    system("gzip -9c public_apt/dists/bookworm/main/binary-amd64/Packages > public_apt/dists/bookworm/main/binary-amd64/Packages.gz 2>/dev/null || true");

    printf("Repository published under public_apt/\n");
    return 0;
}
