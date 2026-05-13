#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>
#include <string.h>
#include <ctype.h>

#define UBLOX_ADDR 0x42
#define NMEA_MAX_LEN 128

static char nmea_buf[NMEA_MAX_LEN];
static int  nmea_len = 0;

/* -----------------------------
 * NMEA PARSER HELPERS
 * ----------------------------- */

static void parse_gnrmc(char *s)
{
    if (*s == '$') s++;

    char *type = strsep(&s, ",");
    if (!type || strcmp(type, "GNRMC") != 0) return;

    char *time_utc = strsep(&s, ",");
    char *status   = strsep(&s, ",");   // A=valid, V=void
    char *lat      = strsep(&s, ",");
    char *lat_hemi = strsep(&s, ",");
    char *lon      = strsep(&s, ",");
    char *lon_hemi = strsep(&s, ",");

    if (!status || *status != 'A') {
        printk("GNRMC: no fix\n");
        return;
    }

    printk("GNRMC: lat=%s %s, lon=%s %s\n",
           lat, lat_hemi, lon, lon_hemi);
}

static void parse_gngga(char *s)
{
    if (*s == '$') s++;

    char *type = strsep(&s, ",");
    if (!type || strcmp(type, "GNGGA") != 0) return;

    char *time_utc = strsep(&s, ",");
    char *lat      = strsep(&s, ",");
    char *lat_hemi = strsep(&s, ",");
    char *lon      = strsep(&s, ",");
    char *lon_hemi = strsep(&s, ",");
    char *fix_q    = strsep(&s, ",");   // 0=no fix, 1=GPS fix
    char *num_sats = strsep(&s, ",");

    if (!fix_q || *fix_q == '0') {
        printk("GNGGA: no fix\n");
        return;
    }

    printk("GNGGA: fix=%s sats=%s lat=%s %s lon=%s %s\n",
           fix_q, num_sats, lat, lat_hemi, lon, lon_hemi);
}

static void parse_nmea_sentence(char *line)
{
    char *star = strrchr(line, '*');
    if (!star) return;

    uint8_t cs_calc = 0;
    for (char *p = line + 1; p < star; p++) {
        cs_calc ^= (uint8_t)*p;
    }

    uint8_t cs_recv = (uint8_t)strtoul(star + 1, NULL, 16);
    if (cs_calc != cs_recv) {
        return; // bad checksum
    }

    if (strstr(line, "$GNRMC") == line) {
        parse_gnrmc(line);
    } else if (strstr(line, "$GNGGA") == line) {
        parse_gngga(line);
    }
}

/* -----------------------------
 * STREAM BYTE FEEDER
 * ----------------------------- */

static void nmea_feed_byte(uint8_t b)
{
    if (b == '$') {
        nmea_len = 0;
        nmea_buf[nmea_len++] = '$';
        return;
    }

    if (nmea_len == 0) {
        return; // waiting for '$'
    }

    if (nmea_len < NMEA_MAX_LEN - 1) {
        nmea_buf[nmea_len++] = (char)b;
    }

    if (b == '\n') {
        nmea_buf[nmea_len] = '\0';
        parse_nmea_sentence(nmea_buf);
        nmea_len = 0;
    }
}

/* -----------------------------
 * MAIN LOOP
 * ----------------------------- */

void main(void)
{
    const struct device *i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));

    if (!device_is_ready(i2c)) {
        printk("I2C device not ready\n");
        return;
    }

    printk("M9N NMEA reader started\n");

    uint8_t buf[64];

    while (1) {
        int ret = i2c_read(i2c, buf, sizeof(buf), UBLOX_ADDR);

        if (ret == 0) {
            for (int i = 0; i < sizeof(buf); i++) {
                nmea_feed_byte(buf[i]);
            }
        } else {
            // printk("I2C read error: %d\n", ret);
        }

        k_msleep(50);
    }
}
