# fs_firmware

A small Zephyr application that demonstrates the **File System** feature from
the CSSE4011 marking rubric. It mounts a [LittleFS](https://github.com/littlefs-project/littlefs)
volume on the board's internal `storage_partition` and exposes a simple
append-only line logger plus Zephyr shell commands to interact with it.

## What it shows off

* Zephyr `CONFIG_FILE_SYSTEM` / `CONFIG_FILE_SYSTEM_LITTLEFS` with
  power-fail resilience (`fs_sync` after every write).
* A clean, reusable API (`fs_log_init`, `fs_log_write`, `fs_log_read_all`,
  `fs_log_clear`, `fs_log_stats`) protected by a `k_mutex` so it is safe to
  call from multiple threads.
* Custom Zephyr **shell** commands (`fslog write|read|clear|stats`) plus the
  built-in `fs` shell (`ls`, `cat`, `rm`, `mkdir`, `mount`, ...).
* A boot counter line written to flash on every reset, demonstrating that the
  data persists across reboots.

## Build & flash

The firmware uses the standard `storage_partition` defined by virtually every
Zephyr board, so no overlay is required for boards like
`xiao_ble/nrf52840/sense` or `nrf52840dk_nrf52840`.

```sh
# From the workspace root (with Zephyr SDK + west already set up)
west build -p always -b xiao_ble/nrf52840/sense fs_firmware
west flash
```

## Demo script

1. Open the serial console (115200 8N1). You should see:

   ```
   === fs_firmware starting ===
   Logged boot marker (24 bytes)
   LittleFS: total=65536 used=8192 free=57344 bytes
   Log file size: 24 bytes
   uart:~$
   ```

2. Inspect the filesystem with the built-in `fs` shell:

   ```
   uart:~$ fs ls /lfs
   uart:~$ fs cat /lfs/log.txt
   ```

3. Append entries using the custom `fslog` command:

   ```
   uart:~$ fslog write hello from CSSE4011
   wrote 21 bytes
   uart:~$ fslog read
   boot #1 uptime=123 ms
   hello from CSSE4011
   (2 lines)
   uart:~$ fslog stats
   mount:   /lfs
   total:   65536 bytes
   used:    8192 bytes
   free:    57344 bytes
   log:     /lfs/log.txt (45 bytes)
   ```

4. Reset the board and run `fslog read` again - the previous entries are
   still there, plus a fresh `boot #2 ...` line, proving persistence.

5. `fslog clear` truncates the log; `fs rm /lfs/log.txt` removes it
   entirely.

## Files

| File                     | Purpose                                   |
| ------------------------ | ----------------------------------------- |
| `src/main.c`             | Mounts FS, writes boot marker, idles      |
| `src/fs_log.c` / `.h`    | LittleFS mount + thread-safe logger + shell |
| `prj.conf`               | Zephyr configuration (FS, shell, logging) |
| `CMakeLists.txt`         | Build glue                                |
