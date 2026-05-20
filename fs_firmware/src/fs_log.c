/*
 * fs_log.c - LittleFS-backed append-only line logger.
 */

#include "fs_log.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/logging/log.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

LOG_MODULE_REGISTER(fs_log, LOG_LEVEL_INF);

/*
 * Use the `storage_partition` label that virtually every Zephyr board
 * defines in its DTS. If the target board does not define one, add a
 * board overlay defining a fixed-partition with label "storage".
 */
#define STORAGE_PARTITION       storage_partition
#define STORAGE_PARTITION_ID    FIXED_PARTITION_ID(STORAGE_PARTITION)

/* LittleFS configuration for the storage partition. */
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(fs_log_lfs_cfg);

static struct fs_mount_t fs_log_mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &fs_log_lfs_cfg,
    .storage_dev = (void *)STORAGE_PARTITION_ID,
    .mnt_point = FS_LOG_MOUNT_POINT,
};

static K_MUTEX_DEFINE(fs_log_mutex);
static bool fs_log_mounted;

/* -------------------------------------------------------------------------- */
/* Internal helpers                                                            */
/* -------------------------------------------------------------------------- */

static int ensure_log_file(void)
{
    struct fs_file_t f;
    fs_file_t_init(&f);

    /* Open for append; creates the file if it does not already exist. */
    int rc = fs_open(&f, FS_LOG_FILE_PATH, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
    if (rc < 0) {
        LOG_ERR("Unable to create log file %s (%d)", FS_LOG_FILE_PATH, rc);
        return rc;
    }
    (void)fs_close(&f);
    return 0;
}

/* -------------------------------------------------------------------------- */
/* Public API                                                                  */
/* -------------------------------------------------------------------------- */

int fs_log_init(void)
{
    int rc;

    k_mutex_lock(&fs_log_mutex, K_FOREVER);

    if (fs_log_mounted) {
        k_mutex_unlock(&fs_log_mutex);
        return 0;
    }

    rc = fs_mount(&fs_log_mnt);
    if (rc < 0) {
        LOG_ERR("fs_mount(%s) failed: %d", fs_log_mnt.mnt_point, rc);
        k_mutex_unlock(&fs_log_mutex);
        return rc;
    }

    LOG_INF("Mounted LittleFS at %s", fs_log_mnt.mnt_point);
    fs_log_mounted = true;

    rc = ensure_log_file();
    if (rc < 0) {
        /* Roll back the mount so we can retry later. */
        (void)fs_unmount(&fs_log_mnt);
        fs_log_mounted = false;
        k_mutex_unlock(&fs_log_mutex);
        return rc;
    }

    k_mutex_unlock(&fs_log_mutex);
    return 0;
}

int fs_log_write(const char *line)
{
    if (line == NULL) {
        return -EINVAL;
    }
    if (!fs_log_mounted) {
        return -ENOENT;
    }

    size_t len = strlen(line);
    bool need_newline = (len == 0) || (line[len - 1] != '\n');

    struct fs_file_t f;
    fs_file_t_init(&f);

    k_mutex_lock(&fs_log_mutex, K_FOREVER);

    int rc = fs_open(&f, FS_LOG_FILE_PATH, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
    if (rc < 0) {
        k_mutex_unlock(&fs_log_mutex);
        return rc;
    }

    ssize_t written = 0;
    if (len > 0) {
        ssize_t w = fs_write(&f, line, len);
        if (w < 0) {
            (void)fs_close(&f);
            k_mutex_unlock(&fs_log_mutex);
            return (int)w;
        }
        written += w;
    }

    if (need_newline) {
        ssize_t w = fs_write(&f, "\n", 1);
        if (w < 0) {
            (void)fs_close(&f);
            k_mutex_unlock(&fs_log_mutex);
            return (int)w;
        }
        written += w;
    }

    /* fsync ensures data is flushed to flash even if we lose power. */
    (void)fs_sync(&f);
    (void)fs_close(&f);

    k_mutex_unlock(&fs_log_mutex);
    return (int)written;
}

int fs_log_writef(const char *fmt, ...)
{
    if (fmt == NULL) {
        return -EINVAL;
    }

    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n < 0) {
        return -EINVAL;
    }
    /* Truncation is fine; buf is NUL-terminated either way. */
    return fs_log_write(buf);
}

int fs_log_read_all(fs_log_line_cb_t cb, void *user_data)
{
    if (cb == NULL) {
        return -EINVAL;
    }
    if (!fs_log_mounted) {
        return -ENOENT;
    }

    struct fs_file_t f;
    fs_file_t_init(&f);

    k_mutex_lock(&fs_log_mutex, K_FOREVER);

    int rc = fs_open(&f, FS_LOG_FILE_PATH, FS_O_READ);
    if (rc < 0) {
        k_mutex_unlock(&fs_log_mutex);
        return rc;
    }

    char chunk[128];
    char line[160];
    size_t line_len = 0;
    int line_count = 0;

    ssize_t r;
    while ((r = fs_read(&f, chunk, sizeof(chunk))) > 0) {
        for (ssize_t i = 0; i < r; i++) {
            char c = chunk[i];
            if (c == '\n' || line_len == sizeof(line) - 1) {
                line[line_len] = '\0';
                cb(line, user_data);
                line_count++;
                line_len = 0;
                if (c != '\n') {
                    /* Long line continuation - keep the char that didn't fit. */
                    line[line_len++] = c;
                }
            } else if (c != '\r') {
                line[line_len++] = c;
            }
        }
    }

    if (r < 0) {
        (void)fs_close(&f);
        k_mutex_unlock(&fs_log_mutex);
        return (int)r;
    }

    if (line_len > 0) {
        line[line_len] = '\0';
        cb(line, user_data);
        line_count++;
    }

    (void)fs_close(&f);
    k_mutex_unlock(&fs_log_mutex);
    return line_count;
}

int fs_log_clear(void)
{
    if (!fs_log_mounted) {
        return -ENOENT;
    }

    k_mutex_lock(&fs_log_mutex, K_FOREVER);

    /* Removing and re-creating gives us a zero-length file deterministically. */
    int rc = fs_unlink(FS_LOG_FILE_PATH);
    if (rc < 0 && rc != -ENOENT) {
        k_mutex_unlock(&fs_log_mutex);
        return rc;
    }
    rc = ensure_log_file();

    k_mutex_unlock(&fs_log_mutex);
    return rc;
}

int fs_log_stats(size_t *total_bytes, size_t *used_bytes)
{
    if (total_bytes == NULL || used_bytes == NULL) {
        return -EINVAL;
    }
    if (!fs_log_mounted) {
        return -ENOENT;
    }

    struct fs_statvfs st;
    int rc = fs_statvfs(fs_log_mnt.mnt_point, &st);
    if (rc < 0) {
        return rc;
    }

    *total_bytes = (size_t)(st.f_blocks * st.f_frsize);
    *used_bytes  = (size_t)((st.f_blocks - st.f_bfree) * st.f_frsize);
    return 0;
}

int fs_log_file_size(void)
{
    if (!fs_log_mounted) {
        return -ENOENT;
    }

    struct fs_dirent ent;
    int rc = fs_stat(FS_LOG_FILE_PATH, &ent);
    if (rc < 0) {
        return rc;
    }
    return (int)ent.size;
}

/* -------------------------------------------------------------------------- */
/* Shell commands: fslog write|read|clear|stats|size                          */
/* -------------------------------------------------------------------------- */

#include <zephyr/shell/shell.h>

static void shell_line_cb(const char *line, void *user_data)
{
    const struct shell *sh = (const struct shell *)user_data;
    shell_print(sh, "%s", line);
}

static int cmd_fslog_write(const struct shell *sh, size_t argc, char **argv)
{
    if (argc < 2) {
        shell_error(sh, "usage: fslog write <text...>");
        return -EINVAL;
    }

    /* Join argv[1..argc-1] with spaces. */
    char buf[160];
    size_t off = 0;
    for (size_t i = 1; i < argc; i++) {
        int n = snprintf(buf + off, sizeof(buf) - off, "%s%s",
                         (i > 1) ? " " : "", argv[i]);
        if (n < 0 || (size_t)n >= sizeof(buf) - off) {
            break;
        }
        off += n;
    }

    int rc = fs_log_write(buf);
    if (rc < 0) {
        shell_error(sh, "write failed: %d", rc);
        return rc;
    }
    shell_print(sh, "wrote %d bytes", rc);
    return 0;
}

static int cmd_fslog_read(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int n = fs_log_read_all(shell_line_cb, (void *)sh);
    if (n < 0) {
        shell_error(sh, "read failed: %d", n);
        return n;
    }
    shell_print(sh, "(%d line%s)", n, n == 1 ? "" : "s");
    return 0;
}

static int cmd_fslog_clear(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    int rc = fs_log_clear();
    if (rc < 0) {
        shell_error(sh, "clear failed: %d", rc);
        return rc;
    }
    shell_print(sh, "log cleared");
    return 0;
}

static int cmd_fslog_stats(const struct shell *sh, size_t argc, char **argv)
{
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    size_t total = 0, used = 0;
    int rc = fs_log_stats(&total, &used);
    if (rc < 0) {
        shell_error(sh, "stats failed: %d", rc);
        return rc;
    }

    int file_sz = fs_log_file_size();
    shell_print(sh, "mount:   %s", FS_LOG_MOUNT_POINT);
    shell_print(sh, "total:   %u bytes", (unsigned int)total);
    shell_print(sh, "used:    %u bytes", (unsigned int)used);
    shell_print(sh, "free:    %u bytes", (unsigned int)(total - used));
    if (file_sz >= 0) {
        shell_print(sh, "log:     %s (%d bytes)", FS_LOG_FILE_PATH, file_sz);
    } else {
        shell_print(sh, "log:     %s (stat err %d)", FS_LOG_FILE_PATH, file_sz);
    }
    return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    fslog_sub,
    SHELL_CMD_ARG(write, NULL, "Append a line to the log",   cmd_fslog_write, 2, 16),
    SHELL_CMD(   read,  NULL, "Print the log",               cmd_fslog_read),
    SHELL_CMD(   clear, NULL, "Truncate the log",            cmd_fslog_clear),
    SHELL_CMD(   stats, NULL, "Show filesystem statistics",  cmd_fslog_stats),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(fslog, &fslog_sub, "LittleFS log commands", NULL);
