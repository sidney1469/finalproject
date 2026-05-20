/*
 * fs_firmware - Demonstrates LittleFS-backed logging on Zephyr.
 *
 * On boot we mount the filesystem, append a boot marker line, and print
 * filesystem statistics. The `fslog` shell command (registered in fs_log.c)
 * and the built-in `fs` shell can then be used at runtime to write/read
 * log entries and manage files.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include "fs_log.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define BOOT_COUNT_PATH FS_LOG_MOUNT_POINT "/boot.cnt"

/* Read the boot counter from the FS, increment it, and write it back.
 * Returns the new boot count, or a negative errno on failure.
 */
static int update_boot_counter(void)
{
    struct fs_file_t f;
    uint32_t count = 0;

    fs_file_t_init(&f);
    int rc = fs_open(&f, BOOT_COUNT_PATH, FS_O_READ);
    if (rc == 0) {
        (void)fs_read(&f, &count, sizeof(count));
        (void)fs_close(&f);
    } else if (rc != -ENOENT) {
        return rc;
    }

    count++;

    fs_file_t_init(&f);
    rc = fs_open(&f, BOOT_COUNT_PATH, FS_O_CREATE | FS_O_WRITE);
    if (rc < 0) {
        return rc;
    }
    ssize_t w = fs_write(&f, &count, sizeof(count));
    (void)fs_sync(&f);
    (void)fs_close(&f);
    if (w < 0) {
        return (int)w;
    }
    return (int)count;
}

int main(void)
{
    printk("\n=== fs_firmware starting ===\n");

    int rc = fs_log_init();
    if (rc < 0) {
        printk("fs_log_init failed: %d\n", rc);
        return 0;
    }

    int boot_no = update_boot_counter();
    if (boot_no < 0) {
        printk("update_boot_counter failed: %d\n", boot_no);
        boot_no = 0;
    }

    rc = fs_log_writef("boot #%d uptime=%lld ms", boot_no, k_uptime_get());
    if (rc < 0) {
        printk("fs_log_writef failed: %d\n", rc);
    } else {
        printk("Logged boot marker (%d bytes)\n", rc);
    }

    size_t total = 0, used = 0;
    if (fs_log_stats(&total, &used) == 0) {
        printk("LittleFS: total=%u used=%u free=%u bytes\n",
               (unsigned int)total, (unsigned int)used,
               (unsigned int)(total - used));
    }

    int sz = fs_log_file_size();
    if (sz >= 0) {
        printk("Log file size: %d bytes\n", sz);
    }

    printk("Type `fslog read` to print the log, `fslog write <text>` to append,\n"
           "`fslog clear` to wipe it, or use the built-in `fs` shell to browse.\n");

    /* Heartbeat - keep the main thread alive so the shell stays responsive. */
    while (1) {
        k_sleep(K_SECONDS(60));
    }
    return 0;
}
