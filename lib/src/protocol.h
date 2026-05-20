#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <zephyr/data/json.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    double latitude;
    double longitude;
    double altitude_m;
    bool valid;
    uint32_t fix_seq;
} gps_location_t;

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

extern const struct json_obj_descr airplane_descr[];
extern const size_t airplane_descr_len;

extern const struct json_obj_descr gps_descr[];
extern const size_t gps_descr_len;

extern const struct json_obj_descr sensor_message_descr[];
extern const size_t sensor_message_descr_len;

#endif