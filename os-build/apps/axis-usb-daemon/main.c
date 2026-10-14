/*
 * AxisOS Real-Time USB Subsystem & Device Recognition Daemon
 * Written in C using libudev
 * Hardened against command injection (uses fork/execvp instead of system()),
 * handles EINTR & SIGTERM cleanly, and dispatches notifications to the active
 * desktop user's Wayland/DBus session.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <libudev.h>

static volatile sig_atomic_t g_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
}

/* Strip control characters or shell-unsafe bytes from hardware strings */
static void sanitize_string(const char *src, char *dst, size_t dst_size) {
    if (!dst || dst_size == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 1 < dst_size; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c >= 32 && c != 127) {
            dst[j++] = (char)c;
        }
    }
    dst[j] = '\0';
}

/* Find active desktop user UID (>= 1000) with a live /run/user/<uid>/bus socket */
static uid_t find_active_desktop_uid(gid_t *out_gid) {
    DIR *d = opendir("/run/user");
    if (!d) return (uid_t)-1;

    struct dirent *ent;
    uid_t found_uid = (uid_t)-1;
    while ((ent = readdir(d)) != NULL) {
        if (!isdigit((unsigned char)ent->d_name[0])) continue;
        unsigned long val = strtoul(ent->d_name, NULL, 10);
        if (val >= 1000) {
            char bus_path[256];
            snprintf(bus_path, sizeof(bus_path), "/run/user/%lu/bus", val);
            if (access(bus_path, F_OK) == 0) {
                found_uid = (uid_t)val;
                break;
            }
        }
    }
    closedir(d);

    if (found_uid != (uid_t)-1 && out_gid) {
        struct passwd *pw = getpwuid(found_uid);
        *out_gid = pw ? pw->pw_gid : (gid_t)found_uid;
    }
    return found_uid;
}

/* Non-blocking safe child spawner using fork() + execvp() */
static void spawn_async_process(char *const argv[], bool as_desktop_user) {
    pid_t pid = fork();
    if (pid < 0) return;

    if (pid == 0) {
        /* Double-fork or rely on SIGCHLD SIG_IGN so no zombies accumulate */
        if (as_desktop_user && geteuid() == 0) {
            gid_t target_gid = 1000;
            uid_t target_uid = find_active_desktop_uid(&target_gid);
            if (target_uid == (uid_t)-1) {
                _exit(0);
            }
            char runtime_dir[128];
            char dbus_addr[160];
            snprintf(runtime_dir, sizeof(runtime_dir), "/run/user/%u", (unsigned)target_uid);
            snprintf(dbus_addr, sizeof(dbus_addr), "unix:path=/run/user/%u/bus", (unsigned)target_uid);
            setenv("XDG_RUNTIME_DIR", runtime_dir, 1);
            setenv("DBUS_SESSION_BUS_ADDRESS", dbus_addr, 1);
            setenv("WAYLAND_DISPLAY", "wayland-0", 0);
            setenv("DISPLAY", ":0", 0);

            if (setgid(target_gid) != 0 || setuid(target_uid) != 0) {
                _exit(1);
            }
        }

        int devnull = open("/dev/null", O_RDWR);
        if (devnull >= 0) {
            dup2(devnull, STDIN_FILENO);
            dup2(devnull, STDOUT_FILENO);
            dup2(devnull, STDERR_FILENO);
            if (devnull > 2) close(devnull);
        }

        execvp(argv[0], argv);
        _exit(127);
    }
}

static void send_notification(const char *icon, const char *title, const char *body) {
    char safe_icon[128];
    char safe_title[128];
    char safe_body[256];

    sanitize_string(icon ? icon : "drive-removable-media-usb", safe_icon, sizeof(safe_icon));
    sanitize_string(title ? title : "USB Event", safe_title, sizeof(safe_title));
    sanitize_string(body ? body : "Hardware device detected", safe_body, sizeof(safe_body));

    char *const argv[] = {
        "notify-send",
        "-i", safe_icon,
        safe_title,
        safe_body,
        NULL
    };
    spawn_async_process(argv, true);
}

