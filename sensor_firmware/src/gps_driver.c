#include "gps_driver.h"
#include <stdbool.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define UBLOX_ADDR 0x42
#define NMEA_MAX_LEN 128

#define GPS_THREAD_STACK_SIZE 2048
#define GPS_THREAD_PRIORITY 7
#define GPS_POLL_MS 50

static const struct device *gps_i2c;
static char nmea_buf[NMEA_MAX_LEN];
static int nmea_len;

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

static void update_fix(double lat, double lon, double alt)
{
    k_mutex_lock(&g_loc_mutex, K_FOREVER);
    g_loc.latitude = lat;
    g_loc.longitude = lon;
    g_loc.altitude_m = alt;
    g_loc.valid = true;
    g_loc.fix_seq++;
    k_mutex_unlock(&g_loc_mutex);
}

static char *next_csv_field(char **s)
{
    if (!s || !*s) {
        return NULL;
    }

    char *start = *s;
    char *comma = strchr(start, ',');

    if (comma) {
        *comma = '\0';
        *s = comma + 1;
    } else {
        *s = NULL;
    }

    return start;
}

/* Minimal: only accept GNGGA sentences */
static void parse_gngga(char *s)
{
    if (*s == '$') {
        s++;
    }

    char *type = next_csv_field(&s);
    if (!type || strcmp(type, "GNGGA") != 0) {
        return;
    }

    (void)next_csv_field(&s);   /* time */
    char *lat      = next_csv_field(&s);
    char *lat_hemi = next_csv_field(&s);
    char *lon      = next_csv_field(&s);
    char *lon_hemi = next_csv_field(&s);
    char *fix_q    = next_csv_field(&s);
    (void)next_csv_field(&s);   /* sats */
    (void)next_csv_field(&s);   /* hdop */
    char *alt      = next_csv_field(&s);

    if (!fix_q || fix_q[0] == '0') {
        return;
    }

    double lat_dd, lon_dd;
    if (!nmea_to_decimal(lat, lat_hemi, &lat_dd) ||
        !nmea_to_decimal(lon, lon_hemi, &lon_dd)) {
        return;
    }

    double alt_m = 0.0;
    if (alt && alt[0] != '\0') {
        char *endp = NULL;
        double parsed = strtod(alt, &endp);
        if (endp != alt) {
            alt_m = parsed;
        }
    }

    update_fix(lat_dd, lon_dd, alt_m);
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

    if (strncmp(line, "$GNGGA", 6) == 0) {
        parse_gngga(line);
    }
}

static void nmea_feed_byte(uint8_t buf)
{
    if (buf == '$') {
        nmea_len = 0;
        nmea_buf[nmea_len++] = '$';
        return;
    }

    if (nmea_len == 0) {
        return;
    }

    if (nmea_len < (NMEA_MAX_LEN - 1)) {
        nmea_buf[nmea_len++] = (char)buf;
    }

    if (buf == '\n') {
        nmea_buf[nmea_len] = '\0';
        parse_nmea_sentence(nmea_buf);
        nmea_len = 0;
    }
}

static void gps_thread()
{
    gps_i2c = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    if (!device_is_ready(gps_i2c)) {
        return;
    }

    while (1) {
        uint8_t buf[64];
        int ret = i2c_read(gps_i2c, buf, sizeof(buf), UBLOX_ADDR);
        if (ret == 0) {
            for (size_t i = 0; i < sizeof(buf); i++) {
                nmea_feed_byte(buf[i]);
            }
        }

        k_msleep(GPS_POLL_MS);
    }
}

K_THREAD_DEFINE(gps_id, GPS_THREAD_STACK_SIZE, gps_thread,
                NULL, NULL, NULL, GPS_THREAD_PRIORITY, 0, 0);

gps_location_t get_location(void)
{
    gps_location_t out;
    k_mutex_lock(&g_loc_mutex, K_FOREVER);
    out = g_loc;
    k_mutex_unlock(&g_loc_mutex);
    return out;
}