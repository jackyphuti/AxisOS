/*
 * AxisOS Real-Time USB Subsystem & Device Recognition Daemon
 * Written in C using libudev
 * Listens for hardware USB and block storage hotplug events via Netlink socket.
 * Features:
 *  - Sub-millisecond USB device recognition (mice, keyboards, webcams, audio, controllers)
 *  - Automatic storage partition mounting via udisksctl
 *  - Native desktop notifications via notify-send / mako
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libudev.h>
#include <sys/select.h>

static void send_notification(const char* icon, const char* title, const char* body) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "notify-send -i \"%s\" \"%s\" \"%s\" &",
             icon ? icon : "drive-removable-media-usb",
             title ? title : "USB Event",
             body ? body : "Hardware device detected");
    system(cmd);
}

static void handle_usb_device(struct udev_device* dev, const char* action) {
    const char* devtype = udev_device_get_devtype(dev);
    if (!devtype || strcmp(devtype, "usb_device") != 0) {
        return;
    }

    const char* vendor = udev_device_get_property_value(dev, "ID_VENDOR_FROM_DATABASE");
    if (!vendor) vendor = udev_device_get_property_value(dev, "ID_VENDOR");
    if (!vendor) vendor = "USB";

    const char* model = udev_device_get_property_value(dev, "ID_MODEL_FROM_DATABASE");
    if (!model) model = udev_device_get_property_value(dev, "ID_MODEL");
    if (!model) model = "Peripheral Device";

    char dev_name[512];
    snprintf(dev_name, sizeof(dev_name), "%s %s", vendor, model);

    if (strcmp(action, "add") == 0) {
        printf("[USB DAEMON] Connected: %s\n", dev_name);
        fflush(stdout);
        send_notification("input-gaming", "USB Device Connected", dev_name);
    } else if (strcmp(action, "remove") == 0) {
        printf("[USB DAEMON] Disconnected: %s\n", dev_name);
        fflush(stdout);
        send_notification("drive-removable-media-usb", "USB Device Disconnected", dev_name);
    }
}

static void handle_block_device(struct udev_device* dev, const char* action) {
    const char* fs_usage = udev_device_get_property_value(dev, "ID_FS_USAGE");
    const char* devnode = udev_device_get_devnode(dev);
    const char* devtype = udev_device_get_devtype(dev);

    if (!devnode) return;

    if (fs_usage && strcmp(fs_usage, "filesystem") == 0 && strcmp(action, "add") == 0) {
        const char* label = udev_device_get_property_value(dev, "ID_FS_LABEL");
        if (!label) label = "USB Storage Device";

        printf("[USB DAEMON] Mounting filesystem on %s (%s)...\n", devnode, label);
        fflush(stdout);

        char mount_cmd[512];
        snprintf(mount_cmd, sizeof(mount_cmd), "udisksctl mount -b %s >/dev/null 2>&1 &", devnode);
        system(mount_cmd);

        char msg[512];
        snprintf(msg, sizeof(msg), "%s (%s) mounted and ready", label, devnode);
        send_notification("media-flash", "USB Flash Drive Mounted", msg);
    }
}

int main(void) {
    printf("[USB DAEMON] Starting AxisOS Real-Time USB Monitor Daemon...\n");
    fflush(stdout);

    struct udev* udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Failed to initialize libudev.\n");
        return 1;
    }

    struct udev_monitor* mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        fprintf(stderr, "Failed to create netlink udev monitor.\n");
        udev_unref(udev);
        return 1;
    }

    udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", NULL);
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
    udev_monitor_enable_receiving(mon);

    int fd = udev_monitor_get_fd(mon);

    while (1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        int ret = select(fd + 1, &fds, NULL, NULL, NULL);
        if (ret > 0 && FD_ISSET(fd, &fds)) {
            struct udev_device* dev = udev_monitor_receive_device(mon);
            if (dev) {
                const char* action = udev_device_get_action(dev);
                const char* subsystem = udev_device_get_subsystem(dev);

                if (action && subsystem) {
                    if (strcmp(subsystem, "usb") == 0) {
                        handle_usb_device(dev, action);
                    } else if (strcmp(subsystem, "block") == 0) {
                        handle_block_device(dev, action);
                    }
                }
                udev_device_unref(dev);
            }
        }
    }

    udev_monitor_unref(mon);
    udev_unref(udev);
    return 0;
}