static void handle_usb_device(struct udev_device *dev, const char *action) {
    const char *devtype = udev_device_get_devtype(dev);
    if (!devtype || strcmp(devtype, "usb_device") != 0) {
        return;
    }

    const char *vendor = udev_device_get_property_value(dev, "ID_VENDOR_FROM_DATABASE");
    if (!vendor) vendor = udev_device_get_property_value(dev, "ID_VENDOR");
    if (!vendor) vendor = "USB";

    const char *model = udev_device_get_property_value(dev, "ID_MODEL_FROM_DATABASE");
    if (!model) model = udev_device_get_property_value(dev, "ID_MODEL");
    if (!model) model = "Peripheral Device";

    char raw_name[512];
    char dev_name[512];
    snprintf(raw_name, sizeof(raw_name), "%s %s", vendor, model);
    sanitize_string(raw_name, dev_name, sizeof(dev_name));

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

static void handle_block_device(struct udev_device *dev, const char *action) {
    const char *fs_usage = udev_device_get_property_value(dev, "ID_FS_USAGE");
    const char *devnode = udev_device_get_devnode(dev);

    if (!devnode || strncmp(devnode, "/dev/", 5) != 0) return;

    if (fs_usage && strcmp(fs_usage, "filesystem") == 0 && strcmp(action, "add") == 0) {
        const char *raw_label = udev_device_get_property_value(dev, "ID_FS_LABEL");
        char label[128];
        sanitize_string(raw_label ? raw_label : "USB Storage Device", label, sizeof(label));

        printf("[USB DAEMON] Mounting filesystem on %s (%s)...\n", devnode, label);
        fflush(stdout);

        char *const mount_argv[] = {
            "udisksctl",
            "mount",
            "--no-user-interaction",
            "-b",
            (char *)devnode,
            NULL
        };
        spawn_async_process(mount_argv, true);

        char msg[256];
        snprintf(msg, sizeof(msg), "%s (%s) mounted and ready", label, devnode);
        send_notification("media-flash", "USB Flash Drive Mounted", msg);
    }
}

int main(void) {
    printf("[USB DAEMON] Starting AxisOS Real-Time USB Monitor Daemon...\n");
    fflush(stdout);

    /* Automatically reap child processes to prevent zombies */
    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = SIG_IGN;
    sa_chld.sa_flags = SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa_chld, NULL);

    struct sigaction sa_term;
    memset(&sa_term, 0, sizeof(sa_term));
    sa_term.sa_handler = handle_signal;
    sigaction(SIGINT, &sa_term, NULL);
    sigaction(SIGTERM, &sa_term, NULL);

    struct udev *udev = udev_new();
    if (!udev) {
        fprintf(stderr, "Failed to initialize libudev.\n");
        return 1;
    }

    struct udev_monitor *mon = udev_monitor_new_from_netlink(udev, "udev");
    if (!mon) {
        fprintf(stderr, "Failed to create netlink udev monitor.\n");
        udev_unref(udev);
        return 1;
    }

    udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", NULL);
    udev_monitor_filter_add_match_subsystem_devtype(mon, "block", NULL);
    udev_monitor_enable_receiving(mon);

    int fd = udev_monitor_get_fd(mon);

    while (g_running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        int ret = select(fd + 1, &fds, NULL, NULL, NULL);
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("[USB DAEMON] select() error");
            break;
        }

        if (ret > 0 && FD_ISSET(fd, &fds)) {
            struct udev_device *dev = udev_monitor_receive_device(mon);
            if (dev) {
                const char *action = udev_device_get_action(dev);
                const char *subsystem = udev_device_get_subsystem(dev);

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

    printf("[USB DAEMON] Shutting down cleanly.\n");
    udev_monitor_unref(mon);
    udev_unref(udev);
    return 0;
}
