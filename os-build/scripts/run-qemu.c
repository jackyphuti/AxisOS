#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    printf("========================================\n");
    printf("    AxisOS Virtual Machine Launcher     \n");
    printf("========================================\n");

    const char *iso_path = (argc > 1) ? argv[1] : "axisos-live-amd64.iso";
    const char *disk_path = (argc > 2) ? argv[2] : "axisos-disk.qcow2";

    if (access(disk_path, F_OK) != 0) {
        printf("Creating 20GB virtual disk image at %s...\n", disk_path);
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "qemu-img create -f qcow2 \"%s\" 20G 2>/dev/null", disk_path);
        system(cmd);
    }

    char cmd[1024];
    const char *kvm = (access("/dev/kvm", W_OK) == 0)
        ? "-enable-kvm -cpu host"
        : "-accel tcg,thread=multi -cpu Skylake-Client";

    if (access(iso_path, F_OK) == 0) {
        printf("Booting from Live ISO (%s)...\n", iso_path);
        snprintf(cmd, sizeof(cmd), "qemu-system-x86_64 -m 4096 -smp 4 %s -vga virtio -device virtio-net-pci,netdev=net0 -netdev user,id=net0 -drive file=%s,if=virtio -cdrom %s -boot d", kvm, disk_path, iso_path);
    } else {
        printf("Booting from disk (%s)...\n", disk_path);
        snprintf(cmd, sizeof(cmd), "qemu-system-x86_64 -m 4096 -smp 4 %s -vga virtio -device virtio-net-pci,netdev=net0 -netdev user,id=net0 -drive file=%s,if=virtio -boot c", kvm, disk_path);
    }

    return system(cmd);
}
