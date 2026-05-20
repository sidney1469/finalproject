#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/data/json.h>
#include <stdio.h>
#include "ble_transfer.h"
#include "gps_driver.h"
#include "serial.h"

typedef struct {
    char icao[8];
    double alt;
    double lat;
    double lon;
    double spd;
    double head;
    char timestamp[16];
} airplane_t;

typedef struct {
    gps_location_t gps;
    airplane_t plane;
} sensor_message_t;

static const struct json_obj_descr airplane_descr[] = {
    JSON_OBJ_DESCR_PRIM(airplane_t, icao,      JSON_TOK_STRING_BUF),
    JSON_OBJ_DESCR_PRIM(airplane_t, alt,       JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(airplane_t, lat,       JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(airplane_t, lon,       JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(airplane_t, spd,       JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(airplane_t, head,      JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(airplane_t, timestamp, JSON_TOK_STRING_BUF),
};

static const struct json_obj_descr gps_descr[] = {
    JSON_OBJ_DESCR_PRIM(gps_location_t, latitude,   JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(gps_location_t, longitude,  JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(gps_location_t, altitude_m, JSON_TOK_DOUBLE_FP),
    JSON_OBJ_DESCR_PRIM(gps_location_t, valid,      JSON_TOK_TRUE),
    JSON_OBJ_DESCR_PRIM(gps_location_t, fix_seq,    JSON_TOK_NUMBER),
};

static const struct json_obj_descr sensor_message_descr[] = {
    JSON_OBJ_DESCR_OBJECT(sensor_message_t, gps, gps_descr),
    JSON_OBJ_DESCR_OBJECT(sensor_message_t, plane, airplane_descr),
};

static int parse_plane_csv(const char *raw, airplane_t *out)
{
    if (!raw || !out) {
        return -1;
    }

    int matched = sscanf(raw,
                         "%7[^,],%lf,%lf,%lf,%lf,%lf,%15[^\r\n]",
                         out->icao,
                         &out->alt,
                         &out->lat,
                         &out->lon,
                         &out->spd,
                         &out->head,
                         out->timestamp);

    return (matched == 7) ? 0 : -1;
}

int main(void)
{
    int err = serial_init();
    if (err) {
        printk("serial_init failed: %d\n", err);
        return 0;
    }

    err = init_nus();
    if (err) {
        printk("bluetooth_init failed: %d\n", err);
        return 0;
    }
    gps_location_t current_loc;
    current_loc.latitude = -27.497358;
    current_loc.longitude = 153.013259;
    current_loc.altitude_m = 10.00;
    current_loc.valid = true;

    while (1) {
        gps_location_t new_loc = get_location();

        if (new_loc.valid) {
            // Update current location with new gps info
            char msg[200];
            current_loc.latitude = new_loc.latitude;
            current_loc.longitude = new_loc.longitude;
            current_loc.altitude_m = new_loc.altitude_m;
            snprintf(msg, sizeof(msg),
                     "{\"lat\":%.6f,\"lon\":%.6f,\"alt\":%.2f}",
                     new_loc.latitude, new_loc.longitude, new_loc.altitude_m);
            printk("Received GPS: %s\n", msg);
        } else {
            printk("Waiting for GPS fix...\n");
        }
        char* raw_plane_data = serial_read_line();
        if (raw_plane_data) {
            printk("%s\n", raw_plane_data);
        }
        airplane_t plane = {0};

        if (raw_plane_data && parse_plane_csv(raw_plane_data, &plane) == 0) {
    //         printk("plane: %s alt=%.1f lat=%.6f lon=%.6f spd=%.1f head=%.1f ts=%s\n",
    //    plane.icao, plane.alt, plane.lat, plane.lon, plane.spd, plane.head, plane.timestamp);
        }

        sensor_message_t message = {
            .gps = current_loc,
            .plane = plane,
        };

        char ble_msg[247] = {0};

        err = json_obj_encode_buf(sensor_message_descr,
                                  ARRAY_SIZE(sensor_message_descr),
                                  &message,
                                  ble_msg,
                                  sizeof(ble_msg));

        if (err < 0) {
            printk("json_obj_encode_buf failed: %d\n", err);
        } else {
            // printk("%s\n", ble_msg);
            send_over_nus(ble_msg);
        }
        k_msleep(1000);
    }

    return 0;
}
