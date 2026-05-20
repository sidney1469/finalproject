/*
 * fs_log.h - Simple append-only line logger backed by LittleFS.
 *
 * Provides a thin, thread-safe wrapper around Zephyr's file system API
 * to mount a LittleFS partition and read/write/clear log lines.
 */

#ifndef FS_LOG_H
#define FS_LOG_H

#include <stddef.h>
#include <stdint.h>

/* Mount point on the VFS. */
#define FS_LOG_MOUNT_POINT "/lfs"

/* Default log file (under the mount point). */
#define FS_LOG_FILE_PATH   FS_LOG_MOUNT_POINT "/log.txt"

/**
 * Mount the LittleFS partition and ensure the log file exists.
 *
 * @return 0 on success, negative errno on failure.
 */
int fs_log_init(void);

/**
 * Append a line of text to the log file. A newline is added automatically
 * if @p line does not already end with one.
 *
 * @param line NUL-terminated string to append.
 * @return Number of bytes written on success, negative errno on failure.
 */
int fs_log_write(const char *line);

/**
 * Append a formatted line (printf-style) to the log file.
 *
 * @return Number of bytes written on success, negative errno on failure.
 */
int fs_log_writef(const char *fmt, ...);

/**
 * Read the entire log file and invoke @p cb for each line (without newline).
 *
 * @param cb       Callback called per line; @p user_data is forwarded.
 * @param user_data Opaque pointer passed to @p cb.
 * @return Number of lines read on success, negative errno on failure.
 */
typedef void (*fs_log_line_cb_t)(const char *line, void *user_data);
int fs_log_read_all(fs_log_line_cb_t cb, void *user_data);

/**
 * Truncate the log file to zero length.
 *
 * @return 0 on success, negative errno on failure.
 */
int fs_log_clear(void);

/**
 * Get filesystem statistics for the mounted partition.
 *
 * @param[out] total_bytes Total bytes available on the partition.
 * @param[out] used_bytes  Bytes currently in use.
 * @return 0 on success, negative errno on failure.
 */
int fs_log_stats(size_t *total_bytes, size_t *used_bytes);

/**
 * Get the size in bytes of the log file.
 *
 * @return File size on success, negative errno on failure.
 */
int fs_log_file_size(void);

#endif /* FS_LOG_H */
