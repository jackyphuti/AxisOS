/*
 * AxisOS System Update Engine (axis-update)
 * Header Definitions: A/B Partition Staging, Bootloader Fallback,
 * Kexec Instant Reboot, Transactional Rollbacks, and Process Exit Codes
 *
 * Copyright (c) 2026 AxisOS Architecture Team
 */

#ifndef AXIS_UPDATE_H
#define AXIS_UPDATE_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>

#define AXIS_VERSION_STRING "2.0.0-horizon"
#define DEFAULT_UPDATE_CHANNEL "stable"
#define STAGING_MOUNT_POINT "/mnt/axis_staging_rootfs"
#define BOOT_DIR "/boot"
#define GRUBENV_PATH "/boot/grub/grubenv"
#define GRUBENV_BACKUP_PATH "/boot/grub/grubenv.bak"
#define AXIS_SLOT_STATE_FILE "/etc/axisos_boot_slot"

/* Distinct Exit Codes for Transactional Telemetry */
#define EXIT_SUCCESS_UPDATE       0   /* Completed successfully */
#define EXIT_ERR_GENERAL          1   /* General error */
#define EXIT_ERR_DISK_SPACE       2   /* Insufficient storage space on target/boot */
#define EXIT_ERR_NETWORK          3   /* Network failure / payload unreachable */
#define EXIT_ERR_SIGNATURE        4   /* Cryptographic signature or hash mismatch */
#define EXIT_ERR_PARTITION_MOUNT  5   /* Block device format/mount error */
#define EXIT_ERR_KERNEL_STAGE     6   /* Kernel / initramfs generation failure */
#define EXIT_ERR_BOOTLOADER       7   /* GRUB / bootloader configuration failure */
#define EXIT_ERR_KEXEC            8   /* Kexec load or execution failure */
#define EXIT_ERR_ABORTED          130 /* Aborted by SIGINT / Ctrl+C */

/* Partition Slot Identifiers */
typedef enum {
    SLOT_UNKNOWN = 0,
    SLOT_A = 1,
    SLOT_B = 2
} axis_slot_t;

/* Update Transaction State */
typedef enum {
    STATE_IDLE = 0,
    STATE_PREFLIGHT,
    STATE_DOWNLOADING,
    STATE_VERIFYING,
    STATE_STAGING_ROOTFS,
    STATE_ROOTFS_STAGED,
    STATE_KERNEL_STAGED,
    STATE_BOOTLOADER_CONFIGURED,
    STATE_COMMITTED
} transaction_state_t;

/* Update Configuration Options */
typedef struct {
    bool fast_boot;          /* Use kexec to bypass hardware reboot */
    bool dry_run;            /* Dry run without committing changes */
    bool force;              /* Bypass minor preflight warnings */
    char channel[32];        /* Release channel (stable, beta, nightly) */
    char target_version[64]; /* Target version override */
    char payload_url[512];   /* Direct update payload URL */
} update_config_t;

/* System Slot Information */
typedef struct {
    axis_slot_t active_slot;
    axis_slot_t passive_slot;
    char active_dev[128];
    char passive_dev[128];
    char root_uuid[64];
    uint64_t available_bytes;
} slot_info_t;

/* Global transaction context for signal handlers and rollbacks */
typedef struct {
    transaction_state_t state;
    slot_info_t slots;
    update_config_t config;
    char staged_vmlinuz[256];
    char staged_initrd[256];
    bool mount_active;
    bool grubenv_modified;
} transaction_context_t;

extern transaction_context_t g_tx;

/* Function Prototypes */
void init_signal_handlers(void);
void rollback_transaction(int exit_code, const char *reason);
int detect_system_slots(slot_info_t *slots);
int check_preflight_requirements(const slot_info_t *slots, const update_config_t *cfg);
int download_and_verify_payload(const char *url, const char *dest_path, const char *expected_hash);
int stage_passive_rootfs(const slot_info_t *slots, const char *payload_path);
int stage_kernel_and_initramfs(const slot_info_t *slots, char *out_vmlinuz, char *out_initrd);
int configure_bootloader_fallback(const slot_info_t *slots);
int execute_kexec_reboot(const char *vmlinuz_path, const char *initrd_path, axis_slot_t target_slot);
int mark_current_slot_successful(void);
int rollback_to_previous_slot(void);
void print_slot_status(void);

/* Process Execution Helpers with waitpid & Status Parsing */
int run_child_process(char *const argv[], char *output_buf, size_t buf_size);

#endif /* AXIS_UPDATE_H */
