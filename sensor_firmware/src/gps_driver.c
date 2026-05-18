/* gps_driver.c */
#include "gps_driver.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define UBLOX_ADDR    0x42
#define NMEA_MAX_LEN  128

static const struct device *gps_i2c;
static char nmea_buf[NMEA_MAX_LEN];
static int  nmea_len;

static gps_location_t g_loc;
K_MUTEX_DEFINE(g_loc_mutex);

static bool nmea_to_decimal(const char *coord, const char *hemi, double *out_deg)
{
    if (!coord || !hemi || !out_deg || coord[0] == '\0' || hemi[0] == '\0') {
        return false;
    }

    char *endp = NULL;
    double raw = strtod(coord, &endp);
    if (endp == coord || raw <= 0.0) {
        return false;
    }

    int deg = (int)(raw / 100.0);
    double min = raw - (deg * 100.0);
    double dec = (double)deg + (min / 60.0);

    if (hemi[0] == 'S' || hemi[0] == 'W') {
        dec = -dec;
    }

    *out_deg = dec;
    return true;
}

static void update_fix(double lat, double lon, bool has_alt, double alt)
{
    k_mutex_lock(&g_loc_mutex, K_FOREVER);

    g_loc.latitude = lat;
    g_loc.longitude = lon;
    if (has_alt) {
        g_loc.altitude_m = alt;
    }
    g_loc.valid = true;
    g_loc.fix_seq++;

    k_mutex_unlock(&g_loc_mutex);
}

static void parse_gnrmc(char *s)
{
    if (*s == '$') {
        s++;
    }

    char *type = strsep(&s, ",");
    if (!type || strcmp(type, "GNRMC") != 0) {
        return;
    }

    (void)strsep(&s, ",");          /* time_utc */
    char *status   = strsep(&s, ",");
    char *lat      = strsep(&s, ",");
    char *lat_hemi = strsep(&s, ",");
    char *lon      = strsep(&s, ",");
    char *lon_hemi = strsep(&s, ",");

    if (!status || status[0] != 'A') {
        return; /* invalid fix */
    }

    double lat_dd, lon_dd;
    if (!nmea_to_decimal(lat, lat_hemi, &lat_dd) ||
        !nmea_to_decimal(lon, lon_hemi, &lon_dd)) {
        return;
    }

    /* RMC has valid position; keep previous altitude if no new alt in this sentence */
    update_fix(lat_dd, lon_dd, false, 0.0);
}

static void parse_gngga(char *s)
{
    if (*s == '$') {
        s++;
    }

    char *type = strsep(&s, ",");
    if (!type || strcmp(type, "GNGGA") != 0) {
        return;
    }

    (void)strsep(&s, ",");          /* time_utc */
    char *lat      = strsep(&s, ",");
    char *lat_hemi = strsep(&s, ",");
    char *lon      = strsep(&s, ",");
    char *lon_hemi = strsep(&s, ",");
    char *fix_q    = strsep(&s, ",");
    (void)strsep(&s, ",");          /* num_sats */
    (void)strsep(&s, ",");          /* hdop */
    char *alt      = strsep(&s, ",");
    (void)strsep(&s, ",");          /* alt_units */

    if (!fix_q || fix_q[0] == '0') {
        return; /* no fix */
    }

    double lat_dd, lon_dd, alt_m;
    char *endp = NULL;
    alt_m = strtod(alt ? alt : "", &endp);
    if (endp == alt) {
        alt_m = 0.0;
    }

    if (!nmea_to_decimal(lat, lat_hemi, &lat_dd) ||
        !nmea_to_decimal(lon, lon_hemi, &lon_dd)) {
        return;
    }

    /* GGA provides position + altitude */
    update_fix(lat_dd, lon_dd, true, alt_m);
}

static void parse_nmea_sentence(char *line)
{
    char *star = strrchr(line, '*');
    if (!star || line[0] != '$') {
        return;
    }

    uint8_t cs_calc = 0;
    for (char *p = line + 1; p < star; p++) {
        cs_calc ^= (uint8_t)*p;
    }

    uint8_t cs_recv = (uint8_t)strtoul(star + 1, NULL, 16);
    if (cs_calc != cs_recv) {
        return;
    }

    if (strncmp(line, "$GNRMC", 6) == 0) {
        parse_gnrmc(line);
    } else if (strncmp(line, "$GNGGA", 6) == 0) {
        parse_gngga(line);
    }
}

static void nmea_feed_byte(uint8_t b)
{
    if (b == '$') {
        nmea_len = 0;
        nmea_buf[nmea_len++] = '$';
        return;
    }

    if (nmea_len == 0) {
        return;
    }

    if (nmea_len < (NMEA_MAX_LEN - 1)) {
        nmea_buf[nmea_len++] = (char)b;
    }

    if (b == '\n') {
        nmea_buf[nmea_len] = '\0';
        parse_nmea_sentence(nmea_buf);
        nmea_len = 0;
    }
}

int gps_init(void)
{
    gps_i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    if (!device_is_ready(gps_i2c)) {
        printk("GPS: I2C device not ready\n");
        return -ENODEV;
    }

    nmea_len = 0;
    memset(&g_loc, 0, sizeof(g_loc));
    return 0;
}

int gps_poll(void)
{
    if (!gps_i2c) {
        return -ENODEV;
    }

    uint8_t buf[64];
    int ret = i2c_read(gps_i2c, buf, sizeof(buf), UBLOX_ADDR);
    if (ret != 0) {
        return ret;
    }

    for (size_t i = 0; i < sizeof(buf); i++) {
        nmea_feed_byte(buf[i]);
    }

    return 0;
}

gps_location_t get_location(void)
{
    gps_location_t out;
    k_mutex_lock(&g_loc_mutex, K_FOREVER);
    out = g_loc;
    k_mutex_unlock(&g_loc_mutex);
    return out;
}